#include "ring_config.h"


#include <stdint.h>

#include "stdbool.h"

typedef enum
{
    PPG_SUCCESS = 0,
    PPG_FAILD,
} ppg_result;


bool bc_ppg_init(void);

bool bc_ppg_init_hr(void);

bool bc_ppg_init_hrv(void);

bool bc_ppg_init_spo2(void);

bool bc_ppg_init_rawdata_collection(uint16_t frequency);

bool bc_ppg_init_ecg(void);

bool bc_ppg_init_bt(void);

bool bc_ppg_init_pwtt(void);

void bc_ppg_io_irq_handler(void);

void bc_ppg_data_handler_poll(void);

bool bc_ppg_ecg_data_callback_regdister(void *function_callback);

bool bc_ppg_hr_data_callback_regdister(void *function_callback);

bool bc_ppg_spo2_data_callback_regdister(void *function_callback);

bool bc_ppg_spo2_hr_data_callback_regdister(void *function_callback);

bool bc_ppg_pwtt_data_callback_regdister(void *function_callback);

bool bc_ppg_hr_result_callback_regdister(void *function_callback);

bool bc_ppg_hrv_result_callback_regdister(void *function_callback);

bool bc_ppg_spo2_result_callback_regdister(void *function_callback);

bool bc_ppg_spo2_signal_check_callback_regdister(void *function_callback);

bool bc_ppg_hr_signal_check_callback_regdister(void *function_callback);

bool bc_ppg_gary_card_callback_regdister(void *function_callback);

bool bc_ppg_g_sensor_callback_regdister(void *start_callback,void *stop_callback,void *read_callback);

void bc_ppg_unint(void);

bool bc_ppg_hardware_id_check(void);

bool bc_ppg_init_ir(void);

void bc_ppg_hr_unint(void);
	
void bc_ppg_spo2_unint(void);

void bc_ppg_rawdata_collection_unint(void);

void bc_ppg_gray_card_init(void);

void bc_ppg_gray_card_uninit(void);

void bc_ppg_ecg_unint(void);

void bc_ppg_bt_unint(void);

void bc_ppg_pwtt_unint(void);

bool bc_ppg_agc_comp_flg(void);

void bc_ppg_current_val_set(uint8_t CurrentVal_0,uint8_t CurrentVal_1,uint8_t CurrentVal_2);

#pragma pack (1)
struct bc_ppg_acc_data
{
	int16_t acc_x_data;
	int16_t acc_y_data;
	int16_t acc_z_data;
};
#pragma pack ()

struct bc_ppg_collection_pwtt_data
{
    uint32_t pwtt_data[20];
    uint8_t pwtt_length;
};

struct bc_ppg_collection_ecg_data
{
    uint32_t ecg_data[80];
    uint8_t ecg_length;
};



#pragma pack (1)
struct bc_ppg_collection_hr_data
{
	int32_t green_data[30];
	uint8_t green_length;
	struct bc_ppg_acc_data acc_data[16];
};
#pragma pack ()

#if (PPG_DEVIECE_TYPE == 0 || PPG_DEVIECE_TYPE == 4)   //hx 3605

#pragma pack (1)
struct bc_ppg_collection_spo2_data
{
	int32_t red_data[20];
	int32_t ir_data[20];
    uint8_t red_length;
	uint8_t ir_length;
	struct bc_ppg_acc_data acc_data[16];
};
#pragma pack ()

#pragma pack (1)
struct bc_ppg_collection_spo2_hr_data
{
	int32_t red_data[20];
	int32_t ir_data[20];
	int32_t gre_data[20];
	uint8_t red_length;
	uint8_t ir_length;
	uint8_t gre_length;
	struct bc_ppg_acc_data acc_data[16];
};
#pragma pack ()

#elif (PPG_DEVIECE_TYPE == 1)  // afe4403



#elif (PPG_DEVIECE_TYPE == 2 || PPG_DEVIECE_TYPE == 3)  // zspd4000

#pragma pack (1)
struct bc_ppg_collection_spo2_data
{
	int32_t red_data[20];
	int32_t ir_data[20];
	uint8_t red_length;
	uint8_t ir_length;
    struct bc_ppg_acc_data acc_data[16];
};
#pragma pack ()

#pragma pack (1)

struct bc_ppg_collection_spo2_hr_data
{
	int32_t red_data[20];
	int32_t ir_data[20];
	int32_t gre_data[20];
	uint8_t red_length;
	uint8_t ir_length;
	uint8_t gre_length;
	struct bc_ppg_acc_data acc_data[16];
};
#pragma pack ()

#endif


uint8_t bc_ppg_chip_id_get(void);


void bc_ppg_red_led_on(void);

void bc_ppg_gre_led_on(void);

void bc_ppg_ir_led_on(void);

void bc_ppg_led_off(void);
