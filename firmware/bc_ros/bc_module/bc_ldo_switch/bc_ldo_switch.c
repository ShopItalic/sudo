#include "bc_ldo_switch.h"

#include "q_device.h"

#include "bc_logger.h"


#include <string.h>

struct __attribute__((__packed__)) bc_ldo_power
{
	unsigned int ppg_power : 1;
	unsigned int mic_power : 1;
	unsigned int motor_power : 1;
	unsigned int temper_power : 1;
	unsigned int flash_power : 1;
	unsigned int rgb_power : 1;
	unsigned int vbat_power : 1;
	unsigned int puf_power : 1;
	unsigned int mouse_power : 1;
	unsigned int imu_power : 1;
	unsigned int pmic_power : 1;
	unsigned int touch_power : 1;
	unsigned int pressure_sensors_power : 1;
	unsigned int  : 3;
};

static struct bc_ldo_power ldo_power ={0};

enum ldo_power_event
{
	PPG_POWER = 0,
	MIC_POWER,
	MOTOR_POWER,
	TEMPER_POWER,
	FLASH_POWER,
	RGB_POWER,
	VBAT_POWER,
	PUF_POWER,
	MOUSE_POWER,
	IMU_POWER,
	PMIC_POWER,
	TOUCH_POWER,
    PRESSURE_SENSORS_POWER,
};

enum ldo_power_flag
{
	LDO_POWER_DISENABLE =0,
	LDO_POWER_ENABLE =1,
};


static q_device_t *ldo_ppg_vdd_power_dev;
static q_device_t *ldo_ppg_led_power_dev;
static q_device_t *ldo_puf_vdd_power_dev;

#if (defined(HANDWARE_BCL601_151))

static q_device_t *ldo_mic_vdd_power_dev = NULL;
static q_device_t *ldo_flash_vdd_power_dev = NULL;

#endif

#if (defined( HANDWARE_1_12_1) || defined( HANDWARE_1_23_1))

static q_device_t *ic_led_vdd_power_dev = NULL;


#endif

#if (defined( HANDWARE_1_12_1) )


static q_device_t *ppg_reset_dev = NULL;

#endif

#if (defined( HANDWARE_1_5_6))

static q_device_t *pressure_sensor_power_dev = NULL;

#endif

#if (defined( HANDWARE_1_5_6) || defined( HANDWARE_1_23_1))

static q_device_t *motor_power_dev = NULL;

#if (defined(HANDWARE_1_23_2)   || defined(HANDWARE_1_23_3))
static q_device_t *motor_vcc_dev = NULL;  
#endif  

#endif



static q_device_t *ldo_bos_vdd_power_dev = NULL;


static bool ldo_power_ppg_led_lock = false;
static bool ldo_power_bat_lock = false;

static void bc_ldo_power_off(enum ldo_power_event power_event)
{
#if defined(HANDWARE_1_5_3)
	switch(power_event)
	{
		case PPG_POWER:
		{
			if(!ldo_power.flash_power && !ldo_power.mic_power && !ldo_power.motor_power && !ldo_power.temper_power && !ldo_power.rgb_power)
			{
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_vdd_power_dev);
				
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			else if(!ldo_power.rgb_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case MIC_POWER:
		{
			if(!ldo_power.flash_power && !ldo_power.ppg_power && !ldo_power.motor_power && !ldo_power.temper_power)
			{
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_vdd_power_dev);
			}
			ldo_power.mic_power = LDO_POWER_DISENABLE;
			break;
		}
		case MOTOR_POWER:
		{
			if(!ldo_power.flash_power && !ldo_power.mic_power && !ldo_power.ppg_power && !ldo_power.temper_power && !ldo_power.rgb_power)
			{
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_vdd_power_dev);
				
				q_device_ctrl(ldo_bos_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_bos_vdd_power_dev);
			}
			else
			{
				q_device_ctrl(ldo_bos_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_bos_vdd_power_dev);
			}
			ldo_power.motor_power = LDO_POWER_DISENABLE;
			break;
		}
		case TEMPER_POWER:
		{
			if(!ldo_power.flash_power && !ldo_power.mic_power && !ldo_power.motor_power && !ldo_power.ppg_power)
			{
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_vdd_power_dev);
			}
			ldo_power.temper_power = LDO_POWER_DISENABLE;
			break;
		}
		case FLASH_POWER:
		{
			if(!ldo_power.temper_power && !ldo_power.mic_power && !ldo_power.motor_power && !ldo_power.ppg_power)
			{
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_vdd_power_dev);
			}
			ldo_power.flash_power = LDO_POWER_DISENABLE;
			break;
		}
		case RGB_POWER:
		{
			if(!ldo_power.ppg_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.rgb_power = LDO_POWER_DISENABLE;
			break;
		}
		default:
		{
			break;
		}
	}

#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))

	switch(power_event)
	{
		case PPG_POWER:
		{
			if(!ldo_power.vbat_power)
			{
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_vdd_power_dev);
				
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			else
			{
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_vdd_power_dev);
			}
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case VBAT_POWER:
		{
			if(!ldo_power.ppg_power)
			{
				
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			
			ldo_power.vbat_power = LDO_POWER_DISENABLE;
			break;
		}
		case PUF_POWER:
		{
			q_device_ctrl(ldo_puf_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_puf_vdd_power_dev);
			ldo_power.puf_power = LDO_POWER_DISENABLE;
			break;
		}
		
		default:
		{
			break;
		}
	}
#elif defined(HANDWARE_4_0_2) 
	switch(power_event)
	{
		case PPG_POWER:
		{
			if(!ldo_power.vbat_power)
			{
				
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case VBAT_POWER:
		{
			if(!ldo_power.ppg_power)
			{
				
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			
			ldo_power.vbat_power = LDO_POWER_DISENABLE;
			break;
		}
		
		default:
		{
			
			break;
		}
	}
#elif (defined(HANDWARE_4_4_1))

	switch(power_event)
	{
		case PPG_POWER:
		{
			if(!ldo_power.vbat_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case VBAT_POWER:
		{
			if(!ldo_power.ppg_power)
			{
				
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			
			ldo_power.vbat_power = LDO_POWER_DISENABLE;
			break;
		}
		
		default:
		{
			break;
		}
	}
#elif (defined(HANDWARE_BCL601_151))
    switch(power_event)
	{
		case MIC_POWER:
		{
			q_device_ctrl(ldo_mic_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_mic_vdd_power_dev);

			ldo_power.mic_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case FLASH_POWER:
		{
			q_device_ctrl(ldo_flash_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_flash_vdd_power_dev);

			ldo_power.flash_power = LDO_POWER_DISENABLE;
			break;
		}
		
		default:
		{
			break;
		}
	}
#elif (defined(HANDWARE_4_1_3))

	switch(power_event)
	{
		case PPG_POWER:
		{
			if(!ldo_power.vbat_power && !ldo_power.temper_power && !ldo_power.imu_power && !ldo_power.pmic_power  && !ldo_power.touch_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case VBAT_POWER:
		{
			if(!ldo_power.ppg_power && !ldo_power.temper_power && !ldo_power.imu_power && !ldo_power.pmic_power  && !ldo_power.touch_power)
			{
				
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			
			ldo_power.vbat_power = LDO_POWER_DISENABLE;
			break;
		}
		case TEMPER_POWER:
		{
			if( !ldo_power.ppg_power && !ldo_power.vbat_power && !ldo_power.imu_power && !ldo_power.pmic_power  && !ldo_power.touch_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.temper_power = LDO_POWER_DISENABLE;
			break;
		}
		case IMU_POWER:
		{
			if( !ldo_power.ppg_power && !ldo_power.vbat_power && !ldo_power.temper_power  && !ldo_power.pmic_power  && !ldo_power.touch_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.imu_power = LDO_POWER_DISENABLE;
			break;
		}
		case PMIC_POWER:
		{
			if( !ldo_power.ppg_power && !ldo_power.vbat_power && !ldo_power.temper_power && !ldo_power.imu_power && !ldo_power.touch_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.pmic_power = LDO_POWER_DISENABLE;
			break;
		}
		case TOUCH_POWER:
		{
			if( !ldo_power.ppg_power && !ldo_power.vbat_power && !ldo_power.temper_power  && !ldo_power.pmic_power  && !ldo_power.pmic_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.touch_power = LDO_POWER_DISENABLE;
			break;
		}
		
		default:
		{
			break;
		}
	}	
#elif (defined(HANDWARE_1_12_1))

	switch(power_event)
	{
		case PPG_POWER:
		{
			if(!ldo_power.mic_power && !ldo_power.temper_power && !ldo_power.mouse_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
				
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_vdd_power_dev);
			}
			else
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case TEMPER_POWER:
		{
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_ppg_vdd_power_dev);
			ldo_power.temper_power = LDO_POWER_DISENABLE;
			break;
		}
		case MIC_POWER:
		{				
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_ppg_vdd_power_dev);

			ldo_power.mic_power =  LDO_POWER_DISENABLE;
			break;
		}
		case MOUSE_POWER:
		{							
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_ppg_vdd_power_dev);

			ldo_power.mouse_power = LDO_POWER_DISENABLE;
			break;
		}
		case RGB_POWER:
		{					
			q_device_ctrl(ic_led_vdd_power_dev,GPIO_OUTPUT_LOW,0);
			q_device_close(ic_led_vdd_power_dev);
			ldo_power.rgb_power = LDO_POWER_DISENABLE;
			break;
		}
		
		default:
		{
			break;
		}
	}	
#elif (defined(HANDWARE_1_5_8))

	switch(power_event)
	{
		case PPG_POWER:
		{
			if(!ldo_power.rgb_power && !ldo_power.pressure_sensors_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
				
			}
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case RGB_POWER:
		{
			if(!ldo_power.ppg_power && !ldo_power.pressure_sensors_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
				
			}			
			ldo_power.rgb_power = LDO_POWER_DISENABLE;
			break;
		}
		case PRESSURE_SENSORS_POWER:
		{
			if(!ldo_power.ppg_power && !ldo_power.rgb_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
				
			}			
			ldo_power.pressure_sensors_power = LDO_POWER_DISENABLE;
			break;
		}
		
		
		default:
		{
			break;
		}
	}

#elif (defined(HANDWARE_1_5_6))

	switch(power_event)
	{
		case PPG_POWER:
		{

			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_ppg_led_power_dev);
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case PRESSURE_SENSORS_POWER:
		{
			
			q_device_ctrl(pressure_sensor_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(pressure_sensor_power_dev);
	
			ldo_power.pressure_sensors_power = LDO_POWER_DISENABLE;
			break;
		}
		case MOTOR_POWER:
		{
			q_device_ctrl(motor_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(motor_power_dev);
			ldo_power.motor_power = LDO_POWER_DISENABLE;
			break;
		}
		
		
		default:
		{
			break;
		}
	}
	
#elif (defined(HANDWARE_1_9_1))

	switch(power_event)
	{
		case PPG_POWER:
		{

			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_ppg_led_power_dev);
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
				
		default:
		{
			break;
		}
	}
#elif (defined(HANDWARE_1_14_1))

	switch(power_event)
	{
		case PPG_POWER:
		{

			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(ldo_ppg_led_power_dev);
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
				
		default:
		{
			break;
		}
	}		

#elif (defined(HANDWARE_4_5_1))

	switch(power_event)
	{
		case PPG_POWER:
		{
			if(!ldo_power.vbat_power)
			{
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			ldo_power.ppg_power = LDO_POWER_DISENABLE;
			
			break;
		}
		case VBAT_POWER:
		{
			if(!ldo_power.ppg_power)
			{
				
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_LOW,0);	
				q_device_close(ldo_ppg_led_power_dev);
			}
			
			ldo_power.vbat_power = LDO_POWER_DISENABLE;
			break;
		}
		
		default:
		{
			break;
		}
	}	
#elif (defined(HANDWARE_1_23_1))

	switch(power_event)
	{
		case RGB_POWER:
		{
            for(uint8_t i=0; i < 3; i++) {
                uint8_t ret = q_device_ctrl(ic_led_vdd_power_dev,GPIO_OUTPUT_LOW,0);	
                if(RESULT_OK == ret)
                    break;
            }
			q_device_close(ic_led_vdd_power_dev);
				
			ldo_power.rgb_power = LDO_POWER_DISENABLE;
			break;
		}
    case MOTOR_POWER:
		{
			q_device_ctrl(motor_power_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(motor_power_dev);
#if (defined(HANDWARE_1_23_2)  || defined(HANDWARE_1_23_3))
      q_device_ctrl(motor_vcc_dev,GPIO_OUTPUT_LOW,0);	
			q_device_close(motor_vcc_dev);
#endif       
      
			ldo_power.motor_power = LDO_POWER_DISENABLE;
			break;
		}
		
		
		default:
		{
			break;
		}
	}  
  
#endif		

}

static void bc_ldo_power_on(enum ldo_power_event power_event)
{
#if defined(HANDWARE_1_5_3)

	switch(power_event)
	{
		case PPG_POWER:
		{
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);	
			
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case MIC_POWER:
		{
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.mic_power = LDO_POWER_ENABLE;
			break;
		}
		case MOTOR_POWER:
		{
			q_device_open(ldo_bos_vdd_power_dev);	
			q_device_ctrl(ldo_bos_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.motor_power = LDO_POWER_ENABLE;
			break;
		}
		case TEMPER_POWER:
		{
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.temper_power = LDO_POWER_ENABLE;			
			break;
		}
		case FLASH_POWER:
		{
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.flash_power = LDO_POWER_ENABLE;	
			break;
		}
		case RGB_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.rgb_power = LDO_POWER_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}
	
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
	
	switch(power_event)
	{
		case PPG_POWER:
		{
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);	
			
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case VBAT_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.vbat_power = LDO_POWER_ENABLE;
			break;
		}
		case PUF_POWER:
		{
			q_device_open(ldo_puf_vdd_power_dev);	
			q_device_ctrl(ldo_puf_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.puf_power = LDO_POWER_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}
#elif defined(HANDWARE_4_0_2) 

	switch(power_event)
	{
		case PPG_POWER:
		{	
			
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case VBAT_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.vbat_power = LDO_POWER_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}
#elif (defined(HANDWARE_4_4_1))
	
	switch(power_event)
	{
		case PPG_POWER:
		{	
			
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case VBAT_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.vbat_power = LDO_POWER_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}	

#elif (defined(HANDWARE_BCL601_151))
    switch(power_event)
	{
		case MIC_POWER:
		{	
			
			q_device_open(ldo_mic_vdd_power_dev);	
			q_device_ctrl(ldo_mic_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.mic_power = LDO_POWER_ENABLE;
			break;
		}
		case FLASH_POWER:
		{
			q_device_open(ldo_flash_vdd_power_dev);
			q_device_ctrl(ldo_flash_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.flash_power = LDO_POWER_ENABLE;	
			break;
		}
		default:
		{
			break;
		}
	}
#elif (defined(HANDWARE_4_1_3))
	
	switch(power_event)
	{
		case PPG_POWER:
		{	
			
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case VBAT_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.vbat_power = LDO_POWER_ENABLE;
			break;
		}
		case TEMPER_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.temper_power = LDO_POWER_ENABLE;			
			break;
		}
		case IMU_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.imu_power = LDO_POWER_ENABLE;			
			break;
		}
		case PMIC_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.pmic_power = LDO_POWER_ENABLE;			
			break;
		}
		case TOUCH_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.touch_power = LDO_POWER_ENABLE;			
			break;
		}
		
		default:
		{
			break;
		}
	}
#elif (defined(HANDWARE_1_12_1))
	
	switch(power_event)
	{
		case PPG_POWER:
		{	
			if(ldo_power.temper_power || ldo_power.mic_power || ldo_power.mouse_power)
			{
				q_device_open(ldo_ppg_led_power_dev);	
				q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			}
			else
			{
				q_device_open(ldo_ppg_vdd_power_dev);	
				q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			}
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case TEMPER_POWER:
		{			
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.temper_power = LDO_POWER_ENABLE;			
			break;
		}
		case MIC_POWER:
		{				
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.mic_power = LDO_POWER_ENABLE;
			break;
		}
		case MOUSE_POWER:
		{				
			q_device_open(ldo_ppg_vdd_power_dev);
			q_device_ctrl(ldo_ppg_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.mouse_power = LDO_POWER_ENABLE;
			break;
		}
		case RGB_POWER:
		{				
			q_device_open(ic_led_vdd_power_dev);	
			q_device_ctrl(ic_led_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.rgb_power = LDO_POWER_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}
#elif (defined(HANDWARE_1_5_8))
	
	switch(power_event)
	{
		case PPG_POWER:
		{	
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case RGB_POWER:
		{				
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.rgb_power = LDO_POWER_ENABLE;
			break;
		}
		case PRESSURE_SENSORS_POWER:
		{				
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.pressure_sensors_power = LDO_POWER_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}
	
#elif (defined(HANDWARE_1_5_6))
	
	switch(power_event)
	{
		case PPG_POWER:
		{	
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case PRESSURE_SENSORS_POWER:
		{
			
			q_device_open(pressure_sensor_power_dev);
			q_device_ctrl(pressure_sensor_power_dev,GPIO_OUTPUT_HIGH,0);	
	
			ldo_power.pressure_sensors_power = LDO_POWER_ENABLE;
			break;
		}
		case MOTOR_POWER:
		{
			q_device_open(motor_power_dev);
			q_device_ctrl(motor_power_dev,GPIO_OUTPUT_HIGH,0);	
			ldo_power.motor_power = LDO_POWER_ENABLE;
			break;
		}
		
		default:
		{
			break;
		}
	}	
#elif (defined(HANDWARE_1_9_1))
	
	switch(power_event)
	{
		case PPG_POWER:
		{	
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}		
		default:
		{
			break;
		}
	}
#elif (defined(HANDWARE_1_14_1))
	
	switch(power_event)
	{
		case PPG_POWER:
		{	
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}		
		default:
		{
			break;
		}
	}	
#elif (defined(HANDWARE_4_5_1))
	
	switch(power_event)
	{
		case PPG_POWER:
		{	
			
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.ppg_power = LDO_POWER_ENABLE;
			break;
		}
		case VBAT_POWER:
		{
			q_device_open(ldo_ppg_led_power_dev);	
			q_device_ctrl(ldo_ppg_led_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.vbat_power = LDO_POWER_ENABLE;
			break;
		}
		default:
		{
			break;
		}
	}	
#elif (defined(HANDWARE_1_23_1))
	
	switch(power_event)
	{
		case RGB_POWER:
		{				
			q_device_open(ic_led_vdd_power_dev);	
			q_device_ctrl(ic_led_vdd_power_dev,GPIO_OUTPUT_HIGH,0);
			ldo_power.rgb_power = LDO_POWER_ENABLE;
			break;
		}
		case MOTOR_POWER:
		{
			q_device_open(motor_power_dev);
			q_device_ctrl(motor_power_dev,GPIO_OUTPUT_LOW,0);	
#if (defined(HANDWARE_1_23_2)  || defined(HANDWARE_1_23_3))
      q_device_open(motor_vcc_dev);
			q_device_ctrl(motor_vcc_dev,GPIO_OUTPUT_HIGH,0);	
#endif        
			ldo_power.motor_power = LDO_POWER_ENABLE;
      
			break;
		}
		default:
		{
			break;
		}
	}  
#endif		
	
}

void bc_ldo_ppg_power_on(void)
{
	bc_ldo_power_on(PPG_POWER);	
}

void bc_ldo_ppg_power_off(void)
{
	bc_ldo_power_off(PPG_POWER);	
}


void bc_ldo_bat_power_on(void)
{
	bc_ldo_power_on(VBAT_POWER);	
}

void bc_ldo_bat_power_off(void)
{

	bc_ldo_power_off(VBAT_POWER);
	
}

void bc_ldo_puf_power_on(void)
{
	bc_ldo_power_on(PUF_POWER);	
}

void bc_ldo_puf_power_off(void)
{
	bc_ldo_power_off(PUF_POWER);
}

void bc_ldo_motor_power_on(void)
{
	
	bc_ldo_power_on(MOTOR_POWER);		
	
}

void bc_ldo_motor_power_off(void)
{
	bc_ldo_power_off(MOTOR_POWER);
	
}

void bc_ldo_mic_power_on(void)
{
	bc_ldo_power_on(MIC_POWER);
}

void bc_ldo_mic_power_off(void)
{

	bc_ldo_power_off(MIC_POWER);	
	
}

void bc_ldo_rgb_power_on(void)
{
	bc_ldo_power_on(RGB_POWER);					
}

void bc_ldo_rgb_power_off(void)
{
	bc_ldo_power_off(RGB_POWER);	
}

void bc_ldo_temper_power_on(void)
{
	bc_ldo_power_on(TEMPER_POWER);			
}

void bc_ldo_temper_power_off(void)
{
	bc_ldo_power_off(TEMPER_POWER);			
}

void bc_ldo_flash_power_on(void)
{
	
	bc_ldo_power_on(FLASH_POWER);	
			
}

void bc_ldo_flash_power_off(void)
{
	bc_ldo_power_off(FLASH_POWER);	
	
}

void bc_ldo_mouse_power_on(void)
{
	
	bc_ldo_power_on(MOUSE_POWER);	
			
}

void bc_ldo_mouse_power_off(void)
{
	bc_ldo_power_off(MOUSE_POWER);	
	
}

void bc_ldo_imu_power_on(void)
{	
	bc_ldo_power_on(IMU_POWER);				
}

void bc_ldo_imu_power_off(void)
{
	bc_ldo_power_off(IMU_POWER);		
}


void bc_ldo_pmic_power_on(void)
{	
	bc_ldo_power_on(PMIC_POWER);				
}

void bc_ldo_pmic_power_off(void)
{
	bc_ldo_power_off(PMIC_POWER);	
}

void bc_ldo_touch_power_on(void)
{	
	bc_ldo_power_on(TOUCH_POWER);				
}

void bc_ldo_touch_power_off(void)
{
	bc_ldo_power_off(TOUCH_POWER);	
}

void bc_ldo_pressure_sensors_power_on(void)
{	
	bc_ldo_power_on(PRESSURE_SENSORS_POWER);				
}

void bc_ldo_pressure_sensors_power_off(void)
{
	bc_ldo_power_off(PRESSURE_SENSORS_POWER);	
}


void bc_ldo_power_device_find(void)
{
    ldo_ppg_vdd_power_dev = q_device_find("ppg_vdd_en");
	q_device_assert(ldo_ppg_vdd_power_dev);	
	
	ldo_ppg_led_power_dev = q_device_find("ppg_led_en");
	q_device_assert(ldo_ppg_led_power_dev);
	
	ldo_puf_vdd_power_dev = q_device_find("puf_vdd_en");
	q_device_assert(ldo_puf_vdd_power_dev);
	
	ldo_bos_vdd_power_dev = q_device_find("bos_pwr_en");
	q_device_assert(ldo_bos_vdd_power_dev);
	
#if (defined(HANDWARE_BCL601_151))	
	
	ldo_mic_vdd_power_dev = q_device_find("mic_power_en");
	q_device_assert(ldo_mic_vdd_power_dev);
	
	ldo_flash_vdd_power_dev = q_device_find("flash_power_en");
	q_device_assert(ldo_flash_vdd_power_dev);
#endif	
	
#if (defined( HANDWARE_1_12_1) || defined( HANDWARE_1_23_1))
	ic_led_vdd_power_dev = q_device_find("vdd_led_en");
	q_device_assert(ic_led_vdd_power_dev);
	
//	ppg_reset_dev = q_device_find("ppg_reset_en");
//	q_device_assert(ppg_reset_dev);

#endif	

#if (defined( HANDWARE_1_5_6))

	pressure_sensor_power_dev = q_device_find("ts2323a_en");
	q_device_assert(pressure_sensor_power_dev);

#endif

#if (defined( HANDWARE_1_5_6) || defined( HANDWARE_1_23_1))

	motor_power_dev = q_device_find("motor_en");
	q_device_assert(motor_power_dev);
#if (defined(HANDWARE_1_23_2)  || defined(HANDWARE_1_23_3))
  motor_vcc_dev = q_device_find("motor_vcc");
	q_device_assert(motor_vcc_dev);
#endif  
  
//	bc_ldo_motor_power_on();
//	q_device_open(motor_power_dev);
//	q_device_ctrl(motor_power_dev,GPIO_OUTPUT_HIGH,0);

#endif

}



