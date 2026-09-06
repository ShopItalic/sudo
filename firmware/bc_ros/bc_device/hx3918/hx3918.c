#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#include "hx3918.h"
#ifdef  TYHX_DEMO
#include "demo_ctrl.h"
#include "twi_master.h"
#include "SEGGER_RTT.h"
#include "app_timer.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "drv_oled.h"
#include "opr_oled.h"

#include "oled_iic.h"
#include "word.h"
#include "iic.h"
#endif
//#include "tyhx_hrs_custom.h"
#ifdef SAR_ALG_LIB
#include "hx3918_prox.h"
#endif

#ifdef HRS_ALG_LIB
#include "hx3918_hrs_agc.h"
#include "tyhx_hrs_alg.h"
#endif

#ifdef SPO2_ALG_LIB
#include "hx3918_spo2_agc.h"
#include "tyhx_spo2_alg.h"
#endif

#ifdef HRV_ALG_LIB
#include "hx3918_hrv_agc.h"
#include "tyhx_hrv_alg.h"
#endif

#ifdef CHECK_TOUCH_LIB
#include "hx3918_check_touch.h"
#endif
#include "hx3918_factory_test.h"



#ifdef SPO2_VECTOR
#include "spo2_vec.h"
uint32_t spo2_send_cnt = 0;
int32_t red_buf_vec[8];
int32_t ir_buf_vec[8];
int32_t green_buf_vec[8];
#endif

#ifdef HR_VECTOR
#include "hr_vec.h"
uint32_t hrs_send_cnt = 0;
int32_t PPG_buf_vec[8];
#endif

#ifdef HRV_TESTVEC
#include "hrv_testvec.h"
#endif

#ifdef GSENSER_DATA
//#include "lis3dh_drv.h"
#endif

//#include "bc_alg.h"


#include "bc_delay.h"
#include "bc_timer.h"
#include "bc_ppg_driver_port.h"
#include "bc_logger.h"
#include "bc_gsensor.h"

tyhx_hrsresult_t hrsresult;

#ifdef GSENSER_DATA
volatile int16_t gsen_fifo_x[16];  //ppg time 330ms..330/40 = 8.25
volatile int16_t gsen_fifo_y[16];
volatile int16_t gsen_fifo_z[16];
#else
int16_t gen_dummy[64] = {0};
#endif
//SPO2 agc
const uint8_t  hx3918_spo2_agc_green_idac = GREEN_AGC_OFFSET;
const uint8_t  hx3918_spo2_agc_red_idac = RED_AGC_OFFSET;
const uint8_t  hx3918_spo2_agc_ir_idac = IR_AGC_OFFSET;
//hrs agc
const uint8_t  hx3918_hrs_agc_idac = HR_GREEN_AGC_OFFSET;
//hrv agc
const uint8_t  hx3918_hrv_agc_idac = HR_GREEN_AGC_OFFSET;

const uint8_t  red_led_max_init = 200;
const uint8_t  ir_led_max_init = 200;
const uint8_t  green_led_max_init = 150;

uint8_t low_vbat_flag =0;


const uint16_t static_thre_val = 150;
const uint8_t  gsen_lv_val = 0;


//HRS_INFRARED_THRES
const int32_t  hrs_ir_unwear_thres = 1500;
const int32_t  hrs_ir_wear_thres = 3500;
//HRV_INFRARED_THRES
const int32_t  hrv_ir_unwear_thres = 3000;
const int32_t  hrv_ir_wear_thres = 3500;
//SPO2_INFRARED_THRES
const int32_t  spo2_ir_unwear_thres = 2500;
const int32_t  spo2_ir_wear_thres = 3500;
//CHECK_WEAR_MODE_THRES
const int32_t  check_mode_unwear_thre = 3000;
const int32_t  check_mode_wear_thre =3500;

//const int32_t  check_mode_unwear_thre = 0;
//const int32_t  check_mode_wear_thre =0;

int32_t  light_leak_val = 0;
int32_t  sar_wear_thres = 3000;  // 可设定为正常的1/3, 必须小于32767
int32_t  sar_unwear_thres = 2400; // 可设定为 sar_wear_thres *0.8

uint8_t alg_ram[5 * 1024] __attribute__((aligned(4)));

static uint8_t wave_upload_flag;
static uint8_t wave_seq;
//ACC数据发送缓冲区
static int16_t accdata[16][3] = {0};
//心率实时数据存储
static int32_t hrsdata[16];
//static uint32_t agc_green[8];
//static uint8_t led_idac[8]; 
//血氧实时数据存储
static int32_t reddata[16];
static int32_t irdata[16];
//static uint8_t red_idac[8]; 
//static uint8_t ir_idac[8]; 
//static uint16_t red_cur[8];
//static uint16_t ir_cur[8];

static uint8_t datacnt = 0;
const static uint8_t hx3605_address = 0x44;

ppg_sensor_data_t ppg_s_dat;
uint8_t read_fifo_first_flg = 0;
hrs_sports_mode_t hrs_mode = NORMAL_MODE;




//////// spo2 para and switches
const  uint8_t   COUNT_BLOCK_NUM = 50;            //delay the block of some single good signal after a series of bad signal
const  uint8_t   SPO2_LOW_XCORR_THRE = 30;        //(64*xcorr)'s square below this threshold, means error signal
const  uint8_t   SPO2_CALI = 1;                       //spo2_result cali mode
const  uint8_t   XCORR_MODE = 1;                  //xcorr mode switch
const  uint8_t   QUICK_RESULT = 1;                //come out the spo2 result quickly ;0 is normal,1 is quick
const  uint16_t  MEAN_NUM = 32;                  //the length of smooth-average ;the value of MEAN_NUM can be given only 256 and 512
const  uint8_t   G_SENSOR = 0;                      //if =1, open the gsensor mode
const  uint8_t   SPO2_GSEN_POW_THRE = 150;         //gsen pow judge move, range:0-200;
const  uint32_t  SPO2_BASE_LINE_INIT = 168000;    //spo2 baseline init, = 103000 + ratio(a variable quantity,depends on different cases)*SPO2_SLOPE
const  int32_t   SOP2_DEGLITCH_THRE = 5000;     //remove signal glitch over this threshold
const  int32_t   SPO2_REMOVE_JUMP_THRE = 5000;  //remove signal jump over this threshold
const  uint32_t  SPO2_SLOPE = 50000;              //increase this slope, spo2 reduce more
const  uint16_t  SPO2_LOW_CLIP_END_TIME = 1500;   //low clip mode before this data_cnt, normal clip mode after this
const  uint16_t  SPO2_LOW_CLIP_DN  = 150;         //spo2 reduce 0.15/s at most in low clip mode
const  uint16_t  SPO2_NORMAL_CLIP_DN  = 500;      //spo2 reduce 0.5/s at most in normal clip mode
const  uint8_t   SPO2_LOW_SNR_THRE = 40;          //snr below this threshold, means error signal
const  uint16_t  IR_AC_TOUCH_THRE = 200;          //AC_min*0.3
const  uint16_t  IR_FFT_POW_THRE = 200;           //fft_pow_min
const  uint8_t   SLOPE_PARA_MAX = 33;
const  uint8_t   SLOPE_PARA_MIN = 3;

WORK_MODE_T work_mode_flag = HRV_MODE;



typedef void (*ppg_green_data_callback)(int32_t *data,int16_t *acc_data,uint8_t length); 		
typedef void (*ppg_red_and_ir_data_callback)(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,int16_t *acc_data); 		
typedef void (*ppg_red_and_ir_and_gre_data_callback)(void *red_data,uint8_t red_length,void *ir_data,uint8_t ir_length,void *gre_data,uint8_t gre_length,int16_t *acc_data); 	
typedef void (*ppg_hr_result_callback)(uint8_t heart_rate,uint8_t hrv1); 
typedef void (*ppg_spo2_result_callback)(uint8_t spo2,uint8_t heart_rate);

typedef void (*ppg_signal_check_callback)(uint16_t signal_strength); 															

static ppg_green_data_callback	     green_data_callback = NULL;											
static ppg_red_and_ir_data_callback  red_and_ir_data_callback = NULL;	
static ppg_red_and_ir_and_gre_data_callback  red_and_ir_and_gre_data_callback = NULL;

static ppg_hr_result_callback	     hr_result_callback = NULL;	
static ppg_spo2_result_callback		 spo2_result_callback = NULL;
														
static ppg_signal_check_callback     hr_signal_check_callback = NULL;
static ppg_signal_check_callback     spo2_signal_check_callback = NULL;	


static struct hx3918_g_sensor_struct g_sensor_struct = {NULL,NULL,NULL};



enum hx3918_ppg_time_type
{
	HX3918_PPG_TIMEOUT = 0,
	HX3918_PPG_AGC_TIMEOUT,
//	HX3918_PPG_G_SENSOR_GET,
	HX3918_PPG_TIME_TYPE,
};

static bc_timer_struct  ppg_timer[HX3918_PPG_TIME_TYPE] = {
	                                                           {
																 .timer_name = "hx3918 timeout timer",
																 .uxAutoReload = true,
																 .xTimerPeriodInTicks = 320,
																 .timer_callback_function = hx3918_ppg_timeout_handler,   
	                                                           },
															   {
																 .timer_name = "agc_timeout timer",
																 .xTimerPeriodInTicks = 40,
																 .uxAutoReload = true,
																 .timer_callback_function = hx3918_agc_timeout_handler,   
	                                                           },
//															   {
//																 .timer_name = "g_sensor get timer",
//																 .xTimerPeriodInTicks = 40,
//																 .uxAutoReload = true,
//																 .timer_callback_function = hx3918_gesensor_Int_handle,   
//	                                                           }
															   
											                 };

static void app_ppg_timer_start(enum hx3918_ppg_time_type time_id)
{
	bc_timer_start(&ppg_timer[time_id]);
}

static void app_ppg_timer_stop(enum hx3918_ppg_time_type time_id)
{
	bc_timer_stop(&ppg_timer[time_id]);
}



void hx3918_timer_init(void)
{
	for(uint8_t i = 0;i < HX3918_PPG_TIME_TYPE; i++)
	{
		if(!bc_timer_create(&ppg_timer[i]))
		{
			BC_LOG_INFO("create %s fial!! \r\n",ppg_timer[i].timer_name);
			
		}
		else
		{
			BC_LOG_INFO("create %s success!! \r\n",ppg_timer[i].timer_name);
		}		
	}		
	
}

bool ppg_g_sensor_register_callback(void *start_callback,void *stop_callback,void *read_callback)
{
	if(start_callback != NULL && stop_callback != NULL && read_callback != NULL)
	{
        g_sensor_struct.ppg_g_sensor_data_timer_start_callback = start_callback;
		g_sensor_struct.ppg_g_sensor_data_timer_stop_callback = stop_callback;
		g_sensor_struct.ppg_g_sensor_data_read_callback = read_callback;
		return true;
	}
	return false;
}

void ppg_g_sensor_start(void )
{
	if(g_sensor_struct.ppg_g_sensor_data_timer_start_callback != NULL)
	{
        g_sensor_struct.ppg_g_sensor_data_timer_start_callback();
	}
}

void ppg_g_sensor_stop(void )
{
	if(g_sensor_struct.ppg_g_sensor_data_timer_stop_callback != NULL)
	{
        g_sensor_struct.ppg_g_sensor_data_timer_stop_callback();
	}
}


void hx3918_g_sensor_data_get_timer_start(void)
{
	ppg_g_sensor_start();
}

void hx3918_g_sensor_data_get_timer_stop(void)
{
	ppg_g_sensor_stop();
}


void hx3918_delay(uint32_t ms)
{
   bc_delay_ms(ms);
}

bool hx3918_write_reg(uint8_t addr, uint8_t data)
{
   uint8_t temp = data;
	
	bc_ppg_i2c_write(0x44 << 1  ,addr,&temp,1);
    return 1;
}

uint8_t hx3918_read_reg(uint8_t addr)
{
    uint8_t temp = 0;	
	bc_ppg_2c_read(0x44 << 1  ,addr,&temp,1);
    return temp;
}

bool hx3918_brust_read_reg(uint8_t addr, uint8_t *buf, uint8_t length)
{
    bc_ppg_2c_read(0x44 << 1  ,addr,buf,length);
    return true;
}

/*320ms void hx3918_ppg_Int_handle(void)*/
void hx3918_ppg_timer_cfg(bool en)    //320ms
{
//	BC_LOG_INFO("kkkkkkkkkkkkk   %d ",en);
    if (en)
    {
		app_ppg_timer_stop(HX3918_PPG_TIMEOUT);
        app_ppg_timer_start(HX3918_PPG_TIMEOUT);
			   
    }else{
         app_ppg_timer_stop(HX3918_PPG_TIMEOUT);
//		ppg_g_sensor_start();
    }
}

/* 40ms void agc_timeout_handler(void * p_context) */
void hx3918_agc_timer_cfg(bool en)
{
    if (en)
    {
		 app_ppg_timer_stop(HX3918_PPG_AGC_TIMEOUT);
		 app_ppg_timer_start(HX3918_PPG_AGC_TIMEOUT);	
//		 ppg_g_sensor_stop();
		
    }else{
        app_ppg_timer_stop(HX3918_PPG_AGC_TIMEOUT);
//		ppg_g_sensor_start();
    }
}

#if defined(INT_MODE)
void hx3918_gpioint_cfg(bool en)
{
    if(en)
    {
        hx3918_gpioint_enable();
    }
    else
    {
        hx3918_gpioint_disable();
    }

}
#endif

uint8_t hx3918_chip_id = 0;
uint8_t hx3917_chip_id = 0;
uint8_t chip_id = 0;
bool hx3918_chip_check(void)
{
    uint8_t i = 0;

    for(i=0; i<10; i++)
    {
        hx3918_write_reg(0x01, 0x00);
        hx3918_delay(5);
        hx3918_chip_id = hx3918_read_reg(0x00);
        if (hx3918_chip_id == 0x27)             
        {
            TYHX_LOG("chip id  = 0x%x	check ok \r\n",hx3918_chip_id);
            return true;
        }
    }
    TYHX_LOG("hx3918_chip_check fail\r\n");
    return false;
}

void hx3918_light_leak_set(int32_t light_leak_value)
{
    light_leak_val = light_leak_value;
}

bool chip_judge(void)  
{       
    uint8_t i = 0;

    for(i=0; i<10; i++)
    {
        hx3918_write_reg(0x01, 0x00);
        hx3918_delay(5);
        chip_id = hx3918_read_reg(0x00);
    if(chip_id == 0x27)
    {
        TYHX_LOG("chip id  = 0x%x	this is 3918 \r\n",chip_id);
        return true;
    }
    else if(chip_id == 0x28)
    {
        TYHX_LOG("chip id  = 0x%x	this is 3917  \r\n",chip_id);
        return true;
    }
   }
    TYHX_LOG("chip_check fail\r\n");
    return false;
}

bool hx3917_chip_check(void)
{
    uint8_t i = 0;

    for(i=0; i<10; i++)
    {
        hx3918_write_reg(0x01, 0x00);
        hx3918_delay(5);
        hx3918_chip_id = hx3918_read_reg(0x00);
        if (hx3917_chip_id == 0x28)             
        {
            TYHX_LOG("chip id  = 0x%x	check ok \r\n",hx3917_chip_id);
            return true;
        }
    }
    TYHX_LOG("hx3917_chip_check fail\r\n");
    return false;
}

uint8_t hx3918_read_fifo_size(void)
{
    uint8_t fifo_num_temp = 0;
    fifo_num_temp = hx3918_read_reg(0x40)&0x7f;  // ericy 230529

    return fifo_num_temp;
}
uint16_t hx3918_read_fifo_data(int32_t *buf, uint8_t phase_num, uint8_t sig)
{
    uint8_t data_flg_start = 0;
    uint8_t data_flg_end = 0;
    uint8_t databuf[3];
    uint32_t ii=0;
    uint16_t data_len = 0;
    uint16_t fifo_data_length = 0;
    uint16_t fifo_read_length = 0;
    uint16_t fifo_read_bytes = 0;
    uint16_t fifo_out_length = 0;
    uint16_t fifo_out_count = 0;
    uint8_t fifo_data_buf[200] = {0};

    data_len = hx3918_read_reg(0x40);// ericy 230529
    fifo_data_length = data_len;
    //DEBUG_PRINTF("data_len:   %d\r\n",data_len);
    if(fifo_data_length<2*phase_num)
    {
        return 0;
    }
    fifo_read_length = ((fifo_data_length-phase_num)/phase_num)*phase_num;
    fifo_read_bytes = fifo_read_length*3;
    if(read_fifo_first_flg == 1)
    {
        hx3918_brust_read_reg(0x41, databuf, 3);	// ericy 230529
        read_fifo_first_flg = 0;
    }
    hx3918_brust_read_reg(0x41, fifo_data_buf, fifo_read_bytes);	// ericy 230529
//		for(ii=0; ii<fifo_read_bytes; ii++)
//		{
//			DEBUG_PRINTF("%d/%d, %d\r\n", ii+1,fifo_read_bytes,fifo_data_buf[ii]);
//		}
    for(ii=0; ii<fifo_read_length; ii++)
    {
        if(sig==0)
        {
            buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x1f)<<16));
        }
        else
        {
            if((fifo_data_buf[ii*3+2]&0x10)!=0)
            {
                buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x0f)<<16))-1048576;
            }
            else
            {
                buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x1f)<<16));
            }
        }
        //DEBUG_PRINTF("%d/%d, %d %d\r\n", ii+1,fifo_read_length,buf[ii],(fifo_data_buf[ii*3+2]>>5)&0x03);
    }
    data_flg_start = (fifo_data_buf[2]>>5)&0x03;
    data_flg_end = (fifo_data_buf[fifo_read_bytes-1]>>5)&0x03;
    fifo_out_length = fifo_read_length;
    if(data_flg_start>0)
    {
        fifo_out_length = fifo_read_length-phase_num+data_flg_start;
        for(ii=0; ii<fifo_out_length; ii++)
        {
            buf[ii] = buf[ii+phase_num-data_flg_start];
        }
        for(ii=fifo_out_length; ii<fifo_read_length; ii++)
        {
            buf[ii] = 0;
        }
    }
    if(data_flg_end<phase_num-1)
    {
        for(ii=fifo_out_length; ii<fifo_out_length+phase_num-data_flg_end-1; ii++)
            hx3918_brust_read_reg(0x41, databuf, 3);// ericy 230529
        buf[ii] = (int32_t)(databuf[0]|(databuf[1]<<8)|((databuf[2]&0x1f)<<16));
    }
//		for(ii=0; ii<fifo_out_length; ii++)
//		{
//			DEBUG_PRINTF("%d/%d, %d\r\n", ii+1,fifo_out_length,buf[ii]);
//		}
    fifo_out_length = fifo_out_length+phase_num-data_flg_end-1;
    fifo_out_count = fifo_out_length/phase_num;
    if(data_len==64)
    {
        uint8_t reg_0x2d = hx3918_read_reg(0x3d);// ericy 230529
        hx3918_write_reg(0x3d,0x00);// ericy 230529
        hx3918_delay(5);
        hx3918_write_reg(0x3d,reg_0x2d);// ericy 230529
        read_fifo_first_flg = 1;
    }
    return fifo_out_count;
}

void read_data_packet(int32_t *ps_data)  // 3918
{
    uint8_t  databuf1[6] = {0};
    uint8_t  databuf2[6] = {0};
    hx3918_brust_read_reg(0x02, databuf1, 6);
    hx3918_brust_read_reg(0x08, databuf2, 6);

    ps_data[0] = ((databuf1[0]) | (databuf1[1] << 8) | (databuf1[2] << 16));
    ps_data[1] = ((databuf1[3]) | (databuf1[4] << 8) | (databuf1[5] << 16));
    ps_data[2] = ((databuf2[0]) | (databuf2[1] << 8) | (databuf2[2] << 16));
    ps_data[3] = ((databuf2[3]) | (databuf2[4] << 8) | (databuf2[5] << 16));
//    DEBUG_PRINTF(" %d %d %d %d \r\n",  ps_data[0], ps_data[1], ps_data[2], ps_data[3]);
}



void hx3918_vin_check(uint16_t led_vin)
{
    low_vbat_flag = 0;
    if(led_vin < 3700)
    {
        low_vbat_flag = 1;
    }
}

void hx3918_ppg_off(void)
{
    hx3918_write_reg(0x2a, 0x00);
    hx3918_write_reg(0x2b, 0x00);
    hx3918_write_reg(0x16, 0x00);
    hx3918_write_reg(0x01, 0x01);
}

void hx3918_ppg_on(void)
{
    hx3918_write_reg(0x01, 0x00);
    hx3918_delay(5);
}

void hx3918_data_reset(void)
{
#if defined(TIMMER_MODE)
    hx3918_ppg_timer_cfg(false);
    hx3918_agc_timer_cfg(false);
#elif defined(INT_MODE)
    hx3918_gpioint_cfg(false);
#endif
#if defined(HRS_ALG_LIB)
    hx3918_hrs_data_reset();
#endif
#if defined(SPO2_ALG_LIB)
    hx3918_spo2_data_reset();
#endif
#if defined(HRV_ALG_LIB)
    hx3918_hrv_data_reset();
#endif
    TYHX_LOG("hx3918 data reset!\r\n");
}

void Efuse_Mode_Check(void)
{
    uint8_t  REG_45_E, REG_46_E, REG_47_E, REG_48_E;

    uint8_t chip_vision_check = 0;
    chip_vision_check = hx3918_read_reg(0x47)>>6;
    
    if(chip_vision_check > 0)
    {
        return;
    }
    
    hx3918_write_reg(0x4b, 0x10);
    hx3918_write_reg(0x4d, 0x18);
    hx3918_write_reg(0x4e, 0x40);
    hx3918_write_reg(0x4C, 0x04);
    hx3918_write_reg(0x4C, 0x00);

    REG_45_E = hx3918_read_reg(0x45);
    REG_46_E = hx3918_read_reg(0x46);
    REG_47_E = hx3918_read_reg(0x47);
    REG_48_E = hx3918_read_reg(0x48);

    hx3918_write_reg(0X4B,0x20);
    hx3918_write_reg(0x45,REG_45_E);
    hx3918_write_reg(0x46,REG_46_E);
    hx3918_write_reg(0x47,REG_47_E);
    hx3918_write_reg(0x48,REG_48_E);
    hx3918_write_reg(0X4B,0x00);
    
    hx3918_write_reg(0x4d, 0x20);
    hx3918_write_reg(0x4e, 0x00);
    hx3918_write_reg(0x4C, 0x00);
     
}

uint8_t hx3918_read_id(void)
{
    return hx3918_read_reg(0x00);
}

static uint32_t temp_count = 0;

bool hx3918_init(WORK_MODE_T mode,uint8_t flag,uint8_t seq)
{
    wave_upload_flag = flag;
    wave_seq = seq;
    datacnt = 0;
    work_mode_flag = mode;
    hx3918_data_reset();
    hx3918_vin_check(3800);
    hx3918_ppg_on();
    Efuse_Mode_Check();
    switch (work_mode_flag)
    {
    case HRS_MODE:
//        bc_alg_rri_init();
        tyhx_hrs_set_living(0,0,0); //  qu=15  std=30
	    temp_count = 0;
        if(hx3918_hrs_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
        break;

    case LIVING_MODE:
#ifdef CHECK_LIVING_LIB
        if(hx3918_hrs_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
#endif
        break;

    case SPO2_MODE:
		temp_count = 0;
#ifdef SPO2_DATA_CALI
        hx3918_spo2_data_cali_init();
#endif
#ifdef SPO2_ALG_LIB
        tyhx_spo2_para_usuallyadjust(SPO2_LOW_XCORR_THRE,SPO2_LOW_SNR_THRE,COUNT_BLOCK_NUM,SPO2_BASE_LINE_INIT,SPO2_SLOPE,SPO2_GSEN_POW_THRE);
        tyhx_spo2_para_barelychange(MEAN_NUM,SOP2_DEGLITCH_THRE,SPO2_REMOVE_JUMP_THRE,SPO2_LOW_CLIP_END_TIME,SPO2_LOW_CLIP_DN, \
                                    SPO2_NORMAL_CLIP_DN,IR_AC_TOUCH_THRE,IR_FFT_POW_THRE,SPO2_CALI,SLOPE_PARA_MAX,SLOPE_PARA_MIN);
        if(hx3918_spo2_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
#endif
        break;

    case HRV_MODE:
#ifdef HRV_ALG_LIB
        if(hx3918_hrv_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }

        break;
#endif
    case WEAR_MODE:
#ifdef CHECK_TOUCH_LIB

        if(hx3918_check_touch_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
#endif
        break;

    case FT_LEAK_LIGHT_MODE:
        if (!chip_judge())
        {
            AGC_LOG("check id failed!\r\n");
            return false;
        }
        hx3918_factroy_test(WEAR_CHECK_LIGHTLEAK_TEST);
        break;

    case FT_GRAY_CARD_MODE:
        hx3918_factroy_test(GRAY_CARD_TEST);
        break;

    case FT_INT_TEST_MODE:
        hx3918_factroy_test(FT_INT_TEST);
//			  hx3918_gpioint_cfg(true);
        break;

    case LAB_TEST_MODE:
#ifdef LAB_TEST
        if(hx3918_lab_test_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
#endif
        break;

    default:
        break;
    }

    return true;
}


#ifdef ACC_DA267
#define ACC_FIFO_LENGTH 32
#else
#define ACC_FIFO_LENGTH 64
#endif
void hx3918_gesensor_Int_handle(void)
{
    //readFifoByHrsModule();
#ifdef GSENSER_DATA
    uint8_t ii = 0;
//    int16_t data[3] = {0};
	uint16_t acc_num = 0;
	struct hx3918_g_sensor_data_struct g_sensor_data_struct = {0};
    if(work_mode_flag == WEAR_MODE)
    {
        return;
    }
    if(g_sensor_struct.ppg_g_sensor_data_read_callback != NULL)
	{
		g_sensor_struct.ppg_g_sensor_data_read_callback((uint8_t*)&g_sensor_data_struct,&acc_num);
	}
#if (G_SENSOR_DEVIECE_TYPE == 0)   //QMA6100/QMA6100P
//	int32_t temp[3] = {0};
//	bc_gsensor_dataRead(temp);
//	BC_LOG_INFO(" g_sensor_x:%d g_sensor_y:%d g_sensor_z:%d \r\n",(int16_t)temp[0],(int16_t)temp[1],(int16_t)temp[2]);
//	data[0] = (int16_t)temp[0];
//	data[1] = (int16_t)temp[1];
//	data[2] = (int16_t)temp[2];

#elif (G_SENSOR_DEVIECE_TYPE == 1)  // ICM42688
	bc_gsensor_dataRead((int*)&g_sensor_data_struct.g_sensor_data[g_sensor_data_struct.g_sensor_data_index]);

#endif
	 for(ii=8; ii<16; ii++)
    {
        gsen_fifo_x[ii] = g_sensor_data_struct.g_sensor_data[ii - 8].acc[0] ;
        gsen_fifo_y[ii] = g_sensor_data_struct.g_sensor_data[ii - 8].acc[1] ;
        gsen_fifo_z[ii] = g_sensor_data_struct.g_sensor_data[ii - 8].acc[2] ;
//		BC_LOG_INFO("lllllllllllllll g_sensor_x:%d g_sensor_y:%d g_sensor_z:%d \r\n",gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii]);
    }
	

//    LIS3DH_GetAccAxesRaw(&gsen_buf);

//    for(ii=0; ii<9; ii++)
//    {
//        gsen_fifo_x[ii] = gsen_fifo_x[ii+1];
//        gsen_fifo_y[ii] = gsen_fifo_y[ii+1];
//        gsen_fifo_z[ii] = gsen_fifo_z[ii+1];
//		BC_LOG_INFO("lllllllllllllll g_sensor_x:%d g_sensor_y:%d g_sensor_z:%d \r\n",gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii]);
//    }
//    gsen_fifo_x[9] = (int16_t)temp[0] >> 1;
//    gsen_fifo_y[9] = (int16_t)temp[1] >> 1;
//    gsen_fifo_z[9] = (int16_t)temp[2] >> 1;
    //SEGGER_RTT_printf(0,"gsen_x=%d gsen_y=%d gsen_z=%d\r\n", \
    gsen_fifo_x[9],gsen_fifo_y[9],gsen_fifo_z[9]);
#endif
}



void hx3918_agc_timeout_handler(void)
{
#ifdef TIMMER_MODE

    switch (work_mode_flag)
    {
    case HRS_MODE:
    {
#ifdef HRS_ALG_LIB
        HRS_CAL_SET_T cal;
        cal = PPG_hrs_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            //cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case LIVING_MODE:
    {
#ifdef HRS_ALG_LIB
        HRS_CAL_SET_T cal;
        cal = PPG_hrs_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            //cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case HRV_MODE:
    {
#ifdef HRV_ALG_LIB
        HRV_CAL_SET_T cal;
        cal = PPG_hrv_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            //cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case SPO2_MODE:
    {
#ifdef SPO2_ALG_LIB
        SPO2_CAL_SET_T cal;
        cal = PPG_spo2_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: Rled_drv=%d,Irled_drv=%d,RledDac=%d,IrledDac=%d,ambDac=%d,Rledstep=%d,Irledstep=%d,Rrf=%d,Irrf=%d,\r\n", \
            //cal.red_cur, cal.ir_cur,cal.red_idac,cal.ir_idac,cal.amb_idac,cal.red_led_step,cal.red_led_step,cal.ontime,cal.state);
        }
#endif
        break;
    }
    case LAB_TEST_MODE:
    {
#ifdef LAB_TEST
        hx3918_lab_test_Int_handle();
#endif
    }
    default:
        break;
    }
#endif
}

void hx3918_ppg_timeout_handler(void)
{
#if defined(INT_MODE)
    hx3918_agc_Int_handle();
#endif
#ifdef SAR_ALG_LIB
		hx3918_report_prox_status();
#endif
	hx3918_gesensor_Int_handle();
//	hx3918_g_sensor_data_get_timer_stop();
    switch(work_mode_flag)
    {
    case HRS_MODE:
#ifdef HRS_ALG_LIB
        hx3918_hrs_ppg_Int_handle();
#endif

        break;

    case SPO2_MODE:
#ifdef SPO2_ALG_LIB
        hx3918_spo2_ppg_Int_handle();
#endif
        break;

    case HRV_MODE:
#ifdef HRV_ALG_LIB
        hx3918_hrv_ppg_Int_handle();
#endif
        break;

    case WEAR_MODE:
#ifdef CHECK_TOUCH_LIB
        hx3918_wear_ppg_Int_handle();
#endif
        break;

    case LAB_TEST_MODE:
#ifdef LAB_TEST
        hx3918_lab_test_Int_handle();
#endif

    default:
        break;
    }
//	hx3918_g_sensor_data_get_timer_start();

}


#ifdef HRS_ALG_LIB
void hx3918_hrs_ppg_Int_handle(void)
{
    uint8_t        ii=0;
    hrs_results_t alg_results = {MSG_HRS_ALG_NOT_OPEN,0,0,0,0};
    hx3918_hrs_wear_msg_code_t hrs_wear_status = MSG_HRS_INIT;
    int32_t *PPG_buf = &(ppg_s_dat.green_data[0]);
    int32_t *ir_buf = &(ppg_s_dat.ir_data[0]);
    uint8_t *count = &(ppg_s_dat.count);
    int16_t gsen_fifo_x_send[16]= {0};
    int16_t gsen_fifo_y_send[16]= {0};
    int16_t gsen_fifo_z_send[16]= {0};
    //HRS_CAL_SET_T cal= get_hrs_agc_status();
#ifdef BP_CUSTDOWN_ALG_LIB
    bp_results_t    bp_alg_results ;
#endif
#ifdef HR_VECTOR
    for(ii=0; ii<8; ii++)
    {
        PPG_buf_vec[ii] = hrm_input_data[hrs_send_cnt+ii];
        gsen_fifo_x[ii] = gsen_input_data_x[hrs_send_cnt+ii];
        gsen_fifo_y[ii] = gsen_input_data_y[hrs_send_cnt+ii];
        gsen_fifo_z[ii] = gsen_input_data_z[hrs_send_cnt+ii];
    }
    hrs_send_cnt = hrs_send_cnt+8;
    *count = 8;
    tyhx_hrs_alg_send_data(PPG_buf_vec,*count, 0, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z);
#else
    if(hx3918_hrs_read(&ppg_s_dat) == 0)
    {
        return;
    }
#ifndef GSEN_40MS_TIMMER
  //  readFifoByHrsModule();
#endif
    datacnt = 0;


    for(ii=0; ii<*count; ii++)
    {

        #ifdef GSENSER_DATA
        gsen_fifo_x_send[ii] = gsen_fifo_x[16-*count+ii];
        gsen_fifo_y_send[ii] = gsen_fifo_y[16-*count+ii];
        gsen_fifo_z_send[ii] = gsen_fifo_z[16-*count+ii];
        #endif
        if(wave_upload_flag == 1)
        {
        #ifdef GSENSER_DATA
            accdata[datacnt][0] = gsen_fifo_x_send[ii];
            accdata[datacnt][1] = gsen_fifo_y_send[ii];
            accdata[datacnt][2] = gsen_fifo_z_send[ii];
        #endif
            hrsdata[datacnt] = PPG_buf[ii];
        //  hrsdata[datacnt] = ir_buf[ii];
        //  user_hal_acc_data_read(acc[datacnt]);
        //  LOG("%d/%d %d %d \r\n",1+ii,*count,datacnt,hrsdata[datacnt]);
//            BC_LOG_INFO(" g_sensor_x:%d g_sensor_y:%d g_sensor_z:%d \r\n", accdata[datacnt][0],accdata[datacnt][1], accdata[datacnt][2]);        
            if(++datacnt >= *count){
//                ble_ServicesInfo_send_HrsAcc(wave_seq,hrsdata,accdata,datacnt);    //

				if(green_data_callback != NULL)
				{
					
					green_data_callback(hrsdata,&accdata[0][0],datacnt);
				}
                datacnt = 0;
            }
        }
//        else if(wave_upload_flag == 2)
//        {
//            hrsdata[datacnt] = PPG_buf[ii];
//            irdata[datacnt] = ir_buf[ii];
//            agc_green[datacnt] = hrs_s_dat.agc_green;
//            led_idac[datacnt] = calR.led_idac;
//            if(++datacnt == *count)
//            {
//                ble_ServicesInfo_send_HrsAlgRes(hrsdata,irdata,agc_green,led_idac,datacnt);    //
//                datacnt = 0;
//            }
//        }

//        DEBUG_PRINTF("%d %d %d %d %d %d %d \r\n", PPG_buf[ii],ir_buf[ii],gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii],hrs_s_dat.agc_green,calR.led_idac);
//        LOG("PPG:%d , ir:%d \r\n",PPG_buf[ii],ir_buf[ii]);
    }
    hrs_wear_status = hx3918_hrs_get_wear_status();

	if(hr_signal_check_callback != NULL)
	{
		
		hr_signal_check_callback(hrs_wear_status);
	}
    hrsresult.wearstatus = hrs_wear_status;
    if(hrs_wear_status == MSG_HRS_WEAR)
    {
        tyhx_hrs_alg_send_data(PPG_buf, *count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send);
    }
#endif
    alg_results = tyhx_hrs_alg_get_results();
	
	
	if( hr_result_callback!=NULL )
	{
		hr_result_callback(alg_results.hr_result,alg_results.hr_result_qual);
	}
    hrsresult.lastesthrs = alg_results.hr_result;
    DEBUG_PRINTF("hrsresult.wearstatus =%d HR=%d\r\n",hrs_wear_status,alg_results.hr_result);
#ifdef BP_CUSTDOWN_ALG_LIB
    bp_alg_results = tyhx_alg_get_bp_results();
    TYHX_LOG("bp up_value: %d, down_value: %d",bp_alg_results.sbp,bp_alg_results.dbp);
#endif
		
//            TYHX_LOG("living=%d",hrsresult.hr_living);
#ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        for(ii=0; ii<*count; ii++)
        {
            rawdata.vector_flag = HRS_VECTOR_FLAG;
            rawdata.data_cnt = alg_results.data_cnt-*count+ii;
            rawdata.hr_result = alg_results.hr_result;
            rawdata.red_raw_data = PPG_buf[ii];
            rawdata.ir_raw_data = ir_buf[ii];
            rawdata.gsensor_x = gsen_fifo_x[ii];
            rawdata.gsensor_y = gsen_fifo_y[ii];
            rawdata.gsensor_z = gsen_fifo_z[ii];
            rawdata.red_cur = ppg_s_dat.green_cur;
            rawdata.ir_cur = alg_results.hrs_alg_status;

            ble_rawdata_vector_push(rawdata);
        }
    }
    ble_rawdata_send_handler();
#endif
}

#ifdef CHECK_LIVING_LIB
void hx3918_living_Int_handle(void)
{

    uint8_t        ii=0;
    hx3918_living_results_t living_alg_results = {MSG_LIVING_NO_WEAR,0,0,0};

    int32_t *PPG_buf = &(ppg_s_dat.green_data[0]);
    uint8_t *count = &(ppg_s_dat.count);
    hx3918_hrs_wear_msg_code_t     hrs_wear_status = MSG_HRS_NO_WEAR;
    int16_t 	gsen_fifo_x_send[32]= {0};
    int16_t 	gsen_fifo_y_send[32]= {0};
    int16_t 	gsen_fifo_z_send[32]= {0};

    if(hx3918_hrs_read(&ppg_s_dat) == 0)
    {
        return;
    }
#ifndef GSEN_40MS_TIMMER
   // readFifoByHrsModule();
#endif
    for(ii=0; ii<*count; ii++)
    {
        gsen_fifo_x_send[ii] = gsen_fifo_x[32-*count+ii];
        gsen_fifo_y_send[ii] = gsen_fifo_y[32-*count+ii];
        gsen_fifo_z_send[ii] = gsen_fifo_z[32-*count+ii];
        //DEBUG_PRINTF("HX_data:%d/%d %d %d %d %d %d %d\r\n",1+ii,*count,PPG_buf[ii],ir_buf[ii],gsen_fifo_x[ii],gsen_fisignal_qualityfo_y[ii],gsen_fifo_z[ii],hrs_s_dat.agc_green);
    }

    living_alg_results = hx3918_living_get_results();
    //TYHX_LOG("%d %d %d %d\r\n",living_alg_results.data_cnt,living_alg_results.motion_status,living_alg_results.signal_quality,living_alg_results.wear_status);

}
#endif


#ifdef SPO2_ALG_LIB
void hx3918_spo2_ppg_Int_handle(void)
{
    uint8_t        ii=0;
		tyhx_spo2_results_t alg_results = {MSG_SPO2_ALG_NOT_OPEN,0,0,0,0,0,0};
    SPO2_CAL_SET_T cal=get_spo2_agc_status();
    hx3918_spo2_wear_msg_code_t spo2_wear_status = MSG_SPO2_INIT;
    int32_t *green_buf = &(ppg_s_dat.green_data[0]);
    int32_t *red_buf = &(ppg_s_dat.red_data[0]);
    int32_t *ir_buf = &(ppg_s_dat.ir_data[0]);
    uint8_t *count = &(ppg_s_dat.count);
    int16_t gsen_fifo_x_send[10]= {0};
    int16_t gsen_fifo_y_send[10]= {0};
    int16_t gsen_fifo_z_send[10]= {0};
#ifdef SPO2_DATA_CALI
    int32_t red_data_cali, ir_data_cali;
#endif

#ifdef SPO2_VECTOR
    for(ii=0; ii<8; ii++)
    {
        red_buf_vec[ii] = vec_red_data[spo2_send_cnt+ii];
        ir_buf_vec[ii] = vec_ir_data[spo2_send_cnt+ii];
        green_buf_vec[ii] = vec_green_data[spo2_send_cnt+ii];
    }
    spo2_send_cnt = spo2_send_cnt+8;
    *count = 8;
    for(ii=0; ii<10; ii++)
    {
        gsen_fifo_x[ii] = vec_red_data[spo2_send_cnt1+ii];;
        gsen_fifo_y[ii] = vec_ir_data[spo2_send_cnt1+ii];;
        gsen_fifo_z[ii] = vec_green_data[spo2_send_cnt1+ii];;
    }
    spo2_send_cnt1 = spo2_send_cnt1+10;
    hx3918_spo2_alg_send_data(red_buf_vec, ir_buf_vec, green_buf_vec, *count, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z);
#else
    if(hx3918_spo2_read(&ppg_s_dat) == 0)
    {
        return;
    }
#ifndef GSEN_40MS_TIMMER
    //readFifoByHrsModule();
#endif
    
    datacnt = 0;
    for(ii=0; ii<*count; ii++)
    {
	
//        bc_alg_check_wear_ir_append(ir_buf[ii]);
#ifdef GSENSER_DATA
        gsen_fifo_x_send[ii] = gsen_fifo_x[16-*count+ii];
        gsen_fifo_y_send[ii] = gsen_fifo_y[16-*count+ii];
        gsen_fifo_z_send[ii] = gsen_fifo_z[16-*count+ii];
#endif
//                    DEBUG_PRINTF("%d/%d %d %d %d %d %d %d \r\n" ,1+ii,*count,\
//                    red_buf[ii],ir_buf[ii],cal.red_idac,cal.ir_idac,cal.red_cur,cal.ir_cur);
        if(wave_upload_flag == 1)
        {
            reddata[datacnt] = red_buf[ii];
            irdata[datacnt] = ir_buf[ii];
#ifdef GSENSER_DATA
            accdata[datacnt][0] = gsen_fifo_x_send[ii];
            accdata[datacnt][1] = gsen_fifo_y_send[ii];
            accdata[datacnt][2] = gsen_fifo_z_send[ii];
#endif
            if(++datacnt >= *count){
//                ble_ServicesInfo_send_Spo2Acc(wave_seq,reddata,irdata,accdata,datacnt);    //????
				if(red_and_ir_data_callback != NULL)
				{
					red_and_ir_data_callback(reddata,datacnt,irdata,datacnt,&accdata[0][0]);
				}
                datacnt = 0;
            }
        }
//        else if(wave_upload_flag == 2)
//        {
//            reddata[datacnt] = red_buf[ii];
//            irdata[datacnt] = ir_buf[ii];
//            red_idac[datacnt] = cal.red_idac;
//            ir_idac[datacnt] =  cal.ir_idac;
//            red_cur[datacnt] = cal.red_cur;
//            ir_cur[datacnt] = cal.ir_cur;
//            if(++datacnt >= *count){
//                ble_ServicesInfo_send_Spo2AlgRes(reddata,irdata,red_idac,ir_idac,red_cur,ir_cur,datacnt);    //????
//                datacnt = 0;
//            }
//        }
//                    DEBUG_PRINTF("%d/%d %d %d %d %d %d %d \r\n" ,1+ii,*count,\
//                    red_buf[ii],ir_buf[ii],cal.red_idac,cal.ir_idac,cal.red_cur,cal.ir_cur);
    }

    spo2_wear_status = hx3918_spo2_get_wear_status();    
	if(spo2_signal_check_callback != NULL)
	{
		spo2_signal_check_callback(spo2_wear_status);
	}
	hrsresult.wearstatus = spo2_wear_status;
    if(spo2_wear_status == MSG_SPO2_WEAR)
    {
        tyhx_spo2_alg_send_data(red_buf, ir_buf, cal.red_idac, cal.ir_idac, *count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send);
    }
#endif
    alg_results = tyhx_spo2_alg_get_results();
	if( spo2_result_callback!=NULL )
	{
		spo2_result_callback(alg_results.spo2_result,alg_results.hr_result);
	}
    hrsresult.lastestspo2 = alg_results.spo2_result;
    DEBUG_PRINTF("hrsresult.wearstatus =%d,SPO2=%d\r\n",spo2_wear_status,hrsresult.lastestspo2);

#ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        for(ii=0; ii<*count; ii++)
        {
#ifdef SPO2_DATA_CALI
            ir_data_cali = hx3918_ir_data_cali(ir_buf[ii]);
            red_data_cali = hx3918_red_data_cali(red_buf[ii]);
            rawdata.red_raw_data = red_data_cali;
            rawdata.ir_raw_data = ir_data_cali;
#else
            rawdata.red_raw_data = red_buf[ii];
            rawdata.ir_raw_data = ir_buf[ii];
#endif
            rawdata.vector_flag = SPO2_VECTOR_FLAG;
            //rawdata.data_cnt = alg_results.data_cnt-*count+ii;
            rawdata.data_cnt = cal.green_idac;
            rawdata.hr_result = alg_results.spo2_result;
//            rawdata.gsensor_x = gsen_fifo_x[ii];
//            rawdata.gsensor_y = gsen_fifo_y[ii];
//            rawdata.gsensor_z = gsen_fifo_z[ii];
            rawdata.gsensor_x = green_buf[ii]>>5;
            rawdata.gsensor_y = cal.red_idac;
            rawdata.gsensor_z = cal.ir_idac;
            rawdata.red_cur = cal.red_cur;
            rawdata.ir_cur = cal.ir_cur;
            ble_rawdata_vector_push(rawdata);
        }
    }
    ble_rawdata_send_handler();
#endif
}
#endif

#ifdef HRV_ALG_LIB
void hx3918_hrv_ppg_Int_handle(void)
{
    uint8_t        ii=0;
    hrv_results_t alg_hrv_results= {MSG_HRV_ALG_NOT_OPEN,0,0,0,0,0};
    int32_t *PPG_buf = &(ppg_s_dat.green_data[0]);
    int32_t *ir_buf = &(ppg_s_dat.ir_data[0]);
    uint8_t *count = &(ppg_s_dat.count);
    hx3918_hrv_wear_msg_code_t wear_mode;

#ifdef HRV_TESTVEC
    int32_t hrm_raw_data;
    hrm_raw_data = vec_data[vec_data_cnt];
    vec_data_cnt++;
    alg_hrv_results = hx3918_hrv_alg_send_data(hrm_raw_data, 0, 0);
#else
    if(hx3918_hrv_read(&ppg_s_dat) == NULL)
    {
        return;
    }
    for(ii=0; ii<*count; ii++)
    {
			//DEBUG_PRINTF("HX_data:%d/%d %d\r\n" ,1+ii,*count,PPG_buf[ii]);
    }
    wear_mode = hx3918_hrv_get_wear_status();
	hrsresult.wearstatus = wear_mode;
    if(wear_mode==MSG_HRV_WEAR)
    {
//        alg_hrv_results = tyhx_hrv_alg_send_bufdata(PPG_buf, *count, 0);
//        hrsresult.lastesthrv = alg_hrv_results .hrv_result;
//        TYHX_LOG("get the hrv = %d\r\n",alg_hrv_results .hrv_result);
    }
#endif

#ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;

        HRS_CAL_SET_T cal= get_hrs_agc_status();
        for(ii=0; ii<*count; ii++)
        {
            rawdata.vector_flag = HRS_VECTOR_FLAG;
            rawdata.data_cnt = 0;
            rawdata.hr_result = alg_hrv_results.hrv_result;
            rawdata.red_raw_data = PPG_buf[ii];
            rawdata.ir_raw_data = 0;
            rawdata.gsensor_x = gsen_fifo_x[ii];
            rawdata.gsensor_y = gsen_fifo_y[ii];
            rawdata.gsensor_z = gsen_fifo_z[ii];
            rawdata.red_cur = cal.led_cur;
            rawdata.ir_cur = 0;
            ble_rawdata_vector_push(rawdata);
        }
    }
    ble_rawdata_send_handler();
#endif
}
#endif


#ifdef CHECK_TOUCH_LIB
void hx3918_wear_ppg_Int_handle(void)
{
    uint8_t count = 0;
    uint8_t ii = 0;
    hx3918_wear_msg_code_t hx3918_check_mode_status = MSG_NO_WEAR;
    count = hx3918_check_touch_read(&ppg_s_dat);
    hx3918_check_mode_status = hx3918_check_touch(ppg_s_dat.s_buf,count);
    hrsresult.wearstatus = hx3918_check_mode_status;
    

#ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        //for(ii=0; ii<*count; ii++)
        //app_ctrl_test = true;
        {
            rawdata.vector_flag = HRS_VECTOR_FLAG;
            rawdata.data_cnt = 0;
            rawdata.hr_result = 0;
            rawdata.red_raw_data = 0;
            rawdata.ir_raw_data = glp[0];
            rawdata.gsensor_x = gbl[0];
            rawdata.gsensor_y = gdiff[1];
            rawdata.gsensor_z = nv_base_data[0];
            rawdata.red_cur = 0;
            rawdata.ir_cur = 0;

            ble_rawdata_vector_push(rawdata);
        }
    }
    ble_rawdata_send_handler();
#endif

}
#endif


void hx3918_lab_test_Int_handle(void)
{
#ifdef LAB_TEST
#if defined(TIMMER_MODE)
    uint32_t data_buf[4];
    hx3918_lab_test_read_packet(data_buf);
    //DEBUG_PRINTF("%d %d %d %d\r\n", data_buf[0], data_buf[1], data_buf[2], data_buf[3]);
#else
    uint8_t count = 0;
    int32_t buf[32];
    count = hx3918_read_fifo_data(buf, 2, 1);
    for (uint8_t i=0; i<count; i++)
    {
        //DEBUG_PRINTF("%d/%d %d %d\r\n" ,1+i,count,buf[i*2],buf[i*2+1]);
    }
#endif
#endif
}
#endif //CHIP_HX3918


bool hx3918_hr_data_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	green_data_callback = (ppg_green_data_callback)function_callback;
	return true;
}

bool hx3918_spo2_data_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	red_and_ir_data_callback = (ppg_red_and_ir_data_callback)function_callback;
	return true;
}


bool hx3918_spo2_hr_data_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	red_and_ir_and_gre_data_callback = (ppg_red_and_ir_and_gre_data_callback)function_callback;
	return true;
}

bool hx3918_hr_result_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	hr_result_callback = (ppg_hr_result_callback)function_callback;
	return true;
}

bool hx3918_spo2_result_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	spo2_result_callback = (ppg_spo2_result_callback)function_callback;
	return true;
}

bool hx3918_spo2_signal_check_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	spo2_signal_check_callback = (ppg_signal_check_callback)function_callback;
	return true;
}

bool hx3918_hr_signal_check_callback_register(void *function_callback)
{
	if(function_callback == NULL)
	{
		return false;
	}
	hr_signal_check_callback = (ppg_signal_check_callback)function_callback;
	return true;
}



