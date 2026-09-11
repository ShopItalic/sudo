#include "q_device.h"

#include <string.h>


#include "nrf52840.h"




struct  BSP_SYS
{
	const char   *name;
	q_device_t dev;
};


static struct BSP_SYS bsp_list = 
{
	.name = "system",
	.dev = {0},	
};




static void bsp_sys_reboot(void)
{
	NVIC_SystemReset();
}

static void print_reset_reason(uint32_t resetReason) {
    // 输出复位原因
    if (resetReason & (1 << 0)) {
        Q_DEVICE_LOG_INFO("Power-on Reset\n");
    }
    if (resetReason & (1 << 1)) {
        Q_DEVICE_LOG_INFO("Reset Pin Reset\n");
    }
    if (resetReason & (1 << 2)) {
        Q_DEVICE_LOG_INFO("Watchdog Reset\n");
    }
    if (resetReason & (1 << 3)) {
       Q_DEVICE_LOG_INFO("Soft Reset\n");
    }
    if (resetReason & (1 << 16)) {
       Q_DEVICE_LOG_INFO("CPU Lock-up Reset\n");
    }
    if (resetReason & (1 << 17)) {
        Q_DEVICE_LOG_INFO("Lost Debug Connection Reset\n");
    }
    if (resetReason & (1 << 18)) {
       Q_DEVICE_LOG_INFO("Software System Reset Request\n");
    }
}

static uint32_t bsp_sys_reset_reason(void)
{
	  // 读取RESETREAS寄存器获取复位原因
	 uint32_t resetReason = 0;
//    uint32_t resetReason = *(volatile uint32_t*)(NRF_POWER_BASE + 0x114);

//    // 清除复位原因
//    *(volatile uint32_t*)(NRF_POWER_BASE + 0x118) = resetReason;
	sd_power_reset_reason_get(&resetReason);
	sd_power_reset_reason_clr(resetReason);
	Q_DEVICE_LOG_INFO("reset source %x",resetReason);
    // 输出复位原因
    print_reset_reason(resetReason);	
    return resetReason;
}

static void bsp_sys_device_mac_get(uint8_t *mac_buff)
{
//   	device_mac[0] = NRF_FICR->DEVICEADDR[0];
//	device_mac[1] = NRF_FICR->DEVICEADDR[1];
	*(uint32_t*)mac_buff = NRF_FICR->DEVICEADDR[0];
	*(uint16_t*)&mac_buff[4] = (uint16_t)NRF_FICR->DEVICEADDR[1];
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
static int bsp_sys_ctrl(q_device_t *dev, int cmd, void *args)
{
	switch(cmd)
	{
		case SYS_RESET_REASON_GET:
		{
			*(uint32_t*)args = bsp_sys_reset_reason();
			break;
		}
		case SYS_REBOOT:
		{
			bsp_sys_reboot();
			break;
		}
		case SYS_DEVICE_MAC_GET:
		{
			bsp_sys_device_mac_get(args);
			break;
		}			
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
	.control = bsp_sys_ctrl,
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
static void bsp_sys_register(void)
{

	bsp_list.dev.name = bsp_list.name;
	bsp_list.dev.dops  = &ops;
	q_device_register(&bsp_list.dev);		
}

device_initcall(bsp_sys_register);







































