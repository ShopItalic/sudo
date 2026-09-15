#include <assert.h>
#include <stdio.h>
#include "bc_rtos.h"
#include "q_device.h"
#include "bc_linear_motor.h"
#include "app_factory_controls.h"
static bool enabled = true, powered, initialized, irq_pending, in_irq;
static unsigned starts, closes, idle_calls, depth, checks, delay_action;
static void (*stopped)(void);
uint32_t fixture_ticks;
TaskHandle_t fixture_task;
static q_device_t device = {"pwm0",0};
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x); return 1; } } while(0)
void fixture_enter(void) { assert(!in_irq); ++depth; }
void fixture_leave(void) {
    assert(depth); --depth;
    if (!depth && irq_pending && initialized) {
        irq_pending = false; in_irq = true; stopped(); in_irq = false;
    }
}
bool app_factory_controls_haptics(void) { return enabled; }
void bc_ldo_motor_power_on(void) { assert(depth && !in_irq); powered = true; }
void bc_ldo_motor_power_off(void) { assert(depth && !in_irq); powered = false; }
void bc_delay_ms(uint32_t ms) {
    unsigned action = delay_action; delay_action = 0;
    assert(ms == 20 && !depth && !in_irq);
    if (action == 1) bc_linear_motor_stop();
    if (action == 2) { enabled = false; bc_linear_motor_silence(); enabled = true; }
    if (action == 3) bc_linear_motor_strong_vibration_start();
}
q_device_t *q_device_find(const char *name) { (void)name; return &device; }
int q_device_open(q_device_t *d) {
    (void)d; assert(depth && !initialized && !in_irq);
    initialized = true; irq_pending = false; return 0;
}
int q_device_close(q_device_t *d) {
    (void)d; assert(depth && !in_irq);
    if (initialized) {
        /* Simulate STOPPED becoming pending inside stop(false). The critical
         * section must prevent reentrant uninit; close disables its IRQ. */
        irq_pending = true;
        assert(depth);
        initialized = false; irq_pending = false; ++closes;
    }
    return 0;
}
int q_device_cfg(q_device_t *d, void *a, void *v) {
    (void)d; (void)a; (void)v; assert(depth && !in_irq); return 0;
}
int q_device_ctrl(q_device_t *d, int cmd, void *a) {
    (void)d; (void)a; assert(depth && initialized && !in_irq);
    if (cmd == PWM_CTRL_START) { assert(enabled && powered); ++starts; }
    return 0;
}
int q_device_reg_callback(q_device_t *d, int p, void *cb) {
    (void)d; (void)p; stopped = cb; return 0;
}
static void idle(void) { assert(depth && !in_irq); ++idle_calls; }
static void finish_irq(void) {
    assert(initialized && !depth); in_irq = true; stopped(); in_irq = false;
}
int main(void) {
    struct pwm_config config = {0}; uint16_t seq[3] = {3200,3200,10000};
    config.pwm_parameter_config.p_common = seq;
    config.pwm_parameter_config.length = 3;
    config.pwm_parameter_config.flags = PWM_FLAG_STOP;
    bc_linear_motor_device_find(); bc_linear_motor_pwm_idie_register_callback(idle);
    bc_linear_motor_start(LINEAR_MOTOR_MIC_START); CHECK(starts == 1 && powered);
    unsigned before_close = closes;
    finish_irq(); CHECK(closes == before_close && powered && idle_calls == 0);
    bc_linear_motor_service(); CHECK(!powered && !initialized && idle_calls == 1);
    bc_linear_motor_service(); CHECK(idle_calls == 1);
    bc_linear_motor_start(LINEAR_MOTOR_MIC_STOP); finish_irq();
    enabled = false; bc_linear_motor_silence();
    unsigned before_idle = idle_calls; bc_linear_motor_service();
    CHECK(idle_calls == before_idle && !powered);
    bc_linear_motor_start(LINEAR_MOTOR_MIC_START);
    bc_linear_motor_strong_vibration_start(); bc_linear_motor_continuous_vibration_start();
    bc_linear_motor_pwm_out(&config); CHECK(starts == 2 && !powered);
    enabled = true;
    delay_action = 1; bc_linear_motor_start(LINEAR_MOTOR_MIC_START); CHECK(starts == 2 && !powered);
    delay_action = 2; bc_linear_motor_start(LINEAR_MOTOR_MIC_START); CHECK(starts == 2 && !powered);
    delay_action = 3; bc_linear_motor_start(LINEAR_MOTOR_MIC_START); CHECK(starts == 3 && powered);
    finish_irq();
    bc_linear_motor_start(LINEAR_MOTOR_MIC_START); // retires old completion
    bc_linear_motor_service(); CHECK(starts == 4 && powered);
    delay_action = 1; bc_linear_motor_pwm_out(&config); CHECK(starts == 4 && !powered);
    delay_action = 3; bc_linear_motor_pwm_out(&config); CHECK(starts == 5 && powered);
    bc_linear_motor_pwm_out(&config); CHECK(starts == 6);
    config.pwm_parameter_config.length = 201;
    bc_linear_motor_pwm_out(&config); CHECK(starts == 6);
    bc_linear_motor_stop(); CHECK(!powered && !initialized && !depth);
    printf("PASS P05 motor: %u checks; IRQ deferral, close serialization, stale completion, stop/mute during settling, competing starts, bounded manual copy\n",checks);
    return 0;
}
