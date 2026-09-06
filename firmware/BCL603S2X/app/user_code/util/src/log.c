#include "log.h"

#include "q_device.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

//static q_device_t *serial_log_dev;
//struct serial_configure serial_config;
#if defined(RTT_LOGS)	

void uart_1_recv_callback(serial_uart_package *uart_package)
{
	printf("uart = %d, length = %d \r\n",uart_package->uart_serial_port,uart_package->uart_data_leng);
	//rtt_output_hex("uart 1 recv:",uart_package->uart_data_buff,uart_package->uart_data_leng);
	q_device_write(serial_log_dev,0,uart_package->uart_data_buff,uart_package->uart_data_leng);
}


#endif



void log_init(void)
{
#if defined(RTT_LOGS)	
	
	serial_config.baud_rate = UART_BAUD_RATE_115200;
	
  serial_log_dev = q_device_find("debug_uart");
	
	q_device_cfg(serial_log_dev,&serial_config,NULL);
	
	q_device_open(serial_log_dev);
	
	q_device_reg_callback(serial_log_dev,0,uart_1_recv_callback);
	
#endif	
}













