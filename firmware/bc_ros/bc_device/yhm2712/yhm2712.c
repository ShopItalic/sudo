#include "yhm2712.h"

#include "bc_pmic_device_port.h"
#include "bc_delay.h"


extern bool YHM2710_read_stacmdLevel(void);

//******************************************************************************
static void YHM2710_write_multi_byte(char addr_i,int nbyte_i,char* data_i)
{
     bc_pmic_stacmd_wirte(addr_i,nbyte_i,(uint8_t*)data_i);
}
//******************************************************************************
static void YHM2710_read_multi_byte(char addr_i,int nbyte_i,char* data_i)
{
     bc_pmic_stacmd_read(addr_i,nbyte_i,(uint8_t*)data_i);   
}


char rxdata111[20] = {0};

uint8_t YHM2710_read_charge_status(void)
{
	if(YHM2710_read_stacmdLevel() == 0)
	
//	if(pmic_io_irq_status() == 0)
    {
        return 1;
    }
	else
	{
		//----------------read reg0~reg8----------------------   
		YHM2710_read_multi_byte(0,9, &rxdata111[0]);
		if((rxdata111[6]&0xF0) == 0x20 || (rxdata111[6]&0xF0) == 0x80)
		{
			return 0;
		}
		else if((rxdata111[6]&0xF0) == 0xD0)
		{
			return 2;
		}
//		else if((rxdata111[6]&0xF0) == 0xC0)
//		{
//			return 1;
//		}
	}
}


uint8_t YHM2710_read_id(void)
{
    //----------------read reg0~reg8----------------------   
    YHM2710_read_multi_byte(0,9, &rxdata111[0]);
    
//--------------reg8----------------------------    
    if((uint8_t)rxdata111[8]==0xA0){
      //NRF_LOG_INFO("Chip ID is right\r\n");
    }else{
      //NRF_LOG_INFO("Chip ID is not right\r\n"); 
    }
    return rxdata111[8];
}
void YHM2710_read_all(uint8_t *pdata)
{
    YHM2710_read_multi_byte(0,9, (char*)&pdata[0]);
}
ret_code_t YHM2710_init(void)
{
    //----------------read reg0~reg8----------------------   
    YHM2710_read_multi_byte(0,9, &rxdata111[0]);
  

    
    //LOG_DUMP_BYTE(rxdata,9);
//--------------reg8----------------------------    
    if((uint8_t)rxdata111[8]==0xA0){
        //NRF_LOG_INFO("Chip ID is right\r\n");
    }else{
        //NRF_LOG_INFO("Chip ID is not right\r\n"); 
        return NRF_ERROR_TIMEOUT;
    }
    
    
    bool res;
    uint8_t txdata = 0;
    //----------------set reg0--------------------------
//    txdata=0xF0;
#if defined(HANDWARE_1_9_1)
   txdata=0x60;
#elif defined(HANDWARE_1_23_3)
    txdata=0x60;
#else
    txdata=0x00;
#endif	
	
    YHM2710_write_multi_byte(0x0,1,(char*)&txdata);  //write Vreg=4.25V, 
                                                     //150mV Headroom,
                                                     //Vtrickle=2.8V, 
                                                     //Vsys-5mV>Vin for Reverse block; 
                                                     //550mA Ilimit    
                                              
     //----------------set reg1--------------------------   
//    txdata=0x10;
//	 txdata=0x40;
#if ( HARDWARE_1171_ENABLED == 1)	
    txdata=0xcc;   //60ma  24.9K 20*3=60
#else
    txdata=0x4c;   //14ma  24.9K 20*0.7=14
#endif	

//    txdata=0x0c;   //14ma  24.9K 20*0.7=14

     YHM2710_write_multi_byte(0x1,1,(char*)&txdata);  //Ireg 20mA
                                                    //Icc=Ireg
                                                 //Iterm=0.7*Ireg
                                                 //Itrickle=0.05*Ireg 
                                                  //Ipre=1mA
                                                  //Isense rate is fixed 5000   
    //----------------set reg2--------------------------                                                  
//	txdata=0x28;
		txdata=0xA0;
     YHM2710_write_multi_byte(0x2,1,(char*)&txdata);  //set to sleep mode 
	 
	 txdata=0x42;
    YHM2710_write_multi_byte(0x3,1,(char*)&txdata);  //Q2 is in CC mode, not in fully on mode
                                                 //Disable watch dog function
                                                 //battery re-charge threshold is 200mV
                                                 //no enter current test mode when Vin>5.5V      
                                                 //hiccup time is 2ms
//	bc_rtos_delay(100);
   
    //----------------set reg3--------------------------    
    txdata=0x02;
    YHM2710_write_multi_byte(0x3,1,(char*)&txdata);  //Q2 is in CC mode, not in fully on mode
                                                 //Disable watch dog function
                                                 //battery re-charge threshold is 200mV
                                                 //no enter current test mode when Vin>5.5V      
                                                 //hiccup time is 2ms                                                                                             
                                                                                           
    return NRF_SUCCESS;
}

void YHM2710_set_shipmode(void)
{
    uint8_t txdata = 0;
    
    txdata=0x88;
    YHM2710_write_multi_byte(0x2,1,(char*)&txdata);   //set to DisCharge mode
    
    txdata=0x18;
    YHM2710_write_multi_byte(0x2,1,(char*)&txdata);   //set to shipping mode 
}

void YHM2710_set_sleepmode(void)
{
	uint8_t txdata = 0;
	txdata=0x28;
     YHM2710_write_multi_byte(0x2,1,(char*)&txdata);  //set to sleep mode 
}

void YHM2710_set_startmode(void)
{
	uint8_t txdata = 0;
	txdata=0xA0;
     YHM2710_write_multi_byte(0x2,1,(char*)&txdata);  //set to sleep mode 
}


