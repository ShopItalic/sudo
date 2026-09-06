#include "bc_ic_led.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bc_device_info.h"
#include "bc_rtos.h"
#include "tx1812n5.h"

static unsigned checks;
static unsigned failures;

static void check_condition(bool condition, const char *expression,
                            unsigned line)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

static const uintptr_t event_handle_value = 0x1234U;
static EventGroupHandle_t event_handle;
static bc_event_bits pending_bits;
static unsigned event_set_calls;
static unsigned isr_event_set_calls;
static unsigned wait_calls;
static unsigned thread_create_calls;
static TaskFunction_t worker_task;
static jmp_buf worker_stop;
static bool worker_stop_armed;

static unsigned rgb_init_calls;
static unsigned rgb_write_calls;
static struct rgb_struct rgb_writes[32];
static unsigned rgb_power_on_calls;
static unsigned rgb_power_off_calls;
static unsigned delay_calls;
static uint32_t delay_total_ms;
static unsigned device_info_calls;

EventGroupHandle_t bc_rtos_event_group_create(void)
{
    event_handle = (EventGroupHandle_t)event_handle_value;
    return event_handle;
}

bc_event_bits bc_rtos_event_group_wait_bits(EventGroupHandle_t handle,
                                             bc_event_bits bits_to_wait_for,
                                             BaseType_t clear_on_exit,
                                             BaseType_t wait_for_all_bits,
                                             uint32_t ticks_to_wait)
{
    bc_event_bits result;

    CHECK(handle == event_handle);
    CHECK(bits_to_wait_for != 0U);
    CHECK(clear_on_exit != 0);
    CHECK(wait_for_all_bits == 0);
    CHECK(ticks_to_wait == UINT32_MAX);

    ++wait_calls;
    if (wait_calls > 1U)
    {
        CHECK(worker_stop_armed);
        longjmp(worker_stop, 1);
    }

    result = pending_bits;
    pending_bits = 0U;
    return result;
}

bc_event_bits bc_rtos_event_group_set_bits(EventGroupHandle_t handle,
                                            bc_event_bits bits_to_set)
{
    CHECK(handle == event_handle);
    CHECK((bits_to_set & 0xff000000U) == 0U); /* FreeRTOS reserves the top byte. */
    ++event_set_calls;
    pending_bits |= bits_to_set;
    return pending_bits;
}

bc_base_type_t bc_rtos_thread_create(TaskFunction_t task_code,
                                     const char *name,
                                     uint16_t stack_depth,
                                     void *parameters,
                                     UBaseType_t priority,
                                     TaskHandle_t *created_task)
{
    CHECK(task_code != NULL);
    CHECK(name != NULL);
    CHECK(stack_depth != 0U);
    CHECK(parameters != NULL);
    (void)priority;
    ++thread_create_calls;
    worker_task = task_code;
    if (created_task != NULL)
        *created_task = (TaskHandle_t)(uintptr_t)0x2U;
    return bc_pdPASS;
}

BaseType_t xEventGroupSetBitsFromISR(EventGroupHandle_t handle,
                                     bc_event_bits bits_to_set,
                                     BaseType_t *higher_priority_task_woken)
{
    CHECK(handle == event_handle);
    ++isr_event_set_calls;
    pending_bits |= bits_to_set;
    if (higher_priority_task_woken != NULL)
        *higher_priority_task_woken = pdFALSE;
    return pdFALSE;
}

void tx1812n5_rgb_init(void)
{
    ++rgb_init_calls;
}

void tx1812n5_RGB(struct rgb_struct *rgb_config, uint16_t count)
{
    CHECK(rgb_config != NULL);
    CHECK(count == 1U);
    if (rgb_config == NULL || count != 1U)
        return;
    if (rgb_write_calls < (sizeof(rgb_writes) / sizeof(rgb_writes[0])))
        rgb_writes[rgb_write_calls] = *rgb_config;
    ++rgb_write_calls;
}

void bc_ldo_rgb_power_on(void)
{
    ++rgb_power_on_calls;
}

void bc_ldo_rgb_power_off(void)
{
    ++rgb_power_off_calls;
}

void bc_delay_ms(uint32_t milliseconds)
{
    ++delay_calls;
    delay_total_ms += milliseconds;
}

void bc_device_info_led_motor_mode_info_get(
    bc_device_led_motor_mode_info *info)
{
    ++device_info_calls;
    if (info == NULL)
        return;
    /* Deliberately choose blue so a persisted color cannot pass as green. */
    info->ble_connect_color = 0x01U;
    info->ble_disconnect_color = 0x01U;
    info->recording_color = 0x01U;
}

static void reset_observations(void)
{
    pending_bits = 0U;
    event_set_calls = 0U;
    isr_event_set_calls = 0U;
    wait_calls = 0U;
    rgb_init_calls = 0U;
    rgb_write_calls = 0U;
    rgb_power_on_calls = 0U;
    rgb_power_off_calls = 0U;
    delay_calls = 0U;
    delay_total_ms = 0U;
    device_info_calls = 0U;
}

static void run_worker_once(void)
{
    CHECK(worker_task != NULL);
    worker_stop_armed = true;
    if (setjmp(worker_stop) == 0)
        worker_task(NULL);
    worker_stop_armed = false;
}

static void check_last_rgb(uint8_t green, uint8_t red, uint8_t blue)
{
    CHECK(rgb_write_calls > 0U);
    if (rgb_write_calls == 0U)
        return;
    CHECK(rgb_writes[rgb_write_calls - 1U].rgb_g == green);
    CHECK(rgb_writes[rgb_write_calls - 1U].rgb_r == red);
    CHECK(rgb_writes[rgb_write_calls - 1U].rgb_b == blue);
}

static void test_sudo_ble_notifications_are_noops(void)
{
    reset_observations();

    bc_ic_led_ble_connect();
    bc_ic_led_ble_connect_from_isr();
    bc_ic_led_ble_disconnect();
    bc_ic_led_ble_disconnect_from_isr();

    CHECK(event_set_calls == 0U);
    CHECK(isr_event_set_calls == 0U);
    CHECK(pending_bits == 0U);
    CHECK(delay_calls == 0U);
    CHECK(rgb_write_calls == 0U);
    CHECK(rgb_power_on_calls == 0U);
    CHECK(rgb_power_off_calls == 0U);
}

static void test_sudo_recording_indicator(void)
{
    reset_observations();
    bc_ic_led_mic_offline_recording_on();
    CHECK(event_set_calls == 1U);
    CHECK(pending_bits == (1U << 2));
    run_worker_once();
    check_last_rgb(5U, 0U, 0U);
    CHECK(device_info_calls == 0U);

    reset_observations();
    bc_ic_led_mic_offline_recording_off();
    CHECK(event_set_calls == 1U);
    CHECK(pending_bits == (1U << 6));
    run_worker_once();
    check_last_rgb(0U, 0U, 0U);

    /* Both event bits collapse while stop is the latest request. */
    reset_observations();
    bc_ic_led_mic_offline_recording_on();
    bc_ic_led_mic_offline_recording_off();
    CHECK(event_set_calls == 2U);
    CHECK(pending_bits == ((1U << 2) | (1U << 6)));
    run_worker_once();
    check_last_rgb(0U, 0U, 0U);
    CHECK(device_info_calls == 0U);

    /* Both event bits collapse while start is the latest request. */
    reset_observations();
    bc_ic_led_mic_offline_recording_off();
    bc_ic_led_mic_offline_recording_on();
    CHECK(event_set_calls == 2U);
    CHECK(pending_bits == ((1U << 2) | (1U << 6)));
    run_worker_once();
    check_last_rgb(5U, 0U, 0U);
    CHECK(device_info_calls == 0U);
}

static void test_legacy_color_is_serialized_and_cannot_override_recording(void)
{
    uint8_t blue[3] = {0, 0, 20};
    bc_ic_led_mic_offline_recording_off();
    reset_observations();
    bc_ic_led_set(blue);
    CHECK(rgb_write_calls == 0U && rgb_power_on_calls == 0U && delay_calls == 0U);
    CHECK(event_set_calls == 1U);
    run_worker_once();
    check_last_rgb(0U, 0U, 20U);

    reset_observations();
    bc_ic_led_set(blue);
    bc_ic_led_mic_offline_recording_on();
    bc_ic_led_stop();
    bc_ic_led_set(blue);
    /* Legacy stop/set is ignored after recording takes ownership. */
    CHECK(event_set_calls == 2U && rgb_write_calls == 0U);
    run_worker_once();
    check_last_rgb(5U, 0U, 0U);

    reset_observations();
    bc_ic_led_mic_online_recording_off();
    bc_ic_led_mic_offline_recording_on();
    run_worker_once();
    check_last_rgb(5U, 0U, 0U);

    reset_observations();
    bc_ic_led_mic_offline_recording_off();
    run_worker_once();
    check_last_rgb(0U, 0U, 0U); /* Never resurrect the queued idle blue. */

    reset_observations();
    bc_ic_led_set(blue);
    bc_ic_led_stop();
    run_worker_once();
    check_last_rgb(0U, 0U, 0U);
}

static void test_master_feedback_policy(void)
{
    uint8_t blue[3] = {0U, 0U, 20U};
    reset_observations();
    bc_ic_led_set(blue);
    bc_ic_led_feedback_enable(false);
    bc_ic_led_feedback_enable(true);
    CHECK(rgb_write_calls == 0U); /* All hardware writes stay on the owner. */
    run_worker_once();
    check_last_rgb(0U, 0U, 0U); /* Never replay the old blue after unmute. */

    reset_observations();
    bc_ic_led_feedback_enable(false);
    bc_ic_led_mic_offline_recording_on();
    bc_ic_led_set(blue);
    bc_ic_led_test_cmd(5U, 5U, 5U);
    bc_ic_led_ble_connect();
    bc_ic_led_mic_online_recording_on();
    CHECK(rgb_write_calls == 0U);
    run_worker_once();
    check_last_rgb(0U, 0U, 0U);
    /* The harness re-enters task initialization (which writes black). No
     * muted request may produce a nonzero RGB value, including earlier writes. */
    for (unsigned i = 0U; i < rgb_write_calls; ++i)
        CHECK(rgb_writes[i].rgb_g == 0U && rgb_writes[i].rgb_r == 0U && rgb_writes[i].rgb_b == 0U);

    reset_observations();
    bc_ic_led_feedback_enable(true);
    bc_ic_led_mic_offline_recording_on();
    run_worker_once();
    check_last_rgb(5U, 0U, 0U); /* A fresh recording can share the policy wake. */

    reset_observations();
    bc_ic_led_feedback_enable(false);
    run_worker_once();
    check_last_rgb(0U, 0U, 0U);
    CHECK(rgb_power_off_calls >= 1U);

    reset_observations();
    bc_ic_led_feedback_enable(true);
    run_worker_once();
    check_last_rgb(0U, 0U, 0U); /* Unmute does not revive the prior recording. */

    reset_observations();
    bc_ic_led_set(blue);
    run_worker_once();
    check_last_rgb(0U, 0U, 20U); /* A new command is accepted normally. */
    bc_ic_led_stop();
}

int main(void)
{
    bc_ic_led_init();
    CHECK(event_handle != NULL);
    CHECK(thread_create_calls == 1U);
    CHECK(worker_task != NULL);

    test_sudo_ble_notifications_are_noops();
    test_sudo_recording_indicator();
    test_legacy_color_is_serialized_and_cannot_override_recording();
    test_master_feedback_policy();

    if (failures != 0U)
    {
        fprintf(stderr, "FAIL: %u/%u checks failed\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks\n", checks);
    return 0;
}
