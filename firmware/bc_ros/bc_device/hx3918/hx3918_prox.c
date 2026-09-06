/*****************************************************************************
    * @file     hx90xx.c
    * @author
    * @date
    * @Emial
    * @brief    HX3918 module
******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "hx3918_prox.h"
#include "hx3918.h"

#ifdef TYHX_DEMO
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble_hci.h"
#include "ble_gap.h"
#include "ble_err.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "segger_rtt.h"
#include "twi_master.h"
#include "app_timer.h"
#endif

/* 产品需要改动的地方
#define NORMALWEARDIFF      6000     //HX3918 靠近感应佩戴时感应的结果, 一般是宽松佩戴结果
hx_set_wear_detect_mode(NORMAL_WEAR_MODE)  // 在手表平常测试佩戴用
hx_set_wear_detect_mode(SLEEP_WEAR_MODE)    //在手表检测到进入睡眠后需要调用
*/

/* --------------------------------程序说明------------------------------------------------------------
1. 本程序HX3918作为佩戴感应部分的控制程序
2.  HX3918 内置3通道CAP传感器;
3. 读取的RAW DATA是最原始的模拟量转换过来的数字量,
   LP是经过一次滤波来的(滤除高频环境噪声),
   BL是RAW经过两次滤波来的（滤除低频噪声包含温度、湿度、压力带来的漂移），
4. 有两种工作模式供实际使用选择,
    相对值模式, 开机后,芯片自校准感应值到0附近, 靠近和远离,LP-BL会产生一个大的差值, 相对值模式;
	绝对值模式,需要用户在工厂的时候, 产测程序要记录手表对空的一个BASE值(LP值), 后面用LP-BASE值来做判断;
	相对值模式流程稍微简单, 但是因为上电会自我校准, 所以佩戴着开机无法检测佩戴情况. 绝对值模式无此问题.
5. 	hx3918_chip_id 是芯片ID的变量, 正确的ID值是0x27, 因为HX3918 是PPG和CAP, 用这个变量判断是不是芯片已经上电,
	且PPG部分已经初始化, 如果等于0X27, 这边控制不需要上电下电, 如果碰到产测模式,CAP传感器需要单独工作,
	PPG部分没有同时被初始化, 所以读到的hx3918_chip_id = 0, 那这里就是会加上芯片的上下电控制
6.  注意CSx和CHx, CSx是指外部PAD接到芯片的对应CS管脚. CH是指内部采集数据的3个通道, CSx可配置在不同的CH上;
-------------------------------- -------------------------------- -----------------------------------*/

#ifdef  SAR_ALG_LIB
//#define TEMP_COMPENSATION

#define INDEPENDDENT_CH  // 独立通道宏定义, 就是一个PAD对应一个CH
//#define DIFFERENCE_CH      //差分通道定义, 两个PAD作为一个CH的差分正端和负端, 挨着胳膊的佩戴为正端


//#define HRS_VDD_ALWAYS_ON   //HRS 模块长供电

extern uint8_t hx3918_chip_id;   // 芯片ID, 应该等于0x27
int16_t nv_wear_offset[2] = {0};    // 芯片产测校准的两个通道的OFFSET值, 值记录在fash里面
int16_t nv_base_data[2] = {0};     // 芯片产测校准的两个通道的对空LP值, 值记录在flash里面
//extern uint16_t sendcnt;    // 蓝牙数据采集计数, 这个是采数用的, 可以不用
int32_t gdiff[2] = {0};      // gdiff[0], gdiff[1] 分别保存WEAR_CH, REF_CH的结果
int16_t glp[2] = {0};
int16_t gbl[2] = {0};

//flash 读写变量, 依次记录nv_wear_offset[0],nv_wear_offset[1],nv_base_data[0],nv_base_data[1]
uint32_t fds_read[4] = {0}; //读出flash 存储数据的变量;
uint32_t fds_write[4] = {0}; //写flash的变量, flash里面分别记录


static uint8_t data_updata_ready = 0;
static uint8_t data_convert_ready = 0;
uint8_t  is_factory_mode = 0;    // 产测模式, 用来读取整机对空时OFFSET和对空的nv_base值;
uint8_t  is_offset_manual = 0;    // = 0自动校准模式, 用来读取整机对空时OFFSET和对空的nv_base值;
static bool cap_enable = true;


void hx3918_set_cap_enable(bool enable)
{
	cap_enable = enable;
}

void hx3918_set_cap_data_ready(uint8_t data_ready)
{
	data_updata_ready = data_ready;
}

/**
 * 读一组16bit 数据值
 * @param addh 寄存器地址
 * @param addl 寄存器地址
*/
int16_t hx3918_read_two_reg_low_triple(uint8_t addh, uint8_t addl)
{
    uint8_t val_l = 0;
    uint8_t val_h = 0;
    uint8_t val_m= 0;
    int32_t data = 0;
		hx3918_write_reg(0x35,0x08); // man lock
    val_l = hx3918_read_reg(addl-1);
    val_m = hx3918_read_reg(addl);
    val_h = hx3918_read_reg(addh);
		hx3918_write_reg(0x35,0x00); // unlock
    data =((val_l>>4) | (val_m<<4)|((val_h & 0x0f) << 12)) ;
    if (data > 32767)
        return (int16_t)(data - 65536);
    else
        return (int16_t)data;
}

/**
 * 芯片上电,  对应的电源控制管脚跟实际电路相关,根据实际电路做好控制
* @param NULL
*/
void hx3918_power_up(void)
{
#ifdef HRS_VDD_ALWAYS_ON
    nrf_gpio_pin_set(4);		//芯片的电源打开
#else
    nrf_gpio_pin_set(4); //io 控制 心率VDD打开
#endif
    nrf_delay_ms(50);
}

/**
 * 芯片下电,  对应的电源控制管脚跟实际电路相关,根据实际电路做好控制
* @param NULL
*/
void hx3918_power_down(void)
{

#ifdef HRS_VDD_ALWAYS_ON
    nrf_gpio_pin_set(4);		//芯片电源打开
#else
    nrf_gpio_pin_clear(4); //io 控制 心率VDD关闭
#endif

    nrf_delay_ms(50);
}

/**
 * 芯片重新上电
* @param NULL
*/
void hx3918_power_restart(void)
{
    hx3918_power_down();
    hx3918_power_up();
}

/**
 * HX3918 读芯片ID
* @param NULL
*/
uint8_t hx3918_check_device_id(void)
{
    return hx3918_read_reg(0x00);
}


/**
 * HX3918 写OFFSET的值
* @param offset_value 写OFFSET的值
* @param chx_offset 通道0/1/2 选择
*/
void  hx3918_write_offset(uint16_t offset_value, uint8_t chx_offset)
{
    uint8_t  write_val = 0;
    uint8_t byte_high =0,  byte_low = 0;

    byte_high = 0x61 + chx_offset*2;
    byte_low =	0x60 + chx_offset*2;
    write_val = ((uint8_t)(offset_value >> 8))&0xF;
    hx3918_write_reg(byte_high,write_val);
    write_val = (uint8_t)offset_value;
    hx3918_write_reg(byte_low,write_val);

}

/**
 * HX3918写OFFSET的值, 用在产测程序里面的OFFSET读取
* @param offset_value 写OFFSET的值
* @param chx_offset 通道0/1/2 选择
*/
static int16_t readlp_after_writeoffset(int16_t offset,uint8_t chx_offset)
{
    int16_t lpout;
    hx3918_write_offset(offset,chx_offset);
    hx3918_delay(32);
    lpout = hx3918_read_diff_lp(chx_offset);
    hx3918_delay(33);
    lpout = hx3918_read_diff_lp(chx_offset);
    return lpout;
}

/**
 * HX3918 读OFFSET的值
* @param chx_offset 通道0/1/2 选择
*/
uint16_t  hx3918_read_offset(uint8_t chx_offset)
{
    uint16_t writeoffset = 0;
    int16_t lp=0;
    uint16_t offset = 0;
    uint16_t recordoffset = 0;
    uint16_t mid;
    static uint8_t cnt = 0;
    hx3918_write_reg(0x8F,0x07); 	//offset_dac_en_i2c
    recordoffset = 1020;
    cnt = 0;
    while(writeoffset<=recordoffset)
    {
        mid =(writeoffset+recordoffset)/2;
        lp = readlp_after_writeoffset(mid,chx_offset);

        if(hx_abs(lp)>READ_DEVATION)
        {
            if(lp>READ_DEVATION)
            {
                writeoffset = mid+1;
            }
            else
            {
                recordoffset = mid-1;
            }
            cnt ++;
        }
        else
        {
            offset = mid;
            TYHX_LOG(" final ch=%d,offset =%d lp=%d\r\n",chx_offset,offset,lp);
            break;
        }
        //TYHX_LOG(" ch=%d,lp =%d,writeoffset=%d,recordoffset=%d\r\n",chx_offset,lp,writeoffset,recordoffset);
    }
    return offset;
}


/**
 * HX3918读取raw data 或者baseline  data  //读取的是raw还是bl 通过0x38 寄存器设置
* @param chx_raw 通道0/1/2 选择
*/
int16_t  hx3918_read_raw_bl(uint8_t chx_raw)
{
    int16_t raw = 0;
    uint8_t byte_high =0,  byte_low = 0;

    byte_high = 0xBE + chx_raw*3;
    byte_low = 0xBD + chx_raw*3;

    raw = hx3918_read_two_reg_low_triple(byte_high,byte_low);
    return raw;
}


/**
 * HX3918 读取LP data 或者diff data   //读取的是lp还是diff通过0x38 寄存器设置
* @param chx_raw 通道0/1/2 选择
*/
int16_t  hx3918_read_diff_lp(uint8_t chx_diff)
{
    int16_t diff = 0;
    uint8_t byte_high =0,  byte_low = 0;

    byte_high = 0xC7+ chx_diff*3;
    byte_low =	0xC6 + chx_diff*3;

    diff = hx3918_read_two_reg_low_triple(byte_high,byte_low);
    return diff;
}

/**
 * HX3918 设置接近阈值和远离阈值
* @param chx 通道0/1/2 选择
* @param high_thres 接近阈值
* @param low_thres 远离阈值
* @param base_data
*/
void hx3918_config_thres(uint8_t chx,int16_t high_thres, int16_t low_thres, int16_t base_data)
{
    
    //TYHX_LOG("[config_thres]CHX = %d, high thres =%d, low_thres=%d, base_data=%d\r\n",chx,high_thres,low_thres,base_data);
    //if ((low_thres + base_data) <= 0)
    {
      //  return;
    }
    //TYHX_LOG("[config_thres]CHX = %d, high thres =%d, low_thres=%d\r\n",chx,high_thres,low_thres);
    int16_t thres = 0;

 	
		thres = (high_thres + base_data)/32;
		if(thres<0)
		{
			thres = 0;
		}
    
    TYHX_LOG("[config_thres]CHX = %d, high thres =%d\r\n",chx,thres);
    hx3918_write_reg(0x9D+1*chx, (thres&0x00FF));
    hx3918_write_reg(0xA0+1*chx, (thres&0xFF00)>>8);

    thres = (low_thres + base_data)/32;
		if(thres<0)
		{
			thres = 0;
		}

    TYHX_LOG("[config_thres]CHX = %d,low_thres=%d\r\n",chx,thres);
    hx3918_write_reg(0xA5+1*chx, (thres&0x00FF));
    hx3918_write_reg(0xA8+1*chx, (thres&0xFF00)>>8);
}


/**
 * HX3918 init
* @param NULL
*/
bool hx3918_prox_reg_init(void)
{
    bool write_ok = true;
		data_updata_ready = 0;
		data_convert_ready = 0;
//prf 默认25HZ
//    if (hx3918_chip_id != PROX_CHIPID)
//    {
//        TYHX_LOG("100hz\r\n");
//        write_ok &= hx3918_write_reg(0x18,0x00);    //prf =100HZ
//        write_ok &= hx3918_write_reg(0x19,0x05);    //
//    }
    write_ok &= hx3918_write_reg(0x6a,0x00);    //CH0~N disable
//    write_ok &= hx3918_write_reg(0x51,0x00);    //default
//    write_ok &= hx3918_write_reg(0x52,0x02);    //default
//    write_ok &= hx3918_write_reg(0x53,0x00);    //default


#ifdef INDEPENDDENT_CH //由实际硬件决定, 要看电路图
    write_ok &= hx3918_write_reg(0x55,0x30);   //CS2->CH0
    write_ok &= hx3918_write_reg(0x57,0x0C);   //CS1->CH1 检测通道
#elif defined(DIFFERENCE_CH) //由实际硬件决定, 要看电路图
    write_ok &= hx3918_write_reg(0x55,0x30);   // CS2->CH1
    write_ok &= hx3918_write_reg(0x57,0x2c);   // CS1->CH0[P],CS2->CH0[N]
#endif
		write_ok &= hx3918_write_reg(0x59,0x0c);   //CH2  CS1


#ifdef FULL25PF
    write_ok &= hx3918_write_reg(0x5B,0x11);   //full range = 2.5pf 0x11=2.5pf 0x22 = 3.75pf, 0x33 = 5pf
#elif defined(FULL125PF)
		write_ok &= hx3918_write_reg(0x5B,0x00);   //full range 0x00 = 1.25pf
#elif defined(FULL375PF)
    write_ok &= hx3918_write_reg(0x5B,0x22);   //full range = 2.5pf 0x11=2.5pf 0x22 = 3.75pf, 0x33 = 5pf
#elif defined(FULL50PF)
    write_ok &= hx3918_write_reg(0x5B,0x33);   //full range = 2.5pf 0x11=2.5pf 0x22 = 3.75pf, 0x33 = 5pf
#endif
    write_ok &= hx3918_write_reg(0x5D,0x25);   //OSR AVG
    write_ok &= hx3918_write_reg(0x5E,0x22);   //OSR nosr1_num_i2c nosr2_num_i2c
    write_ok &= hx3918_write_reg(0x5F,0x11);   //AVG ch1_avg_num_i2c ch2_avg_num_i2c

    write_ok &= hx3918_write_reg(0x66,0x08);   //采样时间
    write_ok &= hx3918_write_reg(0x67,0x00);   //
    write_ok &= hx3918_write_reg(0x68,0x08);   //积分时间
    write_ok &= hx3918_write_reg(0x69,0x00);   //

    write_ok &= hx3918_write_reg(0x6E,0x11);   //LP alpha
    write_ok &= hx3918_write_reg(0x77,0x70);   //output 选择LP和BL

    //write_ok &= hx3918_write_reg(0x99,0x40);  // ch0 coe  增益配置
		//write_ok &= hx3918_write_reg(0x9A,0x40);   // ch1 coe  增益配置
		//write_ok &= hx3918_write_reg(0x94,0x02);   // diff_en_ch01
		
				
    if(is_offset_manual == 0)
    {
        TYHX_LOG("is_offset_manual = 0\r\n");

        write_ok &= hx3918_write_reg(0x90,(0x1<<WEAR_CH)); //ch1=P+ ch0=N-
        write_ok &= hx3918_write_reg(0x8F,0x00);//00
        write_ok &= hx3918_write_reg(0x95,0x00);
        hx3918_config_thres(WEAR_CH,sar_wear_thres, sar_unwear_thres,0); // 
        hx3918_config_thres(REF_CH,REF_HIGH_DIFF, REF_LOW_DIFF,0); // 

    }
    else
    {

        write_ok &= hx3918_write_reg(0x90,(0x1<<WEAR_CH));////ch1=P+ ch0=N-
				write_ok &= hx3918_write_reg(0x8F,(0x1<<WEAR_CH)); // ch1 offset_dac_en
        write_ok &= hx3918_write_reg(0x95,0x00);//0xaa
        hx3918_write_offset(nv_wear_offset[0], WEAR_CH);
       //hx3918_write_offset(nv_wear_offset[1], REF_CH);
        hx3918_config_thres(WEAR_CH,sar_wear_thres, sar_unwear_thres,0); // wear0
        hx3918_config_thres(REF_CH,REF_HIGH_DIFF, REF_LOW_DIFF,0);
				
				//write_ok &= hx3918_write_reg(0x7A,0x07|(0x10<<WEAR_CH)); // 打开转换中断
				write_ok &= hx3918_write_reg(0x9c,0x00); // lp_out_delta_bypass
				write_ok &= hx3918_write_reg(0x85,0x0f); // prox_int_high_num_i2c
				write_ok &= hx3918_write_reg(0xa3,0xff); //lp_out_delta_thres_ch1[7:0]
				write_ok &= hx3918_write_reg(0xa4,0x0f); // lp_out_delta_thres_ch1[11:8]
				write_ok &= hx3918_write_reg(0x70,0x11); // RA_UP_ALP_1_0_CFG
				//write_ok &= hx3918_write_reg(0x71,0x11); //
				//write_ok &= hx3918_write_reg(0x72,0x11); //
				//write_ok &= hx3918_write_reg(0x73,0x11); //

    }
  	write_ok &= hx3918_write_reg(0x6a,0x03); //CH0, CH1 enable
   // hx3918_delay(50);
    return write_ok;
}


//蓝牙发送数据给APP
//extern uint32_t sendDataToClient(int16_t sendnum1,int16_t sendnum2,int16_t sendnum3,int16_t sendnum4);

/**
 * hx3918 获取靠近感应的结果,结果=0, prox感应的结果为非佩戴, =1 佩戴
* @param NULL
*/
uint8_t hx3918_report_prox_status(void)
{
    glp[0] = hx3918_read_diff_lp(WEAR_CH);
    gbl[0] = hx3918_read_raw_bl(WEAR_CH);

    glp[1] = hx3918_read_diff_lp(REF_CH);
    gbl[1] = hx3918_read_raw_bl(REF_CH);

		if ((glp[0] == 0) && (gbl[0] == 0))  // agc 调节时出现
		{
			data_updata_ready = 0;
		}
		
		if (data_convert_ready == 1&& (glp[0] != gbl[0]))
		{
			if (is_offset_manual == 0)
			{
				gdiff[0] = glp[0]- gbl[0];
			}
			else
			{
				gdiff[0] = glp[0]- nv_base_data[0];
			}
			gdiff[1] = glp[1]- gbl[1];
			data_updata_ready = 1;
			hx3918_update_min_nv_data(gbl[0]);
		}
		
		if ((glp[0] == gbl[0]) && (glp[0] != 0) && is_offset_manual ==1)
		{
			if (data_convert_ready == 0)
			{
				// 恢复寄存器
				hx3918_write_reg(0x9c,0x07); // lp_out_delta_bypass
				//hx3918_write_reg(0x85,0x0f); // prox_int_high_num_i2c
				//hx3918_write_reg(0xa3,0xff); //lp_out_delta_thres_ch1[7:0]
				//hx3918_write_reg(0xa4,0x0f); // lp_out_delta_thres_ch1[11:8]
				hx3918_write_reg(0x70,0x00); // RA_UP_ALP_1_0_CFG
				data_convert_ready = 1;
				TYHX_LOG("data_convert_ready == 1\r\n");
			}
		}
		
		TYHX_LOG("SARresult ready: %d, lp,%d,%d, bl,%d,%d, giff, %d,%d, offset, %d, nv_data,%d\r\n",data_updata_ready, glp[0],glp[1], gbl[0],gbl[1],gdiff[0],gdiff[1], nv_wear_offset[0],nv_base_data[0]);
	return 0;
}


int32_t hx90xx_get_prox_data(void)
{
	if (data_updata_ready == 1)
	{
		// 异常处理
		if (nv_base_data[0] <-16384|| nv_base_data[0] > 25000 || hx3918_chip_id != PROX_CHIPID)
		{
			gdiff[0] = 32767;
		}	
	}
	
	if (is_offset_manual == 0 || cap_enable == false)
	{
		gdiff[0] = 32767;
	}
	//DEBUG_PRINTF("gdiff[0]: %d \r\n", gdiff[0]); 

	return gdiff[0];
}

	//红外值很小时校准
void hx3918_cali_with_ir(int32_t infrared_data, int32_t ir_thres)   
{
	//设定 对空红外值 校准
	int32_t unwear_infrared_data = ir_thres/4;
	if (data_updata_ready == 1)
	{
		if (infrared_data < unwear_infrared_data)
		{
			if ((gdiff[0] > sar_unwear_thres/4)&& (gdiff[0] < sar_unwear_thres))
			{
					nv_base_data[0] = gbl[0];
			}
		}
	}
	DEBUG_PRINTF("infrared_data: %d , unwear_infrared_data %d, sar_unwear_thres: %d,gdiff[0],%d\r\n", infrared_data, unwear_infrared_data,sar_unwear_thres,gdiff[0]); 
}


//更新最小值
void hx3918_update_min_nv_data(int16_t min_data)
{
	uint8_t nv_change_flag = 0;
	if (is_offset_manual == 1&&data_updata_ready == 1)
	{
		// 取得最小信号量值
		if(min_data < nv_base_data[0]-50)
		{  
			nv_base_data[0] = min_data;
			nv_change_flag = 1;
			
		}

		if (nv_change_flag == 1)
		{
//			hx90xx_config_thres(CH0,ch0_high_diff, ch0_low_diff,nv_base_data[0]); // wear0
//			hx90xx_config_thres(CH1,ch1_high_diff, ch1_low_diff,nv_base_data[1]); // wear1
			DEBUG_PRINTF("90XX update_min_nv_data nv_base_data0: %d,\r\n", nv_base_data[0]);
		}
	}
}


// 在充电状态下调用, 每次充电只调用一次
void hx3918_update_nv_data()
{
	int16_t min_data0 = {0};
	uint8_t nv_change_flag = 0;
	
	glp[0] = hx3918_read_diff_lp(WEAR_CH);
	min_data0 = hx3918_read_raw_bl(WEAR_CH);

	if (is_offset_manual == 1)
	{
		//nv_wear_offset[0] = hx3918_read_offset(CH0_OFFSET);
		// 取得对空信号值
		if((min_data0 - nv_base_data[0]) > 100 )
		{  
			nv_base_data[0] = nv_base_data[0] + (min_data0 - nv_base_data[0])/2;
			nv_change_flag = 1;
		}

		// 异常处理
		if (nv_base_data[0] <-16384|| nv_base_data[0] > 20000 || glp[0] > 32000)
		{
			hx3918_prox_factory_hanging_cali();
			return;
		}
		if (nv_change_flag == 1)
		{
			hx3918_factory_cali_save_nv();
		}
		hx3918_prox_init();
		DEBUG_PRINTF("90XX update_nv_data nv_base_data0: %d, nv_base_data1: %d\r\n", nv_base_data[0], nv_base_data[1]);
	}
	else if (is_offset_manual == 0)
	{
		hx3918_prox_factory_hanging_cali();
		DEBUG_PRINTF("is_offset_manual == 0 %d, %d \r\n", nv_base_data[0], nv_wear_offset[0]);
	}
}


/**
 * hx3918 从flash读取对空校准后的OFFSET和NV_BASE
* @param NULL
*/
void hx3918_read_nv(void)
{

    //从flash读取4个值, 分别是以前保存的值; 这个函数用户需要自己完成, flash存储的位置不能被用户操作删掉
    //flash_fds_data_get(4);
			nv_wear_offset[0] = fds_read[0]; //fds_read[0];
			nv_wear_offset[1] =  fds_read[1];//fds_read[1];
			nv_base_data[0] = fds_read[2];//fds_read[2];
			nv_base_data[1] = fds_read[3];//fds_read[3];
//    nv_wear_offset[0] = 681; //fds_read[0];
//    nv_wear_offset[1] =  404;//fds_read[1];
//    nv_base_data[0] = -556;//fds_read[2];
//    nv_base_data[1] = 10;//fds_read[3];

	//如果flash记录的值大于上限, 则让靠近感应进入相对值模式
    if ( (nv_wear_offset[0] == 0&& nv_base_data[0] == 0) || nv_wear_offset[0] == 0xFFFF)
    {
			nv_wear_offset[0] = 0;
			nv_base_data[0] = 0;
			nv_wear_offset[1] = 0;
			nv_base_data[1] = 0;
			cap_enable = false;
			is_offset_manual = 0;
    }
		else
		{
			is_offset_manual = 1;
			cap_enable = true;
		}

		TYHX_LOG("hx3918_read_nv ,OFF[0]:	%d,  OFF[1]:%d\r\n",nv_wear_offset[0],nv_wear_offset[1]);
		TYHX_LOG("BASE[0]:	%d, BASE[1]:%d,mode: %d \r\n",nv_base_data[0],nv_base_data[1],is_offset_manual);


    
}

/**
 * hx3918 靠近感应初始化, 通过对hx3918_chip_id 判断,来判断靠近感应是独立工作还是跟PPG一起工作
* @param NULL
*/
bool hx3918_prox_init(void)
{
    int32_t i = 0;
    bool ret = false;
    TYHX_LOG("[hx3918_prox_init]prox device_id is : 0x%.2x !\r\n", hx3918_chip_id);
    if (hx3918_chip_id != PROX_CHIPID)
    {
        hx3918_power_restart();
        //i2c_hrs_init();
				twi_master_init();
        hx3918_read_nv();
        hx3918_ppg_on();
        
        for (i = 0; i<5; i++)
        {
            hx3918_chip_id = hx3918_check_device_id();
            TYHX_LOG(" power restart ,prox device_id is : 0x%.2x !\r\n", hx3918_chip_id);
            if (hx3918_chip_id == PROX_CHIPID)
            {
								hx3918_prox_reg_init();
                break;
            }
        }

        if(hx3918_chip_id != PROX_CHIPID)
        {
            TYHX_LOG("no proximity device\r\n");
            //i2c_hrs_uninit();
            hx3918_power_down();
        }
    }
    else
    {
        hx3918_chip_id = hx3918_check_device_id();
        TYHX_LOG("read prox device_id is : 0x%.2x !\r\n", hx3918_chip_id);
        hx3918_read_nv();
        hx3918_prox_reg_init();
    }
		//hx3918_ppg_off();
    return ret;
}


/**
 * hx3918 靠近感应初始化, 通过对hx3918_chip_id 判断,来判断靠近感应是独立工作还是跟PPG一起工作
* @param NULL
*/
bool hx3918_prox_factory_init(void)
{

    int32_t i = 0;
    bool ret = false;

    hx3918_chip_id = 0;

    hx3918_power_restart();
    //i2c_hrs_init();
    hx3918_ppg_on();
    for (i = 0; i<5; i++)
    {
        hx3918_chip_id = hx3918_check_device_id();
        TYHX_LOG(" power restart ,prox device_id is : 0x%.2x !\r\n", hx3918_chip_id);
        if (hx3918_chip_id == PROX_CHIPID)
        {
						hx3918_write_reg(0x18,0x40);    //prf =100HZ
						hx3918_write_reg(0x19,0x01);    //
					  hx3918_prox_reg_init();
            break;
        }
    }
		
    if(hx3918_chip_id != PROX_CHIPID)
    {
        TYHX_LOG("no proximity device\r\n");
        hx3918_power_down();
    }
    return ret;
}


//佩戴数据
void hx3918_factory_cali_read_data(void)
{
  int16_t lp[2] = {0};
	int16_t bl[3] = {0};
	int32_t diff[3] = {0};
	
	lp[0] = hx3918_read_diff_lp(WEAR_CH);
	bl[0] = hx3918_read_raw_bl(WEAR_CH);

	//lp[1] = hx3918_read_diff_lp(REF_CH);
	//bl[1] = hx3918_read_raw_bl(REF_CH);

	if (is_offset_manual ==1)
	{
		diff[0] = lp[0]- nv_base_data[0];
	}
	else
	{
		diff[0] = lp[0] - bl[0];
	}


  DEBUG_PRINTF("id: 0x%x,offset0,%%d, lp[0], %d, bl[0], %d, diff[0],%d\r\n", hx3918_chip_id,nv_wear_offset[0],lp[0],bl[0],diff[0]);
}

/**
 * hx3918 靠近感应部分 工厂产测程序, 需将手表悬空或者远离其他可能影响电容值的物体.
* @param NULL
*/
void hx3918_prox_factory_hanging_cali(void)
{
    uint16_t offset0 = 0;
    uint16_t offset1 = 0;
    int16_t lp[2] = {0};
    //uint8_t device_id = 0x00;
		is_offset_manual = 0;
    hx3918_prox_factory_init();
    hx3918_delay(50);//delay 100ms

    offset0 = hx3918_read_offset(WEAR_CH);
    offset1 = hx3918_read_offset(REF_CH);


    lp[0] = hx3918_read_diff_lp(WEAR_CH);
    lp[1] = hx3918_read_diff_lp(REF_CH);

    //TYHX_LOG("offset0,offset1=	%d  %d\r\n", offset0,offset1);

    nv_wear_offset[0] = offset0;
    nv_wear_offset[1] = offset1;
    nv_base_data[0] = lp[0];
    nv_base_data[1] = lp[1];
    //TYHX_LOG("base0,base1=	 %d  %d\r\n", lp[0],lp[1]);
    TYHX_LOG("RESULT =	%d	%d	%d	%d\r\n", offset0,offset1,lp[0],lp[1]);

    //将差分数据读到的OFFSET, NV_BASE记录到FLASH里面, 校准存储把自学习的记录也需要清掉重新计算

    if((offset0 == 510) &&(offset1 == 510)&&(lp[0]==0)&&(lp[1]==0))
    {
        TYHX_LOG("ERR,  read offset err, please power down and up, redo this!!!\r\n");
    }
    else
    {
        hx3918_factory_cali_save_nv();
    }

		hx3918_prox_init();
	//hx3918_delay(1000);//上位机延时 1000ms	再去数据 hx3918_factory_cali_read_data

}

/**
 * hx3918 保存校准后的值到FLASH, 此处FLASH空间不能被用户擦除,
 也不能被出厂设置等操作格式化掉;
* @param NULL
*/
void hx3918_factory_cali_save_nv(void)
{

    fds_write[0] = nv_wear_offset[0];
    fds_write[1] = nv_wear_offset[1];

    fds_write[2] = nv_base_data[0];
    fds_write[3] = nv_base_data[1];


    TYHX_LOG("[hx3918_factory_cali_save_nv]");
    for(int i=0; i<4; i++)
    {
        TYHX_LOG("%d   ",fds_write[i]);
    }
	//用户需要flash保存
    //flash_fds_data_write(4);
}

#endif // SAR_ALG_LIB






