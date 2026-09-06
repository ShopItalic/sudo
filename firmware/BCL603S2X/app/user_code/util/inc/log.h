#ifndef __LOG_H__
#define __LOG_H__

#include "stdio.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "SEGGER_RTT.h"



#if NRF_LOG_ENABLED

//#define LOG_INFO(format, ...)                   SEGGER_RTT_printf(0, "[INFO:%s(%d)] " format, __MODULE__, __LINE__, ##__VA_ARGS__); 
//#define LOG_DEBUG(format, ...)                  SEGGER_RTT_printf(0, "[DEBUG:%s(%d)] " format, __MODULE__, __LINE__, ##__VA_ARGS__); 
//#define LOG_ERROR(format, ...)                  SEGGER_RTT_printf(0, "[ERROR: %s(%d)] " format, __MODULE__, __LINE__, ##__VA_ARGS__); 
//#define LOG_WARN(format, ...)                   SEGGER_RTT_printf(0, "[WARN:%s(%d)] " format, __MODULE__, __LINE__, ##__VA_ARGS__); 
//#define LOG_PRINTF(format, ...)                 SEGGER_RTT_printf(0,format,##__VA_ARGS__); 

//#define LOG_HEX(chars,data,length)               LOG_PRINTF("%s",chars); \
//	                                             for(uint32_t i = 0; i < length; i++) \
//												 { \
//													 LOG_PRINTF("0x%02x ",data[i]); \
//												 }; \
//												 LOG_PRINTF("\r\n");
												 
#define LOG_INFO(format, ...)         			printf("[INFO:%s(%d)] " format, __MODULE__, __LINE__, ##__VA_ARGS__); 												 
#define LOG_DEBUG(format, ...)        			printf("[DEBUG %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);
#define LOG_ERROR(format, ...)        			printf("[ERROR %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);
#define LOG_WARN(format, ...)         			printf("[WARN %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);	
#define LOG_PRINTF(format, ...)       			printf(format,##__VA_ARGS__); 
												 
#define LOG_HEX(chars,data,length)               printf("%s",chars); \
	                                             for(uint32_t i = 0; i < length; i++) \
												 { \
													printf("0x%02x ",data[i]); \
												 }; \
												 printf("\r\n");												 
												 
										
#else 

#define LOG_INFO(...)
#define LOG_DEBUG(...)
#define LOG_ERROR(...)
#define LOG_WARN(...)
#define LOG_PRINTF(...)
#define LOG_HEX(chars,data,length)             LOG_PRINTF("%s,%d,%d",chars,data[0],length) ;

#endif

//#if defined(RTT_LOGS)
////#include "SEGGER_RTT.h"
//////#include "nrf_log.h"
//////#include "nrf_log_ctrl.h"
//////#include "nrf_log_default_backends.h"





////#define DEBUG_LOG(format, ...)      SEGGER_RTT_printf(0, "[%s(%d)] " format, __MODULE__, __LINE__, ##__VA_ARGS__); 
////#define DEBUG(format, ...)          SEGGER_RTT_printf(0,format,##__VA_ARGS__); 
////#define DEBUG_RTT_LOG_INIT(...)     SEGGER_RTT_Init(); 

////#define LOG_INFO(format, ...)         printf("[INFO %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);
//#define LOG_INFO(format, ...)         printf("[INFO]: "format,##__VA_ARGS__);
//#define LOG_DEBUG(format, ...)        printf("[DEBUG %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);
//#define LOG_ERROR(format, ...)        printf("[ERROR %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);
//#define LOG_WARN(format, ...)         printf("[WARN %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);
//#define LOG_FATAL(format, ...)        printf("[FATAL %s:%d]: "format, __MODULE__, __LINE__, ##__VA_ARGS__);

//#define log_print(format, ...)        printf(format,##__VA_ARGS__);

//#else


//#define DEBUG_LOG(...)
//#define DEBUG_RTT_LOG_INIT(...)

//#define LOG_INFO(...)
//#define LOG_DEBUG(...)
//#define LOG_ERROR(...)
//#define LOG_WARN(...)
//#define LOG_FATAL(...)
//#define log_print(...)

//#endif

void log_init(void);

#endif /*__DEBUG_H__*/


