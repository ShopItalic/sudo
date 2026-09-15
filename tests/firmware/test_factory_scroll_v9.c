#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "app_factory_scroll.c"

static unsigned checks, critical, scheduler_suspended, app_irqs_masked, sent, wakes, enabled = 15;
static unsigned change_before_suspend;
static bool change_before_irq_guard;
static bool recording, notify = true;
static uint32_t init_error, send_error, sec_error, cccd_error;
static uint8_t security_level = 2, last_report[5];
static uint16_t cccd_length = 2;
uint32_t fixture_ticks;
TaskHandle_t fixture_task = 2;
app_factory_ptt_state factory_ptt_diagnostics;
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#v); return 1; } } while (0)
void fixture_enter(void) { ++critical; }
void fixture_leave(void) { assert(critical); --critical; }
void fixture_suspend(void) {
    assert(!critical && !scheduler_suspended);
    /* A task may run after the queue lock is released and before suspension. */
    if (change_before_suspend == 1) ++epoch;
    if (change_before_suspend == 2) recording = true;
    if (change_before_suspend == 3) fixture_ticks += pdMS_TO_TICKS(251);
    change_before_suspend = 0;
    ++scheduler_suspended;
}
int fixture_resume(void) { assert(!critical && !app_irqs_masked && scheduler_suspended == 1); --scheduler_suspended; return 0; }
uint32_t sd_nvic_critical_region_enter(uint8_t *nested) {
    assert(!critical && scheduler_suspended == 1);
    if (change_before_irq_guard) { ++epoch; change_before_irq_guard=false; }
    *nested=app_irqs_masked; app_irqs_masked=1; return 0;
}
uint32_t sd_nvic_critical_region_exit(uint8_t nested) {
    assert(!critical && scheduler_suspended == 1 && app_irqs_masked);
    app_irqs_masked=nested; return 0;
}
void fixture_notify(TaskHandle_t task) { assert(!critical && task==fixture_task); ++wakes; }
bool app_pdm_work_status(void) { return recording; }
bool app_factory_controls_swipe_enabled(unsigned direction) { return direction < 4 && (enabled & (1U << direction)); }
uint32_t ble_hids_init(ble_hids_t *hid, const ble_hids_init_t *init) {
    assert(!critical && init->is_mouse && init->inp_rep_count == 1);
    assert(init->p_inp_rep_array->max_len == 5 && init->p_inp_rep_array->rep_ref.report_id == 1);
    assert(init->p_inp_rep_array->sec.cccd_wr == SEC_JUST_WORKS);
    assert(init->p_inp_rep_array->sec.wr == SEC_NO_ACCESS);
    assert(init->rep_map.p_data == scroll_report_map && init->rep_map.data_len == sizeof(scroll_report_map));
    hid->inp_rep_array[0].char_handles.cccd_handle = 42;
    return init_error;
}
uint32_t sd_ble_gap_conn_sec_get(uint16_t handle, ble_gap_conn_sec_t *sec) {
    assert(!critical && scheduler_suspended == 1 && app_irqs_masked && handle == connection && handle != BLE_CONN_HANDLE_INVALID);
    sec->sec_mode.sm = 1; sec->sec_mode.lv = security_level; return sec_error;
}
uint32_t sd_ble_gatts_value_get(uint16_t handle, uint16_t attr, ble_gatts_value_t *value) {
    assert(!critical && scheduler_suspended == 1 && app_irqs_masked && handle == connection && attr == 42 && value->len == 2 && value->offset == 0);
    value->p_value[0] = notify; value->p_value[1] = 0; value->len = cccd_length; return cccd_error;
}
uint32_t ble_hids_inp_rep_send(ble_hids_t *hid, uint8_t index, uint16_t length, uint8_t *data, uint16_t handle) {
    assert(!critical && scheduler_suspended == 1 && app_irqs_masked && hid == &factory_scroll_hids && index == 0 && length == 5 && handle == connection);
    assert(data[0] == 0 && data[1] == 0 && data[2] == 0);
    memcpy(last_report, data, 5); ++sent; return send_error;
}
static void ble(unsigned type, uint16_t handle) {
    ble_evt_t event = {0}; event.header.evt_id = type; event.evt.gap_evt.conn_handle = handle;
    event.evt.gap_evt.params.conn_sec_update.conn_sec.sec_mode.sm = 1;
    event.evt.gap_evt.params.conn_sec_update.conn_sec.sec_mode.lv = 2;
    scroll_ble_event(&event, NULL);
}
static void hid_event(unsigned type) { ble_hids_evt_t e = {type}; scroll_hid_event(&factory_scroll_hids,&e); }
static void swipe(unsigned direction) {
    app_factory_scroll_input(direction); app_factory_scroll_service();
    assert(!critical && !scheduler_suspended && !app_irqs_masked);
}
int main(void) {
    swipe(0); CHECK(sent == 0); /* uninitialized */
    init_error = 12; app_factory_scroll_init(); CHECK(factory_scroll_init_error == 12);
    ble(BLE_GAP_EVT_CONNECTED,0); ble(BLE_GAP_EVT_CONN_SEC_UPDATE,0); swipe(0); CHECK(sent == 0);
    init_error = 0; app_factory_scroll_init(); CHECK(initialized);
    ble(BLE_GAP_EVT_DISCONNECTED,0); swipe(0); CHECK(sent == 0);
    ble(BLE_GAP_EVT_CONNECTED,0); swipe(0); CHECK(sent == 0); /* handle zero is valid, encryption required */
    ble(BLE_GAP_EVT_CONN_SEC_UPDATE,0);
    worker=0; app_factory_scroll_input(0); CHECK(count==0 && wakes==0);
    app_factory_scroll_service(); /* register the actual command worker */
    for (unsigned mask=0; mask<16; ++mask) {
        enabled = mask;
        for (unsigned direction=0; direction<4; ++direction) {
            unsigned before=sent; swipe(direction);
            CHECK(sent == before + ((mask >> direction) & 1U));
            CHECK(wakes==sent);
            if (sent != before) {
                const uint8_t expected[4][5] = {{0,0,0,252,0},{0,0,0,4,0},{0,0,0,0,4},{0,0,0,0,252}};
                CHECK(memcmp(last_report,expected[direction],5) == 0);
            }
        }
    }
    enabled=15; unsigned before=sent;
    for (unsigned change=1; change<=3; ++change) {
        app_factory_scroll_input(0); change_before_suspend=change; app_factory_scroll_service();
        CHECK(sent==before && !critical && !scheduler_suspended);
        recording=false;
    }
    app_factory_scroll_input(0); change_before_irq_guard=true; app_factory_scroll_service();
    CHECK(sent==before && !scheduler_suspended && !app_irqs_masked);
    swipe(4); swipe(0xffffffffU); CHECK(sent == before);
    notify=false; swipe(0); CHECK(sent == before); notify=true;
    cccd_length=1; swipe(0); CHECK(sent == before); cccd_length=2;
    cccd_error=7; swipe(0); CHECK(sent == before && factory_scroll_last_error==7); cccd_error=0;
    sec_error=9; swipe(0); CHECK(sent == before && factory_scroll_last_error==9); sec_error=0;
    security_level=1; swipe(0); CHECK(sent == before); security_level=2;
    hid_event(BLE_HIDS_EVT_HOST_SUSP); swipe(0); CHECK(sent == before); hid_event(BLE_HIDS_EVT_HOST_EXIT_SUSP);
    hid_event(BLE_HIDS_EVT_BOOT_MODE_ENTERED); swipe(0); CHECK(sent == before); hid_event(BLE_HIDS_EVT_REPORT_MODE_ENTERED);
    recording=true; swipe(0); CHECK(sent == before); recording=false;
    bool *blocks[]={&factory_ptt_diagnostics.wanted,&factory_ptt_diagnostics.owned,&factory_ptt_diagnostics.memo_owned,
                    &factory_ptt_diagnostics.toggle_pending,&factory_ptt_diagnostics.settings_lock};
    for (unsigned i=0;i<5;++i) { *blocks[i]=true; swipe(0); CHECK(sent==before); *blocks[i]=false; }
    app_factory_scroll_input(0); recording=true; app_factory_scroll_service(); CHECK(sent==before); recording=false;
    app_factory_scroll_input(0); enabled=0; app_factory_scroll_service(); CHECK(sent==before); enabled=15;
    app_factory_scroll_input(0); app_factory_scroll_reset(); app_factory_scroll_service(); CHECK(sent==before);
    app_factory_scroll_input(0); ble(BLE_GAP_EVT_DISCONNECTED,0); ble(BLE_GAP_EVT_CONNECTED,0);
    ble(BLE_GAP_EVT_CONN_SEC_UPDATE,0); app_factory_scroll_service(); CHECK(sent==before);
    app_factory_scroll_input(0); hid_event(BLE_HIDS_EVT_NOTIF_DISABLED); app_factory_scroll_service(); CHECK(sent==before);
    notify=false; app_factory_scroll_input(0); notify=true; hid_event(BLE_HIDS_EVT_NOTIF_ENABLED);
    app_factory_scroll_service(); CHECK(sent==before); /* no pre-subscription replay */
    app_factory_scroll_input(0); fixture_ticks += pdMS_TO_TICKS(251); app_factory_scroll_service(); CHECK(sent==before);
    fixture_ticks=0xfffffff0U; app_factory_scroll_input(1); fixture_ticks=10; app_factory_scroll_service(); CHECK(sent==before+1);
    before=sent;
    for(unsigned i=0;i<10;++i) app_factory_scroll_input(i%4);
    CHECK(count==4);
    for(unsigned i=0;i<10;++i) app_factory_scroll_service();
    CHECK(sent==before+4 && count==0);
    for(unsigned error=1;error<32;++error) {
        before=sent; send_error=error; swipe(0);
        CHECK(sent==before+1 && factory_scroll_last_error==error);
        send_error=0; app_factory_scroll_service(); CHECK(sent==before+1); /* no retry */
    }
    scroll_error(99); CHECK(factory_scroll_last_error==99);
    CHECK(critical==0 && scheduler_suspended==0 && app_irqs_masked==0);
    printf("PASS P09 scroll transport: %u checks\n",checks);
    return 0;
}
