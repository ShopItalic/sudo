#ifndef TEST_TOUCH_TUNING_FREERTOS_H
#define TEST_TOUCH_TUNING_FREERTOS_H

extern unsigned test_touch_tuning_critical_depth;
extern unsigned test_touch_tuning_critical_enters;
extern unsigned test_touch_tuning_critical_exits;

void test_touch_tuning_enter(void);
void test_touch_tuning_exit(void);

#define taskENTER_CRITICAL() test_touch_tuning_enter()
#define taskEXIT_CRITICAL() test_touch_tuning_exit()

#endif
