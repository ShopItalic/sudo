#ifndef SUDO_MOTOR_TEST_APP_ERROR_H
#define SUDO_MOTOR_TEST_APP_ERROR_H
#include <stdlib.h>
#define NRF_ERROR_NO_MEM 4U
#define APP_ERROR_HANDLER(error) do { (void)(error); abort(); } while (0)
#endif
