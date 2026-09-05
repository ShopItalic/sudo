#include "bc_wifi.h"


#include "bc_wifi_port.h"

#include "bc_logger.h"
#include "bc_delay.h"
#if ( HARDWARE_1191_ENABLED == 1)	

#include "bc_led_pwm.h"

#endif	


void  bc_wifi_open(void)
{
  bc_wifi_device_disable();	
  bc_delay_ms(100);
  bc_wifi_device_enable();	
 #if ( HARDWARE_1191_ENABLED == 1)	
  bc_led_white_breathe_start(LED_WHITE_BREATHE_2S);
#endif	
}

void  bc_wifi_close(void)
{
  bc_wifi_device_disable();	
}

//uint8_t bc_wifi_write_buff[1024] = {0};
static uint8_t bc_wifi_read_buff[1024] = {0};
void bc_wifi_send(uint8_t *data,uint16_t length)
{

  bc_wifi_spi_cs_low();
  bc_wifi_spi_write_and_read(data,length,bc_wifi_read_buff,length);
  bc_wifi_spi_cs_high();
}
