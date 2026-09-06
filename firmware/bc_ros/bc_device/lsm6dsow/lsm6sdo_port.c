#include "lsm6sdo_port.h"


#include "lsm6dso.h"
#include "q_device.h"

#if !defined(HANDWARE_1_23_3)
#include "bc_gsensor.h"
#include "bc_g_sensor_device_port.h"
#endif
#include "bc_delay.h"
#include "bc_logger.h"

static LSM6DSO_Object_t LSM6DSO_Object = {0};

static LSM6DSO_IO_t LSM6DSO_IO = {0};


#if (!defined(HANDWARE_1_23_3) && !defined(HANDWARE_1_23_4))
static int32_t lsm6sdo_io_init(void)
{
    return LSM6DSO_OK;
}


static int32_t lsm6sdo_io_deinit(void)
{
    return LSM6DSO_OK;
}

static int32_t lsm6sdo_io_write(uint16_t slave_addr,uint16_t reg_addr,uint8_t *write_data,uint16_t write_length)
{
    if(bc_g_sensor_i2c_write(slave_addr ,(uint8_t)reg_addr,write_data,write_length))
    {
        return LSM6DSO_ERROR;
    }
    return LSM6DSO_OK;
}

static int32_t lsm6sdo_io_read(uint16_t slave_addr,uint16_t reg_addr,uint8_t *read_data,uint16_t read_length)
{
    if(bc_g_sensor_i2c_read(slave_addr ,(uint8_t)reg_addr,read_data, read_length))
    {
        return LSM6DSO_ERROR;
    }
    return LSM6DSO_OK;
}

static void lsm6sdo_io_delay(uint32_t ms)
{
    bc_delay_ms(ms);
}

static int32_t lsm6sdo_io_tick_get(void)
{

}


uint8_t lsm6sdo_init(void)
{
    int res = 1;
    
    LSM6DSO_IO.Init = lsm6sdo_io_init;
    LSM6DSO_IO.DeInit = lsm6sdo_io_deinit;
    LSM6DSO_IO.Delay = lsm6sdo_io_delay;
    LSM6DSO_IO.GetTick = lsm6sdo_io_tick_get;
    LSM6DSO_IO.ReadReg = lsm6sdo_io_read;
    LSM6DSO_IO.WriteReg = lsm6sdo_io_write;
    LSM6DSO_IO.BusType = LSM6DSO_I2C_BUS;
    LSM6DSO_IO.Address = LSM6DSO_I2C_ADD_L ;
    
    LSM6DSO_RegisterBusIO(&LSM6DSO_Object, &LSM6DSO_IO);
    res = LSM6DSO_Init(&LSM6DSO_Object);
    LSM6DSO_ACC_Enable(&LSM6DSO_Object);
    
//    LSM6DSO_GYRO_Enable(&LSM6DSO_Object);
    BC_LOG_INFO("lsm6sdo_init res = %d \r\n",res);
    
//    LSM6DSO_ACC_SetFullScale(&LSM6DSO_Object, 0x02);
    LSM6DSO_ACC_Enable_Wake_Up_Detection(&LSM6DSO_Object, LSM6DSO_INT1_PIN);
//    LSM6DSO_ACC_Set_Wake_Up_Threshold(&LSM6DSO_Object, 0x10);
    
//    LSM6DSO_FIFO_Set_Watermark_Level(&LSM6DSO_Object,20);
    
    
    LSM6DSO_ACC_SetOutputDataRate_With_Mode(&LSM6DSO_Object,26.0, LSM6DSO_ACC_LOW_POWER_NORMAL_MODE);
//    LSM6DSO_GYRO_SetOutputDataRate_With_Mode(&LSM6DSO_Object, LSM6DSO_GY_ODR_26Hz,LSM6DSO_GYRO_LOW_POWER_NORMAL_MODE);
;
//    LSM6DSO_ACC_Enable_Pedometer(&LSM6DSO_Object);
//    LSM6DSO_ACC_Enable_DRDY_On_INT1(&LSM6DSO_Object);
//    LSM6DSO_ACC_Set_Power_Mode(&LSM6DSO_Object, LSM6DSO_LOW_NORMAL_POWER_MD);
//    LSM6DSO_ACC_Enable(&LSM6DSO_Object);
    
    LSM6DSO_FIFO_ACC_Set_BDR(&LSM6DSO_Object,26.0);
}


void lsm6sdo_on_and_off(bool status)
{
  if(status){
    LSM6DSO_ACC_Enable(&LSM6DSO_Object);
  }
  else{
    LSM6DSO_ACC_Disable(&LSM6DSO_Object);
  }
}

uint8_t lsm6sdo_get_chip_id(void)
{
    uint8_t chip_id = 0;
    LSM6DSO_ReadID(&LSM6DSO_Object, &chip_id);
    BC_LOG_INFO("lsm6dso id : 0x%x llllllllllllll\r\n",chip_id);
    return chip_id;
}

void lsm6sdo_get_steps(uint16_t *steps)
{
    LSM6DSO_ACC_Get_Step_Count(&LSM6DSO_Object,steps);
    
//    uint16_t test1 = 0;
//    uint8_t test2 = 0;
//    uint8_t test3 = 0;
//    lsm6dso_pedo_steps_period_get(&LSM6DSO_Object.Ctx,&test1);
//    lsm6dso_pedo_debounce_steps_get(&LSM6DSO_Object.Ctx,&test2);
//    lsm6dso_timestamp_get(&LSM6DSO_Object.Ctx,&test3);
//    BC_LOG_DEBUG("--------- %d %d %d \r\n",test1,test2,test3);
}

void lsm6sdo_clear_steps(void)
{
    LSM6DSO_ACC_Step_Counter_Reset(&LSM6DSO_Object);
}

void lsm6sdo_enable_anymotion(void)
{
    LSM6DSO_ACC_Enable_Wake_Up_Detection(&LSM6DSO_Object, LSM6DSO_INT1_PIN);
}

void lsm6sdo_disable_anymotion(void)
{
    LSM6DSO_ACC_Disable_Wake_Up_Detection(&LSM6DSO_Object);
}

void lsm6sdo_get_accData(LSM6DSO_Axes_t *data)
{
    LSM6DSO_ACC_GetAxes(&LSM6DSO_Object,data);
}

//¸Äacc²ÉÑùÂÊ
void lsm6sdo_change_acc_odr(uint8_t odr)
{
    uint8_t reg_val = 0;
    if(odr == 25)
    {
        LSM6DSO_ACC_SetOutputDataRate_With_Mode(&LSM6DSO_Object,26, LSM6DSO_ACC_LOW_POWER_NORMAL_MODE);
    }
    else if(odr == 50)
    {
        LSM6DSO_ACC_SetOutputDataRate_With_Mode(&LSM6DSO_Object,52, LSM6DSO_ACC_LOW_POWER_NORMAL_MODE);
    }
    else
    {
        LSM6DSO_ACC_SetOutputDataRate_With_Mode(&LSM6DSO_Object,12.5f, LSM6DSO_ACC_LOW_POWER_NORMAL_MODE);
    }
}

//ÇÐ»»ÔË¶¯Ä£Ê½
void lsm6sdo_sport_state(uint8_t odr)
{
    lsm6sdo_change_acc_odr(odr);
    LSM6DSO_FIFO_ACC_Set_BDR(&LSM6DSO_Object,26.0);
}

//ÔÚ°²¾²Ä£Ê½ÏÂµÈ´ýanymotionÖÐ¶Ï
void lsm6sdo_silent_state(void)
{
    LSM6DSO_FIFO_ACC_Set_BDR(&LSM6DSO_Object,0);
}

//¶Áfifo
void lsm6sdo_get_data_from_fifo(LSM6DSO_AxesRaw_t *acc_data,LSM6DSO_AxesRaw_t *gyr_data,uint8_t *data_num)
{
    uint16_t num = 0;
    uint8_t wmflag = 0;
    uint8_t reg_tag;
    LSM6DSO_Axes_t acc_data_temp;
    
    /* Read number of samples in FIFO */
    LSM6DSO_FIFO_Get_Num_Samples(&LSM6DSO_Object,&num);
    BC_LOG_INFO("lsm6dso fifo num : %d \r\n",num);
    if (num > 0)
    {
        *data_num = num;
        for(uint16_t i = 0;i < num;i++)
        {
            /* Read FIFO tag */
            LSM6DSO_FIFO_Get_Tag(&LSM6DSO_Object, &reg_tag);
            switch (reg_tag)
            {
                case LSM6DSO_XL_NC_TAG:
                {
                    LSM6DSO_FIFO_ACC_Get_Axes(&LSM6DSO_Object,&acc_data_temp);
                    acc_data[i].x = acc_data_temp.x;
                    acc_data[i].y = acc_data_temp.y;
                    acc_data[i].z = acc_data_temp.z;
                }
                break;
                
                case LSM6DSO_GYRO_NC_TAG:
                    break;
                
                default:
                    break;
            }
        }
    }
}

#else
/* SPI Éè±¸¾ä±ú */
static q_device_t *spi_dev = NULL;

/* SPI 数据包缓冲区 */
static uint8_t spi_tx_buf[32];
static uint8_t spi_rx_buf[32];
static struct spi_package spi_pack = {0};

/* ============ SPI 底层操作 ============ */

static void lsm6sdo_spi_cs_low(void)
{
    if (spi_dev == NULL) {
        return;
    }
    q_device_ctrl(spi_dev, GPIO_OUTPUT_LOW, 0);
}

static void lsm6sdo_spi_cs_high(void)
{
    if (spi_dev == NULL) {
        return;
    }
    q_device_ctrl(spi_dev, GPIO_OUTPUT_HIGH, 0);
}

/*
 * SPI 写寄存器
 *   reg_addr: 寄存器地址（7-bit，最高位由函数内部处理）
 *   write_data: 待写数据
 *   write_length: 数据长度（>1 时使用地址自增）
 */
static int32_t lsm6sdo_spi_write_reg(uint8_t reg_addr, uint8_t *write_data, uint16_t write_length)
{
    uint16_t total_len = write_length + 1;  /* 地址 + 数据 */

    if (spi_dev == NULL) {
        return LSM6DSO_ERROR;
    }
    if (write_data == NULL || write_length == 0) {
        return LSM6DSO_ERROR;
    }
    if (total_len > sizeof(spi_tx_buf)) {
        return LSM6DSO_ERROR;
    }

    /* 组装发送数据: 写命令（bit7=0）+ 数据 */
    spi_tx_buf[0] = reg_addr & 0x7F;
    memcpy(&spi_tx_buf[1], write_data, write_length);

    spi_pack.write_buff   = spi_tx_buf;
    spi_pack.write_length = total_len;
    spi_pack.read_buff    = spi_rx_buf;
    spi_pack.read_length  = total_len;

    lsm6sdo_spi_cs_low();
    if (q_device_write(spi_dev, 0, &spi_pack, 0) != RESULT_OK) {
        lsm6sdo_spi_cs_high();
        return LSM6DSO_ERROR;
    }
    lsm6sdo_spi_cs_high();

    return LSM6DSO_OK;
}

/*
 * SPI 读寄存器
 *   reg_addr: 寄存器地址（7-bit，最高位由函数内部处理）
 *   read_data: 读数据缓冲区
 *   read_length: 数据长度（>1 时使用地址自增）
 */
static int32_t lsm6sdo_spi_read_reg(uint8_t reg_addr, uint8_t *read_data, uint16_t read_length)
{
    uint16_t total_len = read_length + 1;  /* 地址 + 数据 */

    if (spi_dev == NULL) {
        return LSM6DSO_ERROR;
    }
    if (read_data == NULL || read_length == 0) {
        return LSM6DSO_ERROR;
    }
    if (total_len > sizeof(spi_tx_buf)) {
        return LSM6DSO_ERROR;
    }

    /* 组装发送数据: 读命令（bit7=1）+ 空时钟 */
    spi_tx_buf[0] = reg_addr | 0x80;
    memset(&spi_tx_buf[1], 0, read_length);

    spi_pack.write_buff   = spi_tx_buf;
    spi_pack.write_length = total_len;
    spi_pack.read_buff    = spi_rx_buf;
    spi_pack.read_length  = total_len;

    lsm6sdo_spi_cs_low();
    if (q_device_write(spi_dev, 0, &spi_pack, 0) != RESULT_OK) {
        lsm6sdo_spi_cs_high();
        return LSM6DSO_ERROR;
    }
    lsm6sdo_spi_cs_high();

    /* 第 1 字节是地址字节的 MISO 回读（无效），有效数据从第 2 字节开始 */
    memcpy(read_data, &spi_rx_buf[1], read_length);

    return LSM6DSO_OK;
}

/* ============ IO 接口函数（供 ST 驱动调用） ============ */

static int32_t lsm6sdo_io_init(void)
{
    return LSM6DSO_OK;
}

static int32_t lsm6sdo_io_deinit(void)
{
    return LSM6DSO_OK;
}

/*
 * 写寄存器接口（适配 ST 驱动的 WriteReg 函数指针）
 *   slave_addr: SPI 模式下未使用（I2C 兼容）
 *   reg_addr: 寄存器地址
 *   write_data: 数据指针
 *   write_length: 数据长度
 */
static int32_t lsm6sdo_io_write(uint16_t slave_addr, uint16_t reg_addr,
                                uint8_t *write_data, uint16_t write_length)
{
    (void)slave_addr;  /* SPI 模式无需从机地址 */

    if (lsm6sdo_spi_write_reg((uint8_t)reg_addr, write_data, write_length) != LSM6DSO_OK) {
        return LSM6DSO_ERROR;
    }
    return LSM6DSO_OK;
}

/*
 * 读寄存器接口（适配 ST 驱动的 ReadReg 函数指针）
 *   slave_addr: SPI 模式下未使用（I2C 兼容）
 *   reg_addr: 寄存器地址
 *   read_data: 数据缓冲区
 *   read_length: 数据长度
 */
static int32_t lsm6sdo_io_read(uint16_t slave_addr, uint16_t reg_addr,
                               uint8_t *read_data, uint16_t read_length)
{
    (void)slave_addr;  /* SPI 模式无需从机地址 */

    if (lsm6sdo_spi_read_reg((uint8_t)reg_addr, read_data, read_length) != LSM6DSO_OK) {
        return LSM6DSO_ERROR;
    }
    return LSM6DSO_OK;
}

static void lsm6sdo_io_delay(uint32_t ms)
{
    bc_delay_ms(ms);
}

static int32_t lsm6sdo_io_tick_get(void)
{
    return 0;
}

/* ============ SPI 设备查找与打开/关闭 ============ */

/*
 * 查找 SPI 设备（在 bc_module_init 阶段调用）
 */
void lsm6sdo_spi_device_find(void)
{
    spi_dev = q_device_find("spi_3");
    q_device_assert(spi_dev);
    BC_LOG_INFO("lsm6sdo spi_3 device found\r\n");
}

/*
 * 打开 SPI 总线并上电
 */
void lsm6sdo_spi_open(void)
{
    if (spi_dev == NULL) {
        return;
    }
    q_device_open(spi_dev);
    bc_delay_ms(2);  /* 等待 SPI 总线稳定 */
}

/*
 * 关闭 SPI 总线
 */
void lsm6sdo_spi_close(void)
{
    if (spi_dev == NULL) {
        return;
    }
    q_device_close(spi_dev);
}

/* ============ 传感器初始化 ============ */

uint8_t lsm6sdo_init(void)
{
    int res = 1;

    /* 填充 IO 接口表（SPI 4 线模式） */
    LSM6DSO_IO.Init    = lsm6sdo_io_init;
    LSM6DSO_IO.DeInit  = lsm6sdo_io_deinit;
    LSM6DSO_IO.Delay   = lsm6sdo_io_delay;
    LSM6DSO_IO.GetTick = lsm6sdo_io_tick_get;
    LSM6DSO_IO.ReadReg  = lsm6sdo_io_read;
    LSM6DSO_IO.WriteReg = lsm6sdo_io_write;
    LSM6DSO_IO.BusType  = LSM6DSO_SPI_4WIRES_BUS;  /* SPI 4 线模式 */
    LSM6DSO_IO.Address  = 0;                        /* SPI 模式无地址 */

    LSM6DSO_RegisterBusIO(&LSM6DSO_Object, &LSM6DSO_IO);
    res = LSM6DSO_Init(&LSM6DSO_Object);
    LSM6DSO_ACC_Enable(&LSM6DSO_Object);

    BC_LOG_INFO("lsm6sdo_init (SPI) res = %d \r\n", res);

    LSM6DSO_ACC_Enable_Wake_Up_Detection(&LSM6DSO_Object, LSM6DSO_INT1_PIN);

    LSM6DSO_ACC_SetOutputDataRate_With_Mode(&LSM6DSO_Object, 26.0,
                                            LSM6DSO_ACC_LOW_POWER_NORMAL_MODE);

    LSM6DSO_FIFO_ACC_Set_BDR(&LSM6DSO_Object, 26.0);

    return (res == LSM6DSO_OK) ? 0 : 1;
}

void lsm6sdo_on_and_off(bool status)
{
    if (status) {
        LSM6DSO_ACC_Enable(&LSM6DSO_Object);
    } else {
        LSM6DSO_ACC_Disable(&LSM6DSO_Object);
    }
}

uint8_t lsm6sdo_get_chip_id(void)
{
    uint8_t chip_id = 0;
    LSM6DSO_ReadID(&LSM6DSO_Object, &chip_id);
    BC_LOG_INFO("lsm6dso (SPI) id : 0x%x\r\n", chip_id);
    return chip_id;
}

void lsm6sdo_get_steps(uint16_t *steps)
{
    LSM6DSO_ACC_Get_Step_Count(&LSM6DSO_Object, steps);
}

void lsm6sdo_clear_steps(void)
{
    LSM6DSO_ACC_Step_Counter_Reset(&LSM6DSO_Object);
}

void lsm6sdo_enable_anymotion(void)
{
    LSM6DSO_ACC_Enable_Wake_Up_Detection(&LSM6DSO_Object, LSM6DSO_INT1_PIN);
}

void lsm6sdo_disable_anymotion(void)
{
    LSM6DSO_ACC_Disable_Wake_Up_Detection(&LSM6DSO_Object);
}

void lsm6sdo_get_accData(LSM6DSO_Axes_t *data)
{
    LSM6DSO_ACC_GetAxes(&LSM6DSO_Object, data);
}

void lsm6sdo_change_acc_odr(uint8_t odr)
{
    if (odr == 25) {
        LSM6DSO_ACC_SetOutputDataRate_With_Mode(&LSM6DSO_Object, 26,
                                                LSM6DSO_ACC_LOW_POWER_NORMAL_MODE);
    } else if (odr == 50) {
        LSM6DSO_ACC_SetOutputDataRate_With_Mode(&LSM6DSO_Object, 52,
                                                LSM6DSO_ACC_LOW_POWER_NORMAL_MODE);
    } else {
        LSM6DSO_ACC_SetOutputDataRate_With_Mode(&LSM6DSO_Object, 12.5f,
                                                LSM6DSO_ACC_LOW_POWER_NORMAL_MODE);
    }
}

void lsm6sdo_sport_state(uint8_t odr)
{
    lsm6sdo_change_acc_odr(odr);
    LSM6DSO_FIFO_ACC_Set_BDR(&LSM6DSO_Object, 26.0);
}

void lsm6sdo_silent_state(void)
{
    LSM6DSO_FIFO_ACC_Set_BDR(&LSM6DSO_Object, 0);
}

void lsm6sdo_get_data_from_fifo(LSM6DSO_AxesRaw_t *acc_data,
                                LSM6DSO_AxesRaw_t *gyr_data,
                                uint8_t *data_num)
{
    uint16_t num = 0;
    uint8_t reg_tag;
    LSM6DSO_Axes_t acc_data_temp;

    LSM6DSO_FIFO_Get_Num_Samples(&LSM6DSO_Object, &num);
    BC_LOG_INFO("lsm6dso (SPI) fifo num : %d \r\n", num);

    if (num > 0) {
        *data_num = (uint8_t)num;
        for (uint16_t i = 0; i < num; i++) {
            LSM6DSO_FIFO_Get_Tag(&LSM6DSO_Object, &reg_tag);
            switch (reg_tag) {
                case LSM6DSO_XL_NC_TAG:
                    LSM6DSO_FIFO_ACC_Get_Axes(&LSM6DSO_Object, &acc_data_temp);
                    acc_data[i].x = acc_data_temp.x;
                    acc_data[i].y = acc_data_temp.y;
                    acc_data[i].z = acc_data_temp.z;
                    break;

                case LSM6DSO_GYRO_NC_TAG:
                    break;

                default:
                    break;
            }
        }
    }
}

#endif




















