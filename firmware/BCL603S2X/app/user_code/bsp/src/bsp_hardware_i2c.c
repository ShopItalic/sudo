#include "q_device.h"

#include "nrf_drv_twi.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include <string.h>

#include "nrf_gpio.h"


/* Indicates if operation on TWI has ended. */
static volatile bool i2c_0_xfer_done = false;
static volatile bool i2c_0_err_flag = false;

/* Indicates if operation on TWI has ended. */
static volatile bool i2c_1_xfer_done = false;
static volatile ret_code_t nrf_transfer_status;


struct  BSP_GPIO_I2C
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
static void i2c_1_callback_handler(nrf_drv_twi_evt_t const *p_event, void *p_context);

static struct BSP_GPIO_I2C bsp_list[2] =    //0--i2c_0(twi0)   1--i2c_1(twi1)
{
	{
	  .name = "i2c_0_acc",
		.lock = false,
	  .bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,29),
		.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,27),
		.bsp_i2c_handler = NRF_DRV_TWI_INSTANCE(0),    //不可更换i2c 编号 
		.i2c_callback_handler = i2c_0_callback_handler,
		.dev = {0},
	},
	
	{
	  .name = "i2c_1_ppg",
		.lock = false,
	  .bsp_io_scl_pin = NRF_GPIO_PIN_MAP(0,6),
		.bsp_io_sda_pin = NRF_GPIO_PIN_MAP(0,8),
		.bsp_i2c_handler = NRF_DRV_TWI_INSTANCE(1),    //不可更换i2c 编号 
		.i2c_callback_handler = i2c_1_callback_handler,
		.dev = {0},
	},
	
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
    // NRF_LOG_INFO("i2c_0_handler:%x",p_event->type);
    switch (p_event->type)
    {
    case NRF_DRV_TWI_EVT_DONE:
//        NRF_LOG_INFO("i2c_0: NRF_DRV_TWI_EVT_DONE");
        i2c_0_xfer_done = true;
        break;

    case NRF_DRV_TWI_EVT_ADDRESS_NACK:
        NRF_LOG_INFO("i2c_0: NRF_DRV_TWI_EVT_ADDRESS_NACK");
        i2c_0_xfer_done = true;
        i2c_0_err_flag = true;
        break;

    case NRF_DRV_TWI_EVT_DATA_NACK:
        NRF_LOG_INFO("i2c_0: NRF_DRV_TWI_EVT_DATA_NACK");
        i2c_0_xfer_done = true;
        i2c_0_err_flag = true;
        break;
    default:
        NRF_LOG_INFO("i2c_0: DEFAULT (%d) in mEventHandler", p_event->type);
        i2c_0_xfer_done = true;
        i2c_0_err_flag = true;
        break;
    }
}




/*******************************************************************************
 * Function Name     :  i2c_1_callback_handler
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static void i2c_1_callback_handler(nrf_drv_twi_evt_t const *p_event, void *p_context)
{
    // NRF_LOG_INFO("i2c_1_handler:%x",p_event->type);
    switch (p_event->type)
    {
    case NRF_DRV_TWI_EVT_DONE:
        // NRF_LOG_INFO("NRF_DRV_TWI_EVT_DONE in mEventHandler", p_event->type);
        i2c_1_xfer_done = true;
        nrf_transfer_status = NRF_SUCCESS;
        break;

    case NRF_DRV_TWI_EVT_ADDRESS_NACK:
        NRF_LOG_INFO("i2c_1: NRF_DRV_TWI_EVT_ADDRESS_NACK");
        i2c_1_xfer_done = true;
        nrf_transfer_status = NRF_ERROR_NOT_FOUND;
        break;

    case NRF_DRV_TWI_EVT_DATA_NACK:
        NRF_LOG_INFO("i2c_1: NRF_DRV_TWI_EVT_DATA_NACK");
        i2c_1_xfer_done = true;
        nrf_transfer_status = NRF_ERROR_INVALID_PARAM;
        break;
    default:
        NRF_LOG_INFO("hrs_twi: DEFAULT (%d) in mEventHandler", p_event->type);
        i2c_1_xfer_done = true;
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
static uint8_t bsp_i2c_0_writereg(uint8_t slave, uint8_t reg_add, uint8_t reg_dat,nrf_drv_twi_t *bsp_i2c_handler)
{
    uint32_t timeout_cnt = 0; 
    uint8_t reg[2] = {reg_add, reg_dat};
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
    return NRF_SUCCESS;
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
static uint8_t bsp_i2c_0_writeregs(uint8_t slave, uint8_t reg_add,const uint8_t *buf, uint8_t num,nrf_drv_twi_t *bsp_i2c_handler)
{
    if(num > 63){
 //       NRF_LOG_INFO("1111111111");
        return NRF_ERROR_INVALID_LENGTH;
    }
    uint32_t timeout_cnt = 0; 
    uint8_t reg[64];
    reg[0] = reg_add;
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
    return NRF_SUCCESS;
}

/*******************************************************************************
 * Function Name     :  bsp_i2c_1_writereg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static uint8_t bsp_i2c_1_writereg(uint8_t slave, uint8_t reg_add, uint8_t reg_dat,nrf_drv_twi_t *bsp_i2c_handler)
{
    uint32_t timeout_cnt = 0; 
    ret_code_t err_code;
    uint8_t reg[2] = {reg_add, reg_dat};
    i2c_1_xfer_done = false;
    err_code = nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 2, false);
    while (i2c_1_xfer_done == false){
        if(timeout_cnt++ >= 100000){
            break;
        }
    }
    return err_code;
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
static uint8_t bsp_i2c_0_readreg(uint8_t slave, uint8_t reg_add, uint8_t *buf, uint8_t num,nrf_drv_twi_t *bsp_i2c_handler)
{
    uint32_t timeout_cnt = 0; 
    uint8_t reg[1];
    reg[0] = reg_add;
    i2c_0_xfer_done = false;
    i2c_0_err_flag = false;
    nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 1, true);
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
    return NRF_SUCCESS;
}


/*******************************************************************************
 * Function Name     : bsp_i2c_1_readreg
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static uint8_t bsp_i2c_1_readreg(uint8_t slave, uint8_t reg_add, uint8_t *buf, uint8_t num,nrf_drv_twi_t *bsp_i2c_handler)
{
    uint32_t timeout_cnt = 0; 
    uint8_t reg[1];
    reg[0] = reg_add;
    i2c_1_xfer_done = false;
    nrf_drv_twi_tx(bsp_i2c_handler, slave, reg, 1, true);
    while (i2c_1_xfer_done == false){
        if(timeout_cnt++ >= 100000){
            break;
        }
    }
    if (nrf_transfer_status != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("nrf_drv_twi_tx fail: 0x%02X", slave);
        return nrf_transfer_status;
    }

    i2c_1_xfer_done = false;
    nrf_drv_twi_rx(bsp_i2c_handler, slave, buf, num);
    timeout_cnt = 0;
    while (i2c_1_xfer_done == false){
        if(timeout_cnt++ >= 100000){
            break;
        }
    }
    if (nrf_transfer_status != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("nrf_drv_twi_rx fail: 0x%02X", slave);
        return nrf_transfer_status;
    }
    return NRF_SUCCESS;
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
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(bsp_list[i].lock)
			{
				return RESULT_OK;
			}
        const nrf_drv_twi_config_t config = {
            .scl = bsp_list[i].bsp_io_scl_pin,
            .sda = bsp_list[i].bsp_io_sda_pin,
            .frequency = NRF_DRV_TWI_FREQ_100K,
            .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
            .clear_bus_init = false,
            //.hold_bus_uninit = NRF_GPIO_PIN_NOPULL
            .hold_bus_uninit = NRF_GPIO_PIN_PULLUP
            
        };
        ret_code_t err_code = nrf_drv_twi_init(&bsp_list[i].bsp_i2c_handler, &config, (nrf_drv_twi_evt_handler_t)bsp_list[i].i2c_callback_handler, NULL);
        APP_ERROR_CHECK(err_code);
        
        nrf_drv_twi_enable(&bsp_list[i].bsp_i2c_handler);
//        NRF_LOG_INFO("open %s ",bsp_list[i].name);			

			bsp_list[i].lock = true;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
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
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_OK;
			}
			nrf_drv_twi_disable(&bsp_list[i].bsp_i2c_handler);
			nrf_drv_twi_uninit(&bsp_list[i].bsp_i2c_handler);
			if(i == 0)
			{
				*(volatile uint32_t *)0x40003FFC = 0;
				*(volatile uint32_t *)0x40003FFC;
				*(volatile uint32_t *)0x40003FFC = 1;
			}
			else
			{
			  *(volatile uint32_t *)0x40004FFC = 0;
				*(volatile uint32_t *)0x40004FFC;
				*(volatile uint32_t *)0x40004FFC = 1;
			}
//        NRF_LOG_INFO("close %s",bsp_list[i].name);			

			bsp_list[i].lock = false;
			return RESULT_OK;
		}
	}
	return RESULT_GPIO_OUTPUT_DEV_NULL_ERR;	
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
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_I2C_DEV_UNOPENED_ERR;
			}
			if(i == 0)
			{
				if(package->write_length == 1)
				{
				  err_code = bsp_i2c_0_writereg(package->slave_addr, package->reg_addr, package->write_buff[0],&bsp_list[i].bsp_i2c_handler);
				}
				else
				{
					err_code = bsp_i2c_0_writeregs(package->slave_addr, package->reg_addr, package->write_buff, package->write_length,&bsp_list[i].bsp_i2c_handler);
				}
			}
			else
			{
				err_code = bsp_i2c_1_writereg(package->slave_addr, package->reg_addr, package->write_buff[0],&bsp_list[i].bsp_i2c_handler);
				
			}
			if(err_code != NRF_SUCCESS)
			{
				return RESULT_I2C_SEND_ERR;
			}
			return RESULT_OK;
		}
	}
  return RESULT_I2C_DEV_NULL_ERR;
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
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		if (!strcmp(bsp_list[i].name, dev->name))
		{
			if(!bsp_list[i].lock)
			{
				return RESULT_I2C_DEV_NULL_ERR;
			}
//			NRF_LOG_INFO(" %s i2c read： s:%x, r:%x, l:%d",bsp_list[i].name,package->slave_addr, package->reg_addr,package->read_length);
			if(i == 0)
			{
				err_code = bsp_i2c_0_readreg(package->slave_addr, package->reg_addr,package->read_buff, package->read_length,&bsp_list[i].bsp_i2c_handler);
			}
			else
			{
				err_code = bsp_i2c_1_readreg(package->slave_addr, package->reg_addr,package->read_buff, package->read_length,&bsp_list[i].bsp_i2c_handler);
			}
			if(err_code != NRF_SUCCESS)
			{
				return RESULT_I2C_READ_ERR;
			}
			return RESULT_OK;
		}
	}
  return RESULT_I2C_DEV_NULL_ERR;
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
static void bsp_i2c_register(void)
{
	for (uint8_t i = 0; i < array_size(bsp_list); i++) 
	{
		bsp_list[i].dev.name = bsp_list[i].name;
		bsp_list[i].dev.dops  = &ops;
		q_device_register(&bsp_list[i].dev);		
	}
}

device_initcall(bsp_i2c_register);

