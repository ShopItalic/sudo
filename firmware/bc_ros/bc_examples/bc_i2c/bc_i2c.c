/*******************************************************************************
此为i2c demo文件，使用q_device api接口。
写sy6103 chip id，读qma6100 chip id

q_device规则
日  期：2024年1月18日
编写人：邱成凯
 *******************************************************************************/

#include "bc_i2c.h"

#include "q_device.h"

#include "bc_timer.h"
#include "bc_logger.h"

#include "string.h"


static q_device_t *i2c_sys_dev;                                     //设备描述

static q_device_t *i2c_acc_dev;                                     //设备描述

static q_device_t *i2c_nfc_dev;                                     //设备描述

static q_device_t *i2c_ppg_dev;                                     //设备描述


static struct i2c_package i2c_acc_pack = {
																					 .slave_addr = 0x12,          //i2c从机地址
																					 .write_length = 1,
																					 .read_length = 1,
																				 };

static struct i2c_package i2c_sys_pack = {
																					 .slave_addr = 0x0E,          //i2c从机地址
																					 .write_length = 1,
																					 .read_length = 1,
                                         };	

static struct i2c_package i2c_nfc_pack = {
																					 .slave_addr = 0x50,          //i2c从机地址
																					 .write_length = 1,
																					 .read_length = 1,
                                         };	

static struct i2c_package i2c_ppg_pack = {
																					 .slave_addr = 0x5B,          //i2c从机地址
																					 .write_length = 1,
																					 .read_length = 1,
                                         };	
																		 

static void test_timer_callback(void * p_context);									 
static bc_timer_struct  test_timer = {
	.timer_name = "test timer",                          //定时器名字
	.uxAutoReload = true,                                //周期定时器
	.xTimerPeriodInTicks = 3000,                         //定时器时间
	.timer_callback_function = test_timer_callback,      //定时器回调
};
									 

static void i2c_write(q_device_t *i2c_dev,struct i2c_package *pack,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length)
{
	pack->reg_addr = reg_addr;
	if(write_data != NULL)
	{
		memcpy(pack->write_buff,write_data,write_length);
	}
	pack->write_length = write_length;
	q_device_write(i2c_nfc_dev,0,pack,0);  
}

static void i2c_read(q_device_t *i2c_dev,struct i2c_package *pack,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length)
{
	pack->reg_addr = reg_addr;
	pack->read_length = read_length;
	q_device_read(i2c_dev,0,pack,0);
	if(read_data != NULL)
	{
		memcpy(read_data,pack->read_buff,pack->read_length);
	}
}

/*******************************************************************************
 * Function Name     : sys_i2c_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void sys_i2c_read(uint8_t reg_add ,uint8_t *data,uint8_t length)
{
	i2c_sys_pack.reg_addr = reg_add;
	i2c_sys_pack.read_length = length;
	if(q_device_read(i2c_sys_dev,0,&i2c_sys_pack,0) == RESULT_OK)
	{
//		BC_LOG_INFO("sys i2c read ok reg:%02x  data:%x \r\n",reg_add,i2c_sys_pack.read_buff[0]);
		data[0] = i2c_sys_pack.read_buff[0];
		
	}
	else
	{
//		BC_LOG_INFO("sys i2c read error reg:%02x  data:%x \r\n",reg_add,i2c_sys_pack.read_buff[0]);
		
	}
}
/*******************************************************************************
 * Function Name     : i2c_sys_test
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void i2c_sys_test(void)
{
	q_device_open(i2c_sys_dev);
	uint8_t data = 0;
	sys_i2c_read(0x0A, &data, 1);
	BC_LOG_INFO("sy6103 id:%02x \r\n",data);
	q_device_close(i2c_sys_dev);
}
/*******************************************************************************
 * Function Name     : i2c_acc_test
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void i2c_acc_test(void)
{
	i2c_acc_dev = q_device_find("i2c_1");
	q_device_assert(i2c_acc_dev);
	q_device_open(i2c_acc_dev);                                                                        //打开设备
 
	i2c_acc_pack.reg_addr = 00;                                                                        //寄存器地址
	i2c_acc_pack.read_length =1;  
	q_device_read(i2c_acc_dev,0,&i2c_acc_pack,0);                                                      //读
	if((i2c_acc_pack.read_buff[0] == 0xfa)||((i2c_acc_pack.read_buff[0] & 0xF0) == 0x90))
	{
			BC_LOG_INFO("qma6100 find   g_qma6100.chip_id:%02x\n",i2c_acc_pack.read_buff[0]);
	}
	BC_LOG_INFO("qma6100 find  lll  g_qma6100.chip_id:%02x\n",i2c_acc_pack.read_buff[0]);

	q_device_close(i2c_acc_dev);                                                                     //关闭设备
}


static void test_nfc(void)
{
	uint8_t a = 0xc1;
	uint8_t b = 0x7F;
	uint8_t id = 0;
	i2c_write(i2c_nfc_dev,&i2c_nfc_pack,a,NULL,1);
	i2c_write(i2c_nfc_dev,&i2c_nfc_pack,b,&id,1);
	BC_LOG_INFO("id:%02x",id);
}

static void test_ppg(void)
{
	uint16_t  reg_value[2] ;
//	q_device_open(i2c_ppg_dev); 
	i2c_read(i2c_ppg_dev,&i2c_ppg_pack,0,(uint8_t*)reg_value, 4);
	BC_LOG_INFO("i2c recv :%04x %04x \r\n",reg_value[0],reg_value[1]);
//	q_device_close(i2c_ppg_dev);  
}


/*******************************************************************************
 * Function Name     : test_timer_callback
 * Description       : 定时器回调
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void test_timer_callback(void * p_context)
{
//	i2c_sys_test();
//	i2c_acc_test();
//	test_nfc();
	test_ppg();
}

/*******************************************************************************
 * Function Name     : test_io_output_led_device_find
 * Description       : 查找led设备
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static void test_i2c_device_find(void)
{
//	/**********  led  **********/
//	i2c_sys_dev = q_device_find("i2c_2_sys");                          //查找设备
//	q_device_assert(i2c_sys_dev);	                                   //断言

//	i2c_acc_dev = q_device_find("i2c_1_acc");
//	q_device_assert(i2c_acc_dev);
	
//	i2c_nfc_dev = q_device_find("i2c_1");
//	q_device_assert(i2c_nfc_dev);
	
	  i2c_ppg_dev = q_device_find("i2c_0");
	  q_device_assert(i2c_ppg_dev);
	q_device_open(i2c_ppg_dev); 
}


/*******************************************************************************
 * Function Name     : test_timer_create
 * Description       : 创建led设备
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/

void test_timer_create(void)
{
	test_i2c_device_find();
	if(!bc_timer_create(&test_timer))                            //创建led定时器
	{
		BC_LOG_WARN("create %s fial!! \r\n",test_timer.timer_name);              
	}
	else
	{
		BC_LOG_INFO("create %s success!! \r\n",test_timer.timer_name);
		bc_timer_start(&test_timer);                             //启动led定时器
	}
	
}

