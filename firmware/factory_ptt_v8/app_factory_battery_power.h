/* P08: included after the supplier power module private declarations.
 * Checked acquisition and ownership adapted from the tested Sudo path.
 * The factory voltage curve, +200 mV correction and acquisition time remain unchanged. */
#include "app_factory_battery.h"
#include "bc_rtos.h"

static volatile bool vbat_adc_transaction_active;

/*
 * Motor load and recovery can transiently bias the divider reading around
 * LDO/PWM activity.  Use a wrap-safe tick window; this is a nonblocking
 * acquisition exclusion, and the existing PMIC protections remain unchanged.
 */
#define BC_POWER_MOTOR_SETTLING_MS    250U
#define BC_POWER_MOTOR_SETTLING_TICKS \
    ((uint32_t)(((uint64_t)BC_POWER_MOTOR_SETTLING_MS * \
                 (uint64_t)configTICK_RATE_HZ + 999U) / 1000U))

static bool bc_power_motor_adc_allowed(uint32_t *generation)
{
    bc_linear_motor_activity_t activity;
    uint32_t now;

    bc_linear_motor_activity_get(&activity);
    if(activity.active)
        return false;

    if(activity.has_finished)
    {
        now = (uint32_t)bc_rtos_task_get_tick_count();
        if((uint32_t)(now - activity.last_finished_tick) <
            BC_POWER_MOTOR_SETTLING_TICKS)
        {
            return false;
        }
    }

    if(generation != NULL)
        *generation = activity.generation;
    return true;
}

static bool bc_power_try_acquire_adc(void)
{
    bool acquired;

    bc_rtos_taskENTER_CRITICAL();
    acquired = !vbat_adc_transaction_active;
    if (acquired)
        vbat_adc_transaction_active = true;
    bc_rtos_taskEXIT_CRITICAL();
    return acquired;
}

static void bc_power_release_adc(void)
{
    bc_rtos_taskENTER_CRITICAL();
    vbat_adc_transaction_active = false;
    bc_rtos_taskEXIT_CRITICAL();
}

uint16_t bc_power_get_adc_value(void)
{
	uint32_t total = 0U;
	uint16_t adc_temp = 0U;
	uint16_t value = BC_POWER_ADC_ERROR;
	uint8_t i;
	bool powered = false;
	bool opened = false;
	int result;
	uint32_t motor_generation;
	bc_linear_motor_activity_t motor_activity;

	if(!bc_power_motor_adc_allowed(&motor_generation))
		return BC_POWER_ADC_ERROR;

	if(!bc_power_try_acquire_adc())
		return BC_POWER_ADC_ERROR;
	if(!bc_power_motor_adc_allowed(&motor_generation))
	{
		bc_power_release_adc();
		return BC_POWER_ADC_ERROR;
	}

	if(vbat_adc_device_handler != NULL)
	{
		bc_ldo_bat_power_on();
		powered = true;
		result = q_device_open(vbat_adc_device_handler);
		if(result == RESULT_OK)
		{
			opened = true;
			bc_delay_ms(10);
			for(i = 0U; i < 3U; ++i)
			{
				adc_temp = 0U;
				result = q_device_read(vbat_adc_device_handler, 0, &adc_temp, 1);
				if(result != RESULT_OK || adc_temp > 4095U)
					goto cleanup;
				total += adc_temp;
			}
			value = (uint16_t)(total / 3U);
		}
	}

cleanup:
	if(opened && q_device_close(vbat_adc_device_handler) != RESULT_OK)
		value = BC_POWER_ADC_ERROR;
	if(powered)
		bc_ldo_bat_power_off();
	if(value != BC_POWER_ADC_ERROR)
	{
		bc_linear_motor_activity_get(&motor_activity);
		if(motor_activity.active ||
			motor_activity.generation != motor_generation)
			value = BC_POWER_ADC_ERROR;
	}
	bc_power_release_adc();

	if(value != BC_POWER_ADC_ERROR)
		BC_LOG_INFO("bat temp:%d \r\n", value);
	return value;
}

bool bc_power_check_vbat(void)
{
	uint16_t temp = 0U;
	bool powered = false;
	bool opened = false;
	bool healthy = false;
	bool report_hardware_error = false;
	int result;

	if(!bc_power_try_acquire_adc())
		return false;

	if(vbat_adc_device_handler != NULL)
	{
		bc_ldo_bat_power_on();
		powered = true;
		result = q_device_open(vbat_adc_device_handler);
		if(result == RESULT_OK)
		{
			opened = true;
			bc_delay_ms(10);
			result = q_device_read(vbat_adc_device_handler, 0, &temp, 1);
			if(result == RESULT_OK && temp <= 4095U)
			{
				BC_LOG_INFO("vbat_adc:%d", temp);
				healthy = temp >= 100U;
				report_hardware_error = !healthy;
			}
		}
	}

	if(opened && q_device_close(vbat_adc_device_handler) != RESULT_OK)
		healthy = false;
	if(powered)
		bc_ldo_bat_power_off();
	bc_power_release_adc();

	if(report_hardware_error && !healthy && hardware_error_callback != NULL)
		hardware_error_callback();
	return healthy;
}
