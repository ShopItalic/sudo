#include "q_device.h"

#include "nrf_drv_twi.h"
#include <string.h>

#include "nrf_gpio.h"


#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

/* Indicates if operation on TWI has ended. */
static volatile bool i2c_0_xfer_done = false;
static volatile bool i2c_0_err_flag = false;

static volatile ret_code_t nrf_transfer_status;


struct  BSP_GPIO_I2C_0
{
	const char   *name;
	bool          lock;
	uint32_t      bsp_io_scl_pin;
	uint32_t      bsp_io_sda_pin;
	nrf_drv_twi_t bsp_i2c_handler;
	void *i2c_callback_handler;
	q_device_t dev;
};

static void i2c_0_callback_handler(nrf_drv_twi_evt_t const *p_event, void *p_context);




static struct BSP_GPIO_I2C_0 bsp_list =    //0--i2c_0(twi0) 
{
#if defined(HANDWARE_1_5_3)
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(1,11),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(1,13),
#elif defined(HANDWARE_1_12_1)
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,14),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,15),		
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,15),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,16),
#elif (defined(HANDWARE_4_1_3))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,8),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,7),	
#elif (defined(HANDWARE_4_4_1))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,16),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,15),	
#elif (defined(HANDWARE_1_5_8))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(1,11),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(1,14),	
#elif (defined(HANDWARE_1_5_6))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(1,13),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(1,11),
#elif (defined(HANDWARE_4_5_1))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,29),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,28),	
#elif (defined(HANDWARE_1_9_1))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,12),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,11),
#elif (defined(HANDWARE_1_14_1))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,17),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,14),
#elif (defined(HANDWARE_1_17_1))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,17),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,14),
#elif (defined(HANDWARE_1_18_1))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(1,13),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(1,11),  
#elif (defined(HANDWARE_1_23_1))
#if (defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,21),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,18), 
#else
	.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(1,13),
	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(1,11), 
#endif
  
//.bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,15),
//	.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,11), 
#endif		
	
	.name = "i2c_0",
	.lock = false,
	
	.bsp_i2c_handler = NRF_DRV_TWI_INSTANCE(0),    //不可更换i2c 编号 
	.i2c_callback_handler = i2c_0_callback_handler,
	.dev = {0},	
};

/*******************************************************************************
 * Function Name     :  i2c_0_callback_handler
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void i2c_0_callback_handler(nrf_drv_twi_evt_t const *p_event, void *p_context)
{
     //Q_DEVICE_LOG_INFO("i2c_0_handler:%x",p_event->type);
    switch (p_event->type)
    {
			case NRF_DRV_TWI_EVT_DONE:
	        //Q_DEVICE_LOG_INFO("i2c_0: NRF_DRV_TWI_EVT_DONE");
					i2c_0_xfer_done = true;
					nrf_transfer_status = NRF_SUCCESS;
					break;

			case NRF_DRV_TWI_EVT_ADDRESS_NACK:
					//Q_DEVICE_LOG_INFO("i2c_0: NRF_DRV_TWI_EVT_ADDRESS_NACK \r\n");
					i2c_0_xfer_done = true;
					i2c_0_err_flag = true;
			        nrf_transfer_status = NRF_ERROR_NOT_FOUND;
					break;

			case NRF_DRV_TWI_EVT_DATA_NACK:
					//Q_DEVICE_LOG_INFO("i2c_0: NRF_DRV_TWI_EVT_DATA_NACK \r\n");
					i2c_0_xfer_done = true;
					i2c_0_err_flag = true;
					nrf_transfer_status = NRF_ERROR_INVALID_PARAM;
					break;
			default:
					//Q_DEVICE_LOG_INFO("i2c_0: DEFAULT (%d) in mEventHandler \r\n", p_event->type);
					i2c_0_xfer_done = true;
					i2c_0_err_flag = true;
					break;
    }
}


/*******************************************************************************
 * Function Name     :  bsp_i2c_0_writereg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static uint8_t bsp_i2c_0_writereg(uint8_t slave, uint16_t reg_add, uint8_t reg_dat,nrf_drv_twi_t *bsp_i2c_handler)
{

#if (defined(HANDWARE_1_12_1) || defined(HANDWARE_1_9_1x) || defined(HANDWARE_1_14_1) )
	
	uint32_t timeout_cnt = 0; 
    uint8_t reg[2] = {0};
	reg[0] = reg_dat ;
	reg[1] = reg_dat;
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
    nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 1, false);
    while (i2c_0_xfer_done == false){
        if(timeout_cnt++ >= 10000){
            break;
        }
    }
    if (i2c_0_err_flag == true)
    {
        return NRF_ERROR_INVALID_ADDR;
    }
//	Q_DEVICE_LOG_HEX("write:",reg,2);
//	Q_DEVICE_LOG_INFO("write addr:%02x \r\n",reg_add);
    return NRF_SUCCESS;
	
#elif ( defined(HANDWARE_1_5_6))
	
	if(slave == 0x14)
	{
		uint32_t timeout_cnt = 0; 
		uint8_t reg[2] = {0};
		reg[0] = reg_dat ;
		reg[1] = reg_dat;
		i2c_0_xfer_done = false;
		i2c_0_err_flag = false;
		nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 1, false);
		while (i2c_0_xfer_done == false){
			if(timeout_cnt++ >= 10000){
				break;
			}
		}
		if (i2c_0_err_flag == true)
		{
			return NRF_ERROR_INVALID_ADDR;
		}
	//	Q_DEVICE_LOG_HEX("write:",reg,2);
	//	Q_DEVICE_LOG_INFO("write addr:%02x \r\n",reg_add);
		return NRF_SUCCESS;
	}
	else
	{
		 uint32_t timeout_cnt = 0; 
		ret_code_t err_code;
		uint8_t reg[2] = {reg_add, reg_dat};
		i2c_0_xfer_done = false;
	//	Q_DEVICE_LOG_INFO(" %s i2c send writereg: s:%x, r:%x, l:%d \r\n",bsp_list.name,slave, reg_add,2);
	//	BC_LOG_HEX("send writereg",reg,2);
		err_code = nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 2, false);
		while (i2c_0_xfer_done == false){
			if(timeout_cnt++ >= 100000){
				break;
			}
		}
		
		return err_code;
		
	}
	
#else
	
	

	uint32_t timeout_cnt = 0; 
    uint8_t reg[2] = {0};
	reg[0] = reg_add ;
	reg[1] = reg_dat;
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
    nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 2, false);
    while (i2c_0_xfer_done == false){
        if(timeout_cnt++ >= 10000){
            break;
        }
    }
    if (i2c_0_err_flag == true)
    {
        return NRF_ERROR_INVALID_ADDR;
    }
	//Q_DEVICE_LOG_HEX("write:",reg,2);
	//Q_DEVICE_LOG_INFO("write addr:%02x \r\n",reg_add);
    return NRF_SUCCESS;
#endif	
	
    
}


/*******************************************************************************
 * Function Name     :  bsp_i2c_0_writeregs
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static uint8_t bsp_i2c_0_writeregs(uint8_t slave, uint16_t reg_add,const uint8_t *buf, uint16_t num,nrf_drv_twi_t *bsp_i2c_handler)
{
	
	
#if (defined(HANDWARE_1_12_1) || defined(HANDWARE_1_9_1x) || defined(HANDWARE_1_14_1) )
	
	 if(num > 255){
        return NRF_ERROR_INVALID_LENGTH;
    }
    uint32_t timeout_cnt = 0; 
    uint8_t reg[256];
	reg[0] = reg_add ;
    for(uint8_t i = 0;i < num;i++){
       reg[i] = buf[i];
    }
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
	
    ret_code_t err_code;
    do
    {
        err_code = nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, num , false);
        if(err_code != NRF_ERROR_BUSY)
        {
            APP_ERROR_CHECK(err_code);
        }
    }while(err_code != NRF_SUCCESS && err_code != NRF_ERROR_BUSY);

    while (i2c_0_xfer_done == false){
        if(timeout_cnt++ >= 10000){
            break;
        }
    }
    if (i2c_0_err_flag == true)
    {
        return NRF_ERROR_INVALID_ADDR;
    }
//	Q_DEVICE_LOG_HEX("write:",reg,num+1);
//	Q_DEVICE_LOG_INFO("write addr:%02x \r\n",reg_add);
    return NRF_SUCCESS;
	
#elif ( defined(HANDWARE_1_5_6))
	
	if(slave == 0x14)
	{
		if(num > 255){
        return NRF_ERROR_INVALID_LENGTH;
		}
		uint32_t timeout_cnt = 0; 
		uint8_t reg[256];
		reg[0] = reg_add ;
		for(uint8_t i = 0;i < num;i++){
		   reg[i] = buf[i];
		}
		i2c_0_xfer_done = false;
		i2c_0_err_flag = false;
		
		ret_code_t err_code;
		do
		{
			err_code = nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, num , false);
			if(err_code != NRF_ERROR_BUSY)
			{
				APP_ERROR_CHECK(err_code);
			}
		}while(err_code != NRF_SUCCESS && err_code != NRF_ERROR_BUSY);

		while (i2c_0_xfer_done == false){
			if(timeout_cnt++ >= 10000){
				break;
			}
		}
		if (i2c_0_err_flag == true)
		{
			return NRF_ERROR_INVALID_ADDR;
		}
	//	Q_DEVICE_LOG_HEX("write:",reg,num+1);
	//	Q_DEVICE_LOG_INFO("write addr:%02x \r\n",reg_add);
		return NRF_SUCCESS;
	}
	else
	{
		 if(num > 80){
        return NRF_ERROR_INVALID_LENGTH;
		}
		uint32_t timeout_cnt = 0; 
		uint8_t reg[80];
		reg[0] = reg_add;
		for(uint8_t i = 0;i < num;i++){
		   reg[i+1] = buf[i];
		}
	//	Q_DEVICE_LOG_INFO(" %s i2c send writeregs: s:%x, r:%x, l:%d \r\n",bsp_list.name,slave, reg_add,num);
	//	BC_LOG_HEX("send writeregs",reg,num + 1);
		i2c_0_xfer_done = false;
		ret_code_t err_code;
		do
		{
			err_code = nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, num + 1, false);
			if(err_code != NRF_ERROR_BUSY)
			{
				APP_ERROR_CHECK(err_code);
			}
		}while(err_code != NRF_SUCCESS && err_code != NRF_ERROR_BUSY);

		while (i2c_0_xfer_done == false){
			if(timeout_cnt++ >= 10000){
				break;
			}
		}
		return NRF_SUCCESS;
	}
	
#else
	
	 if(num > 80){
        return NRF_ERROR_INVALID_LENGTH;
    }
    uint32_t timeout_cnt = 0; 
    uint8_t reg[80];
	reg[0] = reg_add ;
    for(uint8_t i = 0;i < num;i++){
       reg[i+1] = buf[i];
    }
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
	
    ret_code_t err_code;
    do
    {
        err_code = nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, num + 1, false);
        if(err_code != NRF_ERROR_BUSY)
        {
            APP_ERROR_CHECK(err_code);
        }
    }while(err_code != NRF_SUCCESS && err_code != NRF_ERROR_BUSY);

    while (i2c_0_xfer_done == false){
        if(timeout_cnt++ >= 10000){
            break;
        }
    }
    if (i2c_0_err_flag == true)
    {
        return NRF_ERROR_INVALID_ADDR;
    }
	//Q_DEVICE_LOG_HEX("write:",reg,num+1);
	//Q_DEVICE_LOG_INFO("write addr:%02x \r\n",reg_add);
    return NRF_SUCCESS;
#endif	
		
   
}



/*******************************************************************************
 * Function Name     : bsp_i2c_0_readreg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static uint8_t bsp_i2c_0_readreg(uint8_t slave, uint16_t reg_add, uint8_t *buf, uint16_t num,nrf_drv_twi_t *bsp_i2c_handler)
{
	
#if (defined(HANDWARE_1_12_1) || defined(HANDWARE_1_9_1x) || defined(HANDWARE_1_14_1) )

	
	uint32_t timeout_cnt = 0; 
    uint8_t reg[2]; 
	
    reg[0] = (reg_add >> 8) & 0xff ;
	reg[1] = reg_add& 0xff ;
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
    nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 2, true);
    while (i2c_0_xfer_done == false){
        if(timeout_cnt++ >= 10000){
            break;
        }
    }
    if (i2c_0_err_flag == true)
    {
        return NRF_ERROR_INVALID_ADDR;
    }
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
    nrf_drv_twi_rx(bsp_i2c_handler, slave, buf, num);
    while (i2c_0_xfer_done == false){
        if(timeout_cnt++ >= 10000){
            break;
        }
    }
    if (i2c_0_err_flag == true)
    {
        return NRF_ERROR_INVALID_ADDR;
    }
//		Q_DEVICE_LOG_HEX("recv:",buf,num);
//		Q_DEVICE_LOG_INFO("reg addr:%02x",reg_add);
    return NRF_SUCCESS;
	
#elif ( defined(HANDWARE_1_5_6))
	
	if(slave == 0x14)
	{
		uint32_t timeout_cnt = 0; 
		uint8_t reg[2]; 
		
		reg[0] = (reg_add >> 8) & 0xff ;
		reg[1] = reg_add& 0xff ;
		i2c_0_xfer_done = false;
		i2c_0_err_flag = false;
		nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 2, true);
		while (i2c_0_xfer_done == false){
			if(timeout_cnt++ >= 10000){
				break;
			}
		}
		if (i2c_0_err_flag == true)
		{
			return NRF_ERROR_INVALID_ADDR;
		}
		i2c_0_xfer_done = false;
		i2c_0_err_flag = false;
		nrf_drv_twi_rx(bsp_i2c_handler, slave, buf, num);
		while (i2c_0_xfer_done == false){
			if(timeout_cnt++ >= 10000){
				break;
			}
		}
		if (i2c_0_err_flag == true)
		{
			return NRF_ERROR_INVALID_ADDR;
		}
	//		Q_DEVICE_LOG_HEX("recv:",buf,num);
	//		Q_DEVICE_LOG_INFO("reg addr:%02x",reg_add);
		return NRF_SUCCESS;
	}
	else
	{
		uint32_t timeout_cnt = 0; 
		uint8_t reg[1];
		reg[0] = reg_add;
		i2c_0_xfer_done = false;
		nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 1, true);
		while (i2c_0_xfer_done == false){
			if(timeout_cnt++ >= 10000){
				break;
			}
		}
		if (nrf_transfer_status != NRF_SUCCESS)
		{
			Q_DEVICE_LOG_ERROR("nrf_drv_twi_tx fail: 0x%02X", slave);
			return nrf_transfer_status;
		}

		i2c_0_xfer_done = false;
		nrf_drv_twi_rx(bsp_i2c_handler, slave, buf, num);
		timeout_cnt = 0;
		while (i2c_0_xfer_done == false){
			if(timeout_cnt++ >= 10000){
				break;
			}
		}
		if (nrf_transfer_status != NRF_SUCCESS)
		{
			Q_DEVICE_LOG_ERROR("nrf_drv_twi_rx fail: 0x%02X \r\n", slave);
			return nrf_transfer_status;
		}
		return NRF_SUCCESS;
	}
#else
	
	uint32_t timeout_cnt = 0; 
    uint8_t reg[2]; 
	
    reg[0] = reg_add ;
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
    nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 1, true);
    while (i2c_0_xfer_done == false){
        if(timeout_cnt++ >= 10000){
            break;
        }
    }
    
    //Q_DEVICE_LOG_INFO("reg addr:%02x  slave:%02x \r\n",reg_add,slave);
    if (i2c_0_err_flag == true)
    {
        return NRF_ERROR_INVALID_ADDR;
    }
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
    //memset(buf, 0, num);
    nrf_drv_twi_rx(bsp_i2c_handler, slave, buf, num);
    while (i2c_0_xfer_done == false){
        if(timeout_cnt++ >= 10000){
            break;
        }
    }
    if (i2c_0_err_flag == true)
    {
        return NRF_ERROR_INVALID_ADDR;
    }
		//Q_DEVICE_LOG_HEX("recv:",buf,num);
		
    return NRF_SUCCESS;
#endif		
	
    
}


/*******************************************************************************
 * Function Name     : bsp_gpio_i2c_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_i2c_open(q_device_t*dev)
{

	if(bsp_list.lock)
	{
		return RESULT_I2C_OPEN_ERR;
	}
	const nrf_drv_twi_config_t config = {
		.scl = bsp_list.bsp_io_scl_pin,
		.sda = bsp_list.bsp_io_sda_pin,
#if (defined(HANDWARE_1_14_1) || defined(HANDWARE_1_23_3) || defined(HANDWARE_1_23_4))		
		.frequency = NRF_DRV_TWI_FREQ_400K,
#else
		.frequency = NRF_DRV_TWI_FREQ_100K,
#endif		
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init = false,
		.hold_bus_uninit = NRF_GPIO_PIN_NOPULL
		//.hold_bus_uninit = NRF_GPIO_PIN_PULLUP
		
	};
	
	nrf_gpio_cfg(
            bsp_list.bsp_io_scl_pin,
            NRF_GPIO_PIN_DIR_OUTPUT,
            NRF_GPIO_PIN_INPUT_DISCONNECT,
            NRF_GPIO_PIN_NOPULL,
            NRF_GPIO_PIN_H0H1,
            NRF_GPIO_PIN_NOSENSE);
                
        nrf_gpio_cfg(
            bsp_list.bsp_io_sda_pin,
            NRF_GPIO_PIN_DIR_OUTPUT,
            NRF_GPIO_PIN_INPUT_DISCONNECT,
            NRF_GPIO_PIN_NOPULL,
            NRF_GPIO_PIN_H0H1,
            NRF_GPIO_PIN_NOSENSE);
	ret_code_t err_code = nrf_drv_twi_init(&bsp_list.bsp_i2c_handler, &config, (nrf_drv_twi_evt_handler_t)bsp_list.i2c_callback_handler, NULL);
	APP_ERROR_CHECK(err_code);
	
	nrf_drv_twi_enable(&bsp_list.bsp_i2c_handler);
	//Q_DEVICE_LOG_INFO("open %s \r\n",bsp_list.name);			

    disable_irq();
	bsp_list.lock = true;
    enable_irq();
	return RESULT_OK;
	
}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_open
 * Description       : gpio out put open
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_i2c_close(q_device_t*dev)
{

	if(!bsp_list.lock)
	{
		return RESULT_I2C_CLOSE_ERR;
	}
	nrf_drv_twi_disable(&bsp_list.bsp_i2c_handler);
	nrf_drv_twi_uninit(&bsp_list.bsp_i2c_handler);

	*(volatile uint32_t *)0x40003FFC = 0;
	*(volatile uint32_t *)0x40003FFC;
	*(volatile uint32_t *)0x40003FFC = 1;
	// Nordic work around to prevent burning excess power due to chip errata
    // Use 0x400043FFC for TWI0.
//    *(volatile uint32_t *)0x40004FFC = 0;
//    *(volatile uint32_t *)0x40004FFC;
//    *(volatile uint32_t *)0x40004FFC = 1;

//			 nrf_gpio_cfg_input(bsp_list.bsp_io_scl_pin, NRF_GPIO_PIN_NOPULL);
//			 nrf_gpio_cfg_input(bsp_list.bsp_io_sda_pin, NRF_GPIO_PIN_NOPULL);
	 //Q_DEVICE_LOG_INFO("close %s \r\n",bsp_list.name);			
  
    disable_irq();
	bsp_list.lock = false;
    enable_irq();
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
static int bsp_i2c_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct i2c_package *package = (struct i2c_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}
	ret_code_t err_code;

	if(!bsp_list.lock)
	{
		return RESULT_I2C_DEV_UNOPENED_ERR;
	}

	if(package->write_length == 1)
	{
	  err_code = bsp_i2c_0_writereg(package->slave_addr, package->reg_addr, package->write_buff[0],&bsp_list.bsp_i2c_handler);
	}
	else
	{
		err_code = bsp_i2c_0_writeregs(package->slave_addr, package->reg_addr, package->write_buff, package->write_length,&bsp_list.bsp_i2c_handler);
	}

	if(err_code != NRF_SUCCESS)
	{
		return RESULT_I2C_SEND_ERR;
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
static int bsp_i2c_read(q_device_t *dev, int pos, const void *  buffer, int size)
{
	
	struct i2c_package *package = (struct i2c_package *)buffer;
	if(package == NULL)
	{
		return RESULT_I2C_CONFIG_NULL_ERR;
	}
    ret_code_t err_code;

	if(!bsp_list.lock)
	{
		return RESULT_I2C_DEV_NULL_ERR;
	}
//			Q_DEVICE_LOG_INFO(" %s i2c read： s:%x, r:%x, l:%d \r\n",bsp_list[i].name,package->slave_addr, package->reg_addr,package->read_length);

		err_code = bsp_i2c_0_readreg(package->slave_addr, package->reg_addr,package->read_buff, package->read_length,&bsp_list.bsp_i2c_handler);

	if(err_code != NRF_SUCCESS)
	{
		return RESULT_I2C_READ_ERR;
	}
	return RESULT_OK;

}

#endif

#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#include "i2c.h"




#define I2C_1_OP_TIMEOUT  100   //100ms for an Byte operation

struct  BSP_GPIO_I2C_1
{
	const char   *name;
	bool          lock;
	gpio_pin_e      bsp_io_scl_pin;
	gpio_pin_e      bsp_io_sda_pin;
	AP_I2C_TypeDef     *bsp_i2c_handler;
	i2c_dev_t    i2c_number;
	q_device_t dev;
};


static struct BSP_GPIO_I2C_1 bsp_list =    //硬件i2c--i2c_1
{
    .name = "i2c_1_acc",
	.lock = false,
    .bsp_io_scl_pin = (gpio_pin_e)2,
	.bsp_io_sda_pin = (gpio_pin_e)3,
	.bsp_i2c_handler = NULL,
	.i2c_number = I2C_0,
	.dev = {0},

};


/*******************************************************************************
 * Function Name     : bsp_gpio_i2c_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_i2c_open(q_device_t*dev)
{

	if(bsp_list.lock)
	{
		return RESULT_OK;
	}
	
	hal_gpio_pin_init(bsp_list.bsp_io_sda_pin,GPIO_INPUT);
    hal_gpio_pin_init(bsp_list.bsp_io_scl_pin,GPIO_INPUT);
    hal_gpio_pull_set(bsp_list.bsp_io_sda_pin,STRONG_PULL_UP);
    hal_gpio_pull_set(bsp_list.bsp_io_scl_pin,STRONG_PULL_UP);
	hal_i2c_pin_init(bsp_list.i2c_number, bsp_list.bsp_io_sda_pin, bsp_list.bsp_io_scl_pin);
    bsp_list.bsp_i2c_handler=hal_i2c_init(bsp_list.i2c_number,I2C_CLOCK_100K);
   if(bsp_list.bsp_i2c_handler==NULL)
    {
        Q_DEVICE_LOG_ERROR(" %s init fail\n",bsp_list.name);
		return RESULT_I2C_OPEN_ERR;
    }
	Q_DEVICE_LOG_INFO("open %s \r\n",bsp_list.name);			

	bsp_list.lock = true;
	return RESULT_OK;

}


/*******************************************************************************
 * Function Name     : bsp_gpio_output_open
 * Description       : gpio out put open
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_i2c_close(q_device_t*dev)
{

	if(!bsp_list.lock)
	{
		return RESULT_OK;
	}
	
	hal_i2c_deinit(bsp_list.bsp_i2c_handler);
	hal_gpio_pin_init(bsp_list.bsp_io_sda_pin,GPIO_INPUT);
    hal_gpio_pin_init(bsp_list.bsp_io_scl_pin,GPIO_INPUT);
	
	Q_DEVICE_LOG_INFO("close %s \r\n",bsp_list.name);			
  
	bsp_list.lock = false;
	return RESULT_OK;

}


/*******************************************************************************
 * Function Name     : bsp_i2c_write
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_i2c_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	I2C_INIT_TOUT(to);
	uint8_t i = 0;
	struct i2c_package *package = (struct i2c_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}


	if(!bsp_list.lock)
	{
		return RESULT_I2C_DEV_UNOPENED_ERR;
	}
	
//	hal_i2c_addr_update(bsp_list.bsp_i2c_handler, package->slave_addr);
//    HAL_ENTER_CRITICAL_SECTION();
//    hal_i2c_tx_start(bsp_list.bsp_i2c_handler);
//    hal_i2c_send(bsp_list.bsp_i2c_handler, &package->reg_addr, package->write_length+1);
//    HAL_EXIT_CRITICAL_SECTION();	
//    hal_i2c_wait_tx_completed(bsp_list.bsp_i2c_handler);
	bsp_list.bsp_i2c_handler->IC_ENABLE=0;
    bsp_list.bsp_i2c_handler->IC_TAR = package->slave_addr;
    bsp_list.bsp_i2c_handler->IC_ENABLE=1;
	HAL_ENTER_CRITICAL_SECTION();
	bsp_list.bsp_i2c_handler->IC_ENABLE=1;
	bsp_list.bsp_i2c_handler->IC_DATA_CMD = package->reg_addr;
	while(i != package->write_length)
    {
		bsp_list.bsp_i2c_handler->IC_DATA_CMD = package->write_buff[i];
		i++;
    }
	HAL_EXIT_CRITICAL_SECTION();
	 while(1)
    {

        if(bsp_list.bsp_i2c_handler->IC_RAW_INTR_STAT&0x200)//check tx empty
            break;

        I2C_CHECK_TOUT(to, I2C_1_OP_TIMEOUT, "hal_i2c_wait_tx_completed TO\n");
    }
	return RESULT_OK;

}


/*******************************************************************************
 * Function Name     : bsp_i2c_read
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 *******************************************************************************/
static int bsp_i2c_read(q_device_t *dev, int pos, const void *  buffer, int size)
{
	
	struct i2c_package *package = (struct i2c_package *)buffer;
	if(package == NULL)
	{
		return RESULT_I2C_CONFIG_NULL_ERR;
	}	
	
	if(!bsp_list.lock)
	{
		return RESULT_I2C_DEV_NULL_ERR;
	}
	hal_i2c_read(bsp_list.bsp_i2c_handler, package->slave_addr, package->reg_addr, package->read_buff, package->read_length);
	return RESULT_OK;
}


#endif

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
	.write = bsp_i2c_write,
	.read = bsp_i2c_read,
	.open = bsp_i2c_open,
	.close = bsp_i2c_close,
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
static void bsp_i2c0_register(void)
{

	bsp_list.dev.name = bsp_list.name;
	bsp_list.dev.dops  = &ops;
	q_device_register(&bsp_list.dev);		
	
}

device_initcall(bsp_i2c0_register);







