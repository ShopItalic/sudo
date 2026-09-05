#include <stdint.h>
#include <stdio.h>
#include "ring_config.h"

#include "bc_sem.h"
#include "bc_rtos.h"

#define LOG_ENABLE  1

#if (DEBUG_INFO == 1)

													
#if (HARDWARE_ARCH_TYPE_NORDIC == 1)

#include "nrf.h"



//#define BC_LOG_INFO(format, ...)         			printf("\r\n[INFO:%d;%s(%d)] " format,NRF_RTC1->COUNTER, __MODULE__, __LINE__, ##__VA_ARGS__); 												 
//#define BC_LOG_DEBUG(format, ...)        			printf("\r\n[DEBUG:%d;%s:%d]: "format,NRF_RTC1->COUNTER,__MODULE__, __LINE__, ##__VA_ARGS__);
//#define BC_LOG_ERROR(format, ...)        			printf("\r\n[ERROR:%d;%s:%d]: "format,NRF_RTC1->COUNTER, __MODULE__, __LINE__, ##__VA_ARGS__);
//#define BC_LOG_WARN(format, ...)         			printf("\r\n[WARN :%d;%s:%d]: "format,NRF_RTC1->COUNTER,__MODULE__, __LINE__, ##__VA_ARGS__);	
//#define BC_LOG_PRINTF(format, ...)       			printf(format,##__VA_ARGS__); 

#if 1
#if (1 == LOG_ENABLE)
    #define BC_LOG_INFO(format, ...)                printf("\r\n[INFO:%d;%s(%d)] " format,bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__); 
    #define BC_LOG_DEBUG(format, ...)        			printf("\r\n[DEBUG:%d;%s:%d]: "format,bc_rtos_task_get_tick_count(),__MODULE__, __LINE__, ##__VA_ARGS__);
#else
    #define BC_LOG_INFO(format, ...)
    #define BC_LOG_DEBUG(format, ...)
#endif
//#define BC_LOG_INFO(format, ...)         			printf("\r\n[INFO:%d;%s(%d)] " format,bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__); 												 
//#define BC_LOG_DEBUG(format, ...)        			printf("\r\n[DEBUG:%d;%s:%d]: "format,bc_rtos_task_get_tick_count(),__MODULE__, __LINE__, ##__VA_ARGS__);
#define BC_LOG_ERROR(format, ...)        			printf("\r\n[ERROR:%d;%s:%d]: "format,bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__);
#define BC_LOG_WARN(format, ...)         			printf("\r\n[WARN :%d;%s:%d]: "format,bc_rtos_task_get_tick_count(),__MODULE__, __LINE__, ##__VA_ARGS__);	

#else
#if (1 == LOG_ENABLE)
    #define BC_LOG_INFO(format, ...)                do { \
                                                        bc_rtos_sem_take(BC_LOGGER_SEM); \
                                                        printf("\r\n[INFO:%d;%s(%d)] " format,bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__); \
                                                        bc_rtos_sem_give(BC_LOGGER_SEM); \
                                                    } while(0);
    #define BC_LOG_DEBUG(format, ...)        			do { \
                                                        bc_rtos_sem_take(BC_LOGGER_SEM); \
                                                        printf("\r\n[DEBUG:%d;%s:%d]: "format,bc_rtos_task_get_tick_count(),__MODULE__, __LINE__, ##__VA_ARGS__); \
                                                        bc_rtos_sem_give(BC_LOGGER_SEM); \
                                                    } while(0);
#else
    #define BC_LOG_INFO(format, ...)
    #define BC_LOG_DEBUG(format, ...)
#endif
//#define BC_LOG_INFO(format, ...)         			printf("\r\n[INFO:%d;%s(%d)] " format,bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__); 												 
//#define BC_LOG_DEBUG(format, ...)        			printf("\r\n[DEBUG:%d;%s:%d]: "format,bc_rtos_task_get_tick_count(),__MODULE__, __LINE__, ##__VA_ARGS__);
#define BC_LOG_ERROR(format, ...)        			do { \
                                                        bc_rtos_sem_take(BC_LOGGER_SEM); \
                                                        printf("\r\n[ERROR:%d;%s:%d]: "format,bc_rtos_task_get_tick_count(), __MODULE__, __LINE__, ##__VA_ARGS__); \
                                                        bc_rtos_sem_give(BC_LOGGER_SEM); \
                                                    } while(0);
#define BC_LOG_WARN(format, ...)         			do { \
                                                        bc_rtos_sem_take(BC_LOGGER_SEM); \
                                                        printf("\r\n[WARN :%d;%s:%d]: "format,bc_rtos_task_get_tick_count(),__MODULE__, __LINE__, ##__VA_ARGS__); \
                                                        bc_rtos_sem_give(BC_LOGGER_SEM); \
                                                    } while(0);
#endif
#define BC_LOG_PRINTF(format, ...)       			printf(format,##__VA_ARGS__); 
												 
#define BC_LOG_HEX(chars,data,length)               printf("%s",chars); \
												    for(uint32_t i = 0; i < length; i++) \
												    { \
													   printf("0x%02x ",data[i]); \
												    }; \
												    printf("\r\n");
													

												
#elif (HARDWARE_ARCH_TYPE_PHY6222 == 1)	
													
#include "clock.h"
													
#define BC_LOG_INFO(format, ...)         			printf("\r\n[INFO:%d;%s(%d)] " format,hal_systick(), __MODULE__, __LINE__, ##__VA_ARGS__); 												 
#define BC_LOG_DEBUG(format, ...)        			printf("\r\n[DEBUG:%d;%s:%d]: "format,hal_systick(),__MODULE__, __LINE__, ##__VA_ARGS__);
#define BC_LOG_ERROR(format, ...)        			printf("\r\n[ERROR:%d;%s:%d]: "format,hal_systick(), __MODULE__, __LINE__, ##__VA_ARGS__);
#define BC_LOG_WARN(format, ...)         			printf("\r\n[WARN :%d;%s:%d]: "format,hal_systick(),__MODULE__, __LINE__, ##__VA_ARGS__);	
#define BC_LOG_PRINTF(format, ...)       			printf(format,##__VA_ARGS__); 
												 
#define BC_LOG_HEX(chars,data,length)               printf("%s",chars); \
												    for(uint32_t i = 0; i < length; i++) \
												    { \
													   printf("0x%02x ",data[i]); \
												    }; \
												    printf("\r\n");

#endif													
													



#else 

#define BC_LOG_INFO(...)
#define BC_LOG_DEBUG(...)
#define BC_LOG_ERROR(...)
#define BC_LOG_WARN(...)
#define BC_LOG_PRINTF(...)
#define BC_LOG_HEX(chars,data,length)              BC_LOG_PRINTF("%s,%d,%d",chars,data[0],length) ;

#endif

void BC_LOG_BLE(const char* format, ...);

void bc_log_ble_enable(void);

void bc_log_ble_disenable(void);

void BC_LOG_HEX_P(char *_cData, uint8_t *_pu8Data ,uint16_t _u16Len);
													
													
													
													
													
void services_print_log(const char* format, ...);


