/* Included after the factory motor's private declarations. P05 keeps the
 * vendor PWM parameters and driver; only lifecycle ownership changes.
 * All helpers except the flag-only ISR run inside taskENTER_CRITICAL.
 * PWM0 is priority 7, masked by this target's FreeRTOS BASEPRI threshold 1.
 * The selected 1.23.2 LDO path is bounded GPIO access (no delay or I2C).
 * The registered idle callback only stores the factory motor-mode enum. */
static volatile bool factory_motor_stopped;
static uint32_t factory_motor_generation;
static bool factory_motor_live;

static void factory_motor_stop_locked(void)
{
    bool notify = factory_motor_live;
    ++factory_motor_generation;
    factory_motor_stopped = false;
    if (linear_motor_pwm_dev) {
        q_device_close(linear_motor_pwm_dev);
        bc_ldo_motor_power_off();
    }
    pwm_falsh.pwm_status = LINEAR_MOTOR_PWM_IDIE;
    factory_motor_live = false;
    /* Do not let a new start slip between power-off and its mode callback. */
    if (notify && linear_motor_pwm_idie_callback) linear_motor_pwm_idie_callback();
}

static bool factory_motor_begin_locked(uint32_t *generation)
{
    if (!app_factory_controls_haptics()) {
        factory_motor_stop_locked();
        return false;
    }
    /* Retire the old PWM before any settling delay; its completion must not
     * cancel a newer pending start. nrfx init clears the old STOPPED event. */
    if (linear_motor_pwm_dev) q_device_close(linear_motor_pwm_dev);
    factory_motor_stopped = false;
    *generation = ++factory_motor_generation;
    factory_motor_live = true;
    bc_ldo_motor_power_on();
    return true;
}

static bool factory_motor_current_locked(uint32_t generation)
{
    if (generation != factory_motor_generation) return false;
    if (!app_factory_controls_haptics()) {
        factory_motor_stop_locked();
        return false;
    }
    return true;
}

void bc_linear_motor_service(void)
{
    taskENTER_CRITICAL();
    if (factory_motor_stopped) {
        factory_motor_stopped = false;
        if (factory_motor_live && pwm_falsh.pwm_mode == PWM_STOP)
            factory_motor_stop_locked();
    }
    taskEXIT_CRITICAL();
}

void bc_linear_motor_silence(void)
{
    taskENTER_CRITICAL();
    factory_motor_stop_locked();
    taskEXIT_CRITICAL();
}
