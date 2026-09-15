#ifndef TEST_FACTORY_SCROLL_RTOS_H
#define TEST_FACTORY_SCROLL_RTOS_H
#include "../factory_ptt_v2/bc_rtos.h"
void fixture_suspend(void);
int fixture_resume(void);
#define vTaskSuspendAll() fixture_suspend()
#define xTaskResumeAll() fixture_resume()
#endif
