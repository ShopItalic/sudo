/* Exercise the real finite-cue -> PWM BSP -> motor owner -> battery guard.
 * Only the Nordic hardware boundary, RTOS and ADC sample are mocked. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "app_linear_motor_handler.h"
#include "app_factory_battery.h"
#include "app_factory_controls.h"
#include "bc_linear_motor.h"
#include "bc_power.h"
#include "bc_pmic.h"
#include "bc_rtos.h"
#include "q_device.h"
#include "nrfx_pwm.h"

static q_device_t *pwm;
static q_device_t adc = {"vbat_adc", NULL};
static nrfx_pwm_handler_t pwm_handler;
static bool enabled = true, powered, initialized, in_irq;
static unsigned depth, checks, conversions, closes;
static uint32_t playback_flags;
static uint16_t playback_count_seen;
uint32_t fixture_ticks;
TaskHandle_t fixture_task;

#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "FAIL %u: %s\n", __LINE__, #x); return 1; } } while (0)

void fixture_enter(void) { assert(!in_irq); ++depth; }
void fixture_leave(void) { assert(depth); --depth; }
bool app_factory_controls_haptics(void) { return enabled; }
void bc_ldo_motor_power_on(void) { assert(depth && !in_irq); powered = true; }
void bc_ldo_motor_power_off(void) { assert(depth && !in_irq); powered = false; }
void bc_ldo_bat_power_on(void) { assert(!depth && !powered); }
void bc_ldo_bat_power_off(void) { assert(!depth && !powered); }
void bc_delay_ms(uint32_t ms) {
    assert(!depth && !in_irq && (ms == 10 || ms == 20));
    fixture_ticks += pdMS_TO_TICKS(ms);
}
void power_manage(void) {}
enum pmic_charge_status bc_pmic_get_charge_status(void) { return PMIC_CHARGED_NOT; }

int q_device_register(q_device_t *d) { assert(!strcmp(d->name, "pwm0")); pwm = d; return 1; }
q_device_t *q_device_find(const char *name) { return !strcmp(name, "pwm0") ? pwm : &adc; }
int q_device_open(q_device_t *d) { return d == &adc ? RESULT_OK : d->dops->open(d); }
int q_device_close(q_device_t *d) { return d == &adc ? RESULT_OK : d->dops->close(d); }
int q_device_ctrl(q_device_t *d, int cmd, void *arg) { return d->dops->control(d, cmd, arg); }
int q_device_cfg(q_device_t *d, void *arg, void *v) { return d->dops->config(d, arg, v); }
int q_device_reg_callback(q_device_t *d, int pos, void *cb) { return d->dops->register_callback(d, pos, cb); }
int q_device_read(q_device_t *d, int pos, const void *buffer, int size) {
    assert(d == &adc && !depth && !powered && pos == 0 && size == 1);
    ++conversions; *(uint16_t *)buffer = 1500; return RESULT_OK;
}

nrfx_err_t nrfx_pwm_init(nrfx_pwm_t const *i, nrfx_pwm_config_t const *c,
                         nrfx_pwm_handler_t handler) {
    (void)i; assert(depth && !in_irq && !initialized);
    assert(c->base_clock == NRF_PWM_CLK_1MHz && c->top_value == 10000);
    initialized = true; pwm_handler = handler; return NRFX_SUCCESS;
}
void nrfx_pwm_uninit(nrfx_pwm_t const *i) {
    (void)i; assert(depth && !in_irq && initialized); initialized = false; ++closes;
}
bool nrfx_pwm_stop(nrfx_pwm_t const *i, bool wait) {
    (void)i; assert(depth && !in_irq && initialized && !wait); return true;
}
uint32_t nrfx_pwm_simple_playback(nrfx_pwm_t const *i, nrf_pwm_sequence_t const *s,
                                 uint16_t count, uint32_t flags) {
    (void)i; assert(depth && initialized && powered && s->length == 3 && count);
    playback_flags = flags; playback_count_seen = count; return 0;
}
uint32_t nrfx_pwm_complex_playback(nrfx_pwm_t const *i, nrf_pwm_sequence_t const *a,
                                  nrf_pwm_sequence_t const *b, uint16_t count,
                                  uint32_t flags) {
    assert(b->length == 2 && b->values.p_common[0] == 10000);
    return nrfx_pwm_simple_playback(i, a, count, flags);
}

static void finish_playback(void) {
    assert(initialized && !depth);
    in_irq = true;
    pwm_handler(NRFX_PWM_EVT_FINISHED);
    /* Match the vendor NRFX playback shortcut: flags=0 emits FINISHED,
     * but only the STOP bit requests STOPPED. Never manufacture that event. */
    if (playback_flags & PWM_FLAG_STOP) pwm_handler(NRFX_PWM_EVT_STOPPED);
    in_irq = false;
}

int main(void) {
    bc_linear_motor_activity_t activity;
    bc_linear_motor_device_find(); bc_power_vbat_adc_find();
    CHECK(bc_power_get_adc_value() == 1500);
    for (unsigned mode = 0; mode < VIBRATE_MODE_MAX; ++mode) {
        for (unsigned count = 1; count <= 255; ++count) {
            CHECK(app_vibrate_start((vibrate_mode_t)mode, count) == 0);
            CHECK(playback_count_seen == count && powered);
            unsigned before = conversions, prior_closes = closes;
            CHECK(bc_power_get_adc_value() == BC_POWER_ADC_ERROR);
            CHECK(conversions == before); /* Active load still excluded. */
            finish_playback();
            CHECK(powered && closes == prior_closes); /* ISR never closes. */
            bc_linear_motor_service();
            bc_linear_motor_activity_get(&activity);
            if (playback_flags == 0) {
                fixture_ticks += 30U * 60U * configTICK_RATE_HZ;
                CHECK(bc_power_get_adc_value() == BC_POWER_ADC_ERROR);
                CHECK(bc_power_get_vbat_percen() == BC_POWER_PERCENT_UNKNOWN);
                CHECK(conversions == before);
                fprintf(stderr, "Historical flags=0: motor active, ADC=65535 and battery=255 even 30 minutes after playback\n");
            }
            CHECK(!activity.active && activity.has_finished && !powered);
            CHECK(playback_flags == PWM_FLAG_STOP);
            fixture_ticks = activity.last_finished_tick + 255;
            CHECK(bc_power_get_adc_value() == BC_POWER_ADC_ERROR);
            CHECK(conversions == before); /* Full 250 ms at 1024 Hz. */
            fixture_ticks = activity.last_finished_tick + 256;
            CHECK(bc_power_get_adc_value() == 1500);
            CHECK(conversions == before + 3);
        }
    }
    CHECK(bc_power_get_vbat_percen() < 100); /* No fabricated full reading. */

    CHECK(app_vibrate_start(VIBRATE_MODE_SHORT, 0) == 0);
    CHECK(playback_flags == PWM_FLAG_LOOP && playback_count_seen == 1);
    finish_playback(); bc_linear_motor_service();
    fixture_ticks += 100000;
    CHECK(powered && bc_power_get_adc_value() == BC_POWER_ADC_ERROR);
    app_vibrate_stop(); fixture_ticks += 256;
    CHECK(!powered && bc_power_get_adc_value() == 1500);

    enabled = false;
    CHECK(app_vibrate_start(VIBRATE_MODE_SHORT, 1) == 0);
    CHECK(!powered && !initialized && bc_power_get_adc_value() == 1500);
    CHECK(!depth);
    printf("PASS P08 motor/battery: %u checks; all 765 finite cues recover, active/settling/loop guards retained\n", checks);
    return 0;
}
