#include "app_factory_scroll.h"
#include "app_factory_controls.h"
#include "app_factory_ptt.h"
#include "app_pdm_handler.h"
#include "bc_rtos.h"
#include "ble_hids.h"
#include "nrf_sdh_ble.h"
#include "nrf_nvic.h"
#include <stdbool.h>
#include <string.h>

/* A standard relative mouse: buttons, X, Y, vertical Wheel, horizontal AC Pan.
 * X/Y and buttons are always zero. No keyboard, digitizer, screen coordinates,
 * long-running drag, or delayed button release is involved. Report ID is carried
 * by the GATT Report Reference descriptor, not prepended to the five-byte value.
 * USB HID Usage Tables: Generic Desktop Wheel 0x38; Consumer AC Pan 0x0238. */
static uint8_t scroll_report_map[] = {
    0x05,0x01, 0x09,0x02, 0xa1,0x01, 0x85,0x01,
    0x09,0x01, 0xa1,0x00,
    0x05,0x09, 0x19,0x01, 0x29,0x03, 0x15,0x00, 0x25,0x01,
    0x95,0x03, 0x75,0x01, 0x81,0x02,
    0x95,0x01, 0x75,0x05, 0x81,0x01,
    0x05,0x01, 0x09,0x30, 0x09,0x31, 0x09,0x38,
    0x15,0x81, 0x25,0x7f, 0x75,0x08, 0x95,0x03, 0x81,0x06,
    0x05,0x0c, 0x0a,0x38,0x02, 0x95,0x01, 0x81,0x06,
    0xc0,0xc0
};

BLE_HIDS_DEF(factory_scroll_hids, NRF_SDH_BLE_TOTAL_LINK_COUNT, FACTORY_SCROLL_REPORT_BYTES);
static volatile uint16_t connection = BLE_CONN_HANDLE_INVALID;
static volatile uint32_t epoch;
static volatile bool initialized, encrypted, suspended, boot_mode;
volatile uint32_t factory_scroll_init_error, factory_scroll_last_error;
volatile uint32_t factory_scroll_sent, factory_scroll_dropped;
typedef struct { uint32_t epoch, tick; unsigned direction; } scroll_intent;
static scroll_intent intents[FACTORY_SCROLL_QUEUE_SIZE];
static unsigned head, count;
static TaskHandle_t worker;

/* BLE callbacks only publish flags. Worker checks the actual CCCD as bonded
 * subscription restoration need not produce a fresh CCCD-write callback. */
static void scroll_ble_event(ble_evt_t const *event, void *context)
{
    (void)context;
    if (event->header.evt_id == BLE_GAP_EVT_CONNECTED) {
        connection = event->evt.gap_evt.conn_handle;
        encrypted = suspended = boot_mode = false;
        ++epoch;
    } else if (event->header.evt_id == BLE_GAP_EVT_DISCONNECTED &&
               event->evt.gap_evt.conn_handle == connection) {
        connection = BLE_CONN_HANDLE_INVALID;
        encrypted = false;
        ++epoch;
    } else if (event->header.evt_id == BLE_GAP_EVT_CONN_SEC_UPDATE &&
               event->evt.gap_evt.conn_handle == connection) {
        const ble_gap_conn_sec_t *sec = &event->evt.gap_evt.params.conn_sec_update.conn_sec;
        encrypted = sec->sec_mode.sm == 1U && sec->sec_mode.lv >= 2U;
        ++epoch;
    }
}
NRF_SDH_BLE_OBSERVER(factory_scroll_observer, 3, scroll_ble_event, NULL);

static void scroll_hid_event(ble_hids_t *hids, ble_hids_evt_t *event)
{
    (void)hids;
    switch (event->evt_type) {
    case BLE_HIDS_EVT_HOST_SUSP: suspended = true; ++epoch; break;
    case BLE_HIDS_EVT_HOST_EXIT_SUSP: suspended = false; ++epoch; break;
    case BLE_HIDS_EVT_BOOT_MODE_ENTERED: boot_mode = true; ++epoch; break;
    case BLE_HIDS_EVT_REPORT_MODE_ENTERED: boot_mode = false; ++epoch; break;
    case BLE_HIDS_EVT_NOTIF_ENABLED:
    case BLE_HIDS_EVT_NOTIF_DISABLED: ++epoch; break;
    default: break;
    }
}
static void scroll_error(uint32_t error)
{
    /* Optional scrolling must never reset the recorder on a BLE error. */
    factory_scroll_last_error = error;
}
void app_factory_scroll_init(void)
{
    ble_hids_init_t init;
    static ble_hids_inp_rep_init_t report;
    memset(&init, 0, sizeof(init));
    memset(&report, 0, sizeof(report));
    report.max_len = FACTORY_SCROLL_REPORT_BYTES;
    report.rep_ref.report_id = 1;
    report.rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;
    report.sec.cccd_wr = SEC_JUST_WORKS;
    report.sec.rd = SEC_JUST_WORKS;
    report.sec.wr = SEC_NO_ACCESS;
    init.evt_handler = scroll_hid_event;
    init.error_handler = scroll_error;
    init.is_mouse = true;
    init.inp_rep_count = 1;
    init.p_inp_rep_array = &report;
    init.rep_map.p_data = scroll_report_map;
    init.rep_map.data_len = sizeof(scroll_report_map);
    init.rep_map.rd_sec = SEC_JUST_WORKS;
    init.hid_information.bcd_hid = 0x0111;
    init.hid_information.flags = HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK;
    init.hid_information.rd_sec = SEC_JUST_WORKS;
    init.protocol_mode_rd_sec = SEC_JUST_WORKS;
    init.protocol_mode_wr_sec = SEC_JUST_WORKS;
    init.ctrl_point_wr_sec = SEC_JUST_WORKS;
    init.boot_mouse_inp_rep_sec.cccd_wr = SEC_JUST_WORKS;
    init.boot_mouse_inp_rep_sec.rd = SEC_JUST_WORKS;
    init.boot_mouse_inp_rep_sec.wr = SEC_NO_ACCESS;
    factory_scroll_init_error = ble_hids_init(&factory_scroll_hids, &init);
    initialized = factory_scroll_init_error == NRF_SUCCESS;
}
void app_factory_scroll_reset(void)
{
    taskENTER_CRITICAL();
    factory_scroll_dropped += count;
    head = count = 0;
    ++epoch;
    taskEXIT_CRITICAL();
}
static bool allowed(unsigned direction)
{
    return initialized && connection != BLE_CONN_HANDLE_INVALID && encrypted &&
        !suspended && !boot_mode && app_factory_controls_swipe_enabled(direction) &&
        !app_pdm_work_status() && !factory_ptt_diagnostics.wanted &&
        !factory_ptt_diagnostics.owned && !factory_ptt_diagnostics.memo_owned &&
        !factory_ptt_diagnostics.toggle_pending && !factory_ptt_diagnostics.settings_lock;
}
void app_factory_scroll_input(unsigned direction)
{
    TaskHandle_t wake = 0;
    taskENTER_CRITICAL();
    if (worker && allowed(direction) && count < FACTORY_SCROLL_QUEUE_SIZE) {
        scroll_intent *intent = &intents[(head + count) % FACTORY_SCROLL_QUEUE_SIZE];
        intent->direction = direction;
        intent->epoch = epoch;
        intent->tick = xTaskGetTickCount();
        ++count;
        wake = worker;
    } else ++factory_scroll_dropped;
    taskEXIT_CRITICAL();
    /* The factory worker otherwise sleeps for 1000 ticks between commands.
     * Wake after releasing the lock; its queue receive is bounded to 50 ms. */
    if (wake) xTaskNotifyGive(wake);
}
void app_factory_scroll_service(void)
{
    scroll_intent intent;
    uint8_t report[FACTORY_SCROLL_REPORT_BYTES] = {0};
    uint8_t cccd[2] = {0};
    uint8_t nested;
    ble_gatts_value_t value;
    ble_gap_conn_sec_t sec;
    uint32_t result;
    taskENTER_CRITICAL();
    worker = xTaskGetCurrentTaskHandle();
    if (!count) { taskEXIT_CRITICAL(); return; }
    intent = intents[head]; head = (head + 1U) % FACTORY_SCROLL_QUEUE_SIZE; --count;
    taskEXIT_CRITICAL();
    /* The linked factory profile dispatches BLE events in SD_EVT_IRQHandler.
     * Pause task switches AND use Nordic's NVIC region to defer application
     * IRQ callbacks while retaining SoftDevice interrupts and API SVC priority
     * 4. Factory taskENTER_CRITICAL uses BASEPRI 2 and cannot enclose SVC.
     * Revalidate after both guards, including changes since the queue pop. */
    vTaskSuspendAll();
    (void)sd_nvic_critical_region_enter(&nested);
    if (intent.epoch != epoch || !allowed(intent.direction) ||
        (uint32_t)(xTaskGetTickCount() - intent.tick) > pdMS_TO_TICKS(FACTORY_SCROLL_MAX_AGE_MS)) {
        ++factory_scroll_dropped;
        goto finish;
    }
    /* Swipe up moves down the document; left moves right across wide content.
     * The phone's natural-scrolling preference may invert the visual direction. */
    switch (intent.direction) {
    case FACTORY_SWIPE_UP: report[3] = (uint8_t)-FACTORY_SCROLL_STEP; break;
    case FACTORY_SWIPE_DOWN: report[3] = FACTORY_SCROLL_STEP; break;
    case FACTORY_SWIPE_LEFT: report[4] = FACTORY_SCROLL_STEP; break;
    case FACTORY_SWIPE_RIGHT: report[4] = (uint8_t)-FACTORY_SCROLL_STEP; break;
    default: ++factory_scroll_dropped; goto finish;
    }
    memset(&value, 0, sizeof(value)); value.len = sizeof(cccd); value.p_value = cccd;
    result = sd_ble_gap_conn_sec_get(connection, &sec);
    if (result == NRF_SUCCESS && sec.sec_mode.sm == 1U && sec.sec_mode.lv >= 2U) {
        result = sd_ble_gatts_value_get(connection,
            factory_scroll_hids.inp_rep_array[0].char_handles.cccd_handle, &value);
        if (result == NRF_SUCCESS && value.len == 2U && (cccd[0] & 1U))
            result = ble_hids_inp_rep_send(&factory_scroll_hids, 0, sizeof(report), report, connection);
        else if (result == NRF_SUCCESS) result = NRF_ERROR_INVALID_STATE;
    } else if (result == NRF_SUCCESS) result = NRF_ERROR_INVALID_STATE;
    /* Only bounded, nonblocking SVC calls run while task switches are paused.
     * Every attempt is consumed, including BUSY/RESOURCES. No reconnect replay. */
    factory_scroll_last_error = result;
    if (result == NRF_SUCCESS) ++factory_scroll_sent; else ++factory_scroll_dropped;
finish:
    (void)sd_nvic_critical_region_exit(nested);
    (void)xTaskResumeAll();
}
