#include "bc_pmic_device_port.h"

#include "q_device.h"
#include "bc_logger.h"
#include "bc_delay.h"

#if (defined(HANDWARE_4_5_1) )

#include "bc_device_i2c_bus_handler.h"

#endif

#include <string.h>
#include <stdint.h>



static q_device_t *pmic_i2c_dev = NULL;
static q_device_t *pmic_int_chg_dev = NULL;
static q_device_t *pmic_stacmd_dev = NULL;
static q_device_t *pmic_intput_chg_dev = NULL;

static void *int_io_irq_callback = NULL;


static q_device_t *pmic_irq_dev = NULL;

#if (HARDWARE_441_ENABLED == 1 || HARDWARE_182_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_413_ENABLED == 1 || HARDWARE_181_ENABLED == 1  || HARDWARE_191_ENABLED == 1   || \
    HARDWARE_1141_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
	
	static q_device_t *pmic_ship_mode_dev = NULL;
	
#endif	


static struct i2c_package i2c_pack = {
	
#if (HARDWARE_413_ENABLED == 1  )	
										 .slave_addr = 0x0E>>1,
#else
										 .slave_addr = 0x0E,
#endif	
	                                     
	                                     .write_length = 1,
	                                     .read_length = 1,
                                     };

struct stacmd_package pmic_stacmd_package = {0};

void pmic_ship_mode_en(void)
{

#if (HARDWARE_441_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_181_ENABLED == 1 || HARDWARE_182_ENABLED == 1 || HARDWARE_1141_ENABLED == 1 || \
      HARDWARE_191_ENABLED == 1 || HARDWARE_451_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || HARDWARE_1181_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
	
	q_device_open(pmic_ship_mode_dev);
	q_device_ctrl(pmic_ship_mode_dev,GPIO_OUTPUT_HIGH,0);	
	bc_delay_ms(200);
	
#endif		

}


void pmic_i2c_init(void)
{
#if (HARDWARE_413_ENABLED == 1 )	
	pmic_i2c_dev = q_device_find("i2c_0");
	q_device_assert(pmic_i2c_dev);
#elif (HARDWARE_451_ENABLED == 1 )	
	pmic_i2c_dev = q_device_find("i2c_4");
	q_device_assert(pmic_i2c_dev);	
#else
	pmic_i2c_dev = q_device_find("i2c_3");
	q_device_assert(pmic_i2c_dev);
#endif		
	
	


#if (PMIC_DEVIECE_TYPE == 0)   //SY6103 

	pmic_int_chg_dev = q_device_find("sys_int_chg");
	q_device_assert(pmic_int_chg_dev);

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

	pmic_intput_chg_dev = q_device_find("sys_intput_chg");
	q_device_assert(pmic_intput_chg_dev);

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712	
	
#endif	
	
#if (HARDWARE_441_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_413_ENABLED == 1  || HARDWARE_181_ENABLED == 1  || HARDWARE_451_ENABLED == 1 || HARDWARE_1231_ENABLED == 1)	
	
	pmic_ship_mode_dev = q_device_find("ship_mode_en");
	q_device_assert(pmic_ship_mode_dev);
	q_device_open(pmic_ship_mode_dev);
	q_device_ctrl(pmic_ship_mode_dev,GPIO_OUTPUT_LOW,0);	
	
#endif		


}
void pmic_int_chg_open(void)
{
//	q_device_open(pmic_int_chg_dev);
//	q_device_ctrl(pmic_int_chg_dev,GPIO_OUTPUT_HIGH,0);	
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103 

	q_device_open(pmic_int_chg_dev);
	q_device_ctrl(pmic_int_chg_dev,GPIO_OUTPUT_HIGH,0);	

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

	q_device_open(pmic_intput_chg_dev);

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712



#endif		
	
}

void pmic_int_chg_close(void)
{
#if (PMIC_DEVIECE_TYPE == 0)   //SY6103 

	q_device_ctrl(pmic_int_chg_dev,GPIO_OUTPUT_LOW,0);	
	q_device_close(pmic_int_chg_dev);

#elif (PMIC_DEVIECE_TYPE == 1)  // ETH4662

	q_device_close(pmic_intput_chg_dev);

#elif (PMIC_DEVIECE_TYPE == 2)  // YHM2712



#endif	
}

void pmic_i2c_bus_open(void)
{
	q_device_open(pmic_i2c_dev);
	BC_LOG_INFO("bc pmic bus open \r\n");
}


void pmic_i2c_bus_close(void)
{
	q_device_close(pmic_i2c_dev);
	BC_LOG_INFO("bc pmic bus close \r\n");
}	

void pmic_i2c_open(void)
{
	
#if (defined(HANDWARE_4_5_1) )
    bc_i2c_bus_device_open(DEVICE_BUS_TYPE_PMIC);
#else	
	q_device_open(pmic_i2c_dev);
#endif		
	
	
	BC_LOG_INFO("bc pmic open \r\n");
}


void pmic_i2c_close(void)
{
	
#if (defined(HANDWARE_4_5_1) )
    bc_i2c_bus_device_close(DEVICE_BUS_TYPE_PMIC);
#else	
	q_device_close(pmic_i2c_dev);
#endif		
	
	BC_LOG_INFO("bc pmic close \r\n");
}	
	
bool pmic_i2c_write(uint8_t reg_add ,uint8_t data,uint8_t length)
{
	i2c_pack.reg_addr = reg_add;
	i2c_pack.write_length = length;
	i2c_pack.write_buff = &data;
	if(q_device_write(pmic_i2c_dev,0,&i2c_pack,0)== RESULT_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}
																 
bool pmic_i2c_read(uint8_t reg_add ,uint8_t *data,uint8_t length)
{
	i2c_pack.reg_addr = reg_add;
	i2c_pack.read_length = length;
	i2c_pack.read_buff = data;
	if(q_device_read(pmic_i2c_dev,0,&i2c_pack,0) == RESULT_OK)
	{
//		LOG_INFO("sys i2c read ok reg:%02x  data:%x \r\n",reg_add,i2c_pack.read_buff[0]);
//		data[0] = i2c_pack.read_buff[0];
		return true;
	}
	else
	{
//		LOG_INFO("sys i2c read error reg:%02x  data:%x \r\n",reg_add,i2c_pack.read_buff[0]);
		return false;
	}
}

void pmic_delay(uint16_t ms)
{
	bc_delay_ms(ms);
}


void bc_pmic_device_stacmd_open(void)
{
	q_device_open(pmic_stacmd_dev);
}
	
void bc_pmic_device_stacmd_close(void)
{
	q_device_close(pmic_stacmd_dev);
}

void pmic_io_irq_enable(void)
{

	q_device_open(pmic_irq_dev);
    q_device_reg_callback(pmic_irq_dev,GPIOT_CONFIG_POLARITY_LoToHi,int_io_irq_callback);
	q_device_reg_callback(pmic_irq_dev,GPIOT_CONFIG_POLARITY_HiToLo,int_io_irq_callback);	  //×¢²á»Øµ÷

}

void pmic_io_irq_disnable(void)
{
	q_device_close(pmic_irq_dev);
}

bool pmic_irq_register_callback(void *callback)
{
	if(callback != NULL)
	{
		int_io_irq_callback = callback;
		return true;
	}
	return false;
}


uint8_t pmic_io_irq_status(void)
{
	uint8_t io_status = 0;
	q_device_read(pmic_irq_dev,0,&io_status,0);
	return io_status;
}

void bc_pmic_stacmd_wirte(uint8_t reg_addr,uint8_t length,uint8_t *data)
{
//	pmic_io_irq_disnable();
	bc_pmic_device_stacmd_open();
	pmic_stacmd_package.reg_addr = reg_addr;
	pmic_stacmd_package.write_buff = data;
	pmic_stacmd_package.write_length = length;
	q_device_write(pmic_stacmd_dev ,0,&pmic_stacmd_package,0);
	bc_pmic_device_stacmd_close();
//	pmic_io_irq_enable();
}
void bc_pmic_stacmd_read(uint8_t reg_addr,uint8_t length,uint8_t *data)
{
//	pmic_io_irq_disnable();
	bc_pmic_device_stacmd_open();
	pmic_stacmd_package.reg_addr = reg_addr;
	pmic_stacmd_package.read_buff = data;
	pmic_stacmd_package.read_length = length;
	q_device_read(pmic_stacmd_dev ,0,&pmic_stacmd_package,0);
	bc_pmic_device_stacmd_close();
//	pmic_io_irq_enable();
}

void bc_pmic_device_stacmd_find(void)
{
  

	pmic_stacmd_dev = q_device_find("device_stacmd");
	q_device_assert(pmic_stacmd_dev);
	#if defined(HANDWARE_1_23_4)
	pmic_irq_dev = q_device_find("pmic_irq");
	q_device_assert(pmic_irq_dev);	
    #endif

#if (HARDWARE_441_ENABLED == 1 || HARDWARE_BCL601_151_ENABLED == 1 || HARDWARE_182_ENABLED == 1  || HARDWARE_181_ENABLED == 1 || HARDWARE_191_ENABLED == 1 || HARDWARE_156_ENABLED == 1 || \
     HARDWARE_1141_ENABLED == 1 || HARDWARE_1181_ENABLED == 1|| HARDWARE_1231_ENABLED == 1)	
	
	pmic_ship_mode_dev = q_device_find("ship_mode_en");
	q_device_assert(pmic_ship_mode_dev);
	q_device_open(pmic_ship_mode_dev);
	q_device_ctrl(pmic_ship_mode_dev,GPIO_OUTPUT_LOW,0);
	
#endif		
}










