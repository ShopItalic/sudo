#include "bc_power.h"


#include "q_device.h"

//#include "sy6103.h"
#include "bc_pmic.h"

#include "bc_logger.h"
#include "bc_delay.h"
#include "bc_ldo_switch.h"

extern void power_manage(void);

typedef void (*power_adc_hardware_error_callback)(void); 

static power_adc_hardware_error_callback hardware_error_callback = NULL;

static q_device_t *vbat_adc_device_handler;

static uint8_t pre_bat_percent = 100;

/* 电池电压-电量映射表（多点分段线性） */
typedef struct
{
    uint16_t voltage_mv;   /* 电压，单位 mV */
    uint8_t  percent;      /* 电量，单位 % */
} bat_vp_table_t;

#if defined(HANDWARE_1_23_2)
/* HANDWARE_1_23_2 锂电池放电曲线（10点，4200mV~3600mV，按电压从高到低排列） */
static const bat_vp_table_t g_bat_vp_table[] = 
{
    {4200, 100},  /* 满电 */
    {4130, 90},
    {4060, 78},
    {4000, 65},
    {3940, 52},
    {3880, 40},
    {3810, 28},
    {3740, 16},
    {3670, 7},
    {3600, 0},   /* 截止电压 */
};
#define BAT_VP_TABLE_SIZE   (sizeof(g_bat_vp_table) / sizeof(g_bat_vp_table[0]))
#endif /* HANDWARE_1_23_2 */

/*******************************************************************************
 * Function Name     : bat_voltage_to_percent
 * Description       : 通过查表+线性插值将电池电压转换为电量百分比
 * Input             : voltage_mv - 电池电压（mV）
 *                     table - 电压-电量映射表（按电压从高到低排列）
 *                     table_size - 表项数量
 * Return            : 电量百分比（0~100）
 *******************************************************************************/
static uint8_t bat_voltage_to_percent(uint16_t voltage_mv, const bat_vp_table_t *table, uint8_t table_size)
{
    if (table == NULL || table_size == 0)
    {
        return 0;
    }

    /* 电压高于最高电压点，返回 100% */
    if (voltage_mv >= table[0].voltage_mv)
    {
        return 100;
    }

    /* 电压低于最低电压点，返回 0% */
    if (voltage_mv <= table[table_size - 1].voltage_mv)
    {
        return 0;
    }

    /* 查找所在区间，线性插值计算 */
    for (uint8_t i = 0; i < table_size - 1; i++)
    {
        if (voltage_mv <= table[i].voltage_mv && voltage_mv >= table[i + 1].voltage_mv)
        {
            uint16_t v_high = table[i].voltage_mv;
            uint16_t v_low  = table[i + 1].voltage_mv;
            uint8_t  p_high = table[i].percent;
            uint8_t  p_low  = table[i + 1].percent;

            /* 线性插值：percent = p_low + (p_high - p_low) * (voltage - v_low) / (v_high - v_low) */
            uint16_t percent = p_low + ((uint16_t)(p_high - p_low) * (voltage_mv - v_low)) / (v_high - v_low);
            return (uint8_t)percent;
        }
    }

    return 0;
}

static void check_power_status(void)
{
	if(bc_pmic_get_charge_status() != PMIC_CHARGED_NOT)
	{
		pre_bat_percent = 100;
	}
}


uint16_t bc_power_get_adc_value(void)
{
	uint16_t temp = 0;
	uint16_t adc_temp = 0;
	bc_ldo_bat_power_on();
	q_device_open(vbat_adc_device_handler);
	bc_delay_ms(10);
	for(uint8_t i = 0; i < 3; i++)
	{
		q_device_read(vbat_adc_device_handler,0,&adc_temp,1);
		temp += adc_temp;
		adc_temp = 0;
	}
	q_device_close(vbat_adc_device_handler);
	temp = temp /3;
	BC_LOG_INFO("bat temp:%d \r\n",temp);
	bc_ldo_bat_power_off();
	return temp ;	
}

uint8_t bc_power_get_vbat_percen(void)
{
#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1 )		
	uint32_t R1 = 499,R2 = 348;
#elif defined(HANDWARE_1_23_2)
  float R1=2000.0, R2=1000.0;   /* 分压电阻：2MΩ / 1MΩ，分压比 1/3 */
#elif (HARDWARE_BCL601_151_ENABLED)	
   	uint32_t R1 = 2000,R2 = 1000;
#elif(HARDWARE_1181_ENABLED || HARDWARE_1191_ENABLED || HARDWARE_1231_ENABLED == 1)
  float R1=2490.0,R2 = 499.0;  
#elif(HARDWARE_1171_ENABLED || HARDWARE_1231x_ENABLED == 1)
  float R1=2000.0,R2 = 1000.0;   
#endif	
  

	uint32_t bat_adc = 0;

	uint8_t cur_bat_percent = 0;
	check_power_status();

  
  
  float bat_adc_float = 0.0;
  float rect_voltag = 0.0;
  float vout_voltage = 0.0;
  uint16_t voltage = 0;
  float vout = 0.0;
  bat_adc_float = bc_power_get_adc_value();
  vout_voltage = (bat_adc_float*3.6)/4096;
#if(HARDWARE_1181_ENABLED || HARDWARE_1191_ENABLED)
  vout_voltage += 0.025948;
#elif(HARDWARE_1171_ENABLED || HARDWARE_1231x_ENABLED == 1)
  vout_voltage -= 0.010338;
#endif	  
 
  vout = (vout_voltage * (R2 /((R1+R2)) )) ;
  rect_voltag = (vout_voltage * (((R1+R2) / R2) )) ;
#if(HARDWARE_1181_ENABLED )
  rect_voltag += 0.3;
#elif(HARDWARE_1171_ENABLED )

#elif(HARDWARE_1231_ENABLED == 1)
 
 rect_voltag += 0.2;

#endif  
  
  voltage = rect_voltag*1000;
  BC_LOG_INFO("bat adc:%f  vout:%f vout_voltage:%f\r\n",bat_adc_float,vout,vout_voltage);
  BC_LOG_INFO("voltage:%d  rect_voltag:%f \r\n",voltage,rect_voltag);
  
  power_manage();
  bat_adc = voltage;


#if (HARDWARE_411_ENABLED == 1 || HARDWARE_412_ENABLED == 1 || HARDWARE_441_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED || HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1 )	


		if(bat_adc > 4100)
		{
			bat_adc = 100;
		}
		else if(bat_adc > 3800)
		{
			bat_adc = ((bat_adc - 3800) * 55) / 300 + 55;
		}
		else if(bat_adc > 3600)
		{
			bat_adc = ((bat_adc - 3600) * 45 ) / 200;
		}
		else
		{
			bat_adc = 1;
		}
	
#elif (HARDWARE_153_ENABLED == 1  || HARDWARE_181_ENABLED == 1 || HARDWARE_182_ENABLED == 1 || HARDWARE_1141_ENABLED == 1|| HARDWARE_191_ENABLED == 1|| HARDWARE_1121_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
     HARDWARE_158_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1171_ENABLED == 1 || HARDWARE_1191_ENABLED || HARDWARE_1231_ENABLED == 1 && !defined(HANDWARE_1_23_2))	
     

	 bat_adc = voltage;

	 BC_LOG_INFO("bat_ad:%d \r\n",bat_adc);
	
		if(bat_adc > 4150)
		{
			bat_adc = 100;
		}
		else if(bat_adc > 3400)
		{
			bat_adc *= 182;
			bat_adc /= 1000;
			bat_adc -= 655;
		}
		else
		{
			bat_adc = 0;
		}
	
#elif defined(HANDWARE_1_23_2)
    /* HANDWARE_1_23_2 使用多点分段线性映射 + 线性插值，提高电量计算准确度 */
    bat_adc = bat_voltage_to_percent(voltage, g_bat_vp_table, BAT_VP_TABLE_SIZE);
    BC_LOG_INFO("bat_voltage:%d mV, bat_percent:%d %% \r\n", voltage, bat_adc);

#endif	
		
		
		if (bat_adc > 100){
			bat_adc = 100;
		}
		
		cur_bat_percent = (uint8_t)bat_adc;
//		if(cur_bat_percent > pre_bat_percent){
//			cur_bat_percent = pre_bat_percent;
//		}else{
//			pre_bat_percent = cur_bat_percent;
//		}
        pre_bat_percent = cur_bat_percent;
	
	BC_LOG_INFO("cur_bat_percent:%d \r\n",cur_bat_percent);
    return cur_bat_percent;
}

bool bc_power_check_vbat(void)
{
	uint16_t temp = 0;
	bc_ldo_bat_power_on();
	q_device_open(vbat_adc_device_handler);
	bc_delay_ms(10);
	q_device_read(vbat_adc_device_handler,0,&temp, 1);
	BC_LOG_INFO("vbat_adc:%d",temp);
	if(temp < 100)
	{
		q_device_close(vbat_adc_device_handler);
		if(hardware_error_callback != NULL)
		{
			hardware_error_callback();

		}
		return false;
	}
	bc_ldo_bat_power_off();
	q_device_close(vbat_adc_device_handler);
	return true;
}

bool bc_power_adc_hardware_error_register_callback(const void *error_callback)
{
	if(error_callback == NULL)
	{
		return false;
	}
	hardware_error_callback = (power_adc_hardware_error_callback)error_callback;
	return true;

}

void bc_power_vbat_adc_find(void)
{
	vbat_adc_device_handler = q_device_find("vbat_adc");
	q_device_assert(vbat_adc_device_handler);
}



