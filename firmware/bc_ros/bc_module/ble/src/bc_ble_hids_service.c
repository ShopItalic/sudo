
#include "bc_ble_hids_service.h"
#include "app_timer.h"
#include "ble_hids.h"
#include "ble_bas.h"
#include "ble_dis.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "peer_manager_handler.h"

#include "nrf_delay.h"
#include "bc_logger.h"

#include "bc_ble_adv.h"
#include "ring_config.h"

enum phone_screen_type
{
	PHONE_TYPE_IPHONE15 = 0,
	PHONE_TYPE_IPHONE15_PRO,
	PHONE_TYPE_IPHONE15_PRO_MAX,
	PHONE_TYPE_NUM
};

struct phone_screen_size
{
	uint32_t phone_screen_high;
	uint32_t phone_screen_width;
	char  phone_type_name[40];
	uint8_t phone_type_name_length;
};

static struct phone_screen_size  screen_size = {0};

static struct ble_hid_mouse_button_cmd_data mouse_button_cmd_data[MOUSE_BUTTON_CMD_NUM] = {
	{
	  {1,0,0,0,0},
	   5,
	},
	{
	  {2,0,0,0,0},
	   5,
	},
	{
	  {0,0,0,0,0},
	   5,
	},
	{
	  {0,0,0,-127},
	   4,
	},
	{
	  {0,0,0,127},
	   4,
	},
	{
	  {0x10,0,00},
	   3,
	},
	{
	  {0x20,0,00},
	   3,
	}
};

#define MOVEMENT_SPEED                  5                                           /**< Number of pixels by which the cursor is moved each time a button is pushed. */
#define INPUT_REPORT_COUNT              4                                           /**< Number of input reports in this application. */
#define INPUT_REP_BUTTONS_LEN           13                                           /**< Length of Mouse Input Report containing button data. */
#define INPUT_REP_MOVEMENT_LEN          3                                           /**< Length of Mouse Input Report containing movement data. */
#define INPUT_REP_MEDIA_PLAYER_LEN      1                                           /**< Length of Mouse Input Report containing media player data. */
#define INPUT_REP_BUTTONS_INDEX         0                                           /**< Index of Mouse Input Report containing button data. */
#define INPUT_REP_MOVEMENT_INDEX        1                                           /**< Index of Mouse Input Report containing movement data. */
#define INPUT_REP_MPLAYER_INDEX         2                                           /**< Index of Mouse Input Report containing media player data. */
#define INPUT_REP_REF_BUTTONS_ID        1                                           /**< Id of reference to Mouse Input Report containing button data. */
#define INPUT_REP_REF_MOVEMENT_ID       2                                           /**< Id of reference to Mouse Input Report containing movement data. */
#define INPUT_REP_REF_MPLAYER_ID        3                                           /**< Id of reference to Mouse Input Report containing media player data. */

#define INPUT_REPORT_KEYS_MAX_LEN           8                                          /**< Maximum length of the Input Report characteristic. */
#define MAX_KEYS_IN_ONE_REPORT              (INPUT_REPORT_KEYS_MAX_LEN - SCAN_CODE_POS)/**< Maximum number of key presses that can be sent in one Input Report. */
#define OUTPUT_REPORT_INDEX                 0                                          /**< Index of Output Report. */
#define OUTPUT_REPORT_MAX_LEN               1                                          /**< Maximum length of Output Report. */
#define INPUT_REPORT_KEYS_INDEX             3                                          /**< Index of Input Report. */
#define INPUT_REP_REF_ID                    4                                         /**< Id of reference to Keyboard Input Report. */
#define OUTPUT_REP_REF_ID                   5                                          /**< Id of reference to Keyboard Output Report. */
#define FEATURE_REP_REF_ID                  6                                          /**< ID of reference to Keyboard Feature Report. */
#define FEATURE_REPORT_MAX_LEN              2                                          /**< Maximum length of Feature Report. */
#define FEATURE_REPORT_INDEX                0                                          /**< Index of Feature Report. */

#define BASE_USB_HID_SPEC_VERSION       0x0101                                      /**< Version number of base USB HID Specification implemented by this application. */


BLE_HIDS_DEF(m_hids,                                                                /**< HID service instance. */
             NRF_SDH_BLE_TOTAL_LINK_COUNT,
             INPUT_REP_BUTTONS_LEN,
             INPUT_REP_MOVEMENT_LEN,
             INPUT_REP_MEDIA_PLAYER_LEN,
			 INPUT_REPORT_KEYS_MAX_LEN
			 );
             
static bool              m_in_boot_mode = false;                                    /**< Current protocol mode. */
static uint16_t          *m_conn_handle  = NULL;                  /**< Handle of the current connection. */


/**@brief Function for handling HID events.
 *
 * @details This function will be called for all HID events which are passed to the application.
 *
 * @param[in]   p_hids  HID service structure.
 * @param[in]   p_evt   Event received from the HID service.
 */
static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_HIDS_EVT_BOOT_MODE_ENTERED:
            m_in_boot_mode = true;
            break;

        case BLE_HIDS_EVT_REPORT_MODE_ENTERED:
            m_in_boot_mode = false;
            break;

        case BLE_HIDS_EVT_NOTIF_ENABLED:
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing HID Service.
 */
void hids_init(uint16_t  *p)
{
	m_conn_handle = p;
    ret_code_t                err_code;
    ble_hids_init_t           hids_init_obj;
    ble_hids_inp_rep_init_t * p_input_report;
    ble_hids_outp_rep_init_t    * p_output_report;
    ble_hids_feature_rep_init_t * p_feature_report;
	
    uint8_t                   hid_info_flags;

    static ble_hids_inp_rep_init_t inp_rep_array[INPUT_REPORT_COUNT];
	
	static ble_hids_inp_rep_init_t     input_report_array[3];

    static ble_hids_outp_rep_init_t    output_report_array[1];
    static ble_hids_feature_rep_init_t feature_report_array[1];
	
    static uint8_t rep_map_data[] =
    {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x02, // Usage (Mouse)
		
//#if ( HARDWARE_1121_ENABLED == 1)	
//		0xA1, 0x01, // Collection (Application)
//        // Report ID 1: Mouse buttons + scroll/pan
//        0x85, 0x01,       // Report Id 1
//        0x09, 0x01,       // Usage (Pointer)
//        0xA1, 0x00,       // Collection (Physical)
//        0x95, 0x05,       // Report Count (3)
//        0x75, 0x01,       // Report Size (1)
//        0x05, 0x09,       // Usage Page (Buttons)
//        0x19, 0x01,       // Usage Minimum (01)
//        0x29, 0x05,       // Usage Maximum (05)
//        0x15, 0x00,       // Logical Minimum (0)
//        0x25, 0x01,       // Logical Maximum (1)
//        0x81, 0x02,       // Input (Data, Variable, Absolute)
//        0x95, 0x01,       // Report Count (1)
//        0x75, 0x03,       // Report Size (3)
//        0x81, 0x01,       // Input (Constant) for padding
//        0x75, 0x08,       // Report Size (8)
//        0x95, 0x01,       // Report Count (1)
//        0x05, 0x01,       // Usage Page (Generic Desktop)
//        0x09, 0x38,       // Usage (Wheel)
//        0x15, 0x81,       // Logical Minimum (-127)
//        0x25, 0x7F,       // Logical Maximum (127)
//        0x81, 0x06,       // Input (Data, Variable, Relative)
//        0x05, 0x0C,       // Usage Page (Consumer)
//        0x0A, 0x38, 0x02, // Usage (AC Pan)
//        0x95, 0x01,       // Report Count (1)
//        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)	
//#else
        0xA1, 0x01, // Collection (Application)
        // Report ID 1: Mouse buttons + scroll/pan
        0x85, 0x01,       // Report Id 1
        0x09, 0x01,       // Usage (Pointer)
        0xA1, 0x00,       // Collection (Physical)
        0x95, 0x05,       // Report Count (3)
        0x75, 0x01,       // Report Size (1)
        0x05, 0x09,       // Usage Page (Buttons)
        0x19, 0x01,       // Usage Minimum (01)
        0x29, 0x05,       // Usage Maximum (05)
        0x15, 0x00,       // Logical Minimum (0)
        0x25, 0x01,       // Logical Maximum (1)
		0x16,0x01,0xf8, // Global Logical Min
		0x26,0xff,0x07, // Global Logical Max
		0x09,0x30, // Local X
		0x09,0x31, // Local Y
        0x81, 0x02,       // Input (Data, Variable, Absolute)
        0x95, 0x01,       // Report Count (1)
        0x75, 0x03,       // Report Size (3)
        0x81, 0x01,       // Input (Constant) for padding
        0x75, 0x08,       // Report Size (8)
        0x95, 0x01,       // Report Count (1)
        0x05, 0x01,       // Usage Page (Generic Desktop)
        0x09, 0x38,       // Usage (Wheel)
		0x09,0x30, // Local X
		0x09,0x31, // Local Y
        0x15, 0x81,       // Logical Minimum (-127)
        0x25, 0x7F,       // Logical Maximum (127)
        0x81, 0x06,       // Input (Data, Variable, Relative)
        0x05, 0x0C,       // Usage Page (Consumer)
        0x0A, 0x38, 0x02, // Usage (AC Pan)
        0x95, 0x01,       // Report Count (1)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
//#endif			
        	

	

        0xC0,             // End Collection (Physical)



        // Report ID 2: Mouse motion
        0x85, 0x02,       // Report Id 2
        0x09, 0x01,       // Usage (Pointer)
        0xA1, 0x00,       // Collection (Physical)
        0x75, 0x0C,       // Report Size (12)
        0x95, 0x02,       // Report Count (2)
        0x05, 0x01,       // Usage Page (Generic Desktop)
        0x09, 0x30,       // Usage (X)
        0x09, 0x31,       // Usage (Y)
        0x16, 0x01, 0xF8, // Logical maximum (2047)
        0x26, 0xFF, 0x07, // Logical minimum (-2047)
        0x81, 0x06,       // Input (Data, Variable, Relative)
        0xC0,             // End Collection (Physical)
        0xC0,             // End Collection (Application)

        // Report ID 3: Advanced buttons
        0x05, 0x0C,       // Usage Page (Consumer)
        0x09, 0x01,       // Usage (Consumer Control)
        0xA1, 0x01,       // Collection (Application)
        0x85, 0x03,       // Report Id (3)
        0x15, 0x00,       // Logical minimum (0)
        0x25, 0x01,       // Logical maximum (1)
        0x75, 0x01,       // Report Size (1)
        0x95, 0x01,       // Report Count (1)

        0x09, 0xCD,       // Usage (Play/Pause)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x0A, 0x83, 0x01, // Usage (AL Consumer Control Configuration)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x09, 0xB5,       // Usage (Scan Next Track)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x09, 0xB6,       // Usage (Scan Previous Track)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)

        0x09, 0xEA,       // Usage (Volume Down)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x09, 0xE9,       // Usage (Volume Up)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x0A, 0x25, 0x02, // Usage (AC Forward)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x0A, 0x24, 0x02, // Usage (AC Back)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0xC0,              // End Collection

 // 添加键盘报告描述符部分
        0x05, 0x01,       // Usage Page (Generic Desktop)
        0x09, 0x06,       // Usage (Keyboard)
        0xA1, 0x01,       // Collection (Application)
        0x85, 0x04,       // Report ID (4)
        0x05, 0x07,       // Usage Page (Keyboard)
        0x19, 0xE0,       // Usage Minimum (Keyboard LeftControl)
        0x29, 0xE7,       // Usage Maximum (Keyboard Right GUI)
        0x15, 0x00,       // Logical Minimum (0)
        0x25, 0x01,       // Logical Maximum (1)
        0x75, 0x01,       // Report Size (1)
        0x95, 0x08,       // Report Count (8)
        0x81, 0x02,       // Input (Data, Variable, Absolute) - Modifier byte
        0x95, 0x01,       // Report Count (1)
        0x75, 0x08,       // Report Size (8)
        0x81, 0x01,       // Input (Constant) - Reserved byte
        0x95, 0x06,       // Report Count (6)
        0x75, 0x08,       // Report Size (8)
        0x15, 0x00,       // Logical Minimum (0)
        0x25, 0x65,       // Logical Maximum (101) - Keyboard usage max
        0x05, 0x07,       // Usage Page (Keyboard)
        0x19, 0x00,       // Usage Minimum (0)
        0x29, 0x65,       // Usage Maximum (101)
        0x81, 0x00,       // Input (Data, Array)
        0xC0              // End Collection


    };

	 memset((void *)input_report_array, 0, sizeof(ble_hids_inp_rep_init_t));
    memset((void *)output_report_array, 0, sizeof(ble_hids_outp_rep_init_t));
    memset((void *)feature_report_array, 0, sizeof(ble_hids_feature_rep_init_t)); 
	
    memset(inp_rep_array, 0, sizeof(inp_rep_array));
    // Initialize HID Service.
    p_input_report                      = &inp_rep_array[INPUT_REP_BUTTONS_INDEX];
    p_input_report->max_len             = INPUT_REP_BUTTONS_LEN;
    p_input_report->rep_ref.report_id   = INPUT_REP_REF_BUTTONS_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
    p_input_report->sec.wr      = SEC_JUST_WORKS;
    p_input_report->sec.rd      = SEC_JUST_WORKS;

    p_input_report                      = &inp_rep_array[INPUT_REP_MOVEMENT_INDEX];
    p_input_report->max_len             = INPUT_REP_MOVEMENT_LEN;
    p_input_report->rep_ref.report_id   = INPUT_REP_REF_MOVEMENT_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
    p_input_report->sec.wr      = SEC_JUST_WORKS;
    p_input_report->sec.rd      = SEC_JUST_WORKS;

    p_input_report                      = &inp_rep_array[INPUT_REP_MPLAYER_INDEX];
    p_input_report->max_len             = INPUT_REP_MEDIA_PLAYER_LEN;
    p_input_report->rep_ref.report_id   = INPUT_REP_REF_MPLAYER_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
    p_input_report->sec.wr      = SEC_JUST_WORKS;
    p_input_report->sec.rd      = SEC_JUST_WORKS;
	
	 // Initialize HID Service
    p_input_report                      = &inp_rep_array[INPUT_REPORT_KEYS_INDEX];
    p_input_report->max_len             = INPUT_REPORT_KEYS_MAX_LEN;
    p_input_report->rep_ref.report_id   = INPUT_REP_REF_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    p_input_report->sec.cccd_wr = SEC_JUST_WORKS;
    p_input_report->sec.wr      = SEC_JUST_WORKS;
    p_input_report->sec.rd      = SEC_JUST_WORKS;

//    p_output_report                      = &output_report_array[OUTPUT_REPORT_INDEX];
//    p_output_report->max_len             = OUTPUT_REPORT_MAX_LEN;
//    p_output_report->rep_ref.report_id   = OUTPUT_REP_REF_ID;
//    p_output_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_OUTPUT;

//    p_output_report->sec.wr = SEC_JUST_WORKS;
//    p_output_report->sec.rd = SEC_JUST_WORKS;

//    p_feature_report                      = &feature_report_array[FEATURE_REPORT_INDEX];
//    p_feature_report->max_len             = FEATURE_REPORT_MAX_LEN;
//    p_feature_report->rep_ref.report_id   = FEATURE_REP_REF_ID;
//    p_feature_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_FEATURE;

//    p_feature_report->sec.rd              = SEC_JUST_WORKS;
//    p_feature_report->sec.wr              = SEC_JUST_WORKS;

	

    hid_info_flags = HID_INFO_FLAG_REMOTE_WAKE_MSK | HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK;

    memset(&hids_init_obj, 0, sizeof(hids_init_obj));

    hids_init_obj.evt_handler                    = on_hids_evt;
    hids_init_obj.error_handler                  = service_error_handler;
    hids_init_obj.is_kb                          = false;
    hids_init_obj.is_mouse                       = true;
    hids_init_obj.inp_rep_count                  = INPUT_REPORT_COUNT;
    hids_init_obj.p_inp_rep_array                = inp_rep_array;
    hids_init_obj.outp_rep_count                 = 0;
    hids_init_obj.p_outp_rep_array               = NULL;
    hids_init_obj.feature_rep_count              = 0;
    hids_init_obj.p_feature_rep_array            = NULL;
    hids_init_obj.rep_map.data_len               = sizeof(rep_map_data);
    hids_init_obj.rep_map.p_data                 = rep_map_data;
    hids_init_obj.hid_information.bcd_hid        = BASE_USB_HID_SPEC_VERSION;
    hids_init_obj.hid_information.b_country_code = 0;
    hids_init_obj.hid_information.flags          = hid_info_flags;
    hids_init_obj.included_services_count        = 0;
    hids_init_obj.p_included_services_array      = NULL;

    hids_init_obj.rep_map.rd_sec         = SEC_JUST_WORKS;
    hids_init_obj.hid_information.rd_sec = SEC_JUST_WORKS;

    hids_init_obj.boot_mouse_inp_rep_sec.cccd_wr = SEC_JUST_WORKS;
    hids_init_obj.boot_mouse_inp_rep_sec.wr      = SEC_JUST_WORKS;
    hids_init_obj.boot_mouse_inp_rep_sec.rd      = SEC_JUST_WORKS;

    hids_init_obj.protocol_mode_rd_sec = SEC_JUST_WORKS;
    hids_init_obj.protocol_mode_wr_sec = SEC_JUST_WORKS;
    hids_init_obj.ctrl_point_wr_sec    = SEC_JUST_WORKS;

    err_code = ble_hids_init(&m_hids, &hids_init_obj);
//    APP_ERROR_CHECK(err_code);
}

//发送按键
void mouse_button_send(int8_t click, int8_t wheel, int8_t pan)
{
    ret_code_t err_code;
    if(*m_conn_handle == 0xFFFF)
    {
        return;
    }
	if(*m_conn_handle == NULL)
    {
        return;
    }

	uint8_t buffer[INPUT_REP_BUTTONS_LEN];

	APP_ERROR_CHECK_BOOL(INPUT_REP_BUTTONS_LEN == 3);

	buffer[0] = click;
	buffer[1] = wheel;
	buffer[2] = wheel;

	err_code = ble_hids_inp_rep_send(&m_hids,
									 INPUT_REP_BUTTONS_INDEX,
									 INPUT_REP_BUTTONS_LEN,
									 buffer,
									 *m_conn_handle);
    

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   x_delta   Horizontal movement.
 * @param[in]   y_delta   Vertical movement.
 */


void mouse_movement_send(int16_t x_delta, int16_t y_delta)
{
//	 ble_mouse_movement_slide( x_delta, y_delta);
	
    ret_code_t err_code;

    if(*m_conn_handle == 0xFFFF)
    {
        return;
    }
//	if(*m_conn_handle == NULL)
//    {
//        return;
//    }
    if (m_in_boot_mode)
    {
        x_delta = MIN(x_delta, 0x00ff);
        y_delta = MIN(y_delta, 0x00ff);

        err_code = ble_hids_boot_mouse_inp_rep_send(&m_hids,
                                                    0x00,
                                                    (int8_t)x_delta,
                                                    (int8_t)y_delta,
                                                    0,
                                                    NULL,
                                                    *m_conn_handle);
		BC_LOG_INFO("ble_hids_boot_mouse_inp_rep_send %d \r\n",err_code);
    }
    else
    {
        uint8_t buffer[INPUT_REP_MOVEMENT_LEN];

        APP_ERROR_CHECK_BOOL(INPUT_REP_MOVEMENT_LEN == 3);

		
        x_delta = MIN(x_delta, 0x0fff);
        y_delta = MIN(y_delta, 0x0fff);

        buffer[0] = x_delta & 0x00ff;
        buffer[1] = ((y_delta & 0x000f) << 4) | ((x_delta & 0x0f00) >> 8);
        buffer[2] = (y_delta & 0x0ff0) >> 4;

        err_code = ble_hids_inp_rep_send(&m_hids,
                                         INPUT_REP_MOVEMENT_INDEX,
                                         INPUT_REP_MOVEMENT_LEN,
                                         buffer,
                                         *m_conn_handle);
		BC_LOG_INFO("ble_hids_inp_rep_send %d \r\n",err_code);
    }

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}


void send_keyboard_hid_report(uint8_t key)
{
    uint8_t report[INPUT_REPORT_KEYS_MAX_LEN] = {0};

    // 将key存放在键位数组的第一个位置
    report[2] = key;

    ret_code_t err_code = ble_hids_inp_rep_send(&m_hids,
                                                INPUT_REPORT_KEYS_INDEX,
                                                sizeof(report),
                                                report,
                                                *m_conn_handle);
	BC_LOG_INFO("ble_hids_inp_rep_send %d  %d  %d\r\n",report[0],report[1],report[2]);												
    if (err_code != NRF_ERROR_INVALID_STATE)
    {
		BC_LOG_INFO("lltthhhhhhhhhhhhtl  %d \r\n",err_code);
//        APP_ERROR_CHECK(err_code);
    }
}

void send_keyboard_pgup()
{
    send_keyboard_hid_report(0x4B);  // 发送Page Up键按下事件
    send_keyboard_hid_report(0x00);  // 发送按键释放事件
	
//	send_keyboard_hid_report(0x52);  // 发送方向上
//    send_keyboard_hid_report(0x00);  // 发送按键释放事件
}

void send_keyboard_pgdn()
{
    send_keyboard_hid_report(0x4E);  // 发送Page Down键按下事件
    send_keyboard_hid_report(0x00);  // 发送按键释放事件
	
//	 send_keyboard_hid_report(0x51);  // 发送方向下
//    send_keyboard_hid_report(0x00);  // 发送按键释放事件
}

//    data[0] = 0xaa;
//    data[1] = 0x21;
//    data[2] = 0x00;
//    data[3] = 0x00;
//    data[4] = 0x01;
//    data[5] = 0x00;
//    data[6] = 0x01;
//    data[7] = 0x10;
//    data[8] = 0x00;
//    data[9] = 0x10;
//    data[10] = 0x00;
//    data[11] = 0x0;
//    data[12] = 0x0;

//第0个字节为报告的ID,这里为0xaa
//第1个节字data[0]，第0位为判断是否触摸位。我置1
//第1个节字，每三位为IN Range位，判断是否有z轴。我置0。
//第1个节字，每四位为信心位，判断对触摸的确定度，我置1。
//第1个节字其它位为常数0。

//每2个节字，data[1]。为常数位。我置0。
//第3，4节字data[2]=0x00,data[3]=0x01。为X轴，坐标。
//第5，6节字data[4]=0x00，data[5]=0x01。为Y轴，坐标。
//第7，8节字data[6]=0x10,data[7]=0x00。为触摸宽度。
//第9.10节字data[8]=0x10,data[9]=0x00。为触摸高度。
//第11.12节字data[10]=0x00,data[11]=0x00,为常数0

uint16_t map_to_hid_coord(uint16_t coord, uint16_t max_coord, uint16_t hid_max) {
    return (uint16_t)((coord * hid_max) / max_coord);
}



void send_touch_event(uint16_t x, uint16_t y, bool touch) {
	
	if(*m_conn_handle == NULL)
    {
        return;
    }
	ret_code_t err_code;
    uint8_t report_data[5];

    // 填充触摸事件的数据，0x01 表示按下，0x00 表示松开
    report_data[0] = touch ? 0x01 : 0x00;
    report_data[1] = x & 0xFF;           // X 低字节
    report_data[2] = (x >> 8) & 0xFF;    // X 高字节
    report_data[3] = y & 0xFF;           // Y 低字节
    report_data[4] = (y >> 8) & 0xFF;    // Y 高字节


    	err_code = ble_hids_inp_rep_send(&m_hids,
									 INPUT_REP_BUTTONS_INDEX,
									 sizeof(report_data),
									 report_data,
									 *m_conn_handle);
    BC_LOG_INFO("lltttttttl  %d \r\n",err_code);


    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

void process_touch_event(uint16_t touch_x, uint16_t touch_y, uint16_t screen_width, uint16_t screen_height, bool touch) {
    uint16_t hid_x = map_to_hid_coord(touch_x, screen_width, 32767);  // 映射 X 坐标
    uint16_t hid_y = map_to_hid_coord(touch_y, screen_height, 32767); // 映射 Y 坐标

    send_touch_event(hid_x, hid_y, touch);  // 发送触摸事件
}

static void send_gamepad_report( uint8_t buttons, int8_t x, int8_t y, int8_t z, int8_t rz)
{
    uint32_t err_code;
    uint8_t report[6];
    if(*m_conn_handle == NULL)
    {
        return;
    }
    // Populate the report with button and axis data
    report[0] = buttons & 0xFF;           // Lower 8 buttons
    report[1] = (buttons >> 8) & 0xFF;    // Upper 8 buttons
    report[2] = (uint8_t)x;               // X axis
    report[3] = (uint8_t)y;               // Y axis
    report[4] = (uint8_t)z;               // Z axis
    report[5] = (uint8_t)rz;              // Rz axis

      	err_code = ble_hids_inp_rep_send(&m_hids,
									 INPUT_REP_BUTTONS_INDEX,
									 sizeof(report),
									 report,
									 *m_conn_handle);
    BC_LOG_INFO("lltttttttl  %d \r\n",err_code);


    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

void update_joystick_state(int8_t x, int8_t y)
{
    // 假设按钮未按下
    uint8_t buttons = 0x0000;  // 没有按钮按下
    int8_t z = 0;   // Z 轴初始值
    int8_t rz = 0;  // Rz 轴初始值

    // 调用发送函数，传递当前的摇杆状态
    send_gamepad_report( buttons, x, y, z, rz);
}

void joystick_commands_example(void)
{
    // 摇杆居中
    update_joystick_state(0, 0);

    // 延时 (伪代码，用于表示时间间隔)
    nrf_delay_ms(100);

    // 摇杆向上
    update_joystick_state(0, 127);
    nrf_delay_ms(100);

    // 摇杆向下
    update_joystick_state(0, -127);
    nrf_delay_ms(100);

    // 摇杆向左
    update_joystick_state(-127, 0);
    nrf_delay_ms(100);

    // 摇杆向右
    update_joystick_state(127, 0);
    nrf_delay_ms(100);

    // 摇杆向右上
    update_joystick_state(127, 127);
    nrf_delay_ms(100);

    // 摇杆向左下
    update_joystick_state(-127, -127);
    nrf_delay_ms(100);
}

static void ble_mouse_button_send(uint8_t *data,uint8_t length)
{
//	if(*m_conn_handle == NULL)
//    {
//        return;
//    }
	ret_code_t err_code;
		err_code = ble_hids_inp_rep_send(&m_hids,
									 INPUT_REP_BUTTONS_INDEX,
									 length,
									 data,
									 *m_conn_handle);
    BC_LOG_INFO("lltttttttl  %d  \r\n",err_code);

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

void ble_mouse_button_control(enum ble_hid_mouse_button_cmd mouse_button_cmd)
{
	uint16_t ios_delay_time = 25;
	uint16_t android_delay_time = 20  ;
	ret_code_t err_code;
    if(*m_conn_handle == 0xFFFF)
    {
        return;
    }
	
//	if(*m_conn_handle == NULL)
//    {
//        return;
//    }
	 BC_LOG_INFO("llrrrr  %d \r\n",mouse_button_cmd);
	switch(mouse_button_cmd)
	{
		case MOUSE_LEFT_BUTTON_HOLD:
		{
			uint8_t data[7] = {01,0x00,0x00,0x00,0x00,0x00};
			data[0] = 1;
			data[1] = 0x0;
			data[2] = 0x0;
			ble_mouse_button_send(data,3);
			nrf_delay_ms(50);
			data[0] = 0;
			data[1] = 0x0;
			data[2] = 0x0;
			ble_mouse_button_send(data,3);
			return;
		}
		case MOUSE_RIGHT_BUTTON_HOLD:
		{
			uint8_t data[7] = {01,0x00,0x00,0x00,0x00,0x00};
			data[0] = 2;
			data[1] = 0x0;
			data[2] = 0x0;
			ble_mouse_button_send(data,3);
			nrf_delay_ms(50);
			data[0] = 0;
			data[1] = 0x0;
			data[2] = 0x0;
			ble_mouse_button_send(data,3);
			return;
		}
		case MOUSE_PULLEY_UP:
		{
			send_keyboard_pgup();
			return;
		}
		case MOUSE_PULLEY_DOWN:
		{
			send_keyboard_pgdn();
			return;
		}
		case MOUSE_SLIDE_UP:
		{
			send_keyboard_pgup();
			return;
		}
		case MOUSE_SLIDE_DOWN:
		{
			send_keyboard_pgdn();
			return;
		}
		case MOUSE_ANDROID_PULLEY_UP:
		{
			uint8_t data[7] = {01,0x00,0x00,0x00,0x00,0x00};
//		
			mouse_movement_send(-2047,0);
			nrf_delay_ms(20);
			mouse_movement_send(300,0);
			nrf_delay_ms(android_delay_time);
			data[0] = 0;
			data[1] = -20;
			data[2] = 0x0;
			ble_mouse_button_send(data,3);
			nrf_delay_ms(android_delay_time);
			mouse_movement_send(-2047,0);
			nrf_delay_ms(android_delay_time);
			mouse_movement_send(-2047,0);
//			nrf_delay_ms(android_delay_time);
//			mouse_movement_send(-2047,0);
			

			 BC_LOG_INFO("lltttttttl kkkkkkkkkkk  \r\n");
			
			
			return;
		}
		case MOUSE_ANDROID_PULLEY_DOWN:
		{
			uint8_t data[7] = {01,0x00,0x00,0x00,0x00,0x00};
		
			mouse_movement_send(-2047,0);
			nrf_delay_ms(20);
			mouse_movement_send(300,0);
			nrf_delay_ms(android_delay_time);
			data[0] = 0;
			data[1] = 120;
			data[2] = 0x0;
			ble_mouse_button_send(data,3);
			nrf_delay_ms(android_delay_time);
			mouse_movement_send(-2047,0);
			nrf_delay_ms(android_delay_time);
			mouse_movement_send(-2047,0);
//			nrf_delay_ms(android_delay_time);
//			mouse_movement_send(-2047,0);
			return;
		}
		case MOUSE_IOS_PULLEY_UP:
		{
			uint8_t data[7] = {01,0x00,0x00,0x00,0x00,0x00};
			
			int16_t  screen_y = 0;
			int16_t  screen_temp = 0;
			screen_y = (screen_size.phone_screen_high / 3) /120;
			if(screen_y %2 == 0)
			{
				screen_temp = screen_y /2;
			}
			else
			{
				screen_temp = (screen_y /2) + 1;
			}	
			
//			for(uint8_t i = 0; i < screen_temp ;i++)
//			{
//				mouse_movement_send(-127,127);
//			    nrf_delay_ms(ios_delay_time);
//			}
			
			for(uint8_t i = 0; i < (screen_y/3) ;i++)
			{
				mouse_movement_send(-127,-120);
			    nrf_delay_ms(ios_delay_time);
			}
			if(screen_y  % 3 != 0)
			{
				mouse_movement_send(-127,-120);
				nrf_delay_ms(ios_delay_time);
			}

			
			data[0] = 1;
			data[1] = 0x0;
			data[2] = 0x0;
			ble_mouse_button_send(data,3); 
			nrf_delay_ms(ios_delay_time);
			for(uint8_t i = 0; i < (screen_y/3) ;i++)
			{
				mouse_movement_send(0,-120);
			    nrf_delay_ms(ios_delay_time);
			}
			if(screen_y  % 3 != 0)
			{
				mouse_movement_send(0,-120);
				nrf_delay_ms(ios_delay_time);
			}
			
			data[0] = 0;
			data[1] = 0x0;
			data[2] = 0x0;
			nrf_delay_ms(ios_delay_time);
			ble_mouse_button_send(data,3);
			
			for(uint8_t i = 0; i < screen_y ;i++)
			{
				mouse_movement_send(-127,120);
			    nrf_delay_ms(13);
			}


			return;
		}
		case MOUSE_IOS_PULLEY_DOWN:
		{
			uint8_t data[7] = {01,0x00,0x00,0x00,0x00,0x00};
            
			int16_t  screen_y = 0;
			int16_t  screen_temp = 0;
			screen_y = (screen_size.phone_screen_high / 3) /120;
			if(screen_y %2 == 0)
			{
				screen_temp = screen_y /2;
			}
			else
			{
				screen_temp = (screen_y /2) + 1;
			}			
			
			for(uint8_t i = 0; i < screen_temp ;i++)
			{
				mouse_movement_send(-127,127);
			    nrf_delay_ms(ios_delay_time);
			}
			
//			for(uint8_t i = 0; i < 5 ;i++)
//			{
//				mouse_movement_send(0,120);
//			    nrf_delay_ms(ios_delay_time);
//			}
			if(((screen_y / 2) *3) >= 3)
			{
				for(uint8_t i = 0; i < 3 ;i++)
				{
					mouse_movement_send(-127,-120);
					nrf_delay_ms(ios_delay_time);
				}
			}
			else
			{
				for(uint8_t i = 0; i < (screen_y / 2) *3 ;i++)
				{
					mouse_movement_send(-127,-120);
					nrf_delay_ms(ios_delay_time);
				}
		    } 
			
			data[0] = 1;
			data[1] = 0x0;
			data[2] = 0x0;
			nrf_delay_ms(ios_delay_time);
			ble_mouse_button_send(data,3); 
			for(uint8_t i = 0; i < screen_temp ;i++)
			{
				mouse_movement_send(0,120);
			    nrf_delay_ms(ios_delay_time);
			}
			
			data[0] = 0;
			data[1] = 0x0;
			data[2] = 0x0;
			nrf_delay_ms(ios_delay_time);
			ble_mouse_button_send(data,3);
			nrf_delay_ms(ios_delay_time);
			for(uint8_t i = 0; i < screen_y ;i++)
			{
				mouse_movement_send(-127,120);
			    nrf_delay_ms(13);
			}
			
			
			
			return;
		}
		default:
		{
			break;
		}
		
	}
		

	err_code = ble_hids_inp_rep_send(&m_hids,
									 INPUT_REP_BUTTONS_INDEX,
									 mouse_button_cmd_data[mouse_button_cmd].ouse_cmd_data_length,
									 mouse_button_cmd_data[mouse_button_cmd].ouse_cmd_data,
									 *m_conn_handle);
    BC_LOG_INFO("lltttttttl  %d  %d %d %d %d \r\n",err_code,mouse_button_cmd_data[mouse_button_cmd].ouse_cmd_data[0],
	mouse_button_cmd_data[mouse_button_cmd].ouse_cmd_data[1],
	mouse_button_cmd_data[mouse_button_cmd].ouse_cmd_data[2],
	mouse_button_cmd_data[mouse_button_cmd].ouse_cmd_data[3]);

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}


static void media_player_control(uint8_t control)
{
	if(*m_conn_handle == 0xFFFF)
    {
        return;
    }
	uint32_t err_code;

	uint8_t buffer[1];

	buffer[0] = control;

	err_code = ble_hids_inp_rep_send( &m_hids,

	  INPUT_REP_MPLAYER_INDEX,

	  1,

	  buffer,*m_conn_handle);

	if(( err_code != NRF_SUCCESS ) &&

	   ( err_code != NRF_ERROR_INVALID_STATE ) &&


	   ( err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING ))

	{

	APP_ERROR_HANDLER( err_code );

	}
	nrf_delay_ms(200);
	buffer[0] = 0;

	err_code = ble_hids_inp_rep_send( &m_hids,

	 INPUT_REP_MPLAYER_INDEX,

	1,

	buffer,*m_conn_handle);

	if(( err_code != NRF_SUCCESS ) &&

	   ( err_code != NRF_ERROR_INVALID_STATE ) &&


	   ( err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING ))

	   {

		APP_ERROR_HANDLER( err_code );

	   }

}

void ble_hid_phone_screen_set(uint32_t phone_screen_high,uint32_t phone_screen_width,char *phone_type_name,uint8_t phone_type_name_length)
{
	screen_size.phone_screen_high = phone_screen_high;
	screen_size.phone_screen_width = phone_screen_width;
	screen_size.phone_type_name_length = phone_type_name_length;
	memcpy((uint8_t*)screen_size.phone_type_name,phone_type_name,phone_type_name_length);
}

void ble_hid_phone_screen_get(uint32_t *phone_screen_high,uint32_t *phone_screen_width,char *phone_type_name,uint8_t *phone_type_name_length)
{
	*phone_screen_high = screen_size.phone_screen_high ;
	*phone_screen_width = screen_size.phone_screen_width;
	*phone_type_name_length = screen_size.phone_type_name_length;
	memcpy((uint8_t*)phone_type_name,(uint8_t*)screen_size.phone_type_name,screen_size.phone_type_name_length);
}

void ble_hid_send_cmd(enum ble_hid_cmd  hid_cmd)
{

	switch(hid_cmd)
	{
		case BLE_HID_VOLUSE_UP:
		{
			media_player_control(0x20);
			break;
		}
		case BLE_HID_VOLUSE_DOWN:
		{
			media_player_control(0x10);
			break;
		}
		case BLE_HID_PREVIOUS_MUSIC:
		{
			media_player_control(0x08);
			break;
		}
		case BLE_HID_NEXT_MUSIC:
		{
			media_player_control(0x04);
			break;
		}
	}
	
}

