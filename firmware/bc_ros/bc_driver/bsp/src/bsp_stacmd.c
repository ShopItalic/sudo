#include "q_device.h"

#include <string.h>


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
#include "nrf_gpio.h"

#define NORMAL_YHM2710_ADDRESS_00  0x04  //7bit address   0x04  //fpga   0x0c
#define QUICK_YHM2710_ADDRESS_00   0x44  //7bit address   0x44  //fpga   0x4c
#define QUICK_MODE 1            //0 for normaly mode,   1 for quick mode 
#define RETRY_NUMBER 10       //2   



typedef void (*bsp_stacmd_gpio_output_callback)(uint32_t gpio_pin); 

typedef void (*bsp_stacmd_gpio_intput_callback)(uint32_t gpio_pin); 

typedef void (*bsp_stacmd_gpio_output_config_callback)(uint32_t gpio_pin);

typedef void (*bsp_stacmd_gpio_input_config_callback)(uint32_t gpio_pin,nrf_gpio_pin_pull_t pull_config); 

typedef void (*bsp_stacmd_gpio_freed_callback)(uint32_t gpio_pin); 

typedef void (*bsp_stacmd_delay_callback)(uint32_t delay_length); 

typedef uint32_t (*bsp_stacmd_gpio_read_callback)(uint32_t gpio_pin); 


struct bsp_stacmd_config
{
	uint32_t delay_length;
	uint32_t                                    stacmd_io_pin;
	bsp_stacmd_gpio_output_callback             stacmd_gpio_low;
	bsp_stacmd_gpio_output_callback             stacmd_gpio_high;
	bsp_stacmd_gpio_output_config_callback      stacmd_gpio_output_config;
	bsp_stacmd_gpio_input_config_callback       stacmd_gpio_input_config;
	nrf_gpio_pin_pull_t                         stacmd_gpio_input_pull;
	bsp_stacmd_delay_callback                   stacmd_delay;
	bsp_stacmd_gpio_read_callback               stacmd_gpio_read;
	bsp_stacmd_gpio_freed_callback              stacmd_gpio_freed;
	
};

struct  BSP_STACMD
{
	const char   *name;
	bool          lock;
    struct bsp_stacmd_config stacmd_config;
	
	q_device_t dev;
};

static struct BSP_STACMD bsp_list =   
{
	
	  .name = "device_stacmd",  //单总线通信，与yhm2712
		.lock = false,
	  .stacmd_config = {
			              .delay_length = 5,

#if (defined(HANDWARE_1_5_3) || defined(HANDWARE_1_8_1)|| defined(HANDWARE_1_9_1)  || defined(HANDWARE_1_14_1) ||  defined(HANDWARE_1_12_1) || defined(HANDWARE_1_5_8) || \
     defined(HANDWARE_1_5_6) || defined(HANDWARE_1_18_1) || defined(HANDWARE_1_19_1) || defined(HANDWARE_1_23_1))
		  .stacmd_io_pin = NRF_GPIO_PIN_MAP(1,1),
	
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
		  
	.stacmd_io_pin = NRF_GPIO_PIN_MAP(1,1),
		  
#elif (defined(HANDWARE_BCL601_151))
		  
	.stacmd_io_pin = NRF_GPIO_PIN_MAP(0,11),	
		  
#endif		  
			              .stacmd_gpio_low = nrf_gpio_pin_clear,
			              .stacmd_gpio_high = nrf_gpio_pin_set,
						  .stacmd_gpio_output_config = nrf_gpio_cfg_output,
			              .stacmd_gpio_input_config = nrf_gpio_cfg_input,
			              .stacmd_gpio_input_pull = NRF_GPIO_PIN_PULLUP,
			              .stacmd_delay = nrfx_coredep_delay_us,
			              .stacmd_gpio_read = nrf_gpio_pin_read,
			              .stacmd_gpio_freed = nrf_gpio_cfg_default,
					},
		.dev = {0},
	
};



static int total_retry_number;

static int delay_Zero_Point_3_count(void)  //should delay 0.3030us, 1 clocks
{
}
//******************************************************************************
static int delay_B0_count(void)   //should delay 2.424us     CNT_B0 *0.3030us
{
    for(uint32_t i=0;i<13;i++){
        __NOP();__NOP();__NOP();__NOP();
    }
}

//******************************************************************************
static int delay_B1_count(void)    //should delay 7.8787us   CNT_B1*0.3030us
{
    for(uint32_t i=0;i<52;i++){
        __NOP();__NOP();__NOP();__NOP();
    }
}
//******************************************************************************
static int delay_BZ_count(void)     //should delay 27us   CNT_BZ*0.3030us
{
    for(uint32_t i=0;i<209;i++){
        __NOP();__NOP();__NOP();__NOP();
    }
}
//******************************************************************************
static int delay_SA_count(void)    //should delay 4.848us   CNT_SA*0.3030us
{
    for(uint32_t i=0;i<26;i++){
        __NOP();__NOP();__NOP();__NOP();
    }
}


//******************************************************************************
static void drive_reset_plus(void)
{  
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
    bsp_list.stacmd_config.stacmd_gpio_output_config(bsp_list.stacmd_config.stacmd_io_pin);
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
  //  WaitUs(10);  // 10us
    delay_BZ_count();
    delay_Zero_Point_3_count();
   bsp_list.stacmd_config.stacmd_gpio_low(bsp_list.stacmd_config.stacmd_io_pin);
    delay_BZ_count();  //27us
    delay_BZ_count();  //27us
    delay_BZ_count();  //27us
    delay_BZ_count();  //27us
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
  //  WaitUs(10);  // 10us
    delay_Zero_Point_3_count();

}
//******************************************************************************
static void drive_clks_B0(void)
{  
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
    bsp_list.stacmd_config.stacmd_gpio_output_config(bsp_list.stacmd_config.stacmd_io_pin);
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
 //   delay_CNT_1T_SUB_B0_count(); 
    delay_Zero_Point_3_count();
    bsp_list.stacmd_config.stacmd_gpio_low(bsp_list.stacmd_config.stacmd_io_pin);
    delay_B0_count();      //16M    ?         
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
    delay_Zero_Point_3_count(); 
}



//******************************************************************************
static void drive_clks_B1(void)
{   
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
    bsp_list.stacmd_config.stacmd_gpio_output_config(bsp_list.stacmd_config.stacmd_io_pin);
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
  //  delay_CNT_1T_SUB_B1_count();  
   delay_Zero_Point_3_count();         
    bsp_list.stacmd_config.stacmd_gpio_low(bsp_list.stacmd_config.stacmd_io_pin);
    delay_B1_count();               
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
    delay_Zero_Point_3_count(); 
}
//******************************************************************************
static void drive_clks_BZ(void)
{   
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
    bsp_list.stacmd_config.stacmd_gpio_output_config(bsp_list.stacmd_config.stacmd_io_pin);
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
 //   delay_CNT_1T_SUB_BZ_count();  
    delay_Zero_Point_3_count();
    bsp_list.stacmd_config.stacmd_gpio_low(bsp_list.stacmd_config.stacmd_io_pin);
    delay_BZ_count();  //27us            
    bsp_list.stacmd_config.stacmd_gpio_high(bsp_list.stacmd_config.stacmd_io_pin);
    delay_Zero_Point_3_count(); 
}

//******************************************************************************
static void drive_bit(int n)
{   
    if(n==0)
      drive_clks_B0();
    else
      drive_clks_B1();
 }
//******************************************************************************
static void drive_z(void)
{   
      drive_clks_BZ();                                            
 }
//******************************************************************************
static void drive_stop(void)
{
      drive_z();      
}
//******************************************************************************
static void drive_start(void)
{ 
      drive_z();     
}
//******************************************************************************
static void drive_write(void)
{
      drive_bit(0);
}
//******************************************************************************
static void drive_read(void)
{
      drive_bit(1);
}

//******************************************************************************    
static int verify_parity(char data)
{
    int i;
    int value;
    int temp;
    temp=0x100;
    value=0x0;
    for(i=0;i<8;i++)
     {
       temp=data>>i;
       temp=temp&0x01;
       value=value^temp;
     }   

    return value;
    
}
//******************************************************************************
static void drive_ack(char input_value)
{
      int value; 
      value=verify_parity(input_value);
      if(value==1)
         drive_bit(1);
      else
         drive_bit(0);   
}

//******************************************************************************
static bool read_bit(int *value)
{
    int   ow_value;
    int  retrytime;
    bool return_value;
    
    return_value=true;
    retrytime=0;
    bsp_list.stacmd_config.stacmd_gpio_input_config(bsp_list.stacmd_config.stacmd_io_pin,bsp_list.stacmd_config.stacmd_gpio_input_pull);
    do{
        delay_Zero_Point_3_count();
        retrytime++;
        if(retrytime>1000)//1000
          { 
              return_value=false;         
           break;
          }
        }while(bsp_list.stacmd_config.stacmd_gpio_read(bsp_list.stacmd_config.stacmd_io_pin)==1);    //wait for the GPIO1 to "0"
        
    delay_SA_count();     //delay 4.8us
    ow_value= ~bsp_list.stacmd_config.stacmd_gpio_read(bsp_list.stacmd_config.stacmd_io_pin);        //sample the GPIO value
    
    retrytime=0;
    do{
        delay_Zero_Point_3_count();
        retrytime++;
        if(retrytime>1000)//1000
          {
              return_value=false;            
           break;
          }
        }while(bsp_list.stacmd_config.stacmd_gpio_read(bsp_list.stacmd_config.stacmd_io_pin)==0);    //wait for the GPIO1 to "1"
     delay_Zero_Point_3_count(); 
    
    *value=ow_value&0x01;   
    return return_value;
}

//******************************************************************************    
static bool receive_data(char *value)
{
   char receive_data_value;
   int temp_readbit;
   bool return_value;
   int i;
   return_value=true;
   receive_data_value=0;
   for(i=7;i>=0;i=i-1)
   {
        if(read_bit(&temp_readbit))
           receive_data_value+=temp_readbit<<i;  
        else
          { 
           i=0;
           return_value=false;  
           }        
    }
   *value=receive_data_value;     
   return return_value;
}
//******************************************************************************        
static void drive_data(char data,char low_bit)
{
    int i,j;
    if(low_bit==1)
       j=6;
    else 
       j=7;
    
    for (i = j; i>=0; i=i-1)
    {
        if(((data>>i)&0x01)==0x01)
         {
          drive_bit(1);

          }
        else
         {
          drive_bit(0);            

          }
    }
      
}


//******************************************************************************
static bool wait_ack(int *value)
{    
    int ack_value;
    bool return_value;   
    return_value=read_bit(&ack_value);
    *value=ack_value;
    if(return_value==false)
      {
       // DBG_OUT("ack value is false\r\n");  
        }
    return return_value;
}

//******************************************************************************
static bool ACMD_write_fifo_data_quick(char dev_addr,char addr_i,int nbyte_i, char* data_i)
{     
     bool  value;
     char  input_value;
     char  slaveaddress_add_regaddress;
     int   ack0;
     int   i;
     value=true;
     
     
//----------------------check the slave address----------------------------------------      
      if((dev_addr&0x40)==0x00)                 //need slave address is quick slave address, not normal slave address
        { 
            value=false; 
          //  DBG_OUT("the slave address is not quick slave address, it is normal slave address\r\n");
            //-----------meet the error, exit the funciton
            drive_stop();     //send stop command                    
            return value;
        }     
//------------------write device address+register address ------------------------
      slaveaddress_add_regaddress=(dev_addr&0xf0)+addr_i;  //make the slave_address and register to 1byte
      drive_start();
      drive_data(slaveaddress_add_regaddress, 1);       //write 7 bit slave_Address+register address
      drive_write();
      if(wait_ack(&ack0)==false)  
        {
          value=false;
          //-----------meet the error, exit the funciton
          drive_stop();     //send stop command                 
          return value;
         } 
      
      input_value=slaveaddress_add_regaddress<<1;;
      if(ack0!=verify_parity(input_value))  
        {
            value=false;     
           // DBG_OUT("!!!!!!!!dev_addr+register address ack is not right\r\n");
            //-----------meet the error, exit the funciton
            drive_stop();     //send stop command                  
            return value;
        } 
//------------------write register data ------------------------          
      for (i=0; i<nbyte_i; i=i+1)
       {
             drive_data(data_i[i], 0);
          
             if(wait_ack(&ack0)==false)  
                 {
                     value=false;
                    //-----------meet the error, exit the funciton
                    drive_stop();     //send stop command                         
                    return value;
                 } 
          
              if(ack0!=verify_parity(data_i[i]))  
                {
                    value=false;                
                   // DBG_OUT("!!!!!!!!reg_data ack is not right\r\n");
                    //-----------meet the error, exit the funciton
                    drive_stop();     //send stop command    
                    return value;
                }             
        }
 drive_stop();     //send stop command 
     bsp_list.stacmd_config.stacmd_gpio_input_config(bsp_list.stacmd_config.stacmd_io_pin,bsp_list.stacmd_config.stacmd_gpio_input_pull);
return value;  
}
//******************************************************************************   
static bool ACMD_read_fifo_data_quick(char dev_addr,char addr_i,int nbyte_i, char* data_i)
 {
     int i,j;
     char temp_value;
     char data_temp[20];
     bool  value;
     char  input_value;
     char  slaveaddress_add_regaddress;
     int   ack0;
     value=true;         
//----------------------check the slave address----------------------------------------      
     if((dev_addr&0x40)==0x00)                 //need slave address is quick slave address, not normal slave address
      { 
        value=false; 
        //NRF_LOG_INFO("the slave address is not quick slave address, it is normal slave address\r\n");
        //-----------meet the error, exit the funciton
        drive_stop();     //send stop command                 
        return value;
      }    
//------------------write device address+register address------------------------       
     drive_start();     
     slaveaddress_add_regaddress= (dev_addr&0xf0) + addr_i;                     
//     //NRF_LOG_INFO("---%x \r\n",slaveaddress_add_regaddress);
     drive_data(slaveaddress_add_regaddress, 1);  //write 7 bit
     drive_read(); 
     if(wait_ack(&ack0)==false)  
       {
         value=false; 
         //NRF_LOG_INFO("wait_ack(&ack0)==false \r\n");           
         //-----------meet the error, exit the funciton
         drive_stop();     //send stop command                
         return value;
       }                  
   
      input_value=(slaveaddress_add_regaddress<<1)+1;
      if(ack0!=verify_parity(input_value))  
        {
            value=false;          
            //NRF_LOG_INFO("quick dev_addr has no ack: 0x%x 0x%x 0x%x \r\n",ack0,input_value,verify_parity(input_value));
                //-----------meet the error, exit the funciton
            drive_stop();     //send stop command      
            return value;
        }       
//------------------read register datas------------------------                  
      for (i=0; i<nbyte_i; i=i+1)
        {        
          if(receive_data(&data_i[i])==false)
            {
               value=false;
                //NRF_LOG_INFO("receive_data(&data_i[i])==false \r\n");
                    //-----------meet the error, exit the funciton
                drive_stop();     //send stop command                        
                return value;
            }  
          if(i==nbyte_i-1)
          {
           // drive_nack(data_i[i]);
            }
          else
            drive_ack(data_i[i]);
        }
//---------------------------------------------------------------
  drive_stop();     //send stop command 
    bsp_list.stacmd_config.stacmd_gpio_input_config(bsp_list.stacmd_config.stacmd_io_pin,bsp_list.stacmd_config.stacmd_gpio_input_pull);
return value;  
}
//******************************************************************************
static bool YHM2710_write_multi_byte(char addr_i,int nbyte_i,char* data_i)
{
  bool test_value;
  int retry_time;
  retry_time=0;
  do  
    {    
    retry_time++;  
    if(retry_time==RETRY_NUMBER)
        {
        test_value=false;   
       break;   
       }
       //__disable_irq();   //need the single thread 
       drive_reset_plus();
#if QUICK_MODE   
       test_value= ACMD_write_fifo_data_quick(QUICK_YHM2710_ADDRESS_00,addr_i,nbyte_i,data_i);                             
#else
       test_value= ACMD_write_fifo_data_normal(NORMAL_YHM2710_ADDRESS_00,addr_i,nbyte_i,data_i);
#endif       
        // __enable_irq();   //stop the single thread   
       if(test_value==false)
            {
                //NRF_LOG_INFO("write failed--retry number=%d\r\n",retry_time); 
                total_retry_number++;  
            }                                
     }while(test_value==false);  
   return test_value;      
}
//******************************************************************************
static bool YHM2710_read_multi_byte(char addr_i,int nbyte_i,char* data_i)
{
   bool test_value;
  int retry_time;
  int i;
  retry_time=0;
  do  
    {    
    retry_time++;     
    if(retry_time==RETRY_NUMBER)
        {
         for(i=0;i<nbyte_i;i++) 
           data_i[i]=0xff;
         test_value=false;  
       break;
       }
        //__disable_irq();   //need the single thread   
        drive_reset_plus();
#if QUICK_MODE 
       test_value= ACMD_read_fifo_data_quick(QUICK_YHM2710_ADDRESS_00,addr_i,nbyte_i,data_i); 
#else 
       test_value= ACMD_read_fifo_data_normal(NORMAL_YHM2710_ADDRESS_00,addr_i,nbyte_i,data_i); 
#endif                                      
        // __enable_irq();   //stop the single thread  
       if(test_value==false)
          {
             //NRF_LOG_INFO("read failed--retry number=%d\r\n",retry_time);  
             total_retry_number++;           
           }  
      }while(test_value==false); 
   return test_value;          
}

bool YHM2710_read_stacmdLevel(void)
{
    bool level = 0;
    bsp_list.stacmd_config.stacmd_gpio_input_config(bsp_list.stacmd_config.stacmd_io_pin,bsp_list.stacmd_config.stacmd_gpio_input_pull);
    level = bsp_list.stacmd_config.stacmd_gpio_read(bsp_list.stacmd_config.stacmd_io_pin);
    
    return level;
}

/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_stacmd_open(q_device_t*dev)
{

	if(bsp_list.lock)
	{
		return RESULT_OK;
	}
	
	bsp_list.lock = true;
	//Q_DEVICE_LOG_INFO("open %s \r\n",bsp_list.name);		
					
	return RESULT_OK;
}


/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_stacmd_close(q_device_t*dev)
{

	if(!bsp_list.lock)
	{
		return RESULT_OK;
	}
	

//	nrf_gpio_cfg_default(bsp_list.pdm_config.lr_io_pin);
	bsp_list.lock = false;
	//Q_DEVICE_LOG_INFO("close %s \r\n",bsp_list.name);		
	return RESULT_OK;
}





/*******************************************************************************
 * Function Name     : bsp_i2c_write
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_stacmd_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct stacmd_package *package = (struct stacmd_package *)buffer;
	if(package == NULL)
	{
		return RESULT_CONFIG_NULL_ERR;
	}

	if(!bsp_list.lock)
	{
		return RESULT_DEV_UNOPENED_ERR;
	}
    if(!YHM2710_write_multi_byte(package->reg_addr,package->write_length,(char*)package->write_buff))
	{
		return RESULT_SEND_ERR;
	}
	return RESULT_OK;

}



/*******************************************************************************
 * Function Name     : bsp_i2c_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_stacmd_read(q_device_t *dev, int pos, const void *  buffer, int size)
{
	
	struct stacmd_package *package = (struct stacmd_package *)buffer;
	if(package == NULL)
	{
		return RESULT_CONFIG_NULL_ERR;
	}
    ret_code_t err_code;

	if(!bsp_list.lock)
	{
		return RESULT_DEV_NULL_ERR;
	}
//			Q_DEVICE_LOG_INFO(" %s i2c read： s:%x, r:%x, l:%d \r\n",bsp_list[i].name,package->slave_addr, package->reg_addr,package->read_length);
    if(!YHM2710_read_multi_byte(package->reg_addr,package->read_length,(char*)package->read_buff))
	{
		return RESULT_READ_ERR;
	}
	return RESULT_OK;

}



/*******************************************************************************
 * Function Name     : 
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static struct q_device_ops ops =
{
	.write = bsp_stacmd_write,
	.read = bsp_stacmd_read,
	.open = bsp_stacmd_open,
	.close = bsp_stacmd_close,
};

/*******************************************************************************
 * Function Name     : bsp_gpio_output_register
 * Description       : 设备注册
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void bsp_stacmd_register(void)
{

	bsp_list.dev.name = bsp_list.name;
	bsp_list.dev.dops  = &ops;
	q_device_register(&bsp_list.dev);		
	
}

device_initcall(bsp_stacmd_register);






#endif


































