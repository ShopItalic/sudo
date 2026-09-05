#include "app_opus.h"

#include "nrf_gpio.h"
#include "bc_rtos.h"
#include "bc_delay.h"

#include "q_device.h"

#include "app_ppg_data_handler.h"

#include "app_package.h"

#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_wifi_port.h"
#include "app_pdm_handler.h"

#endif	
#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_led_pwm.h"

#endif	


#if ( HARDWARE_1231_ENABLED == 1)	

#include "bc_touch_button.h"
#include "app_touch_button_handler.h"
#include "bc_linear_motor.h"
#include "tx1812n5.h"
#include "bc_ldo_switch.h"
#include "bc_ic_led.h"

#include "bc_led.h"
#include "bc_led_pwm.h"
#endif	

#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
#include "bc_linear_motor_ic.h"
#include "bc_linear_motor_ic_port.h"
#include "bc_fuel_gauge.h"
#include "bc_fuel_gauge_port.h"
#endif

#include "opus.h"
#include "opus_private.h"

#include "app_uart.h"
#include "nrf_uart.h"
#include "nrf_drv_uart.h"


static nrf_drv_uart_t app_uart_inst = NRF_DRV_UART_INSTANCE(APP_UART_DRIVER_INSTANCE);
                                                          

#define OPUS_ARM_ASM
                                                                  
#define LEN_ENCODER_STR             15600

OpusEncoder *enc=NULL;
uint8_t enc_a[LEN_ENCODER_STR] = {0};
                                                                  
void uart_init_cp(void)
{
    uint32_t err_code;
    nrf_drv_uart_config_t config = NRF_DRV_UART_DEFAULT_CONFIG;
    config.baudrate = NRF_UART_BAUDRATE_460800;
    config.hwfc = NRF_UART_HWFC_DISABLED;
    config.interrupt_priority = APP_IRQ_PRIORITY_LOWEST;
    config.parity = NRF_UART_PARITY_EXCLUDED;
    //config.pselcts = p_comm_params->cts_pin_no;
    //config.pselrts = p_comm_params->rts_pin_no;
    config.pselrxd = NRF_GPIO_PIN_MAP(0,3);
    config.pseltxd = NRF_GPIO_PIN_MAP(0,28);

    err_code = nrf_drv_uart_init(&app_uart_inst, &config, NULL);
    printf("nrf_drv_uart_init return :%d\r\n", err_code);
}

void uart_printf(uint8_t *data, uint8_t len)
{
    nrf_drv_uart_tx(&app_uart_inst, data, len);
}

#if defined(USE_OPUS)                                                                  
void opus_init_cp(void)
{
    int error;

    // 1. 获取编码器所需的内存大小
    int enc_size = opus_encoder_get_size(SAMPLE_CHANNELS);
    printf("enc_size:%d\r\n",enc_size);
    // 2. 建议在堆上申请，或定义一个静态全局数组给它
    // 比如：static uint8_t encoder_mem[enc_size];
    enc = (OpusEncoder *)bc_rtos_malloc(enc_size);
    //enc = (OpusEncoder *)enc_a;
    if (enc == NULL) {
        printf("Failed to allocate memory for Opus encoder\n");
        return;
    }

    // 3. 初始化编码器（指定为 VoIP 语音模式优化）
    error = opus_encoder_init(enc, SAMPLE_RATE, SAMPLE_CHANNELS, OPUS_APPLICATION_VOIP);
    if (error != OPUS_OK) {
        printf("Encoder init failed: %d\n", error);
        return;
    }

    // 4. 配置编码器参数
    opus_encoder_ctl(enc, OPUS_SET_BITRATE(BITRATE));
    opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(0));       // 复杂度设为 0 (最重要！MCU 算力有限)
    opus_encoder_ctl(enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE)); // 信号类型为语音
    opus_encoder_ctl(enc, OPUS_SET_VBR(0));              // 0: CBR (固定码率), 1: VBR (可变码率)
//    opus_encoder_ctl(enc, OPUS_SET_FORCE_MODE(MODE_SILK_ONLY));
//    opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(OPUS_AUTO));
//    opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(SAMPLE_CHANNELS));

    printf("Opus Encoder initialized successfully.\n");
    
}

int start_opus_encode(int16_t *in, int frame_size, uint8_t *data, int max_payload_bytes)
{
    int len = -99;
    BC_LOG_INFO("start opus_encode\r\n");
    if(enc) {
        //taskENTER_CRITICAL();
        //opus_encoder_ctl(enc, OPUS_SET_FORCE_MODE(MODE_CELT_ONLY));
        #if 0
        len = opus_encode(enc, in, frame_size, data, max_payload_bytes);
        #else
        len = opus_encode(enc, in, frame_size, &data[8], max_payload_bytes);
        if(len > 0) {
            opus_uint32 enc_final_range;
            data[0] = len>>24;
            data[1] = (len>>16)&0xFF;
            data[2] = (len>>8)&0xFF;
            data[3] = len&0xFF;
            opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range));
            data[4] = enc_final_range>>24;
            data[5] = (enc_final_range>>16)&0xFF;
            data[6] = (enc_final_range>>8)&0xFF;
            data[7] = enc_final_range&0xFF;
            len += 8;
        }
        #endif
    }
    BC_LOG_INFO("opus_encode len :%d\r\n", len);
    //taskEXIT_CRITICAL();
    return len;
}
#endif


void app_opus_create(void)
{
	#if defined(USE_OPUS)
	opus_init_cp();
    #endif
}


