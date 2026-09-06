#ifndef SUDO_LED_TEST_APP_ERROR_H
#define SUDO_LED_TEST_APP_ERROR_H
#include <stdint.h>
#define NRF_ERROR_NO_MEM 4U
void test_task_fatal(uint32_t error);
#define APP_ERROR_HANDLER(error) test_task_fatal(error)
#endif
