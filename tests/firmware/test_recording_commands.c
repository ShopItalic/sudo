#include "recording_test_shim.h"

#include <stdio.h>
#include <stdlib.h>

static unsigned checks;
static unsigned failures;

static void check_result(bool condition, const char *expression, unsigned line)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_result((condition), #condition, __LINE__)

struct observations
{
    unsigned recording_start;
    unsigned recording_stop;
    unsigned capture_start;
    unsigned capture_stop;
    unsigned switch_online_to_offline;
    unsigned clear_touch_flag;
    unsigned ack_count;
    uint16_t ack_length;
    uint8_t ack[250];
};

static struct bc_ble_data_package queued;
static bool queued_valid;
static bool queued_from_isr;
static struct observations observed;

static bool capture_queue_item(bc_queue_type queue_type, void *enqueue_data, bool from_isr)
{
    CHECK(queue_type == BC_QUEUE_TYPE_BLE_RECV);
    CHECK(enqueue_data != NULL);
    if (enqueue_data != NULL)
    {
        memcpy(&queued, enqueue_data, sizeof(queued));
        queued_valid = true;
        queued_from_isr = from_isr;
    }
    return true;
}

bool bc_queue_enqueue(bc_queue_type queue_type, void *enqueue_data)
{
    return capture_queue_item(queue_type, enqueue_data, false);
}

bool bc_queue_isr_enqueue(bc_queue_type queue_type, void *enqueue_data)
{
    return capture_queue_item(queue_type, enqueue_data, true);
}

bool bc_queue_isr_enqueue_not_yield(bc_queue_type queue_type, void *enqueue_data)
{
    return capture_queue_item(queue_type, enqueue_data, true);
}

void app_package_send_enqueue(struct app_cmd_package *package, uint8_t length)
{
    CHECK(package != NULL);
    CHECK(length <= sizeof(observed.ack));
    if (package != NULL && length <= sizeof(observed.ack))
    {
        ++observed.ack_count;
        observed.ack_length = length;
        memcpy(observed.ack, package, length);
    }
}

void app_pdm_start(struct app_cmd_package *package)
{
    (void)package;
}

void app_pdm_stop(struct app_cmd_package *package)
{
    (void)package;
}

bool app_pdm_recording_start(void)
{
    ++observed.recording_start;
    return true;
}

bool app_pdm_recording_stop(void)
{
    ++observed.recording_stop;
    return true;
}

bool app_pdm_capture_recording_start(void)
{
    ++observed.capture_start;
    return true;
}

bool app_pdm_capture_recording_stop(void)
{
    ++observed.capture_stop;
    return true;
}

bool app_pdm_switch_online_to_offline(void)
{
    ++observed.switch_online_to_offline;
    return true;
}

void app_touch_pdm_key_flag_clear(void)
{
    ++observed.clear_touch_flag;
}

bool bc_device_info_set_audio_up_mode(uint8_t mode)
{
    (void)mode;
    return true;
}

uint8_t bc_device_info_get_audio_up_mode(void)
{
    return 0;
}

#define NOOP_HANDLER(name) \
    uint8_t name(struct app_cmd_package *package) \
    { \
        (void)package; \
        return 0; \
    }

NOOP_HANDLER(app_cmd_set_time_callback)
NOOP_HANDLER(app_cmd_get_version_callback)
NOOP_HANDLER(app_cmd_get_vbat)
NOOP_HANDLER(app_cmd_get_hrv)
NOOP_HANDLER(app_cmd_get_spo2)
NOOP_HANDLER(app_cmd_get_tempertion)
NOOP_HANDLER(app_cmd_get_step_count)
NOOP_HANDLER(app_cmd_get_hrstory)
NOOP_HANDLER(app_cmd_set_sys)
void app_test_cmd_handler(struct app_cmd_package *package)
{
    (void)package;
}
NOOP_HANDLER(app_cmd_get_ppg_spo2)
NOOP_HANDLER(app_cmd_puf)
NOOP_HANDLER(app_cmd_authentication)
NOOP_HANDLER(app_cmd_nfc)
NOOP_HANDLER(app_cmd_six_axis_sensor)
NOOP_HANDLER(app_cmd_get_ir)
NOOP_HANDLER(app_cmd_led)
NOOP_HANDLER(app_cmd_hid)
NOOP_HANDLER(app_cmd_config_touch)
NOOP_HANDLER(app_cmd_led_motor_mode_set)
NOOP_HANDLER(app_cmd_led_motor_mode_get)
NOOP_HANDLER(app_cmd_motor)
NOOP_HANDLER(app_cmd_port_mode)
NOOP_HANDLER(app_cmd_app_event)
NOOP_HANDLER(app_cmd_rtc_alarm_clock_event)
NOOP_HANDLER(app_cmd_ppg_led_data_get)
NOOP_HANDLER(app_cmd_wifi_event)
NOOP_HANDLER(app_cmd_ipc_event)

#undef NOOP_HANDLER

static void reset_observations(void)
{
    memset(&queued, 0, sizeof(queued));
    queued_valid = false;
    queued_from_isr = false;
    memset(&observed, 0, sizeof(observed));
}

static void assert_packet(uint8_t subcmd, uint8_t value, uint16_t expected_length,
                          bool expected_isr)
{
    static const uint8_t header[] = {0, 9, CMD_PDM};
    CHECK(queued_valid);
    CHECK(queued_from_isr == expected_isr);
    CHECK(queued.data_length == expected_length);
    CHECK(memcmp(queued.data, header, sizeof(header)) == 0);
    CHECK(queued.data[3] == subcmd);
    if (expected_length >= 5)
        CHECK(queued.data[4] == value);
}

static void parse_queued(void)
{
    CHECK(queued_valid);
    if (queued_valid)
        app_cmd_package_parse(queued.data, queued.data_length);
}

static void test_recording_starts(void)
{
    reset_observations();
    app_package_mic_recording_start();
    parse_queued();
    CHECK(observed.recording_start == 1);
    CHECK(observed.recording_stop == 0);
    assert_packet(0x05, 1, 5, false);
    CHECK(observed.ack_count == 1);
    CHECK(observed.ack_length == 5);
    CHECK(observed.ack[4] == 1);

    reset_observations();
    app_package_mic_capture_recording_start();
    parse_queued();
    CHECK(observed.capture_start == 1);
    CHECK(observed.capture_stop == 0);
    assert_packet(0xFE, 1, 5, false);
    CHECK(observed.ack_count == 0);
}

static void test_recording_stops(void)
{
    reset_observations();
    app_package_mic_recording_stop();
    parse_queued();
    CHECK(observed.recording_stop == 1);
    CHECK(observed.recording_start == 0);
    assert_packet(0x05, 0, 5, false);
    CHECK(observed.ack_count == 1);
    CHECK(observed.ack[4] == 1);

    reset_observations();
    app_package_mic_capture_recording_stop();
    parse_queued();
    CHECK(observed.capture_stop == 1);
    CHECK(observed.capture_start == 0);
    assert_packet(0xFE, 0, 5, false);

    reset_observations();
    app_package_mic_recording_stop_isr();
    parse_queued();
    CHECK(observed.recording_stop == 1);
    assert_packet(0xF9, 0, 5, true);
}

static void test_internal_commands(void)
{
    reset_observations();
    app_package_pdm_switch_online_to_offline();
    parse_queued();
    CHECK(observed.switch_online_to_offline == 1);
    CHECK(observed.recording_stop == 0);
    assert_packet(0xFD, 0, 4, true);

    reset_observations();
    app_package_pdm_key_flag_clear();
    parse_queued();
    CHECK(observed.clear_touch_flag == 1);
    CHECK(observed.recording_stop == 0);
    assert_packet(0xFC, 0, 4, true);
}

static void test_truncation_and_bounds(void)
{
    uint8_t truncated[5] = {0, 9, CMD_PDM, 0x05, 1};
    uint8_t short_packet[3] = {0, 9, CMD_PDM};
    uint8_t oversize_packet[251];

    memset(oversize_packet, 0, sizeof(oversize_packet));
    reset_observations();
    app_cmd_package_parse(truncated, 4);
    CHECK(observed.recording_start == 0);
    CHECK(observed.recording_stop == 0);
    CHECK(observed.ack_count == 0);

    truncated[3] = 0xFE;
    app_cmd_package_parse(truncated, 4);
    CHECK(observed.capture_start == 0);
    CHECK(observed.capture_stop == 0);
    CHECK(observed.recording_stop == 0);

    app_cmd_package_parse(short_packet, 3);
    app_cmd_package_parse(NULL, 4);
    app_cmd_package_parse(oversize_packet, sizeof(oversize_packet));
    CHECK(observed.recording_start == 0);
    CHECK(observed.recording_stop == 0);
    CHECK(observed.capture_start == 0);
    CHECK(observed.capture_stop == 0);
    CHECK(observed.ack_count == 0);

    /* A declared five byte packet remains a valid legacy recording command. */
    reset_observations();
    truncated[3] = 0x05;
    app_cmd_package_parse(truncated, sizeof(truncated));
    CHECK(observed.recording_start == 1);
    CHECK(observed.recording_stop == 0);
}

int main(void)
{
    test_recording_starts();
    test_recording_stops();
    test_internal_commands();
    test_truncation_and_bounds();

    if (failures != 0)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks (recording producers, bounded parser, PDM dispatch)\n", checks);
    return 0;
}
