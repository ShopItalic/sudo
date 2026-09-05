#include "da267.h"
#include "bc_logger.h"
#include "bc_g_sensor_device_port.h"
#include "bc_delay.h"

/*******************************************************************************
Macro definitions - Register define for Gsensor asic
********************************************************************************/
#define NSA_REG_SPI_I2C                 0x00
#define NSA_REG_WHO_AM_I                0x01
#define NSA_REG_ACC_X_LSB               0x02
#define NSA_REG_ACC_X_MSB               0x03
#define NSA_REG_ACC_Y_LSB               0x04
#define NSA_REG_ACC_Y_MSB               0x05
#define NSA_REG_ACC_Z_LSB               0x06
#define NSA_REG_ACC_Z_MSB               0x07 
#define NSA_REG_MOTION_FLAG             0x09 
#define NSA_REG_STEPS_MSB               0x0d
#define NSA_REG_G_RANGE                 0x0f
#define NSA_REG_ODR_AXIS_DISABLE        0x10
#define NSA_REG_POWERMODE_BW            0x11
#define NSA_REG_SWAP_POLARITY           0x12
#define NSA_REG_FIFO_CTRL               0x14
#define NSA_REG_INTERRUPT_SETTINGS0     0x15
#define NSA_REG_INTERRUPT_SETTINGS1     0x16
#define NSA_REG_INTERRUPT_SETTINGS2     0x17
#define NSA_REG_INTERRUPT_MAPPING1      0x19
#define NSA_REG_INTERRUPT_MAPPING2      0x1a
#define NSA_REG_INTERRUPT_MAPPING3      0x1b
#define NSA_REG_INT_PIN_CONFIG          0x20
#define NSA_REG_INT_LATCH               0x21
#define NSA_REG_FREEFALL_DURATION       0x22
#define NSA_REG_FREEFALL_THRESHOLD      0x23
#define NSA_REG_FREEFALL_HYST           0x24
#define NSA_REG_ACTIVE_DURATION         0x27
#define NSA_REG_ACTIVE_THRESHOLD        0x28
#define NSA_REG_TAP_DURATION            0x2A
#define NSA_REG_TAP_THRESHOLD           0x2B
#define NSA_REG_RESET_STEPS             0x2E
#define NSA_REG_STEP_CONFIG1			0x2F
#define NSA_REG_STEP_CONFIG2			0x30
#define NSA_REG_STEP_CONFIG3			0x31
#define NSA_REG_STEP_CONFIG4			0x32
#define NSA_REG_STEP_FILTER				0x33
#define NSA_REG_ENGINEERING_MODE        0x7f
#define NSA_REG_SENS_COMP               0x8c
#define NSA_REG_MEMS_OPTION             0x8f
#define NSA_REG_CHIP_INFO               0xc0

#if (defined ACC_ADO_VDD)
#define DEVICE_ID 0x27
#elif (defined ACC_ADO_GND)
#define DEVICE_ID 0x26
#endif

static const uint8_t DEVICE_ID = 0x26;

int da267_open_fifo(void);
int da267_close_fifo(void);

void delayms(uint32_t msecond)
{
    bc_delay_ms(msecond);
}

int8_t da267_register_read(uint8_t addr, uint8_t *data_m, uint8_t len)
{
//    return user_i2c_read(DEVICE_ID,addr,data_m,len);
  return bc_g_sensor_i2c_read(DEVICE_ID << 1,addr,data_m,len);
  
}

int8_t da267_register_write(uint8_t addr, uint8_t data_m)
{
//    return user_i2c_write(DEVICE_ID,addr,&data_m,1);
  return bc_g_sensor_i2c_write(DEVICE_ID << 1,addr,&data_m,1);
}

int8_t da267_register_mask_write(uint8_t addr, uint8_t mask, uint8_t data)
{
    int8_t	res = 0;
    uint8_t	tmp_data;

    res = da267_register_read(addr, &tmp_data,1);
    if(res){
        return res;
    }

    tmp_data &= ~mask; 
    tmp_data |= data & mask;
    res = da267_register_write(addr, tmp_data);
    return res;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
//Initialization
int8_t da267_init(void)
{
    int8_t res = 0;
    uint8_t data_m = 0;
    //Retry 3 times
    res = da267_register_read(NSA_REG_WHO_AM_I,&data_m,1);
    if(data_m != 0x13){
        res = da267_register_read(NSA_REG_WHO_AM_I,&data_m,1);
        if(data_m != 0x13){
          BC_LOG_INFO("------mir3da read chip id  error= %x-----\r\n",data_m);  
          return -1;
        }
    }
    BC_LOG_INFO("------mir3da chip id = %x-----\r\n",data_m); 
    da267_register_mask_write(0x00, 0x24, 0x24);
    delayms(50);
	res |= da267_register_write(NSA_REG_G_RANGE, 0x01);               //+/-4G
	res |= da267_register_write(NSA_REG_POWERMODE_BW, 0x00);          //normal mode
	res |= da267_register_write(NSA_REG_ODR_AXIS_DISABLE, 0x15);      //ODR = 25hz
//	res |= da267_register_write(NSA_REG_ODR_AXIS_DISABLE, 0x14);      //ODR = 12.5hz
    //Engineering mode
  res |= da267_register_write(NSA_REG_ENGINEERING_MODE, 0x83);
  res |= da267_register_write(NSA_REG_ENGINEERING_MODE, 0x69);
  res |= da267_register_write(NSA_REG_ENGINEERING_MODE, 0xBD);
    //Fifo Init
	da267_open_fifo();
    
    if(DEVICE_ID == 0x26)
    {
        da267_register_write(NSA_REG_SENS_COMP, 0x00);
    }
//    da267_silent_state();
    return res;
}

int8_t da267_get_id(void)
{
    int8_t res = 0;
    uint8_t data_m = 0;
    res = da267_register_read(NSA_REG_WHO_AM_I,&data_m,1);
    return data_m;
}

//enable/disable the chip
int8_t da267_set_enable(uint8_t enable)
{
    int8_t res = 0;
    if(enable)
        res = da267_register_write(NSA_REG_POWERMODE_BW,0x30);
    else    
        res = da267_register_write(NSA_REG_POWERMODE_BW,0x80);
    return res;    
}

int da267_open_fifo(void)
{
	return da267_register_write(NSA_REG_FIFO_CTRL,0x80);
}

int da267_close_fifo(void)
{
	return da267_register_write(NSA_REG_FIFO_CTRL,0x00);
}
//open active interrupt
int8_t da267_open_active_interrupt(uint8_t th){
    int8_t   res = 0;
    res = da267_register_write(NSA_REG_INTERRUPT_SETTINGS1,0x87);
    res |= da267_register_write(NSA_REG_ACTIVE_DURATION,0x00 );
    res |= da267_register_write(NSA_REG_ACTIVE_THRESHOLD,th);
    res |= da267_register_write(NSA_REG_INTERRUPT_MAPPING1,0x04 );
    return res;
}

//close active interrupt
int8_t da267_close_active_interrupt(void){
    int8_t   res = 0;
    res = da267_register_write(NSA_REG_INTERRUPT_SETTINGS1,0x00 );
    res |= da267_register_write(NSA_REG_INTERRUPT_MAPPING1,0x00 );
    return res;
}

int32_t da267_read_fifo(da267_acc_data_t *p_fifo_buf,uint8_t *p_num)
{
    if(p_fifo_buf == NULL || p_num == NULL){
        return false;
    }
    
    uint8_t fifo_state = 0;  
    da267_register_read(0x08, &fifo_state, 1);
    (*p_num) = fifo_state & 0x3f;
    da267_register_read(NSA_REG_ACC_X_LSB, (uint8_t *)p_fifo_buf, 6*(*p_num));
    for(uint8_t i=0;i<*p_num;i++){
        p_fifo_buf[i].x >>= 3;
        p_fifo_buf[i].y >>= 3;
        p_fifo_buf[i].z >>= 3;
    }
    return true;
}


//在安静模式下等待anymotion中断
void da267_silent_state(void)
{
    da267_close_fifo();
    da267_open_active_interrupt(150);
}
//在运动模式下使用fifo
void da267_sport_state(uint8_t odr)
{
    if(odr == 13){
        da267_register_write(NSA_REG_ODR_AXIS_DISABLE, 0x14);//ODR = 12.5hz
    }else{
        da267_register_write(NSA_REG_ODR_AXIS_DISABLE, 0x15);//ODR = 25hz
    }
    da267_open_fifo();
    da267_close_active_interrupt();
}


