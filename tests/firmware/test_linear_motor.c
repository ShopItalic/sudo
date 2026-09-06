#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_linear_motor_handler.h"
#include "bc_device_info.h"
#include "bc_linear_motor.h"
#include "q_device.h"
#include "nrfx_pwm.h"

static nrfx_err_t mock_nrfx_init_result = NRF_SUCCESS;
static int mock_nrfx_init_calls;
static int mock_nrfx_uninit_calls;
static int mock_nrfx_stop_calls;
static int mock_nrfx_simple_calls;
static int mock_nrfx_complex_calls;
static uint16_t mock_nrfx_last_playback_count;
static uint32_t mock_nrfx_last_flags;
static nrf_pwm_sequence_t mock_nrfx_last_sequence0;

#include "../../firmware/bc_ros/bc_driver/bsp/src/bsp_pwm0.c"

static q_device_t pwm_device = {"pwm0", NULL};
static q_device_t *registered_device;
static void (*stopped_callback)(void);
static struct pwm_config last_config;
static uint16_t last_sequence[200];
static int open_calls;
static int close_calls;
static int config_calls;
static int control_start_calls;
static int control_stop_calls;
static int callback_register_calls;
static int power_on_calls;
static int power_off_calls;
static int delay_calls;
static uint32_t last_delay_ms;
static bool motor_powered;
static bool activity_seen_during_delay;
static bool activity_seen_at_start;
static bool io_seen_in_critical;
static bool fail_callback_register;
static bool callback_during_setup_delay;
static bool mute_during_delay, unmute_during_delay, mute_during_config;
static int callback_during_setup_delay_calls;

static enum
{
    FAIL_NONE,
    FAIL_STOP,
    FAIL_CLOSE,
    FAIL_CONFIG,
    FAIL_OPEN,
    FAIL_START,
} failure_stage;

static int failures_left;

uint32_t test_ticks;
unsigned test_critical_depth;

static void note_io(void)
{
    if(test_critical_depth != 0U)
        io_seen_in_critical = true;
}

/* The retained legacy timer table references this callback even when the
 * Sudo tests exercise only finite pulses. Keep the platform boundary linked
 * on ELF hosts, whose section collection differs from Mach-O dead stripping. */
bc_device_hid_info *bc_device_info_get_hid_info(void)
{
    static bc_device_hid_info info;
    return &info;
}

static bool should_fail(int stage)
{
    if((int)failure_stage == stage && failures_left > 0)
    {
        --failures_left;
        return true;
    }
    return false;
}

int q_device_register(q_device_t *dev)
{
    registered_device = dev;
    return 1;
}

q_device_t *q_device_find(const char *name)
{
    return (name != NULL && strcmp(name, "pwm0") == 0) ?
           (registered_device != NULL ? registered_device : &pwm_device) : NULL;
}

nrfx_err_t nrfx_pwm_init(nrfx_pwm_t const *instance,
                        nrfx_pwm_config_t const *config,
                        nrfx_pwm_handler_t handler)
{
    (void)instance;
    (void)config;
    (void)handler;
    ++mock_nrfx_init_calls;
    return mock_nrfx_init_result;
}

void nrfx_pwm_uninit(nrfx_pwm_t const *instance)
{
    (void)instance;
    ++mock_nrfx_uninit_calls;
}

uint32_t nrfx_pwm_simple_playback(nrfx_pwm_t const *instance,
                                  nrf_pwm_sequence_t const *sequence,
                                  uint16_t playback_count,
                                  uint32_t flags)
{
    (void)instance;
    ++mock_nrfx_simple_calls;
    mock_nrfx_last_sequence0 = *sequence;
    mock_nrfx_last_playback_count = playback_count;
    mock_nrfx_last_flags = flags;
    return 0;
}

uint32_t nrfx_pwm_complex_playback(nrfx_pwm_t const *instance,
                                   nrf_pwm_sequence_t const *sequence0,
                                   nrf_pwm_sequence_t const *sequence1,
                                   uint16_t playback_count,
                                   uint32_t flags)
{
    (void)instance;
    (void)sequence1;
    ++mock_nrfx_complex_calls;
    mock_nrfx_last_sequence0 = *sequence0;
    mock_nrfx_last_playback_count = playback_count;
    mock_nrfx_last_flags = flags;
    return 0;
}

bool nrfx_pwm_stop(nrfx_pwm_t const *instance, bool wait_until_stopped)
{
    (void)instance;
    (void)wait_until_stopped;
    ++mock_nrfx_stop_calls;
    return true;
}

int q_device_open(q_device_t *dev)
{
    note_io();
    if(dev == NULL)
        return RESULT_DEV_POINTER_NULL_ERROR;
    ++open_calls;
    return should_fail(FAIL_OPEN) ? RESULT_OPEN_POINTER_NULL_ERROR : RESULT_Q_DEVICE_OK;
}

int q_device_close(q_device_t *dev)
{
    note_io();
    if(dev == NULL)
        return RESULT_DEV_POINTER_NULL_ERROR;
    ++close_calls;
    return should_fail(FAIL_CLOSE) ? RESULT_CLOSE_POINTER_NULL_ERROR : RESULT_Q_DEVICE_OK;
}

int q_device_cfg(q_device_t *dev, void *args, void *var)
{
    note_io();
    struct pwm_config *config = (struct pwm_config *)args;
    (void)var;
    if(dev == NULL || config == NULL)
        return RESULT_CONFIG_NULL_ERR;
    ++config_calls;
    if(mute_during_config)
    {
        mute_during_config = false;
        bc_linear_motor_feedback_enable(false);
        bc_linear_motor_feedback_enable(true);
    }
    last_config = *config;
    if(config->pwm_parameter_config.p_common != NULL &&
       config->pwm_parameter_config.length <= 200)
    {
        memcpy(last_sequence, config->pwm_parameter_config.p_common,
               config->pwm_parameter_config.length * sizeof(last_sequence[0]));
        last_config.pwm_parameter_config.p_common = last_sequence;
    }
    return should_fail(FAIL_CONFIG) ? RESULT_CONFIG_POINTER_NULL_ERROR : RESULT_Q_DEVICE_OK;
}

int q_device_ctrl(q_device_t *dev, int cmd, void *arg)
{
    bc_linear_motor_activity_t activity;

    note_io();
    (void)arg;
    if(dev == NULL)
        return RESULT_DEV_POINTER_NULL_ERROR;
    if(cmd == PWM_CTRL_STOP)
    {
        ++control_stop_calls;
        return should_fail(FAIL_STOP) ? RESULT_CONTROL_POINTER_NULL_ERROR : RESULT_Q_DEVICE_OK;
    }
    if(cmd == PWM_CTRL_START)
    {
        ++control_start_calls;
        bc_linear_motor_activity_get(&activity);
        if(activity.active && motor_powered)
            activity_seen_at_start = true;
        return should_fail(FAIL_START) ? RESULT_CONTROL_POINTER_NULL_ERROR : RESULT_Q_DEVICE_OK;
    }
    return RESULT_CONTROL_POINTER_NULL_ERROR;
}

int q_device_reg_callback(q_device_t *dev, int pos, void *callback)
{
    if(dev == NULL)
        return RESULT_DEV_POINTER_NULL_ERROR;
    ++callback_register_calls;
    if(fail_callback_register)
        return RESULT_REG_CALLBACK_POINTER_NULL_ERROR;
    if(pos == PWM_REGISTER_STOPPED_CALLBACK)
        stopped_callback = (void (*)(void))callback;
    return RESULT_Q_DEVICE_OK;
}

void bc_ldo_motor_power_on(void)
{
    note_io();
    ++power_on_calls;
    motor_powered = true;
}

void bc_ldo_motor_power_off(void)
{
    note_io();
    ++power_off_calls;
    motor_powered = false;
}

void bc_delay_ms(uint32_t ms)
{
    bc_linear_motor_activity_t activity;

    note_io();
    ++delay_calls;
    last_delay_ms = ms;
    if(ms == 20U)
    {
        bc_linear_motor_activity_get(&activity);
        if(activity.active && motor_powered)
            activity_seen_during_delay = true;
    }
    if(mute_during_delay && ms == 20U)
    {
        mute_during_delay = false;
        bc_linear_motor_feedback_enable(false);
        if(unmute_during_delay) bc_linear_motor_feedback_enable(true);
    }
    if(callback_during_setup_delay && ms == 20 && stopped_callback != NULL)
    {
        callback_during_setup_delay = false;
        ++callback_during_setup_delay_calls;
        stopped_callback();
    }
}

static void reset_observations(void)
{
    memset(&last_config, 0, sizeof(last_config));
    memset(last_sequence, 0, sizeof(last_sequence));
    open_calls = 0;
    close_calls = 0;
    config_calls = 0;
    control_start_calls = 0;
    control_stop_calls = 0;
    power_on_calls = 0;
    power_off_calls = 0;
    delay_calls = 0;
    last_delay_ms = 0;
    mock_nrfx_init_result = NRF_SUCCESS;
    mock_nrfx_init_calls = 0;
    mock_nrfx_uninit_calls = 0;
    mock_nrfx_stop_calls = 0;
    mock_nrfx_simple_calls = 0;
    mock_nrfx_complex_calls = 0;
    mock_nrfx_last_playback_count = 0;
    mock_nrfx_last_flags = 0;
    memset(&mock_nrfx_last_sequence0, 0, sizeof(mock_nrfx_last_sequence0));
    motor_powered = false;
    activity_seen_during_delay = false;
    activity_seen_at_start = false;
    io_seen_in_critical = false;
    test_ticks = 0U;
    test_critical_depth = 0U;
    callback_during_setup_delay = false;
    callback_during_setup_delay_calls = 0;
    mute_during_delay = false; unmute_during_delay = false; mute_during_config = false;
    failure_stage = FAIL_NONE;
    failures_left = 0;
}

static int checks;
static int failures;

#define CHECK(condition, message) do { \
    ++checks; \
    if(!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL: %s (line %d)\n", (message), __LINE__); \
    } \
} while(0)

static void finish_pulse(void)
{
    CHECK(stopped_callback != NULL, "PWM stopped callback registered");
    if(stopped_callback != NULL)
        stopped_callback();
}

static void test_device_registration_failure(void)
{
    reset_observations();
    fail_callback_register = true;
    bc_linear_motor_device_find();
    fail_callback_register = false;
    CHECK(!bc_linear_motor_pulse(50, 20),
          "callback registration failure disables pulses");
    CHECK(power_on_calls == 0 && power_off_calls == 0,
          "callback registration failure does not touch motor power");
    bc_linear_motor_device_find();
}

static void test_sudo_bsp_error_propagation(void)
{
    static uint16_t sequence[] = {3200, 3200, 10000};
    struct pwm_config config = {0};

    config.pwm_aisle0_enable_status = true;
    config.pwm_parameter_config.top_value = 10000;
    config.pwm_parameter_config.p_common = sequence;
    config.pwm_parameter_config.length = 3;
    config.pwm_parameter_config.playback_count = 1;
    config.pwm_parameter_config.flags = PWM_FLAG_STOP;
    CHECK(bsp_pwm_config(&bsp_list.dev, &config, NULL) == RESULT_OK,
          "BSP accepts a valid PWM configuration");

    mock_nrfx_init_result = 77;
    CHECK(bsp_pwm_open(&bsp_list.dev) == 77,
          "SUDO BSP propagates PWM init failure");
    CHECK(mock_nrfx_init_calls == 1, "BSP invokes PWM init once on failure");

    mock_nrfx_init_result = NRF_SUCCESS;
    CHECK(bsp_pwm_open(&bsp_list.dev) == RESULT_OK,
          "BSP opens after PWM init succeeds");
    CHECK(bsp_pwm_ctrl(&bsp_list.dev, PWM_CTRL_START, NULL) == RESULT_OK,
          "BSP propagates a valid PWM start");
    CHECK(mock_nrfx_complex_calls == 1 && mock_nrfx_simple_calls == 0 &&
          mock_nrfx_last_playback_count == 1 &&
          mock_nrfx_last_flags == PWM_FLAG_STOP,
          "BSP uses complex STOP playback with the requested count");

    config.pwm_parameter_config.p_common = NULL;
    CHECK(bsp_pwm_config(&bsp_list.dev, &config, NULL) == RESULT_OK,
          "BSP stores malformed configuration for validation at start");
    CHECK(bsp_pwm_ctrl(&bsp_list.dev, PWM_CTRL_START, NULL) == RESULT_CONFIG_NULL_ERR,
          "BSP rejects malformed SUDO start before hardware playback");
    CHECK(mock_nrfx_complex_calls == 1,
          "malformed SUDO start does not call the PWM peripheral");
    CHECK(bsp_pwm_close(&bsp_list.dev) == RESULT_OK,
          "BSP closes after the error-path exercise");
}

static void test_invalid_arguments(void)
{
    int old_power_on;
    int old_power_off;
    int old_open;
    int old_config;
    int old_start;

    reset_observations();
    old_power_on = power_on_calls;
    old_power_off = power_off_calls;
    old_open = open_calls;
    old_config = config_calls;
    old_start = control_start_calls;

    CHECK(!bc_linear_motor_pulse(0, 20), "strength zero rejected");
    CHECK(!bc_linear_motor_pulse(101, 20), "strength above 100 rejected");
    CHECK(!bc_linear_motor_pulse(1, 0), "zero duration rejected");
    CHECK(!bc_linear_motor_pulse(1, 19), "duration below 20 rejected");
    CHECK(!bc_linear_motor_pulse(1, 21), "non-20ms duration rejected");
    CHECK(!bc_linear_motor_pulse(1, 401), "duration above 400 rejected");
    CHECK(!bc_linear_motor_pulse(1, 30), "non-multiple duration rejected");
    CHECK(power_on_calls == old_power_on && power_off_calls == old_power_off,
          "invalid arguments do not touch motor power");
    CHECK(open_calls == old_open && config_calls == old_config &&
          control_start_calls == old_start, "invalid arguments do not touch PWM");
}

static void test_minimum_pulse(void)
{
    bc_linear_motor_activity_t activity;
    uint32_t started_generation;

    reset_observations();
    test_ticks = 1234U;
    CHECK(bc_linear_motor_pulse(1, 20), "minimum pulse starts");
    bc_linear_motor_activity_get(&activity);
    started_generation = activity.generation;
    CHECK(activity.active && activity.active_since_tick == 1234U,
          "pulse publishes its start tick while active");
    CHECK(power_on_calls == 1 && motor_powered, "minimum pulse powers motor");
    CHECK(delay_calls == 1 && last_delay_ms == 20, "minimum pulse keeps power settle delay");
    CHECK(activity_seen_during_delay, "pulse publishes activity through motor settle");
    CHECK(close_calls == 1 && config_calls == 1 && open_calls == 1,
          "minimum pulse checks the PWM setup sequence");
    CHECK(control_start_calls == 1 && control_stop_calls == 0,
          "minimum pulse starts exactly once");
    CHECK(last_config.pwm_parameter_config.top_value == 10000,
          "minimum pulse top is 10000");
    CHECK(last_config.pwm_parameter_config.length == 3 &&
          last_config.pwm_parameter_config.playback_count == 1 &&
          last_config.pwm_parameter_config.repeats == 0 &&
          last_config.pwm_parameter_config.flags == PWM_FLAG_STOP,
          "minimum pulse uses one stop playback with zero repeats");
    CHECK(last_sequence[0] == 9932 && last_sequence[1] == 9932 &&
          last_sequence[2] == 10000, "minimum strength waveform is bounded");
    test_ticks = 1300U;
    finish_pulse();
    bc_linear_motor_activity_get(&activity);
    CHECK(!activity.active && activity.has_finished &&
          activity.last_finished_tick == 1300U &&
          activity.generation != started_generation,
          "completion publishes a finished tick and new generation");
    CHECK(power_off_calls == 1 && !motor_powered, "completion powers motor off");
    finish_pulse();
    CHECK(power_off_calls == 1, "duplicate completion does not power off twice");
}

static void test_maximum_pulse(void)
{
    reset_observations();
    CHECK(bc_linear_motor_pulse(100, 400), "maximum pulse starts");
    CHECK(last_sequence[0] == 3200 && last_sequence[1] == 3200 &&
          last_sequence[2] == 10000, "maximum strength waveform matches factory ceiling");
    CHECK(last_config.pwm_parameter_config.repeats == 19,
          "maximum duration uses nineteen repeats");
    CHECK(activity_seen_during_delay && activity_seen_at_start,
          "maximum pulse keeps activity through settle and PWM start");
    finish_pulse();
    CHECK(power_off_calls == 1, "maximum pulse completes once");
}

static void test_app_finite_after_loop(void)
{
    reset_observations();
    CHECK(app_vibrate_start(VIBRATE_MODE_SHORT, 0) == 0,
          "legacy loop preset starts");
    CHECK(last_config.pwm_parameter_config.flags == PWM_FLAG_LOOP && motor_powered,
          "count zero keeps legacy loop flag");
    CHECK(app_vibrate_start(VIBRATE_MODE_SHORT, 1) == 0,
          "finite preset starts after loop");
    CHECK(control_stop_calls >= 1, "finite start stops the prior loop");
    CHECK(last_config.pwm_parameter_config.flags == PWM_FLAG_STOP &&
          last_config.pwm_parameter_config.playback_count == 1,
          "finite preset uses STOP and one playback");
    CHECK(activity_seen_during_delay && activity_seen_at_start,
          "legacy app publishes activity through settle and PWM start");
    CHECK(power_off_calls == 1 && motor_powered,
          "replacing the loop powers it down before the finite preset");
    finish_pulse();
    CHECK(power_off_calls == 2 && !motor_powered,
          "finite preset after loop releases motor power");
}

static void test_replacing_active_pulse_ignores_late_callback(void)
{
    reset_observations();
    CHECK(bc_linear_motor_pulse(50, 100), "initial pulse starts for replacement");
    callback_during_setup_delay = true;
    CHECK(bc_linear_motor_pulse(25, 40), "replacement pulse starts");
    CHECK(callback_during_setup_delay_calls == 1,
          "old stopped callback arrives during replacement setup delay");
    CHECK(power_off_calls == 1 && motor_powered,
          "late old callback cannot power off the replacement pulse");
    CHECK(last_sequence[0] == 8300 && last_sequence[1] == 8300 &&
          last_sequence[2] == 10000,
          "replacement config is installed after old transfer is closed");
    finish_pulse();
    CHECK(power_off_calls == 2 && !motor_powered,
          "replacement completion powers off exactly once");
}

static void test_driver_failures(void)
{
    const int stages[] = {FAIL_CLOSE, FAIL_CONFIG, FAIL_OPEN, FAIL_START};
    const char *names[] = {"close", "config", "open", "start"};
    size_t i;

    for(i = 0; i < sizeof(stages) / sizeof(stages[0]); ++i)
    {
        reset_observations();
        failure_stage = stages[i];
        failures_left = 1;
        CHECK(!bc_linear_motor_pulse(50, 100), names[i]);
        CHECK(power_off_calls == 1 && !motor_powered, "failure powers motor off");
    }

    reset_observations();
    CHECK(app_vibrate_start(VIBRATE_MODE_SHORT, 0) == 0,
          "setup loop before stop failure");
    /* The finite helper must handle a live loop; force that stop command to fail. */
    failure_stage = FAIL_STOP;
    failures_left = 1;
    CHECK(!bc_linear_motor_pulse(50, 100), "stop failure rejected");
    CHECK(power_off_calls == 1 && !motor_powered, "stop failure powers motor off");

    bc_linear_motor_activity_t activity;
    bc_linear_motor_activity_get(&activity);
    CHECK(activity.active, "failed stop keeps motor activity conservatively active");
    failure_stage = FAIL_NONE;
    failures_left = 0;
    bc_linear_motor_stop();
    bc_linear_motor_activity_get(&activity);
    CHECK(!activity.active && activity.has_finished,
          "successful cleanup finishes the previously failed activity");
}

static void test_direct_entry_activity_paths(void)
{
    bc_linear_motor_activity_t activity;

    reset_observations();
    bc_linear_motor_start(LINEAR_MOTOR_MIC_START);
    CHECK(activity_seen_during_delay && activity_seen_at_start,
          "direct start publishes activity before LDO and PWM");
    bc_linear_motor_stop();
    bc_linear_motor_activity_get(&activity);
    CHECK(!activity.active, "direct start stop finishes activity");

    reset_observations();
    bc_linear_motor_strong_vibration_start();
    CHECK(activity_seen_at_start,
          "strong vibration publishes activity before PWM");
    bc_linear_motor_stop();

    reset_observations();
    bc_linear_motor_continuous_vibration_start();
    CHECK(activity_seen_at_start,
          "continuous vibration publishes activity before PWM");
    bc_linear_motor_stop();
}

static void test_callback_close_failure_is_conservative(void)
{
    bc_linear_motor_activity_t activity;

    reset_observations();
    CHECK(bc_linear_motor_pulse(50U, 100U),
          "pulse starts before callback close failure");
    failure_stage = FAIL_CLOSE;
    failures_left = 1;
    finish_pulse();
    bc_linear_motor_activity_get(&activity);
    CHECK(activity.active && !motor_powered,
          "callback close failure keeps activity active after power off");

    failure_stage = FAIL_NONE;
    failures_left = 0;
    bc_linear_motor_stop();
    bc_linear_motor_activity_get(&activity);
    CHECK(!activity.active && activity.has_finished,
          "later stop completes conservatively retained activity");
}

static void test_master_feedback_policy(void)
{
    reset_observations();
    CHECK(bc_linear_motor_pulse(50U, 100U), "start before mute");
    bc_linear_motor_feedback_enable(false);
    CHECK(!motor_powered && control_stop_calls > 0, "mute stops active pulse and power");
    reset_observations();
    CHECK(!bc_linear_motor_pulse(50U, 100U), "muted pulse cannot claim success");
    bc_linear_motor_start(LINEAR_MOTOR_MIC_START);
    bc_linear_motor_strong_vibration_start();
    bc_linear_motor_continuous_vibration_start();
    CHECK(control_start_calls == 0 && power_on_calls == 0, "all low-level starts muted");
    bc_linear_motor_feedback_enable(true);
    CHECK(control_start_calls == 0, "unmute never restarts old work");
    CHECK(bc_linear_motor_pulse(50U, 100U), "new pulse works after unmute");
    finish_pulse();

    reset_observations();
    mute_during_delay = true;
    CHECK(!bc_linear_motor_pulse(50U, 100U), "mute during settle aborts pulse");
    CHECK(control_start_calls == 0 && !motor_powered, "no delayed muted playback");
    bc_linear_motor_feedback_enable(true);
    reset_observations();
    mute_during_delay = true; unmute_during_delay = true;
    CHECK(!bc_linear_motor_pulse(50U, 100U), "mute/unmute invalidates old settling pulse");
    CHECK(control_start_calls == 0 && !motor_powered, "old pulse cannot replay after unmute");

    reset_observations();
    mute_during_config = true;
    CHECK(!bc_linear_motor_pulse(50U, 100U), "mute during driver setup rejects stale request");
    CHECK(control_start_calls == 0 && !motor_powered, "stale config cannot start PWM");
    CHECK(bc_linear_motor_pulse(50U, 100U), "fresh pulse after config cancellation works");
    finish_pulse();
}

int main(void)
{
    reset_observations();
    CHECK(!bc_linear_motor_pulse(50, 20),
          "pulse requires a discovered PWM device");
    CHECK(power_on_calls == 0 && power_off_calls == 0,
          "undiscovered device does not touch motor power");
    test_device_registration_failure();
    test_sudo_bsp_error_propagation();
    CHECK(callback_register_calls >= 2 && stopped_callback != NULL,
          "PWM stopped callback registered during device init");

    test_invalid_arguments();
    test_minimum_pulse();
    test_maximum_pulse();
    test_replacing_active_pulse_ignores_late_callback();
    test_app_finite_after_loop();
    test_driver_failures();
    test_callback_close_failure_is_conservative();
    test_direct_entry_activity_paths();
    test_master_feedback_policy();
    CHECK(test_critical_depth == 0U && !io_seen_in_critical,
          "activity publication critical sections contain no I/O");

    if(failures != 0)
    {
        fprintf(stderr, "%d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("PASS: %d checks\n", checks);
    return 0;
}
