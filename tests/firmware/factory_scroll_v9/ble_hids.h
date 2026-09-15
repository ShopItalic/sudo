#ifndef TEST_SCROLL_BLE_H
#define TEST_SCROLL_BLE_H
#include <stdint.h>
#include <stdbool.h>
#define NRF_SUCCESS 0U
#define NRF_ERROR_INVALID_STATE 8U
#define NRF_SDH_BLE_TOTAL_LINK_COUNT 1
#define BLE_CONN_HANDLE_INVALID 0xffffU
#define BLE_HIDS_REP_TYPE_INPUT 1
#define HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK 2
#define SEC_JUST_WORKS 2
#define SEC_NO_ACCESS 0
enum { BLE_GAP_EVT_CONNECTED, BLE_GAP_EVT_DISCONNECTED, BLE_GAP_EVT_CONN_SEC_UPDATE };
enum { BLE_HIDS_EVT_HOST_SUSP, BLE_HIDS_EVT_HOST_EXIT_SUSP, BLE_HIDS_EVT_BOOT_MODE_ENTERED,
       BLE_HIDS_EVT_REPORT_MODE_ENTERED, BLE_HIDS_EVT_NOTIF_DISABLED, BLE_HIDS_EVT_NOTIF_ENABLED };
typedef struct { uint8_t sm, lv; } test_sec_mode;
typedef struct { test_sec_mode sec_mode; } ble_gap_conn_sec_t;
typedef struct {
    struct { unsigned evt_id; } header;
    struct { struct { uint16_t conn_handle; struct { struct { ble_gap_conn_sec_t conn_sec; } conn_sec_update; } params; } gap_evt; } evt;
} ble_evt_t;
typedef struct { unsigned evt_type; } ble_hids_evt_t;
typedef struct { uint16_t len, offset; uint8_t *p_value; } ble_gatts_value_t;
typedef struct { unsigned rd, wr, cccd_wr; } test_security;
typedef struct { unsigned max_len; struct { unsigned report_id, report_type; } rep_ref; test_security sec; } ble_hids_inp_rep_init_t;
typedef struct { struct { uint16_t cccd_handle; } char_handles; } test_report;
typedef struct { test_report inp_rep_array[1]; } ble_hids_t;
typedef struct {
    void (*evt_handler)(ble_hids_t *, ble_hids_evt_t *);
    void (*error_handler)(uint32_t);
    bool is_mouse;
    unsigned inp_rep_count;
    const ble_hids_inp_rep_init_t *p_inp_rep_array;
    struct { uint8_t *p_data; unsigned data_len, rd_sec; } rep_map;
    struct { unsigned bcd_hid, flags, rd_sec; } hid_information;
    unsigned protocol_mode_rd_sec, protocol_mode_wr_sec, ctrl_point_wr_sec;
    test_security boot_mouse_inp_rep_sec;
} ble_hids_init_t;
#define BLE_HIDS_DEF(name, links, length) static ble_hids_t name
uint32_t ble_hids_init(ble_hids_t *, const ble_hids_init_t *);
uint32_t ble_hids_inp_rep_send(ble_hids_t *, uint8_t, uint16_t, uint8_t *, uint16_t);
uint32_t sd_ble_gap_conn_sec_get(uint16_t, ble_gap_conn_sec_t *);
uint32_t sd_ble_gatts_value_get(uint16_t, uint16_t, ble_gatts_value_t *);
#endif
