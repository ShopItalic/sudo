/*******************************************************************************
此为pimc api接口文件，通过宏定义来兼容驱动pmic外设库 sy6103、eth4662、yhm27112

日  期：2024年1月17日
编写人：邱成凯
 *******************************************************************************/

#include "bc_pmic.h"

#include "bc_power.h"

#include "bc_pmic_device_port.h"
#include "bc_ldo_switch.h"
#include "bc_delay.h"

#if (PMIC_DEVIECE_TYPE == 0)   //SY6103

#include "sy6103.h"

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

#include "eta4662.h"

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

#include "yhm2712.h"

#endif


//读pmic id
bool bc_pmic_get_id(uint8_t *data)
{  
    
	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103

#if (HARDWARE_413_ENABLED == 1)	
		
		bc_ldo_pmic_power_on();
	    bc_delay_ms(20);
		sy6103_get_chip_id(data); 
	    bc_ldo_pmic_power_off();
		return true;
#else 	
		return sy6103_get_chip_id(data); 
#endif		
	
	
	

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662
    
    return eta4662_get_chip_id(data); 
   
#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

	*data = YHM2710_read_id();
	return true;

#endif
	
	
}

//读充电状态
enum pmic_charge_status bc_pmic_get_charge_status(void)
{
    
	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103

#if (HARDWARE_413_ENABLED == 1)	
		
		bc_ldo_pmic_power_on();
	    bc_delay_ms(20);
		enum pmic_charge_status  charge_status  =  (enum pmic_charge_status)sy6103_charge_get_status();
	    bc_ldo_pmic_power_off();
		return charge_status;
#else 	
	return (enum pmic_charge_status)sy6103_charge_get_status();
#endif
	
	

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

    return (enum pmic_charge_status)eta4662_charge_get_status();

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

	return (enum pmic_charge_status)YHM2710_read_charge_status();

#endif	
	
	
}

//进入shipmode
void bc_pmic_set_shipmode(void)
{
    
	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103
	
#if (HARDWARE_413_ENABLED == 1 || HARDWARE_451_ENABLED == 1)		
	pmic_ship_mode_en();	
#else	
	sy6103_shlp_mode();
#endif
	
#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

    
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1)		
	pmic_ship_mode_en();	
#else	
	eta4662_shlp_mode();
#endif		
		

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

#if (HARDWARE_BCL601_151_ENABLED == 1)		
//	pmic_ship_mode_en();	
	YHM2710_set_shipmode();

#elif (HARDWARE_181_ENABLED == 1 || HARDWARE_182_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)		
    pmic_ship_mode_en();	
#else	
	YHM2710_set_shipmode();
#endif
	

#endif	
	
}


void bc_pmic_set_sleepmode(void)
{
    
	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103
	

	
#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

    
	
#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

    YHM2710_set_sleepmode();

#endif	
	
}

void bc_pmic_set_startmode(void)
{
    
	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103
	

	
#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

    
	
#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

	
	YHM2710_set_startmode();

#endif	
	
}




//喂狗
void bc_pmic_feeddog(void)
{
    
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103

#if (HARDWARE_413_ENABLED == 1)	
		
		bc_ldo_pmic_power_on();
	    bc_delay_ms(20);
		sy6103_feed_dog();
	    bc_ldo_pmic_power_off();
#else 	
	sy6103_feed_dog();
#endif	

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

    eta4662_feed_dog();

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712


#endif		
	
}

//读全部寄存器
void bc_pmic_read_all(uint8_t *data)
{
    
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103

#if (HARDWARE_413_ENABLED == 1)	
		
		bc_ldo_pmic_power_on();
	    bc_delay_ms(20);
		sy6103_gte_reg_all(data);
	    bc_ldo_pmic_power_off();
#else 	
	sy6103_gte_reg_all(data);
#endif		
	

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

    eta4662_gte_reg_all(data);

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

	YHM2710_read_all(data);
#endif	
	
}

//pmic初始化
void bc_pmic_init(void)    
{  
    
	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103

#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_pmic_power_on();
	bc_delay_ms(20);
	sy6103_init();
	bc_ldo_pmic_power_off();
#else 	
	sy6103_init();
#endif		
	

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

    eta4662_init();

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

	YHM2710_init();
#endif	
	
}

void bc_pmic_device_find(void)
{
	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103
	sy6103_find();
	bc_power_vbat_adc_find();

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

    eta4662_find();
	bc_power_vbat_adc_find();

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

	bc_power_vbat_adc_find();
#endif	
}

bool bc_pmic_id_hardware_check(void)
{
	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103

#if (HARDWARE_413_ENABLED == 1)	
		
	bc_ldo_pmic_power_on();
	bc_delay_ms(20);
	bool ret = sy6103_check_chip_id();
	bc_ldo_pmic_power_off();
	return  ret;
#else 	
	return  sy6103_check_chip_id();
#endif		
	

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

     return  eta4662_check_chip_id();

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712

	if(YHM2710_read_id() != 0xA0)
	{
		return false;
	}
	return true;
#endif	
	
}


uint16_t bc_pmic_get_adc_value(void)
{
	return bc_power_get_adc_value();	
	
}

uint8_t bc_pmic_get_vbat_percen(void)
{
	return bc_power_get_vbat_percen();
}

bool bc_pmic_check_vbat(void)
{
	return bc_power_check_vbat();
}


