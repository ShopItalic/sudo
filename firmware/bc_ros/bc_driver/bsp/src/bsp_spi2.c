#include "q_device.h"

#include <string.h>

#if (HARDWARE_ARCH_TYPE_NORDIC == 1)
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_spim.h"
#include "nrf_spi.h"

struct bsp_spi_config
{
	bool          spi_xfer_done;
	uint32_t      cs_io_pin;
	uint32_t      miso_io_pin;
	uint32_t      mosi_io_pin;
	uint32_t      sclk_io_pin;
	nrf_spim_frequency_t frequency; ///< SPI frequency.
    nrf_spim_mode_t      mode;      ///< SPI mode.
	nrfx_spim_t spi_handler;
//	nrf_drv_spi_frequency_t frequency; ///< SPI frequency.
//    nrf_drv_spi_mode_t      mode;      ///< SPI mode.
//	nrf_drv_spi_t spi_handler;
	void *spi_callback_handler;
};

struct  BSP_SPI
{
	const char   *name;
	bool          lock;
	struct bsp_spi_config  spi_config;
	struct bsp_spi_mutex_lock   spi_mutex_lock;
	q_device_t dev;
};



static void spi_2_event_callback_handler(nrf_drv_spi_evt_t const * p_event,void * p_context);

static struct BSP_SPI bsp_list =   
{
	  .name = "spi_2",
	  .lock = false,
	  .spi_config = {
		              .spi_xfer_done = false,
#if defined(HANDWARE_1_5_3)

						.cs_io_pin = NRF_GPIO_PIN_MAP(0,17),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,1),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,4),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,00),
	
#elif (defined(HANDWARE_4_1_1) || defined(HANDWARE_4_1_2) || defined(RONG_WEI_Z2X))
						.cs_io_pin = NRF_GPIO_PIN_MAP(0,17),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,1),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,4),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,00),
					  
#elif (defined(HANDWARE_4_4_1) )
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,04),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,0),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,30),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,01),
#elif (defined(HANDWARE_4_1_3) )
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,15),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,16),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,12),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,14),					  
#elif (defined(HANDWARE_1_5_8) )
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,10),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(1,13),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,28),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,03),
#elif (defined(HANDWARE_4_5_1) )
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,7),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,8),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,5),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,12),		
#elif (defined(HANDWARE_1_14_1) )
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,31),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,12),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,15),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(1,9),			
#elif (defined(HANDWARE_1_17_1) )
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,30),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,28),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,7),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,6),		            
#elif (defined(HANDWARE_BCL601_151))
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,6),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(1,9),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(1,8),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,7),					  
#elif (defined(HANDWARE_1_19_1) )
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,28),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,30),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,00),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,1),	
#elif (defined(HANDWARE_1_23_1) )
#if defined(HANDWARE_1_23_3)
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,31),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,04),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,01),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,00),	
#else
					  .cs_io_pin = NRF_GPIO_PIN_MAP(0,12),
					  .miso_io_pin = NRF_GPIO_PIN_MAP(0,15),
					  .mosi_io_pin = NRF_GPIO_PIN_MAP(0,01),
					  .sclk_io_pin = NRF_GPIO_PIN_MAP(0,00),	
#endif
#endif						  

		              .spi_handler = NRFX_SPIM_INSTANCE(2),
#if (defined(NRF52840_XXAA))					  
					  .frequency = NRF_SPIM_FREQ_32M,
#else
                      .frequency = NRF_SPIM_FREQ_8M,
#endif
					  .mode = NRF_SPIM_MODE_0,

            .spi_callback_handler = spi_2_event_callback_handler,
            },
	  .spi_mutex_lock = {0},
	  .dev = {0},
};
//SPI事件处理函数
static void spi_2_event_callback_handler(nrf_drv_spi_evt_t const * p_event,void * p_context)
{
  //设置SPI传输完成  
	bsp_list.spi_config.spi_xfer_done = true;
//	Q_DEVICE_LOG_INFO("spi %s ok\r\n",bsp_list[0].name);
}




static bool bsp_spi_init(struct BSP_SPI *spi_config)
{
	nrf_gpio_cfg_output(spi_config->spi_config.cs_io_pin);
	
	nrf_gpio_pin_set(spi_config->spi_config.cs_io_pin);
	
	nrfx_spim_config_t config;
//	nrf_drv_spi_config_t config = NRF_DRV_SPI_DEFAULT_CONFIG;
	
    config.miso_pin = spi_config->spi_config.miso_io_pin;
    config.mosi_pin = spi_config->spi_config.mosi_io_pin;
    config.sck_pin  = spi_config->spi_config.sclk_io_pin;
	config.frequency = spi_config->spi_config.frequency;
	config.mode = spi_config->spi_config.mode;
	config.ss_active_high = false;
	config.orc = 0xFF;
	config.irq_priority = NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY,
	config.bit_order = NRF_SPIM_BIT_ORDER_MSB_FIRST;
	
	  //初始化SPI
//    APP_ERROR_CHECK(nrf_drv_spi_init(&spi_config->spi_config.spi_handler, &config, (nrf_drv_spi_evt_handler_t)spi_config->spi_config.spi_callback_handler, NULL));	
	 APP_ERROR_CHECK(nrfx_spim_init(&spi_config->spi_config.spi_handler, &config, (nrfx_spim_evt_handler_t)spi_config->spi_config.spi_callback_handler, NULL));
	spi_config->lock = true;
	return true;
}

static bool bsp_spi_uninit(struct BSP_SPI *spi_config)
{
	nrf_gpio_cfg_default(spi_config->spi_config.cs_io_pin);
	nrfx_spim_uninit(&spi_config->spi_config.spi_handler);
//	nrf_drv_spi_uninit(&spi_config->spi_config.spi_handler);
//	nrf_gpio_cfg_input(spi_config->spi_config.cs_io_pin, NRF_GPIO_PIN_NOPULL);
//	nrf_gpio_cfg_input(spi_config->spi_config.miso_io_pin, NRF_GPIO_PIN_NOPULL);
//	nrf_gpio_cfg_input(spi_config->spi_config.mosi_io_pin, NRF_GPIO_PIN_NOPULL);
//	nrf_gpio_cfg_input(spi_config->spi_config.sclk_io_pin, NRF_GPIO_PIN_NOPULL);
	spi_config->lock = false;
	return true;
}


/*******************************************************************************
 * Function Name     : bsp_spi_read_and_write
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static bool bsp_spi_read_and_write(struct spi_package *package,struct BSP_SPI *spi)
{
	uint32_t count = 0;
	spi->spi_config.spi_xfer_done = false;
	nrfx_spim_xfer_desc_t spim_xfer;
	spim_xfer.tx_length = package->write_length;
	spim_xfer.p_tx_buffer = package->write_buff;
	spim_xfer.rx_length = package->read_length;
	spim_xfer.p_rx_buffer =  package->read_buff;
	  //启动数据传输
    #if 1 // by liukun
    uint32_t ret = 0;
    count = 100;
    do {
        taskENTER_CRITICAL();
        ret =nrfx_spim_xfer(&spi->spi_config.spi_handler, &spim_xfer, 0);
        taskEXIT_CRITICAL();
        if(NRFX_ERROR_BUSY != ret)
            break;
        nrf_delay_us(1);
    } while (count--);
    count = 0;
    #else
	uint32_t ret =nrfx_spim_xfer(&spi->spi_config.spi_handler, &spim_xfer, 0); 
    #endif
//	uint32_t ret = nrf_drv_spi_transfer(&spi->spi_config.spi_handler,package->write_buff,package->write_length, package->read_buff, package->read_length);
//	Q_DEVICE_LOG_INFO("test1  %02x  %02x  %02x  %02x   \r\n",package->write_buff[0],package->write_buff[1],package->write_buff[2],package->write_buff[3]);
//	Q_DEVICE_LOG_INFO("test2  %02x  %02x  %02x  %02x   \r\n",package->write_buff[0],package->read_buff[0],package->read_buff[1],package->read_buff[2]);
//	Q_DEVICE_LOG_INFO("leng  %d   %d  ret:%d \r\n",package->write_length,package->read_length,ret);
	APP_ERROR_CHECK(ret);
	  //等待SPI传输完成
    while(!spi->spi_config.spi_xfer_done)
	{
		count++;
		if(count >= 100000)
		{
			Q_DEVICE_LOG_INFO("spi timeout \r\n");
			return false;
		}
		nrf_delay_us(1);

	}
//	Q_DEVICE_LOG_INFO("test1  %02x  %02x  %02x  %02x   \r\n",package->write_buff[0],package->write_buff[1],package->write_buff[2],package->write_buff[3]);
//	Q_DEVICE_LOG_INFO("test2  %02x  %02x  %02x  %02x   \r\n",package->write_buff[0],package->read_buff[0],package->read_buff[1],package->read_buff[2]);
//	Q_DEVICE_LOG_INFO("leng  %d   %d   \r\n",package->write_length,package->read_length);
    return true;
}



/*******************************************************************************
 * Function Name     : bsp_spi_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_spi_open(q_device_t*dev)
{

	if(bsp_list.lock)
	{
		return RESULT_OK;
	}
	if(bsp_spi_init(&bsp_list))
	{
//		Q_DEVICE_LOG_INFO("open %s \r\n",bsp_list.name);		
	}				
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
static int bsp_spi_close(q_device_t*dev)
{

	if(!bsp_list.lock)
	{
		return RESULT_OK;
	}
	bsp_spi_uninit(&bsp_list);
//	Q_DEVICE_LOG_INFO("close %s \r\n",bsp_list.name);		
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
static int bsp_spi_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct spi_package *package = (struct spi_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}

	if(!bsp_list.lock)
	{
		return RESULT_DEV_UNOPENED_ERR;
	}

	if(!bsp_spi_read_and_write(package,&bsp_list))
	{
		return RESULT_SPI_SEND_ERR;
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
static int bsp_spi_read(q_device_t *dev, int pos, const void *  buffer, int size)
{
	
	struct spi_package *package = (struct spi_package *)buffer;
	if(package == NULL)
	{
		return RESULT_I2C_CONFIG_NULL_ERR;
	}

	if(!bsp_list.lock)
	{
		return RESULT_DEV_NULL_ERR;
	}
//			Q_DEVICE_LOG_INFO(" %s i2c read： s:%x, r:%x, l:%d \r\n",bsp_list[i].name,package->slave_addr, package->reg_addr,package->read_length);
   
	if(!bsp_spi_read_and_write(package,&bsp_list))
	{
		return RESULT_I2C_READ_ERR;
	}

	return RESULT_OK;

}




static int bsp_spi_config(q_device_t *dev, void *args, void *var)
{
	struct bsp_spi_mutex_lock *cfg = (struct bsp_spi_mutex_lock *)args;
	if(cfg == NULL)
	{
		return RESULT_GPIO_CONFIG_NULL_ERR;
	}	
		
	bsp_list.spi_mutex_lock.spi_mutex_lock_enable = cfg->spi_mutex_lock_enable;
	bsp_list.spi_mutex_lock.spi_mutex_lock_take = cfg->spi_mutex_lock_take;
	bsp_list.spi_mutex_lock.spi_mutex_lock_give = cfg->spi_mutex_lock_give;
	return RESULT_OK;
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_spi_cs_ctrl(q_device_t *dev, int cmd, void *args)
{
	if(!bsp_list.lock)
	{
		return RESULT_DEV_UNOPENED_ERR;
	}
	if(cmd == GPIO_OUTPUT_LOW)
	{
		nrf_gpio_pin_clear(bsp_list.spi_config.cs_io_pin);
//		BC_LOG_INFO("spi cs low \r\n");
	}
	else if(cmd == GPIO_OUTPUT_HIGH)
	{
		nrf_gpio_pin_set(bsp_list.spi_config.cs_io_pin);
//		BC_LOG_INFO("spi cs High\r\n");
	}

	
	
	return RESULT_OK;
}


#endif



#if (HARDWARE_ARCH_TYPE_PHY6222 == 1)

#include "spi.h"
#include "dma.h"

struct bsp_spi_config
{

    spi_Cfg_t spi_cfg;
	HAL_DMA_t dma_cfg;
	hal_spi_t spiflash_spi;
};

struct  BSP_SPI
{
	const char   *name;
	bool          lock;
	struct bsp_spi_config  spi_config;
	struct bsp_spi_mutex_lock   spi_mutex_lock;
	q_device_t dev;
};

static void spi_cb(spi_evt_t* evt);
static void dma_cb(DMA_CH_t ch);

static uint8_t dma_flag= false;

static struct BSP_SPI bsp_list =   
{
	  .name = "spi_0",
	  .lock = false,
	  .spi_config.spi_cfg = {
		  						.sclk_pin = GPIO_P02,
								.ssn_pin = GPIO_P07,
								.MOSI = GPIO_P00,
								.MISO = GPIO_P03,

								.baudrate = 2000000,
								.spi_tmod = SPI_TRXD,
								.spi_scmod = SPI_MODE1,
								.spi_dfsmod = SPI_1BYTE,

								#if DMAC_USE
								.dma_tx_enable = false,
								.dma_rx_enable = false,
								#endif

								.int_mode = false,
								.force_cs = false, //true,
								.evt_handler = spi_cb,
	                       },
	  .spi_config.dma_cfg = {
								.dma_channel = DMA_CH_0,
								.evt_handler = dma_cb,		  
	  },
	  .spi_config.spiflash_spi = {
								.spi_index = SPI0,	  
	  },
	  .spi_mutex_lock = {0},
	  .dev = {0},
};


static void spi_cb(spi_evt_t* evt)
{
}

static void dma_cb(DMA_CH_t ch)
{
}

/*******************************************************************************
 * Function Name     : bsp_spi_open
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_spi_open(q_device_t*dev)
{
	uint8_t retval = PPlus_SUCCESS;

	if(bsp_list.lock)
	{
		return RESULT_OK;
	}
	retval = hal_spi_bus_init(&bsp_list.spi_config.spiflash_spi,bsp_list.spi_config.spi_cfg);
	if(retval != PPlus_SUCCESS)
	{
		Q_DEVICE_LOG_ERROR("open %s fail\r\n ",bsp_list.name);		
		return RESULT_OPEN_ERR;
	}
	if(!dma_flag)
	{
		hal_dma_init();
		retval = hal_dma_init_channel(bsp_list.spi_config.dma_cfg);
		if(retval != PPlus_SUCCESS)
		{
			Q_DEVICE_LOG_ERROR("open %s fail\r\n ",bsp_list.name);	
		}
		dma_flag = true;
	}
    hal_gpio_fmux(bsp_list.spi_config.spi_cfg.ssn_pin,Bit_DISABLE);
//	Q_DEVICE_LOG_INFO("open %s\r\n ",bsp_list.name);
	bsp_list.lock = true;	
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
static int bsp_spi_close(q_device_t*dev)
{

	if(!bsp_list.lock)
	{
		return RESULT_OK;
	}
	hal_spi_bus_deinit(&bsp_list.spi_config.spiflash_spi);	
	hal_gpioretention_unregister(bsp_list.spi_config.spi_cfg.ssn_pin);
//	Q_DEVICE_LOG_INFO("close %s \r\n",bsp_list.name);	
	bsp_list.lock = false;	
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
static int bsp_spi_write(q_device_t *dev, int pos,const void *buffer, int size)
{
	
	struct spi_package *package = (struct spi_package *)buffer;
	if(package == NULL)
	{
		return RESULT_UART_CONFIG_NULL_ERR;
	}

	if(!bsp_list.lock)
	{
		return RESULT_DEV_UNOPENED_ERR;
	}
	
	if(package->read_length != 0)
	{
//		Q_DEVICE_LOG_HEX("spi tx 11:",package->write_buff,package->write_length);
		hal_spi_dma_set(&bsp_list.spi_config.spiflash_spi,1,0);
		if(hal_spi_transmit(&bsp_list.spi_config.spiflash_spi,SPI_EEPROM,package->write_buff,package->read_buff,package->write_length,package->read_length) != PPlus_SUCCESS)
		{
			return RESULT_SPI_SEND_ERR;
		}		
	}
	else
	{
//		Q_DEVICE_LOG_HEX("spi tx 22:",package->write_buff,package->write_length);
		hal_spi_dma_set(&bsp_list.spi_config.spiflash_spi,1,0);
		if(hal_spi_transmit(&bsp_list.spi_config.spiflash_spi,SPI_TXD,package->write_buff,NULL,package->write_length,0) != PPlus_SUCCESS)
		{
			return RESULT_SPI_SEND_ERR;
		}			
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

static int bsp_spi_read(q_device_t *dev, int pos, const void *  buffer, int size)
{
	
	struct spi_package *package = (struct spi_package *)buffer;
	if(package == NULL)
	{
		return RESULT_I2C_CONFIG_NULL_ERR;
	}

	if(!bsp_list.lock)
	{
		return RESULT_DEV_NULL_ERR;
	}
//			Q_DEVICE_LOG_INFO(" %s i2c read： s:%x, r:%x, l:%d \r\n",bsp_list[i].name,package->slave_addr, package->reg_addr,package->read_length);
	if(package->write_length != 0)
	{
//		Q_DEVICE_LOG_HEX("spi tx 33:",package->write_buff,package->write_length);
		
		hal_spi_dma_set(&bsp_list.spi_config.spiflash_spi,1,0);
		if(hal_spi_transmit(&bsp_list.spi_config.spiflash_spi,SPI_EEPROM,package->write_buff,package->read_buff,package->write_length,package->read_length) != PPlus_SUCCESS)
		{
//			 Q_DEVICE_LOG_HEX("spi rx dddd:",package->read_buff,package->read_length);	
			return RESULT_SPI_SEND_ERR;
		}
//		Q_DEVICE_LOG_HEX("spi rx 33:",package->read_buff,package->read_length);		
	}
	else
	{
//		Q_DEVICE_LOG_HEX("spi tx 44:",package->write_buff,package->write_length);
		memset(package->write_buff,0,package->read_length);
//		uint8_t test_buff[10] = {0};
//		uint8_t test_rbuff[10] = {0x61,0,0};
		
		hal_spi_dma_set(&bsp_list.spi_config.spiflash_spi,0,1);
		
		
//		uint32_t ret = hal_spi_transmit(&bsp_list.spi_config.spiflash_spi,SPI_TRXD,test_buff,test_rbuff,4,4);
		uint32_t ret = hal_spi_transmit(&bsp_list.spi_config.spiflash_spi,SPI_TRXD,package->write_buff,package->read_buff,package->read_length,package->read_length);
		if(ret!= PPlus_SUCCESS)
		{
//			Q_DEVICE_LOG_INFO("ffff %d   %d \r\n",package->read_length,ret);
//			 Q_DEVICE_LOG_HEX("spi rx fff:",package->read_buff,package->read_length);	
			return RESULT_SPI_SEND_ERR;
		}
//		Q_DEVICE_LOG_INFO("ffffhhh %d   %02x %02x\r\n",package->read_length,test_rbuff[0],test_rbuff[1]);
//      Q_DEVICE_LOG_HEX("spi rx 44:",package->read_buff,package->read_length);		
	}
	return RESULT_OK;

}



static int bsp_spi_config(q_device_t *dev, void *args, void *var)
{
	struct bsp_spi_mutex_lock *cfg = (struct bsp_spi_mutex_lock *)args;
	if(cfg == NULL)
	{
		return RESULT_GPIO_CONFIG_NULL_ERR;
	}	
		
	bsp_list.spi_mutex_lock.spi_mutex_lock_enable = cfg->spi_mutex_lock_enable;
	bsp_list.spi_mutex_lock.spi_mutex_lock_take = cfg->spi_mutex_lock_take;
	bsp_list.spi_mutex_lock.spi_mutex_lock_give = cfg->spi_mutex_lock_give;
	return RESULT_OK;
}

/*******************************************************************************
 * Function Name     : bsp_gpio_output_ctrl
 * Description       : 
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年10月16日
 *******************************************************************************/
static int bsp_spi_cs_ctrl(q_device_t *dev, int cmd, void *args)
{
	if(!bsp_list.lock)
	{
		return RESULT_DEV_UNOPENED_ERR;
	}
	if(cmd == GPIO_OUTPUT_LOW)
	{
		AP_GPIO->swporta_dr &= ~BIT(bsp_list.spi_config.spi_cfg.ssn_pin);
		hal_gpio_pin_init(bsp_list.spi_config.spi_cfg.ssn_pin,GPIO_OUTPUT);
		hal_gpioretention_register(bsp_list.spi_config.spi_cfg.ssn_pin);
//		Q_DEVICE_LOG_INFO(" cs low \r\n");	
	}
	else if(cmd == GPIO_OUTPUT_HIGH)
	{
		AP_GPIO->swporta_dr |= BIT(bsp_list.spi_config.spi_cfg.ssn_pin);
		hal_gpio_pin_init(bsp_list.spi_config.spi_cfg.ssn_pin,GPIO_OUTPUT);
		hal_gpioretention_register(bsp_list.spi_config.spi_cfg.ssn_pin);
//		Q_DEVICE_LOG_INFO(" cs high \r\n");	
	}

	
	
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
	.read = bsp_spi_read,
	.open = bsp_spi_open,
	.close = bsp_spi_close,
	.write = bsp_spi_write,
	.control = bsp_spi_cs_ctrl,
	.config = bsp_spi_config,
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
static void bsp_spi_register(void)
{
	bsp_list.dev.name = bsp_list.name;
	bsp_list.dev.dops  = &ops;
	q_device_register(&bsp_list.dev);	
	
	
}


device_initcall(bsp_spi_register);







