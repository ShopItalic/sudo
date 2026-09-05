#include "icm42688.h"


#include "bc_logger.h"
#include "bc_delay.h"

#include "bc_g_sensor_device_port.h"
#include "ring_config.h"

#if ( HARDWARE_1121_ENABLED == 1 ||  HARDWARE_158_ENABLED == 1 ||  HARDWARE_156_ENABLED == 1 ||  HARDWARE_1141_ENABLED == 1 ||  HARDWARE_1181_ENABLED == 1)	

#define icm42688_ADD 0x68 << 1


#else

#define icm42688_ADD 0x68

#endif	




static uint8_t icm42688_read_reg(uint8_t reg)
{
	uint8_t regval = 0;;
//	bc_g_sensor_i2c_write(icm42688_ADD,reg,,1);
	bc_g_sensor_i2c_read(icm42688_ADD,reg, &regval, 1);

//    nrf_sys_twi_readreg(icm42688_ADD, reg, &regval, 1);
    return regval;
}
static void icm42688_read_regs(uint8_t reg, uint8_t* buf, uint16_t len)
{
   // nrf_sys_twi_readreg(icm42688_ADD, reg, buf, len);
	bc_g_sensor_i2c_read(icm42688_ADD,reg, buf, len);
}
static uint8_t icm42688_write_reg(uint8_t reg, uint8_t value)
{
	 return  bc_g_sensor_i2c_write(icm42688_ADD,reg,&value,1);
   
}

uint8_t get_icm42688_id(void)
{
    uint8_t chip_id = icm42688_read_reg( ICM42688_WHO_AM_I);
    if (chip_id != ICM42688_ID){
        BC_LOG_INFO("icm42688 error:%02x", chip_id);
    }
    return chip_id;
}

static float accSensitivity   = 0.244f;   //加速度的最小分辨率 mg/LSB
static float gyroSensitivity  = 32.8f;    //陀螺仪的最小分辨率

/*ICM42688使用的ms级延时函数，须由用户提供。*/
#define ICM42688DelayMs(_nms) bc_delay_ms(_nms)



float bsp_Icm42688GetAres(uint8_t Ascale)
{
    switch(Ascale)
    {
    // Possible accelerometer scales (and their register bit settings) are:
    // 2 Gs (11), 4 Gs (10), 8 Gs (01), and 16 Gs  (00).
    case AFS_2G:
        accSensitivity = 2000 / 32768.0f;
        break;
    case AFS_4G:
        accSensitivity = 4000 / 32768.0f;
        break;
    case AFS_8G:
        accSensitivity = 8000 / 32768.0f;
        break;
    case AFS_16G:
        accSensitivity = 16000 / 32768.0f;
        break;
    }

    return accSensitivity;
}

float bsp_Icm42688GetGres(uint8_t Gscale)
{
    switch(Gscale)
    {
    case GFS_15_125DPS:
        gyroSensitivity = 15.125f / 32768.0f;
        break;
    case GFS_31_25DPS:
        gyroSensitivity = 31.25f / 32768.0f;
        break;
    case GFS_62_5DPS:
        gyroSensitivity = 62.5f / 32768.0f;
        break;
    case GFS_125DPS:
        gyroSensitivity = 125.0f / 32768.0f;
        break;
    case GFS_250DPS:
        gyroSensitivity = 250.0f / 32768.0f;
        break;
    case GFS_500DPS:
        gyroSensitivity = 500.0f / 32768.0f;
        break;
    case GFS_1000DPS:
        gyroSensitivity = 1000.0f / 32768.0f;
        break;
    case GFS_2000DPS:
        gyroSensitivity = 2000.0f / 32768.0f;
        break;
    }
    return gyroSensitivity;
}



/*******************************************************************************
* 名    称： bsp_IcmGetTemperature
* 功    能： 读取Icm42688 内部传感器温度
* 入口参数： 无
* 出口参数： 无
* 作　　者： 
* 创建日期： 2022-07-25
* 修    改：
* 修改日期：
* 备    注： datasheet page62
*******************************************************************************/
int8_t bsp_IcmGetTemperature(int16_t* pTemp)
{
    uint8_t buffer[2] = {0};
    icm42688_read_regs(ICM42688_TEMP_DATA1, buffer, 2);
    *pTemp = (int16_t)(((int16_t)((buffer[0] << 8) | buffer[1])) / 132.48 + 25);
    BC_LOG_INFO("pTemp:%d", *pTemp);
    return 0;
}

/*******************************************************************************
* 名    称： bsp_IcmGetAccelerometer
* 功    能： 读取Icm42688 加速度的值
* 入口参数： 三轴加速度的值
* 出口参数： 无
* 作　　者： 
* 创建日期： 2022-07-25
* 修    改：
* 修改日期：
* 备    注： datasheet page62
*******************************************************************************/
int8_t bsp_IcmGetAccelerometer(icm42688RawData_t* accData)
{
    uint8_t buffer[6] = {0};

    icm42688_read_regs(ICM42688_ACCEL_DATA_X1, buffer, 6);

    accData->x = ((uint16_t)buffer[0] << 8) | buffer[1];
    accData->y = ((uint16_t)buffer[2] << 8) | buffer[3];
    accData->z = ((uint16_t)buffer[4] << 8) | buffer[5];

    accData->x = (int16_t)(accData->x * accSensitivity);
    accData->y = (int16_t)(accData->y * accSensitivity);
    accData->z = (int16_t)(accData->z * accSensitivity);

    return 0;
}

/*******************************************************************************
* 名    称： bsp_IcmGetGyroscope
* 功    能： 读取Icm42688 陀螺仪的值
* 入口参数： 三轴陀螺仪的值
* 出口参数： 无
* 作　　者： 
* 创建日期： 2022-07-25
* 修    改：
* 修改日期：
* 备    注： datasheet page63
*******************************************************************************/
int8_t bsp_IcmGetGyroscope(icm42688RawData_t* GyroData)
{
    uint8_t buffer[6] = {0};

    icm42688_read_regs(ICM42688_GYRO_DATA_X1, buffer, 6);

    GyroData->x = ((uint16_t)buffer[0] << 8) | buffer[1];
    GyroData->y = ((uint16_t)buffer[2] << 8) | buffer[3];
    GyroData->z = ((uint16_t)buffer[4] << 8) | buffer[5];

    GyroData->x = (int16_t)(GyroData->x * gyroSensitivity);
    GyroData->y = (int16_t)(GyroData->y * gyroSensitivity);
    GyroData->z = (int16_t)(GyroData->z * gyroSensitivity);
    return 0;
}

/*******************************************************************************
* 名    称： bsp_IcmGetRawData
* 功    能： 读取Icm42688加速度陀螺仪数据
* 入口参数： 六轴
* 出口参数： 无
* 作　　者： 
* 创建日期： 2022-07-25
* 修    改：
* 修改日期：
* 备    注： datasheet page62,63
*******************************************************************************/
int8_t bsp_IcmGetRawData(icm42688RawData_t* accData, icm42688RawData_t* GyroData)
{
    uint8_t buffer[12] = {0};

    icm42688_read_regs(ICM42688_ACCEL_DATA_X1, buffer, 12);

    accData->x  = ((uint16_t)buffer[0] << 8)  | buffer[1];
    accData->y  = ((uint16_t)buffer[2] << 8)  | buffer[3];
    accData->z  = ((uint16_t)buffer[4] << 8)  | buffer[5];
    GyroData->x = ((uint16_t)buffer[6] << 8)  | buffer[7];
    GyroData->y = ((uint16_t)buffer[8] << 8)  | buffer[9];
    GyroData->z = ((uint16_t)buffer[10] << 8) | buffer[11];


    accData->x = (int16_t)(accData->x * accSensitivity);
    accData->y = (int16_t)(accData->y * accSensitivity);
    accData->z = (int16_t)(accData->z * accSensitivity);

    GyroData->x = (int16_t)(GyroData->x * gyroSensitivity);
    GyroData->y = (int16_t)(GyroData->y * gyroSensitivity);
    GyroData->z = (int16_t)(GyroData->z * gyroSensitivity);

    return 0;
}

uint16_t bsp_IcmGetStep(void)
{
    uint8_t buffer[4] = {0};
    icm42688_read_regs(ICM42688_APEX_DATA0, buffer, 4);
    uint16_t count = buffer[1] << 8 | buffer[0];
//    NRF_LOG_INFO("step:%02x-%02x-%02x-%02x", buffer[0],buffer[1],buffer[2],buffer[3]);
    return count;
}

/*******************************************************************************
* 名    称： bsp_Icm42688RegCfg
* 功    能： Icm42688 寄存器配置
* 入口参数： 无
* 出口参数： 无
* 作　　者： 
* 创建日期： 2022-07-25
* 修    改：
* 修改日期：
* 备    注：
*******************************************************************************/
uint8_t bsp_Icm42688RegCfg(void)
{
    uint8_t reg_val = 0;
    /* 读取 who am i 寄存器 */
    reg_val = icm42688_read_reg(ICM42688_WHO_AM_I);
    BC_LOG_INFO("reg_val:%x\n",reg_val);  
    if(reg_val == ICM42688_ID)
    {
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0); //设置bank 0区域寄存器
        icm42688_write_reg(ICM42688_DEVICE_CONFIG, 0x01); //软复位传感器
        ICM42688DelayMs(5);
        
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 1); //设置bank 1区域寄存器
        //icm42688_write_reg(ICM42688_INTF_CONFIG4, 0x02); //设置为4线SPI通信
        icm42688_write_reg(ICM42688_INTF_CONFIG4, 0x40); //设置为IIC通信

//        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0); //设置bank 0区域寄存器
//        icm42688_write_reg(ICM42688_FIFO_CONFIG, 0x40); //Stream-to-FIFO Mode(page63)

//        reg_val = icm42688_read_reg(ICM42688_INT_SOURCE0);
//        icm42688_write_reg(ICM42688_INT_SOURCE0, 0x00);
//        icm42688_write_reg(ICM42688_FIFO_CONFIG2, 0x00); // watermark
//        icm42688_write_reg(ICM42688_FIFO_CONFIG3, 0x02); // watermark
//        icm42688_write_reg(ICM42688_INT_SOURCE0, reg_val);
////        icm42688_write_reg(ICM42688_FIFO_CONFIG1, 0x63); // Enable the accel and gyro to the FIFO
//        icm42688_write_reg(ICM42688_FIFO_CONFIG1, 0x00); // Enable the accel and gyro to the FIFO

//        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
//        icm42688_write_reg(ICM42688_INT_CONFIG, 0x36);//配置中断线

//        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
//        reg_val = icm42688_read_reg(ICM42688_INT_SOURCE0);
//        reg_val |= (1 << 2); //FIFO_THS_INT1_ENABLE
//        icm42688_write_reg(ICM42688_INT_SOURCE0, reg_val);

//        bsp_Icm42688GetGres(GFS_1000DPS);
//        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
//        reg_val = icm42688_read_reg(ICM42688_GYRO_CONFIG0);//page73
//        reg_val |= (GFS_1000DPS << 5);   //量程 ±1000dps
//        reg_val |= (GODR_50Hz);     //输出速率 50HZ
//        icm42688_write_reg(ICM42688_GYRO_CONFIG0, reg_val);

        //by me
//Initialize Sensor in a typical configuration
//1. Set accelerometer ODR to 50 Hz (Register 0x50h in Bank 0)
        bsp_Icm42688GetAres(AFS_4G);
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_ACCEL_CONFIG0);//page74
//        reg_val |= (AFS_4G << 5);   //量程 ±4g
//        reg_val |= (AODR_50Hz);     //输出速率 50HZ
//        icm42688_write_reg(ICM42688_ACCEL_CONFIG0, reg_val);
        icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x49);
//2. Set accelerometer to Low Power mode (Register 0x4Eh in Bank 0)
//ACCEL_MODE = 2 and (Register 0x4Eh in Bank 0), ACCEL_LP_CLK_SEL = 0, for low power mode
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_PWR_MGMT0); //读取PWR—MGMT0当前寄存器的值(page72)
//        reg_val |= (1 << 5);//关闭温度测量
//        reg_val |= (2);//设置ACCEL_MODE 0:关闭 1:关闭 2:低功耗 3:低噪声
        reg_val = 0x22;
        icm42688_write_reg(ICM42688_PWR_MGMT0, reg_val);
        
        reg_val = icm42688_read_reg(ICM42688_INTF_CONFIG1);
        reg_val &= ~(1 << 3);
        icm42688_write_reg(ICM42688_INTF_CONFIG1, reg_val);
//3. Set DMP ODR = 50 Hz and turn on Pedometer feature (Register 0x56h in Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG0);
        reg_val = 0x20;//25hz
        icm42688_write_reg(ICM42688_APEX_CONFIG0, reg_val);
//4. Wait 1 millisecond
        ICM42688DelayMs(1);
//Initialize APEX hardware
//1. Set WOM_X_TH to 98 (Register 0x4Ah in Bank 4)
//2. Set WOM_Y_TH to 98 (Register 0x4Bh in Bank 4)
//3. Set WOM_Z_TH to 98 (Register 0x4Ch in Bank 4)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x04);
        reg_val = 220;
        icm42688_write_reg(ICM42688_ACCEL_WOM_X_THR, reg_val);
        icm42688_write_reg(ICM42688_ACCEL_WOM_Y_THR, reg_val);
        icm42688_write_reg(ICM42688_ACCEL_WOM_Z_THR, reg_val);
//4. Wait 1 millisecond
        ICM42688DelayMs(1);
//5. Enable all 3 axes as WOM sources for INT1 by setting bits 2:0 in register INT_SOURCE1 (Register 0x66h in Bank 0) 
//to 1. Or if INT2 is selected for WOM, enable all 3 axes as WOM sources by setting bits 2:0 in register INT_SOURCE4 
//(Register 0x69h in Bank 0) to 1.
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        icm42688_write_reg(ICM42688_INT_SOURCE1, 0x7);
//6. Wait 50 milliseconds
        ICM42688DelayMs(50);
//7. Turn on WOM feature by setting WOM_INT_MODE to 0, WOM_MODE to 1, SMD_MODE to 1 (Register 0x56h in 
//Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        icm42688_write_reg(ICM42688_SMD_CONFIG, 0x5);
        #if 1
//Initialize APEX hardware
//1. Set DMP_MEM_RESET_EN to 1 (Register 0x4Bh in Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_SIGNAL_PATH_RESET);
        reg_val = 0x20;
        icm42688_write_reg(ICM42688_SIGNAL_PATH_RESET, reg_val);
//2. Wait 1 millisecond
        ICM42688DelayMs(1);
//3. Set LOW_ENERGY_AMP_TH_SEL to 10 (Register 0x40h in Bank 4)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x04);
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG1);
        reg_val &= ~0xF;
        reg_val |= 10;
        icm42688_write_reg(ICM42688_APEX_CONFIG1, reg_val);
//4. Set PED_AMP_TH_SEL to 8 (Register 0x41h in Bank 4)
//5. Set PED_STEP_CNT_TH_SEL to 5 (Register 0x41h in Bank 4)
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG2);
        reg_val = 0xDF;
        icm42688_write_reg(ICM42688_APEX_CONFIG2, reg_val);
//6. Set PED_HI_EN_TH_SEL to 1 (Register 0x42h in Bank 4)
//7. Set PED_SB_TIMER_TH_SEL to 4 (Register 0x42h in Bank 4) 
//8. Set PED_STEP_DET_TH_SEL to 2 (Register 0x42h in Bank 4)
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG3);
        reg_val = 0x1;
        reg_val |= (4 << 2);
        reg_val |= (2 << 5);
        icm42688_write_reg(ICM42688_APEX_CONFIG3, reg_val);
//9. Set SENSITIVITY_MODE to 0 (Register 0x48h in Bank 4)
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG9);
        reg_val = 0x0;
        icm42688_write_reg(ICM42688_APEX_CONFIG9, reg_val);
//10. Set DMP_INIT_EN to 1 (Register 0x4Bh in Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_SIGNAL_PATH_RESET);
        reg_val = 0x40;
        icm42688_write_reg(ICM42688_SIGNAL_PATH_RESET, reg_val);
//11. Enable STEP detection, source for INT1 by setting bit 5 in register INT_SOURCE6 (Register 0x4Dh in Bank 4) to 1. Or 
//if INT2 is selected for STEP detection, enable STEP detection source by setting bit 5 in register INT_SOURCE7 
//(Register 0x4Eh in Bank 4) to 1.
//        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x04);
//        reg_val = icm42688_read_reg(ICM42688_INT_SOURCE6);
//        reg_val = 0x30;
//        icm42688_write_reg(ICM42688_INT_SOURCE6, reg_val);
//12. Wait 50 milliseconds
        ICM42688DelayMs(50);
//13. Turn on Pedometer feature by setting PED_ENABLE to 1 (Register 0x56h in Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG0);
        reg_val = 0x20;//25hz
        reg_val |= 0x80;//省电模式关闭
        icm42688_write_reg(ICM42688_APEX_CONFIG0, reg_val);
        ICM42688DelayMs(1);
        #endif
        return NRF_SUCCESS;
    }
    return NRF_ERROR_INVALID_STATE;
}

uint8_t bsp_Icm42688RegCfg_acc_and(enum icm42688_odr  odr_config)
{
	uint8_t reg_val = 0;
	        reg_val = icm42688_read_reg(ICM42688_ACCEL_CONFIG0);//page74
//        reg_val |= (AFS_4G << 5);   //量程 ±4g
//        reg_val |= (AODR_50Hz);     //输出速率 50HZ
//        icm42688_write_reg(ICM42688_ACCEL_CONFIG0, reg_val);
		switch(odr_config)
		 {
			 case ODR_25:
			 {
				 icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x4A);        // 25hz
				 break;
			 }
			 case ODR_50:
			 {
				icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x49);         //50hz   

				 break;
			 }
			 case ODR_100:
			 {
//#if ( HARDWARE_1141_ENABLED == 1)
//				 
//				
				 icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x48);         //100hz 
//#else

//	           icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x4A);         //100hz 

//#endif					 
				 
				 break;
			 }
			 case ODR_150:
			 {
				 icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x47);         //200hz
				 break;
			 }
			 case ODR_200:
			 {
				 icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x47);         //200hz 
				 break;
			 }
			 default :
			 {
				 break;
			 }
		 }
		 
		 
		 switch(odr_config)
		 {
			 case ODR_25:
			 {
				  reg_val |= (GODR_25Hz);          //25hz
				 break;
			 }
			 case ODR_50:
			 {
				  reg_val |= (GODR_50Hz);          //50hz   

				 break;
			 }
			 case ODR_100:
			 {
				   
//#if ( HARDWARE_1141x_ENABLED == 1)
//				 
//				reg_val |= (GODR_25Hz);          //25hz
//#else

	            reg_val |= (GODR_100Hz);         //100hz  

//#endif					 
				 break;
			 }
			 case ODR_150:
			 {
				 reg_val |= (GODR_200Hz);         //200hz   
				 break;
			 }
			 case ODR_200:
			 {
				  reg_val |= (GODR_200Hz);         //200hz  
				 break;
			 }
			 default :
			 {
				 break;
			 }
		 }		 
}
/*******************************************************************************
* 名    称： bsp_Icm42688RegCfg
* 功    能： Icm42688 寄存器配置
* 入口参数： 无
* 出口参数： 无
* 作　　者： 
* 创建日期： 2022-07-25
* 修    改：
* 修改日期：
* 备    注：
*******************************************************************************/
uint8_t bsp_Icm42688RegCfg_acc_and_gyro_data(enum icm42688_odr  odr_config)
{
    uint8_t reg_val = 0;
    /* 读取 who am i 寄存器 */
    reg_val = icm42688_read_reg(ICM42688_WHO_AM_I);
    BC_LOG_INFO("reg_val:%x\n",reg_val);  
    if(reg_val == ICM42688_ID)
    {
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0); //设置bank 0区域寄存器
        icm42688_write_reg(ICM42688_DEVICE_CONFIG, 0x01); //软复位传感器
        ICM42688DelayMs(5);
        
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 1); //设置bank 1区域寄存器
        //icm42688_write_reg(ICM42688_INTF_CONFIG4, 0x02); //设置为4线SPI通信
        icm42688_write_reg(ICM42688_INTF_CONFIG4, 0x40); //设置为IIC通信


        bsp_Icm42688GetGres(GFS_1000DPS);
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_GYRO_CONFIG0);//page73
        reg_val |= (GFS_1000DPS << 5);   //量程 ±1000dps
		 switch(odr_config)
		 {
			 case ODR_25:
			 {
				  reg_val |= (GODR_25Hz);          //25hz
				 break;
			 }
			 case ODR_50:
			 {
				  reg_val |= (GODR_50Hz);          //50hz   

				 break;
			 }
			 case ODR_100:
			 {
				   
#if ( HARDWARE_1141x_ENABLED == 1)
				 
				reg_val |= (GODR_25Hz);          //25hz
#else

	            reg_val |= (GODR_100Hz);         //100hz  

#endif					 
				 break;
			 }
			 case ODR_150:
			 {
				 reg_val |= (GODR_200Hz);         //200hz   
				 break;
			 }
			 case ODR_200:
			 {
				  reg_val |= (GODR_200Hz);         //200hz  
				 break;
			 }
			 default :
			 {
				 break;
			 }
		 }
       
        icm42688_write_reg(ICM42688_GYRO_CONFIG0, reg_val);

        //by me
//Initialize Sensor in a typical configuration
//1. Set accelerometer ODR to 50 Hz (Register 0x50h in Bank 0)
        bsp_Icm42688GetAres(AFS_4G);
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_ACCEL_CONFIG0);//page74
//        reg_val |= (AFS_4G << 5);   //量程 ±4g
//        reg_val |= (AODR_50Hz);     //输出速率 50HZ
//        icm42688_write_reg(ICM42688_ACCEL_CONFIG0, reg_val);
		switch(odr_config)
		 {
			 case ODR_25:
			 {
				 icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x4A);        // 25hz
				 break;
			 }
			 case ODR_50:
			 {
				icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x49);         //50hz   

				 break;
			 }
			 case ODR_100:
			 {
#if ( HARDWARE_1141_ENABLED == 1)
				 
				
				 icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x48);         //100hz 
#else

	           icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x4A);         //100hz 

#endif					 
				 
				 break;
			 }
			 case ODR_150:
			 {
				 icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x47);         //200hz
				 break;
			 }
			 case ODR_200:
			 {
				 icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x47);         //200hz 
				 break;
			 }
			 default :
			 {
				 break;
			 }
		 }		 
//        icm42688_write_reg(ICM42688_ACCEL_CONFIG0, 0x49);
//2. Set accelerometer to Low Power mode (Register 0x4Eh in Bank 0)
//ACCEL_MODE = 2 and (Register 0x4Eh in Bank 0), ACCEL_LP_CLK_SEL = 0, for low power mode
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_PWR_MGMT0); //读取PWR—MGMT0当前寄存器的值(page72)
//        reg_val |= (1 << 5);//关闭温度测量
//        reg_val |= (2);//设置ACCEL_MODE 0:关闭 1:关闭 2:低功耗 3:低噪声
//        reg_val = 0x22;
		reg_val = 0x2F;
        icm42688_write_reg(ICM42688_PWR_MGMT0, reg_val);
        
        reg_val = icm42688_read_reg(ICM42688_INTF_CONFIG1);
        reg_val &= ~(1 << 3);
        icm42688_write_reg(ICM42688_INTF_CONFIG1, reg_val);
//3. Set DMP ODR = 50 Hz and turn on Pedometer feature (Register 0x56h in Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG0);
        reg_val = 0x20;//25hz
        icm42688_write_reg(ICM42688_APEX_CONFIG0, reg_val);
//4. Wait 1 millisecond
        ICM42688DelayMs(1);
//Initialize APEX hardware
//1. Set WOM_X_TH to 98 (Register 0x4Ah in Bank 4)
//2. Set WOM_Y_TH to 98 (Register 0x4Bh in Bank 4)
//3. Set WOM_Z_TH to 98 (Register 0x4Ch in Bank 4)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x04);
        reg_val = 220;
        icm42688_write_reg(ICM42688_ACCEL_WOM_X_THR, reg_val);
        icm42688_write_reg(ICM42688_ACCEL_WOM_Y_THR, reg_val);
        icm42688_write_reg(ICM42688_ACCEL_WOM_Z_THR, reg_val);
//4. Wait 1 millisecond
        ICM42688DelayMs(1);
//5. Enable all 3 axes as WOM sources for INT1 by setting bits 2:0 in register INT_SOURCE1 (Register 0x66h in Bank 0) 
//to 1. Or if INT2 is selected for WOM, enable all 3 axes as WOM sources by setting bits 2:0 in register INT_SOURCE4 
//(Register 0x69h in Bank 0) to 1.
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        icm42688_write_reg(ICM42688_INT_SOURCE1, 0x7);
//6. Wait 50 milliseconds
        ICM42688DelayMs(50);
//7. Turn on WOM feature by setting WOM_INT_MODE to 0, WOM_MODE to 1, SMD_MODE to 1 (Register 0x56h in 
//Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        icm42688_write_reg(ICM42688_SMD_CONFIG, 0x5);
        #if 1
//Initialize APEX hardware
//1. Set DMP_MEM_RESET_EN to 1 (Register 0x4Bh in Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_SIGNAL_PATH_RESET);
        reg_val = 0x20;
        icm42688_write_reg(ICM42688_SIGNAL_PATH_RESET, reg_val);
//2. Wait 1 millisecond
        ICM42688DelayMs(1);
//3. Set LOW_ENERGY_AMP_TH_SEL to 10 (Register 0x40h in Bank 4)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x04);
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG1);
        reg_val &= ~0xF;
        reg_val |= 10;
        icm42688_write_reg(ICM42688_APEX_CONFIG1, reg_val);
//4. Set PED_AMP_TH_SEL to 8 (Register 0x41h in Bank 4)
//5. Set PED_STEP_CNT_TH_SEL to 5 (Register 0x41h in Bank 4)
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG2);
        reg_val = 0xDF;
        icm42688_write_reg(ICM42688_APEX_CONFIG2, reg_val);
//6. Set PED_HI_EN_TH_SEL to 1 (Register 0x42h in Bank 4)
//7. Set PED_SB_TIMER_TH_SEL to 4 (Register 0x42h in Bank 4) 
//8. Set PED_STEP_DET_TH_SEL to 2 (Register 0x42h in Bank 4)
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG3);
        reg_val = 0x1;
        reg_val |= (4 << 2);
        reg_val |= (2 << 5);
        icm42688_write_reg(ICM42688_APEX_CONFIG3, reg_val);
//9. Set SENSITIVITY_MODE to 0 (Register 0x48h in Bank 4)
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG9);
        reg_val = 0x0;
        icm42688_write_reg(ICM42688_APEX_CONFIG9, reg_val);
//10. Set DMP_INIT_EN to 1 (Register 0x4Bh in Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_SIGNAL_PATH_RESET);
        reg_val = 0x40;
        icm42688_write_reg(ICM42688_SIGNAL_PATH_RESET, reg_val);
//11. Enable STEP detection, source for INT1 by setting bit 5 in register INT_SOURCE6 (Register 0x4Dh in Bank 4) to 1. Or 
//if INT2 is selected for STEP detection, enable STEP detection source by setting bit 5 in register INT_SOURCE7 
//(Register 0x4Eh in Bank 4) to 1.
//        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x04);
//        reg_val = icm42688_read_reg(ICM42688_INT_SOURCE6);
//        reg_val = 0x30;
//        icm42688_write_reg(ICM42688_INT_SOURCE6, reg_val);
//12. Wait 50 milliseconds
        ICM42688DelayMs(50);
//13. Turn on Pedometer feature by setting PED_ENABLE to 1 (Register 0x56h in Bank 0)
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_APEX_CONFIG0);
        reg_val = 0x20;//25hz
        reg_val |= 0x80;//省电模式关闭
        icm42688_write_reg(ICM42688_APEX_CONFIG0, reg_val);
        ICM42688DelayMs(1);
        #endif
        return NRF_SUCCESS;
    }
    return NRF_ERROR_INVALID_STATE;
}


ret_code_t icm42688_init(void)
{
//    nrf_gpio_cfg_output(AP_AD0);
//    nrf_gpio_pin_write(AP_AD0, 0);
//    nrf_gpio_cfg_output(AP_CS);
//    nrf_gpio_pin_write(AP_CS, 1);
//    sys_twi_init();     // icm42688
    ret_code_t ret_code = bsp_Icm42688RegCfg();
    return ret_code;
}

ret_code_t icm42688_run(void)
{
    #if 0
    uint8_t reg_val = 0;
    if(icm42688_int_flag)
    {
        icm42688_int_flag = 0;
        icm42688_write_reg(ICM42688_REG_BANK_SEL, 0x00);
        reg_val = icm42688_read_reg(ICM42688_INT_STATUS2);//page73
        BC_LOG_INFO("wake int:%x",reg_val);
        
        reg_val = icm42688_read_reg(ICM42688_INT_STATUS3);//page73
        BC_LOG_INFO("step int:%x",reg_val);

        if(reg_val & 0x20){ //Step Detection Interrupt, clears on read
            BC_LOG_INFO("Step Detection1");
            bsp_IcmGetStep();
        }else if(reg_val & 0x10){ //Step Count Overflow Interrupt, clears on read
            BC_LOG_INFO("Step Count Overflow1");
            bsp_IcmGetStep();
        }
    }
    #endif
}
