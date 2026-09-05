#include "bc_spi_flash.h"

#include "q_device.h"
#include "string.h"

#include "bc_spi_flash_port.h"
#include "bc_delay.h"




static uint8_t spi_flash_write_buff[10] = {0};
static uint8_t spi_flash_read_buff[10] = {0};


void spi_flash_device_lowpower(void)
{
//	 bc_spi_flash_device_open();
	 bc_spi_flash_cs_low();
    bc_delay_ms(10);
	 spi_flash_write_buff[0] = 0xB9;
	 bc_spi_flash_write_and_read(spi_flash_write_buff,1,spi_flash_read_buff,0);
    bc_delay_ms(10);
	 bc_spi_flash_cs_high();
//	 bc_spi_flash_device_close();

}

void spi_flash_device_wakeup(void)
{

//	 bc_spi_flash_device_open();
	 bc_spi_flash_cs_low();
    bc_delay_ms(10);
	 spi_flash_write_buff[0] = 0xAB;
	 bc_spi_flash_write_and_read(spi_flash_write_buff,1,spi_flash_read_buff,0);
    bc_delay_ms(10);
      bc_spi_flash_cs_high();
//	 bc_spi_flash_device_close();

	

}

bool sFLASH_CheckStatus(uint8_t checkflg)
{
	memset(spi_flash_read_buff,0,sizeof(spi_flash_read_buff));
	spi_flash_write_buff[0] = 0xC0;
	bc_spi_flash_write_and_read(spi_flash_write_buff,1,spi_flash_read_buff,1);
    if((spi_flash_read_buff[0] & checkflg) == checkflg )
        return true;
    else
        return false;
}

bool sFLASH_WaitReady(void)
{
    uint32_t time_cnt = 0;
    while(1){
        if(false == sFLASH_CheckStatus(0x01)){
//            NRF_LOG_NANDFLASH("sFLASH_WaitReady:%d",time_cnt);
            return true;
        }
        nrf_delay_us(10);
        time_cnt++;
        if(time_cnt > 1000){
            BC_LOG_INFO("device is busy and TIMEOUT \r\n");
            return false;
        }
    }
}

bool sFLASH_Reset(void)
{
    /*!< Select the FLASH: Chip Select low  选择FLASH:芯片选择低电平*/
    bc_spi_flash_cs_low();

    /*!< Send "Write Enable" instruction 发送“写使能”指令 */
   
	memset(spi_flash_read_buff,0,sizeof(spi_flash_read_buff));
	spi_flash_write_buff[0] = 0xFF;
	bc_spi_flash_write_and_read(spi_flash_write_buff,1,spi_flash_read_buff,0);
    /*!< Deselect the FLASH: Chip Select high  取消选择FLASH:芯片选择高*/
    bc_spi_flash_cs_high();
     bc_spi_flash_cs_low();
    if(sFLASH_WaitReady() == true)
    {
		bc_spi_flash_cs_high();
        return true;
    }   
    else
    {
		bc_spi_flash_cs_high();
        return false;
	}
}

uint32_t spi_flash_device_get_id(void)
{
	
	uint32_t Temp = 0;
	
	 bc_spi_flash_device_open();
	bc_delay_ms(100);
//	 spi_flash_device_wakeup();
	 bc_spi_flash_cs_low();
	 bc_delay_ms(50);
//	 sFLASH_Reset();
	memset(spi_flash_read_buff,0,sizeof(spi_flash_read_buff));
	spi_flash_write_buff[0] = 0x9F;
	bc_spi_flash_write_and_read(spi_flash_write_buff,1,spi_flash_read_buff,4);
	
	memset(spi_flash_write_buff,0,sizeof(spi_flash_write_buff));;
	 bc_spi_flash_write_and_read(spi_flash_write_buff,1,spi_flash_read_buff,3);
	Temp |= spi_flash_read_buff[0] << 16;
	Temp |=	spi_flash_read_buff[1] << 8;
	Temp |= spi_flash_read_buff[2];
	BC_LOG_INFO("spi flash id:%8x\r\n",Temp);
	
	 bc_spi_flash_cs_high();
//	spi_flash_device_lowpower();
	bc_spi_flash_device_close();
	
	return   Temp;
}



bool spi_flash_device_check_id(void)
{
	uint32_t Temp = spi_flash_device_get_id();
//	Temp = spi_flash_device_get_id();
//	Temp = spi_flash_device_get_id();
	
#if (HARDWARE_441_ENABLED == 1)	

	if(Temp == 0xC84016)
	{
		
		return true;
	}
#elif (HARDWARE_451_ENABLED == 1)	

	if(Temp == 0xC84019)
	{
		
		return true;
	}	
#elif (defined(HANDWARE_1_19_1))	

	if(Temp == 0xC8401B)
	{
		
		return true;
	}	  
#elif (HARDWARE_1141_ENABLED == 1 || HARDWARE_1171_ENABLED == 1)	

	if(Temp == 0xC8401B)
	{
		
		return true;
	}		
#elif (HARDWARE_158_ENABLED == 1)	

	if(Temp == 0x161616)
	{
		
		return true;
	}	
#else
	if(Temp == 0xEF4013)
	{
		
		return true;
	}
	
#endif		

	
	return false;
}










