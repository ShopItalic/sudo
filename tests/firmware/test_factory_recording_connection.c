/* Actual BLE callbacks, packet producers, PTT state machine and command tails;
 * only RTOS/radio/recorder IO are modeled. No physical-device claim. */
#include "fixture.h"
#include "app_factory_ptt.h"

static unsigned checks;
#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr, "FAIL line %u: %s\n", __LINE__, #expr); exit(1); \
} } while (0)

uint32_t fixture_ticks;
TaskHandle_t fixture_task = 1;
static bool working, app_pdm_key_flag, ble_notify_inhibit;
static unsigned actions[3] = {1,0,2};
static unsigned hold_delay_ms = 500;
static unsigned starts, stops, fault_count, blue_connect, blue_disconnect;
static uint32_t ble_connect_tick;
static struct bc_ble_data_package commands[16];
static unsigned head, tail;

bool app_pdm_work_status(void) { return working; }
bool app_pdm_recording_start(void) { if (!working) ++starts; working = true; return true; }
bool app_pdm_recording_stop(void) { if (working) ++stops; working = false; return true; }
unsigned app_pdm_mode_get(void) { return PDM_MODE_OFFLINE; }
void app_pdm_touch_stop(void) { CHECK(false); }
void factory_capture_fault(void) { ++fault_count; }
unsigned app_factory_controls_action(unsigned g) { return actions[g]; }
unsigned app_factory_controls_hold_delay_ms(void) { return hold_delay_ms; }
void app_package_send_enqueue(struct app_cmd_package *p, unsigned n) { (void)p; (void)n; }

static void enqueue(unsigned type, struct bc_ble_data_package *p) {
    CHECK(type == BC_QUEUE_TYPE_BLE_RECV && tail < 16);
    commands[tail++] = *p;
}
#define bc_queue_isr_enqueue_not_yield enqueue
#define bc_queue_isr_enqueue enqueue
static void bc_ic_led_ble_connect_from_isr(void) { ++blue_connect; }
static void bc_ic_led_ble_disconnect_from_isr(void) { ++blue_disconnect; }
static void bc_log_ble_disenable(void) {}
static void app_connect_idie_timer_start_from_isr(unsigned timer) { (void)timer; }
#include "callbacks.inc"

static void drain(void) {
    fixture_task = 1;
    while (head < tail) {
        struct bc_ble_data_package *p = &commands[head++];
        struct app_cmd_package command = {0};
        CHECK(p->data_length <= sizeof(command));
        memcpy(&command, p->data, p->data_length);
        app_factory_ptt_before_command(p->data, p->data_length);
        CHECK(command.cmd == 0x71);
        /* The F9/FC cases below are extracted from the candidate's actual
         * app_cmd_pdm handler, after its production PTT ownership hook. */
        struct app_cmd_package *cmd_package = &command;
        switch (command.subcmd) {
#include "command-cases.inc"
        default: CHECK(false);
        }
        app_factory_ptt_worker_service();
    }
    head = tail = 0;
}

static void report(bool hold) {
    uint8_t data[8] = {0};
    if (hold) { data[0] = 8; data[3] = 1; data[4] = data[6] = 10; }
    app_factory_ptt_report(data, true);
    app_factory_ptt_worker_service();
}
static void reset(void) {
    memset(&factory_ptt_diagnostics, 0, sizeof(factory_ptt_diagnostics));
    starts = stops = fault_count = head = tail = blue_connect = blue_disconnect = 0;
    working = false; fixture_ticks = 0; fixture_task = 1; hold_delay_ms = 500;
    app_factory_ptt_worker_init(); report(false);
}
static void start_hold(void) { reset(); report(true); CHECK(working && starts == 1); }
static void continue_hold(void) {
    for (unsigned i = 0; i < 100; ++i) {
        fixture_ticks += 102; report(true);
        CHECK(working && starts == 1 && stops == 0);
    }
}

int main(void) {
    start_hold(); app_ble_disconnect_callback(); drain();
    CHECK(blue_disconnect == 1 && stops == 0 && working);
    continue_hold(); report(false); CHECK(stops == 1 && !working);

    start_hold(); app_ble_connect_callback(); drain();
    CHECK(blue_connect == 1 && stops == 0 && working);
    CHECK(factory_ptt_diagnostics.contact && factory_ptt_diagnostics.timeouts == 0 && fault_count == 0);
    continue_hold(); report(false); CHECK(stops == 1 && !working);

    start_hold();
    for (unsigned i = 0; i < 12; ++i) {
        app_ble_disconnect_callback(); drain(); CHECK(working);
        fixture_ticks += 102; report(true); app_ble_connect_callback(); drain();
        CHECK(stops == 0 && starts == 1 && working);
    }
    CHECK(blue_connect == 12 && blue_disconnect == 12);
    report(false); CHECK(stops == 1 && !working);

    reset(); CHECK(app_factory_ptt_triple_tap()); app_factory_ptt_worker_service();
    CHECK(working && factory_ptt_diagnostics.memo_owned);
    app_ble_disconnect_callback(); drain(); app_ble_connect_callback(); drain();
    CHECK(working && stops == 0 && starts == 1);
    CHECK(app_factory_ptt_triple_tap()); app_factory_ptt_worker_service();
    CHECK(!working && stops == 1);

    /* A delayed connection callback must not leave a stop queued for a
     * recording that starts before the command worker drains that queue. */
    reset(); app_ble_connect_callback(); report(true); drain();
    CHECK(working && stops == 0); report(false); CHECK(stops == 1);

    /* Preserve a hold that is still waiting for its configured start delay. */
    reset(); hold_delay_ms = 1000; report(true); CHECK(!working);
    app_ble_connect_callback(); drain();
    fixture_ticks += pdMS_TO_TICKS(600); report(true);
    CHECK(working && starts == 1); report(false); CHECK(stops == 1);

    reset(); app_ble_connect_callback(); drain();
    CHECK(!working && starts == 0 && stops == 0);

    /* Deliberate lifecycle stop commands retain their existing semantics. */
    start_hold(); app_package_mic_recording_stop_isr(); drain();
    CHECK(!working && stops == 1);
    report(true); CHECK(starts == 1 && !working);
    report(false); report(true); CHECK(starts == 2 && working);

    /* Recording independence does not disable release/fault protections. */
    start_hold(); app_ble_disconnect_callback(); drain();
    fixture_ticks += pdMS_TO_TICKS(751); app_factory_ptt_worker_service();
    CHECK(!working && stops == 1 && factory_ptt_diagnostics.timeouts == 1);
    start_hold(); app_ble_connect_callback(); drain();
    app_factory_ptt_report(NULL, false); app_factory_ptt_worker_service();
    CHECK(!working && stops == 1 && fault_count == 1);

    printf("PASS %u checks: PTT/memo survive BLE transitions; delayed callbacks, hold delay, explicit stop, release and fault handling preserved\n", checks);
    return 0;
}
