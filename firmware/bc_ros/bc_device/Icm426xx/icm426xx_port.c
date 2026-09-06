#include "icm426xx_port.h"

#include "Icm426xxDriver_HL.h"
#include "Icm426xxExtFunc.h"

#include "bc_g_sensor_device_port.h"
#include "bc_logger.h"
#include "bc_delay.h"
#include "ring_config.h"
#if ( HARDWARE_1121_ENABLED == 1 ||  HARDWARE_158_ENABLED == 1 ||  HARDWARE_156_ENABLED == 1 ||  HARDWARE_1141_ENABLED == 1)	

#define icm42688_ADD 0x68 << 1


#else

#define icm42688_ADD 0x68

#endif	

#define ICM426XX_LOG_INFO 1

#if (ICM426XX_LOG_INFO == 1)
#define ICM426xx_LOG  BC_LOG_INFO
#else

#define ICM426xx_LOG(...)

#endif



/*
 * ICM mounting matrix
 * Coefficients are coded as Q30 integer
 */
#if defined(ICM_FAMILY_CPLUS)
static int32_t icm_mounting_matrix[9] = { 0, -(1 << 30), 0, (1 << 30), 0, 0, 0, 0, (1 << 30) };
#else
static int32_t icm_mounting_matrix[9] = { (1 << 30), 0, 0, 0, (1 << 30), 0, 0, 0, (1 << 30) };
#endif

static struct inv_icm426xx g_icm_dev;

static int icm426xx_read(struct inv_icm426xx_serif *serif, uint8_t reg, uint8_t *buf, uint32_t len)
{
    (void)serif;
    if (!bc_g_sensor_i2c_read(icm42688_ADD, reg, buf, (uint8_t)len))
    {
        return -1;
    }
    return 0;
}

static int icm426xx_write(struct inv_icm426xx_serif *serif, uint8_t reg, const uint8_t *buf, uint32_t len)
{
    (void)serif;
    if (!bc_g_sensor_i2c_write(icm42688_ADD, reg, (uint8_t *)buf, (uint8_t)len))
    {
        return -1;
    }
    return 0;
}

static int icm426xx_configure(struct inv_icm426xx_serif *serif)
{
    (void)serif;
    return 0;
}

static void apply_mounting_matrix(const int32_t matrix[9], int16_t raw[3])
{
	unsigned i;
	int64_t  data_q30[3];

	for (i = 0; i < 3; i++) {
		data_q30[i] = ((int64_t)matrix[3 * i + 0] * raw[0]);
		data_q30[i] += ((int64_t)matrix[3 * i + 1] * raw[1]);
		data_q30[i] += ((int64_t)matrix[3 * i + 2] * raw[2]);
	}
	raw[0] = (int16_t)(data_q30[0] >> 30);
	raw[1] = (int16_t)(data_q30[1] >> 30);
	raw[2] = (int16_t)(data_q30[2] >> 30);
}


void HandleInvDeviceDataRegisters(inv_icm426xx_sensor_event_t *event)
{
	uint64_t irq_timestamp = 0;

	/*
	 * Extract the timestamp that was buffered when current packet IRQ fired. See 
	 * ext_interrupt_cb() in main.c for more details.
	 * As timestamp buffer is filled in interrupt handler, we should pop it with
	 * interrupts disabled to avoid any concurrent access.
	 */
//	inv_disable_irq();
//	if (!RINGBUFFER_EMPTY(&timestamp_buffer))
//		RINGBUFFER_POP(&timestamp_buffer, &irq_timestamp);
//	inv_enable_irq();

	apply_mounting_matrix(icm_mounting_matrix, event->accel);
	apply_mounting_matrix(icm_mounting_matrix, event->gyro);

	/*
	 * Output data on UART link
	 */
	if ((event->accel[0] != INVALID_VALUE_FIFO) && (event->gyro[0] != INVALID_VALUE_FIFO))
  {
		ICM426xx_LOG("%u: %d, %d, %d, %d, %d, %d, %d", (uint32_t)irq_timestamp,event->accel[0], event->accel[1], event->accel[2], event->temperature, event->gyro[0], event->gyro[1], event->gyro[2]);
  }
	else if (event->gyro[0] != INVALID_VALUE_FIFO)
  {
		ICM426xx_LOG( "%u: NA, NA, NA, %d, %d, %d, %d", (uint32_t)irq_timestamp,
		        event->temperature, event->gyro[0], event->gyro[1], event->gyro[2]);
  }
	else if (event->accel[0] != INVALID_VALUE_FIFO)
  {
		ICM426xx_LOG("%u: %d, %d, %d, %d, NA, NA, NA", (uint32_t)irq_timestamp,
		        event->accel[0], event->accel[1], event->accel[2], event->temperature);
  }
}

static int icm42688_driver_init_internal(void)
{


    struct inv_icm426xx_serif serif = {
        .context   = NULL,
        .read_reg  = icm426xx_read,
        .write_reg = icm426xx_write,
        .configure = icm426xx_configure,
        .max_read  = 1024 * 32,
        .max_write = 1024 * 32,
        .serif_type = ICM426XX_UI_I2C,
    };

    int status = inv_icm426xx_init(&g_icm_dev, &serif, HandleInvDeviceDataRegisters);
    if (status == 0)
    {
    }
    return status;
}
static int icm42688_configure_fifo_runtime(void)
{
    int status = 0;
    uint8_t fifo_cfg1 = 0;
        status |= inv_icm426xx_configure_fifo(&g_icm_dev, INV_ICM426XX_FIFO_ENABLED);
        status |= inv_icm426xx_configure_fifo_wm(&g_icm_dev, 1);
        status |= inv_icm426xx_reset_fifo(&g_icm_dev);

        status |= inv_icm426xx_read_reg(&g_icm_dev, MPUREG_FIFO_CONFIG1, 1, &fifo_cfg1);
        fifo_cfg1 |= (uint8_t)(BIT_FIFO_CONFIG1_ACCEL_MASK | BIT_FIFO_CONFIG1_GYRO_MASK |
                               BIT_FIFO_CONFIG1_TEMP_MASK | BIT_FIFO_CONFIG1_TMST_FSYNC_MASK);
        status |= inv_icm426xx_write_reg(&g_icm_dev, MPUREG_FIFO_CONFIG1, 1, &fifo_cfg1);
    return status;
}



ret_code_t icm42688_driver_init(void)
{
    /* init ICM42688 via HL driver, enable ACC/GYRO + FIFO (4g/1000dps, 25Hz) */
    int status = 0;

    /* 1) HL driver init (includes device reset and basic config) */
    status = icm42688_driver_init_internal();
    if (status != 0)
    {
        BC_LOG_INFO("icm42688_driver_init_internal fail:%d", status);
        return NRF_ERROR_INTERNAL;
    }

    /* 2) Configure range and ODR close to legacy settings */
    status |= inv_icm426xx_set_accel_fsr(&g_icm_dev, ICM426XX_ACCEL_CONFIG0_FS_SEL_4g);
    status |= inv_icm426xx_set_gyro_fsr(&g_icm_dev, ICM426XX_GYRO_CONFIG0_FS_SEL_1000dps);
//    bsp_Icm42688GetGres(GFS_1000DPS);
//    bsp_Icm42688GetAres(AFS_4G);
    


    
//    switch(odr_config)
//		 {
//			 case ODR_25:
//			 {
          status |= inv_icm426xx_set_accel_frequency(&g_icm_dev, ICM426XX_ACCEL_CONFIG0_ODR_25_HZ);
          status |= inv_icm426xx_set_gyro_frequency(&g_icm_dev, ICM426XX_GYRO_CONFIG0_ODR_25_HZ);
//				 break;
//			 }
//			 case ODR_50:
//			 {
//          status |= inv_icm426xx_set_accel_frequency(&g_icm_dev, ICM426XX_ACCEL_CONFIG0_ODR_50_HZ);
//          status |= inv_icm426xx_set_gyro_frequency(&g_icm_dev, ICM426XX_GYRO_CONFIG0_ODR_50_HZ);
//				 break;
//			 }
//			 case ODR_100:
//			 {

//          status |= inv_icm426xx_set_accel_frequency(&g_icm_dev, ICM426XX_ACCEL_CONFIG0_ODR_100_HZ);
//          status |= inv_icm426xx_set_gyro_frequency(&g_icm_dev, ICM426XX_GYRO_CONFIG0_ODR_100_HZ);
//				 break;
//			 }
//			 case ODR_150:
//			 {
//          status |= inv_icm426xx_set_accel_frequency(&g_icm_dev, ICM426XX_ACCEL_CONFIG0_ODR_200_HZ);
//          status |= inv_icm426xx_set_gyro_frequency(&g_icm_dev, ICM426XX_GYRO_CONFIG0_ODR_200_HZ);
//				 break;
//			 }
//			 case ODR_200:
//			 {
//          status |= inv_icm426xx_set_accel_frequency(&g_icm_dev, ICM426XX_ACCEL_CONFIG0_ODR_200_HZ);
//          status |= inv_icm426xx_set_gyro_frequency(&g_icm_dev, ICM426XX_GYRO_CONFIG0_ODR_200_HZ);
//				 break;
//			 }
//			 default :
//			 {
//				 break;
//			 }
//		 }		 
    /* 3) Enable low-noise mode for accel and gyro */
        status |= inv_icm426xx_enable_accel_low_noise_mode(&g_icm_dev);
        status |= inv_icm426xx_enable_gyro_low_noise_mode(&g_icm_dev);

    /* 4) Enable FIFO and route ACC/GYRO/TEMP/timestamp into FIFO */
    status |= icm42688_configure_fifo_runtime();
    if (status != 0)
    {
        BC_LOG_INFO("icm42688_driver_init fifo cfg fail:%d", status);
        return NRF_ERROR_INTERNAL;
    }

    /* 5) Enable FIFO threshold interrupt on INT1 and print config bits */
    {
        inv_icm426xx_interrupt_parameter_t int1_cfg;
        int rc = inv_icm426xx_get_config_int1(&g_icm_dev, &int1_cfg);
        if (rc == 0)
        {
            int1_cfg.INV_ICM426XX_FIFO_THS = INV_ICM426XX_ENABLE;
            (void)inv_icm426xx_set_config_int1(&g_icm_dev, &int1_cfg);

            /* read back and log to verify */
            rc = inv_icm426xx_get_config_int1(&g_icm_dev, &int1_cfg);
            BC_LOG_INFO(
                "icm42688 int1 cfg: drdy:%d fifo_ths:%d fifo_full:%d",
                int1_cfg.INV_ICM426XX_UI_DRDY,
                int1_cfg.INV_ICM426XX_FIFO_THS,
                int1_cfg.INV_ICM426XX_FIFO_FULL
            );
        }
        else
        {
            BC_LOG_INFO("icm42688 get int1 cfg fail:%d", rc);
        }
    }

    BC_LOG_INFO("icm42688_driver_init ok");
    return NRF_SUCCESS;
}

void inv_icm426xx_sleep_us(uint32_t us)
{
    bc_delay_us(us);
}

uint64_t inv_icm426xx_get_time_us(void)
{
    // 32768Hz£¨√øtick‘º 30us
    return (uint64_t)bc_systick_get() * (1000000ULL / 32768ULL);
}








