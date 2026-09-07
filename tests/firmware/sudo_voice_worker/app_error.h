#ifndef TEST_VOICE_APP_ERROR_H
#define TEST_VOICE_APP_ERROR_H
#include <stdint.h>
void test_voice_fatal(uint32_t code);
#define APP_ERROR_HANDLER(code) test_voice_fatal(code)
#endif
