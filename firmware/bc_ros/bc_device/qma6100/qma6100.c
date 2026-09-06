#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "qma6100.h"
#include "bc_g_sensor_device_port.h"
#include "bc_delay.h"
#include "bc_logger.h"

#define QMA6100_LOG        BC_LOG_INFO
#define QMA6100_IRQ_LOG    BC_LOG_INFO
#define QMA6100_ERR        BC_LOG_INFO

typedef struct
{
    int16_t sign[3];
    uint16_t map[3];
}qst_convert;

typedef struct
{
    uint8_t                    slave;
    uint8_t                    chip_id;
    int32_t                lsb_1g;
    uint8_t                    layout;
    qst_convert            cvt;
    qma6100_fifo_mode    fifo_mode;
    int32_t                fifo_len;    
    int32_t                raw[3];
}qma6100_data;

static const qst_convert qst_map[] = 
{
    { { 1, 1, 1}, {0, 1, 2} },
    { {-1, 1, 1}, {1, 0, 2} },
    { {-1,-1, 1}, {0, 1, 2} },
    { { 1,-1, 1}, {1, 0, 2} },

    { {-1, 1, -1}, {0, 1, 2} },
    { { 1, 1, -1}, {1, 0, 2} },
    { { 1,-1, -1}, {0, 1, 2} },
    { {-1,-1, -1}, {1, 0, 2} }
};

static qma6100_data g_qma6100;


void qma6100_delay(uint32_t delay)
{
    bc_delay_ms(delay);
}
#if defined(QMA6100_QST_USE_SPI)

int32_t qma6100_writereg(uint8_t reg_add,uint8_t reg_dat)
{
    uint8_t spi_tx_buf[2] = {reg_add,reg_dat};
    acc_spim_transfer(spi_tx_buf,2,NULL,0);
    return 1;
}

int32_t qma6100_readreg(uint8_t reg_add,uint8_t *buf,uint16_t num)
{
    uint8_t tx_buffer[1] = {reg_add};
    acc_spim_transfer(tx_buffer,1,buf,num);
    return 1;
}
#else

//#if (QMA_ADO_TYPE == 1)
//	  uint8_t slave_addr = 0x12;
//#elif (QMA_ADO_TYPE == 1)
//     uint8_t slave_addr = 0x13;
//#endif

#if (HARDWARE_451_ENABLED == 1)	

 uint8_t slave_addr = 0x13 << 1;	
 
 #else
  uint8_t slave_addr = 0x13;	
#endif

int32_t qma6100_writereg(uint8_t reg_add,uint8_t reg_dat)
{
	int32_t ret = QMA6100_FAIL;
	int32_t retry = 0;

	while((ret==QMA6100_FAIL) && (retry++ < 5))
	{
		if(bc_g_sensor_i2c_write(slave_addr,reg_add,&reg_dat,1))
		{
			return QMA6100_SUCCESS;
		}
	}
	return ret;
}

int32_t qma6100_readreg(uint8_t reg_add,uint8_t *buf,uint16_t num)
{
	int32_t ret = QMA6100_FAIL;
	int32_t retry = 0;

	while((ret==QMA6100_FAIL) && (retry++ < 5))
	{
		if(bc_g_sensor_i2c_read(slave_addr,reg_add,buf,num))
		{		
			return QMA6100_SUCCESS;
		}
		qma6100_delay(5);
	}

	return ret;
}

#endif
uint8_t qma6100_chip_id()
{
    uint8_t chip_id;
    int32_t ret = QMA6100_FAIL;
    ret = qma6100_readreg(QMA6100_CHIP_ID, &chip_id, 1);
//    QMA6100_LOG("qma6100_chip_id =0x%x ret=%d \r\n", chip_id, ret);    
    return chip_id;
}

uint8_t qma6100_die_wafe_id(void)
{
    uint8_t die_buf_id[2] = {0x00};
    uint8_t wafe_id;
    uint16_t dieid;
    int32_t ret = QMA6100_FAIL;

    ret = qma6100_readreg(0x47, die_buf_id, 2);
    dieid = die_buf_id[1]<<8 |die_buf_id[0] ;
    ret = qma6100_readreg(0x5a, &wafe_id, 1);
    QMA6100_LOG("dieid =0x%x wafe_id=0x%x",dieid, wafe_id);
    return ret;
}

int32_t qma6100_set_range(uint8_t range)
{
    int32_t ret = 0;
    if(range == QMA6100_RANGE_4G)
        g_qma6100.lsb_1g = 2048;
    else if(range == QMA6100_RANGE_8G)
        g_qma6100.lsb_1g = 1024;
    else if(range == QMA6100_RANGE_16G)
        g_qma6100.lsb_1g = 512;
    else if(range == QMA6100_RANGE_32G)
        g_qma6100.lsb_1g = 256;
    else
        g_qma6100.lsb_1g = 4096;

    ret = qma6100_writereg(QMA6100_REG_RANGE, range);
    return ret;
}

int32_t qma6100_set_mode_odr(int32_t mode, int32_t mclk, int32_t div, int32_t lpf)
{
    int32_t ret = 0;
    uint8_t mclk_reg = (uint8_t)mclk;
    uint8_t odr_reg = (uint8_t)div;
    uint8_t lpf_reg = (uint8_t)lpf;
    if(mode >= QMA6100_MODE_ACTIVE)
    {
        ret = qma6100_writereg(QMA6100_REG_POWER_CTL, 0x80|mclk_reg);
        ret = qma6100_writereg(QMA6100_REG_BW_ODR, lpf_reg|odr_reg);
    }else{
        ret = qma6100_writereg(QMA6100_REG_POWER_CTL, 0x00);
    }
    return ret;
}


void qma6100_dump_reg(void)
{
    uint8_t reg_data = 0;
    int32_t i=0;
    uint8_t reg_map[]=
    {
        0x0f,0x10,0x11,0x09,0x0a,0x0b,0x0c,
        0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,
        0x20,0x21,0x30,0x31,0x37,0x3e,0x4a,0x50,0x56
    };

    QMA6100_LOG("qma6100_dump_reg");
    for(i=0; i< sizeof(reg_map)/sizeof(reg_map[0]); i++)
    {
        qma6100_readreg(reg_map[i],&reg_data,1);
        QMA6100_LOG("0x%x = 0x%x", reg_map[i], reg_data);
    }
}

#if defined(QMA6100_DATA_READY)
void qma6100_drdy_config(int32_t int_map, int32_t enable)
{
    uint8_t reg_17 = 0;
    uint8_t reg_1a = 0;
    uint8_t reg_1c = 0;

    QMA6100_LOG("qma6100_drdy_config %d", enable);
    qma6100_readreg(0x17, &reg_17, 1);
    qma6100_readreg(0x1a, &reg_1a, 1);
    qma6100_readreg(0x1c, &reg_1c, 1);

    if(enable)
    {
        reg_17 |= 0x10;
        reg_1a |= 0x10;
        reg_1c |= 0x10;
        qma6100_writereg(0x17, reg_17);
        if(int_map == QMA6100_MAP_INT1)
            qma6100_writereg(0x1a, reg_1a);
        else if(int_map == QMA6100_MAP_INT2)
            qma6100_writereg(0x1c, reg_1c);
    }
    else
    {
        reg_17 &= (~0x10);
        reg_1a &= (~0x10);
        reg_1c &= (~0x10);
        qma6100_writereg(0x17, reg_17);
        qma6100_writereg(0x1a, reg_1a);
        qma6100_writereg(0x1c, reg_1c);
    }

}
#endif

#if defined(QMA6100_FIFO_FUNC)
static uint8_t qma6100_fifo_reg[64*6];

void qma6100_fifo_config(int32_t int_map, int32_t enable)
{
    uint8_t    reg_17, reg_1a, reg_1c=0;

    QMA6100_LOG("qma6100_fifo_config enable:%d", enable);
    qma6100_readreg(0x17, &reg_17, 1);
    qma6100_readreg(0x1a, &reg_1a, 1);
    qma6100_readreg(0x1c, &reg_1c, 1);

    if(enable)
    {
        g_qma6100.fifo_mode = QMA6100_FIFO_MODE_FIFO;
        if(g_qma6100.fifo_mode == QMA6100_FIFO_MODE_FIFO)
        {
            //qma6100_writereg(0x31, 0x40);
            qma6100_writereg(0x3E, 0x47);    //bit[6:7] 0x00:BYPASS 0x40:FIFO 0x80:STREAM
            qma6100_writereg(0x17, reg_17|0x20);
            if(int_map == QMA6100_MAP_INT1)
                qma6100_writereg(0x1a, reg_1a|0x20);
            else if(int_map == QMA6100_MAP_INT2)
                qma6100_writereg(0x1c, reg_1c|0x20);
        }
        else if(g_qma6100.fifo_mode == QMA6100_FIFO_MODE_STREAM)
        {    
            qma6100_writereg(0x31, 0x20);    // 0x3f
            qma6100_writereg(0x3E, 0x87);    //bit[6:7] 0x00:BYPASS 0x40:FIFO 0x80:STREAM
            qma6100_writereg(0x17, reg_17|0x40);
            if(int_map == QMA6100_MAP_INT1)
                qma6100_writereg(0x1a, reg_1a|0x40);
            else if(int_map == QMA6100_MAP_INT2)
                qma6100_writereg(0x1c, reg_1c|0x40);
        }
        else if(g_qma6100.fifo_mode == QMA6100_FIFO_MODE_BYPASS)
        {
            qma6100_writereg(0x3E, 0x07);    //bit[6:7] 0x00:BYPASS 0x40:FIFO 0x80:STREAM
            qma6100_writereg(0x17, reg_17|0x20);
            if(int_map == QMA6100_MAP_INT1)
                qma6100_writereg(0x1a, reg_1a|0x20);
            else if(int_map == QMA6100_MAP_INT2)
                qma6100_writereg(0x1c, reg_1c|0x20);
        }
    }
    else
    {
        g_qma6100.fifo_mode = QMA6100_FIFO_MODE_NONE;
        reg_17 &= (~0x60);
        reg_1a &= (~0x60);
        reg_1c &= (~0x60);
        qma6100_writereg(0x17, reg_17);
        qma6100_writereg(0x1a, reg_1a);
        qma6100_writereg(0x1c, reg_1c);
    }
}

int32_t qma6100_read_fifo(uint8_t *fifo_buf)
{
    int32_t ret = 0;
    uint8_t databuf[2];

#if defined(QMA6100_INT_LATCH)
    ret = qma6100_readreg(QMA6100_INT_STAT2, databuf, 1);
#endif
    ret = qma6100_readreg(QMA6100_FIFO_STATE, databuf, 1);
    if(ret != QMA6100_SUCCESS)
    {
        QMA6100_ERR("qma6100_read_fifo state error");
        return ret;
    }
    g_qma6100.fifo_len = databuf[0]&0x7f;
    if(g_qma6100.fifo_len > 64)
    {
        QMA6100_ERR("qma6100_read_fifo depth(%d) error",g_qma6100.fifo_len);
        return QMA6100_FAIL;
    }
#if 1
    qma6100_readreg(0x3f, fifo_buf, g_qma6100.fifo_len*6);
#else
    for(int icount=0; icount<g_qma6100.fifo_len; icount++)
    {
        qma6100_readreg(0x3f, &fifo_buf[icount*6], 6);
    }
#endif
    if(g_qma6100.fifo_mode == QMA6100_FIFO_MODE_FIFO)
    {
        ret = qma6100_writereg(0x3e, 0x47);
    }
    else if(g_qma6100.fifo_mode == QMA6100_FIFO_MODE_STREAM)
    {
        ret = qma6100_writereg(0x3e, 0x87);
    }
    else if(g_qma6100.fifo_mode == QMA6100_FIFO_MODE_BYPASS)
    {
        ret = qma6100_writereg(0x3e, 0x07);
    }
// log fifo
    return ret;
}

void qma6100_exe_fifo(uint8_t *fifo_buf)
{
    int32_t icount;    
    int16_t raw_data[3];
    //float acc_data[3];

    QMA6100_ERR("fifo_depth=%d", g_qma6100.fifo_len);
// log fifo
    for(icount=0; icount<g_qma6100.fifo_len; icount++)
    {
        raw_data[0]  = (int16_t)(((int16_t)(fifo_buf[1+icount*6]<<8)) |(fifo_buf[0+icount*6]));
        raw_data[1]  = (int16_t)(((int16_t)(fifo_buf[3+icount*6]<<8)) |(fifo_buf[2+icount*6]));
        raw_data[2]  = (int16_t)(((int16_t)(fifo_buf[5+icount*6]<<8)) |(fifo_buf[4+icount*6]));
        raw_data[0]  = raw_data[0]>>2;
        raw_data[1]  = raw_data[1]>>2;
        raw_data[2]  = raw_data[2]>>2;
        QMA6100_LOG("%d:%d    %d    %d    ",icount,raw_data[0],raw_data[1],raw_data[2]);
        if((icount%4==0))
        {
            QMA6100_LOG("\r");
        }
        //acc_data[0] = (raw_data[0]*9.807f)/(g_qma6100.lsb_1g);            //GRAVITY_EARTH_1000
        //acc_data[1] = (raw_data[1]*9.807f)/(g_qma6100.lsb_1g);
        //acc_data[2] = (raw_data[2]*9.807f)/(g_qma6100.lsb_1g);
    }    
    QMA6100_LOG("\r");
// log fifo
}
#endif

#if defined(QMA6100_STEPCOUNTER)
uint32_t qma6100_read_stepcounter(void)
{
    uint8_t data[3];
    uint8_t ret;
    uint32_t step_num;
    int32_t step_dif;
    static uint32_t step_last = 0;

    ret = qma6100_readreg(QMA6100_STEP_CNT_L, data, 2);    
    if(ret != QMA6100_SUCCESS)
    {
        step_num = step_last;
        return QMA6100_SUCCESS;
    }
    ret = qma6100_readreg(QMA6100_STEP_CNT_M, &data[2], 1);    
    if(ret != QMA6100_SUCCESS)
    {
        step_num = step_last;
        return QMA6100_SUCCESS;
    }
        
    step_num = (uint32_t)(((uint32_t)data[2]<<16)|((uint32_t)data[1]<<8)|data[0]);

#if 1//defined(QMA6100_CHECK_ABNORMAL_DATA)
    step_dif = (int32_t)(step_num-step_last);
    if(QMA6100_ABS(step_dif) > 100)
    {
        uint32_t step_num_temp[3];

        ret = qma6100_readreg(QMA6100_STEP_CNT_L, data, 2);    
        ret = qma6100_readreg(QMA6100_STEP_CNT_M, &data[2], 1);
        step_num_temp[0] = (uint32_t)(((uint32_t)data[2]<<16)|((uint32_t)data[1]<<8)|data[0]);
        qma6100_delay(2);
        
        ret = qma6100_readreg(QMA6100_STEP_CNT_L, data, 2);    
        ret = qma6100_readreg(QMA6100_STEP_CNT_M, &data[2], 1);
        step_num_temp[1] = (uint32_t)(((uint32_t)data[2]<<16)|((uint32_t)data[1]<<8)|data[0]);
        qma6100_delay(2);
        
        ret = qma6100_readreg(QMA6100_STEP_CNT_L, data, 2);    
        ret = qma6100_readreg(QMA6100_STEP_CNT_M, &data[2], 1);
        step_num_temp[2] = (uint32_t)(((uint32_t)data[2]<<16)|((uint32_t)data[1]<<8)|data[0]);
        qma6100_delay(2);
        if((step_num_temp[0]==step_num_temp[1])&&(step_num_temp[1]==step_num_temp[2]))
        {
            QMA6100_LOG("qma6100 check data, confirm!");
            step_num = step_num_temp[0];
        }
        else
        {    
            QMA6100_LOG("qma6100 check data, abnormal!");
            return step_last;
        }
    }
#endif
    step_last = step_num;

    return step_num;
}




void qma6100_stepcounter_config(int32_t enable)
{ 
//  int32_t odr = 27;   // 100Hz
  uint8_t reg_12 = 0x00;
  uint8_t reg_14 = 0x00;
  uint8_t reg_15 = 0x00;
  uint8_t reg_1e = 0x00;

  qma6100_writereg(0x13, 0x80);
  qma6100_delay(1);
//  qma6100_writereg(0x13, 0x7f);     //  0x7f(P2P/16), 0.977*16*LSB
  qma6100_writereg(0x13, 0x7f);     //  0x7f(P2P/16), 0.977*16*LSB
  // Windows time
  if(enable)
  {
#if defined(QMA6100_ODR_27)
    reg_12 = 0x84;
    reg_14 = 0x05;      //((200*odr)/(1000));      // about:200 ms
    reg_15 = 0x07;      //(((2200/8)*odr)/1000);   // 2000 ms
#elif defined(QMA6100_ODR_55)
    if(g_qma6100.chip_id == 0x90)
    {
        reg_12 = 0x89;  //0x89
        reg_14 = 0x0b;    //0x0c  //((200*odr)/(1000)); 
        reg_15 = 0x0c;     //0x0e //(((2200/8)*odr)/1000);   // 2000 ms
    }
    else if(g_qma6100.chip_id == 0xfa)
    {
        reg_12 = 0x89;  //0x89
        reg_14 = 0x0b;    //0x0c  //((200*odr)/(1000)); 
        reg_15 = 0x0e;     //0x0e //(((2200/8)*odr)/1000);   // 2000 ms
    }
#elif defined(QMA6100_ODR_100)
    reg_12 = 0x8f;
    reg_14 = 0x1c;      //((280*odr)/(1000));      // about:280 ms
    reg_15 = 0x19;      //(((2200/8)*odr)/1000);   // 2000 ms
#endif
//    QMA6100_LOG("step time config 0x14=0x%x 0x15=0x%x", reg_14, reg_15);
    qma6100_writereg(0x12, reg_12);
    qma6100_writereg(0x14, reg_14);
    qma6100_writereg(0x15, reg_15);
    // lpf
    qma6100_readreg(0x1e, &reg_1e, 1);
    reg_1e &= 0x3f;
    qma6100_writereg(0x1e, (uint8_t)(reg_1e|QMA6100_STEP_LPF_0));   // default 0x08
    // start count, p2p, fix peak
//    qma6100_writereg(0x1f, (uint8_t)QMA6100_STEP_START_40|0x08);    // 0x10
    if(g_qma6100.chip_id == 0x90)
    {
        qma6100_writereg(0x1f, (uint8_t)QMA6100_STEP_START_24|0x09);    // 0x10
    }
    else if(g_qma6100.chip_id == 0xfa)
    {
        qma6100_writereg(0x1f, (uint8_t)QMA6100_STEP_START_24|0x0a);
    }
  }
}


void qma6100_clear_step(void)
{
    qma6100_writereg(0x13, 0x80);        // clear step    
    qma6100_delay(10);
    qma6100_writereg(0x13, 0x80);        // clear step
    qma6100_delay(10);
    qma6100_writereg(0x13, 0x80);        // clear step
    qma6100_delay(10);
    qma6100_writereg(0x13, 0x80);        // clear step        
    qma6100_writereg(0x13, 0x7f);        // clear step        
}



#if defined(QMA6100_STEP_INT)
void qma6100_step_int_config(int32_t int_map, int32_t enable)
{
    uint8_t    reg_16=0;
    uint8_t    reg_19=0;
    uint8_t    reg_1b=0;

    qma6100_readreg(0x16, &reg_16, 1);
    qma6100_readreg(0x19, &reg_19, 1);
    qma6100_readreg(0x1b, &reg_1b, 1);
    if(enable)
    {
        reg_16 |= 0x08;
        reg_19 |= 0x08;
        reg_1b |= 0x08;
        qma6100_writereg(0x16, reg_16);
        if(int_map == QMA6100_MAP_INT1)
            qma6100_writereg(0x19, reg_19);
        else if(int_map == QMA6100_MAP_INT2)
            qma6100_writereg(0x1b, reg_1b);
    }
    else
    {
        reg_16 &= (~0x08);
        reg_19 &= (~0x08);
        reg_1b &= (~0x08);

        qma6100_writereg(0x16, reg_16);
        qma6100_writereg(0x19, reg_19);
        qma6100_writereg(0x1b, reg_1b);
    }
}
#endif

#if defined(QMA6100_SIGNIFICANT_STEP_INT)
void qma6100_sigstep_int_config(int32_t int_map, int32_t enable)
{
    uint8_t    reg_16=0;
    uint8_t    reg_19=0;
    uint8_t    reg_1b=0;

    qma6100_readreg(0x16, &reg_16, 1);
    qma6100_readreg(0x19, &reg_19, 1);
    qma6100_readreg(0x1b, &reg_1b, 1);
    
    qma6100_writereg(0x1d, 0x1a);
    if(enable)
    {
        reg_16 |= 0x40;
        reg_19 |= 0x40;
        reg_1b |= 0x40;
        qma6100_writereg(0x16, reg_16);
        if(int_map == QMA6100_MAP_INT1)
            qma6100_writereg(0x19, reg_19);
        else if(int_map == QMA6100_MAP_INT2)
            qma6100_writereg(0x1b, reg_1b);
    }
    else
    {
        reg_16 &= (~0x40);
        reg_19 &= (~0x40);
        reg_1b &= (~0x40);

        qma6100_writereg(0x16, reg_16);
        qma6100_writereg(0x19, reg_19);
        qma6100_writereg(0x1b, reg_1b);
    }
}
#endif


#endif

#if defined(QMA6100_ANY_MOTION)
void qma6100_anymotion_config(int32_t int_map, int32_t enable)
{
    uint8_t reg_0x18 = 0;
    uint8_t reg_0x1a = 0;
    uint8_t reg_0x1c = 0;
    uint8_t reg_0x2c = 0;
#if defined(QMA6100_SIGNIFICANT_MOTION)
    uint8_t reg_0x19 = 0;
    uint8_t reg_0x1b = 0;
#endif

    QMA6100_LOG("qma6100_anymotion_config %d", enable);
    qma6100_readreg(0x18, &reg_0x18, 1);
    qma6100_readreg(0x1a, &reg_0x1a, 1);
    qma6100_readreg(0x1c, &reg_0x1c, 1);
    qma6100_readreg(0x2c, &reg_0x2c, 1);
    reg_0x2c |= 0x01;

    qma6100_writereg(0x2c, reg_0x2c);
    if(g_qma6100.chip_id == 0x90)
    {
        qma6100_writereg(0x2e, 0x10);        // 0.488*16*32 = 250mg
    }
    else if(g_qma6100.chip_id == 0xfa)
    {
        qma6100_writereg(0x2e, 0x10);
    }
    qma6100_writereg(0x2f, 0x00);        // 0.488*16*32 = 250mg
    // add by yang, tep counter, raise wake, and tap detector,any motion by pass LPF
    qma6100_writereg(0x30, 0x80|0x40|0x3f);    // default 0x3f
    // add by yang, tep counter, raise wake, and tap detector,any motion by pass LPF
    if(enable)
    {
        reg_0x18 |= 0x07;
        reg_0x1a |= 0x01;
        reg_0x1c |= 0x01;

        qma6100_writereg(0x18, reg_0x18);    // enable any motion
        if(int_map == QMA6100_MAP_INT1)
        {
            qma6100_writereg(0x1a, reg_0x1a);
        }
        else if(int_map == QMA6100_MAP_INT2)
            qma6100_writereg(0x1c, reg_0x1c);
    }
    else
    {
        reg_0x18 &= (~0x07);
        reg_0x1a &= (~0x01);
        reg_0x1c &= (~0x01);
        
        qma6100_writereg(0x18, reg_0x18);
        qma6100_writereg(0x1a, reg_0x1a);
        qma6100_writereg(0x1c, reg_0x1c);
    }
    
#if defined(QMA6100_SIGNIFICANT_MOTION)
    qma6100_readreg(0x19, &reg_0x19, 1);
    qma6100_readreg(0x1b, &reg_0x1b, 1);
    
    qma6100_writereg(0x2f, 0x01);        // bit0: selecat significant motion
    if(enable)
    {
        reg_0x19 |= 0x01;
        reg_0x1b |= 0x01;
        if(int_map == QMA6100_MAP_INT1)
            qma6100_writereg(0x19, reg_0x19);
        else if(int_map == QMA6100_MAP_INT2)
            qma6100_writereg(0x1b, reg_0x1b);
    }
    else
    {
        reg_0x19 &= (~0x01);
        reg_0x1b &= (~0x01);
        qma6100_writereg(0x19, reg_0x19);
        qma6100_writereg(0x1b, reg_0x1b);
    }
#endif    
}
#endif

#if defined(QMA6100_NO_MOTION)
void qma6100_nomotion_config(int32_t int_map, int32_t enable)
{
    uint8_t reg_0x18 = 0;
    uint8_t reg_0x1a = 0;
    uint8_t reg_0x1c = 0;
    uint8_t reg_0x2c = 0;

    QMA6100_LOG("qma6100_nomotion_config %d", enable);

    qma6100_readreg(0x18, &reg_0x18, 1);
    qma6100_readreg(0x1a, &reg_0x1a, 1);
    qma6100_readreg(0x1c, &reg_0x1c, 1);
    qma6100_readreg(0x2c, &reg_0x2c, 1);
    reg_0x2c |= 0x24;        // 10s
    //reg_0x2c |= 0xc0;         // 100s

    qma6100_writereg(0x2c, reg_0x2c);
    qma6100_writereg(0x2d, 0x14);
    if(enable)
    {
        reg_0x18 |= 0xe0;
        reg_0x1a |= 0x80;
        reg_0x1c |= 0x80;        
        qma6100_writereg(0x18, reg_0x18);
        if(int_map == QMA6100_MAP_INT1)
            qma6100_writereg(0x1a, reg_0x1a);
        else if(int_map == QMA6100_MAP_INT2)
            qma6100_writereg(0x1c, reg_0x1c);
    }
    else
    {
        reg_0x18 &= (~0xe0);
        reg_0x1a &= (~0x80);
        reg_0x1c &= (~0x80);
        
        qma6100_writereg(0x18, reg_0x18);
        qma6100_writereg(0x1a, reg_0x1a);
        qma6100_writereg(0x1c, reg_0x1c);
    }

}
#endif

#if defined(QMA6100_TAP_FUNC)
void qma6100_tap_config(int32_t tap_type, int32_t int_map, int32_t enable)
{
    uint8_t reg_16,reg_19,reg_1a,reg_1b,reg_1c;
    uint8_t tap_reg = (uint8_t)tap_type;

    qma6100_readreg(0x16, &reg_16, 1);
    qma6100_readreg(0x19, &reg_19, 1);
    qma6100_readreg(0x1a, &reg_1a, 1);
    qma6100_readreg(0x1b, &reg_1b, 1);
    qma6100_readreg(0x1c, &reg_1c, 1);

    //qma6100_writereg(0x1e, 0x03);        // TAP_QUIET_TH 31.25*8 = 250mg 
    //qma6100_writereg(0x2a, 0x43);        // tap config1
    //qma6100_writereg(0x2b, (0xc0+6));        // tap config2
    // add by yang, tep counter, raise wake, and tap detector,any motion by pass LPF
    //qma6100_writereg(0x30, 0x80|0x40|0x3f);    // default 0x3f
    // add by yang, tep counter, raise wake, and tap detector,any motion by pass LPF

    qma6100_writereg(0x1e, TAP_QUIET_TH);        // TAP_QUIET_TH 31.25*8 = 250mg 
    /*single tap param*/
    //qma6100_writereg(0x2a, TAP_QUIET_TIME_20MS|TAP_SHOCK_TIME_50MS|TAP_TRIPLE_NOT_WAIT_QUAD_DELAY_Y|TAP_EARIN_N|TAP_DUR_250MS);
    qma6100_writereg(0x2a, TAP_QUIET_TIME_30MS|TAP_SHOCK_TIME_75MS|TAP_TRIPLE_NOT_WAIT_QUAD_DELAY_Y|TAP_EARIN_N|TAP_DUR_500MS);// tap config1
    qma6100_writereg(0x2b, TAP_AXIS_Z|TAP_SHOCK_TH);        // tap config2



    if(enable)
    {
        reg_16 |= tap_reg;
        qma6100_writereg(0x16, reg_16);

        if(int_map == QMA6100_MAP_INT1)
        {
            if(tap_type & QMA6100_TAP_QUARTER)
            {
                reg_1a |= 0x02;
                qma6100_writereg(0x1a, reg_1a);
            }

            reg_19 |= tap_reg;
            qma6100_writereg(0x19, reg_19);

        }
        else if(int_map == QMA6100_MAP_INT2)
        {
            if(tap_type & QMA6100_TAP_QUARTER)
            {
                reg_1c |= 0x02;
                qma6100_writereg(0x1c, reg_1c);
            }

            reg_1b |= tap_reg;
            qma6100_writereg(0x1b, reg_1b);
        }
    }
    else
    {    
        reg_16 &= (~tap_reg);
        qma6100_writereg(0x16, reg_16);

        if(int_map == QMA6100_MAP_INT1)
        {
            if(tap_type &QMA6100_TAP_QUARTER)
            {
                reg_1a &= (~0x02);
                qma6100_writereg(0x1a, reg_1a);
            }

            reg_19 &= (~tap_reg);
            qma6100_writereg(0x19, reg_19);

        }
        else if(int_map == QMA6100_MAP_INT2)
        {
            if(tap_type & QMA6100_TAP_QUARTER)
            {
                reg_1c &= (~0x02);
                qma6100_writereg(0x1c, reg_1c);
            }

            reg_1b &= (~tap_reg);
            qma6100_writereg(0x1b, reg_1b);

        }
    }
}
#endif

#if defined(QMA6100_HAND_RAISE_DOWN)
void qma6100_set_hand_up_down(int layout)
{
#if 1//defined(QMA7981_SWAP_XY)
    unsigned char reg_0x42 = 0;
#endif
    unsigned char reg_0x1e = 0;
    unsigned char reg_0x34 = 0;
    unsigned char yz_th_sel = 4;
    char y_th = -3; //-2;                // -16 ~ 15
    unsigned char x_th = 6;        // 0--7.5
    char z_th = 6;                // -8--7

#if 1//defined(QMA7981_SWAP_XY)    // swap xy
    if(layout%2)
    {
        qma6100_readreg(0x42, &reg_0x42, 1);
        reg_0x42 |= 0x80;        // 0x42 bit 7 swap x and y
        qma6100_writereg(0x42, reg_0x42);
    }
#endif

    if((layout >=0) && (layout<=3))
    {
        z_th = 3;
        if((layout == 2)||(layout == 3))
            y_th = 3; 
        else if((layout == 0)||(layout == 1))    
            y_th = -3;
    }
    else if((layout >=4) && (layout<=7))
    {
        z_th = -3;
        
        if((layout == 6)||(layout == 7))
            y_th = 3; 
        else if((layout == 4)||(layout == 5))    
            y_th = -3;
    }

    // 0x34 YZ_TH_SEL[7:5]    Y_TH[4:0], default 0x9d  (YZ_TH_SEL   4   9.0 m/s2 | Y_TH  -3  -3 m/s2)
    //qmaX981_writereg(0x34, 0x9d);    //|yz|>8 m/s2, y>-3 m/m2
    if((y_th&0x80))
    {
        reg_0x34 |= yz_th_sel<<5;
        reg_0x34 |= (y_th&0x0f)|0x10;
        qma6100_writereg(0x34, reg_0x34);
    }
    else
    {    
        reg_0x34 |= yz_th_sel<<5;
        reg_0x34 |= y_th;
        qma6100_writereg(0x34, reg_0x34);    //|yz|>8m/s2, y<3 m/m2
    }
    //Z_TH<7:4>: -8~7, LSB 1 (unit : m/s2)    X_TH<3:0>: 0~7.5, LSB 0.5 (unit : m/s2) 
    //qmaX981_writereg(0x1e, 0x68);    //6 m/s2, 4 m/m2

    qma6100_writereg(0x22, (0x19|(0x03<<6)));            // 12m/s2 , 0.5m/s2
    qma6100_writereg(0x23, (0x7c|(0x03>>2)));
    //qmaX981_writereg(0x2a, (0x19|(0x02<<6)));            // 12m/s2 , 0.5m/s2
    //qmaX981_writereg(0x2b, (0x7c|(0x02)));
    qma6100_writereg(0x25, 0x0e);          //50    
    //qmaX981_readreg(0x1e, &reg_0x1e, 1);
    if((z_th&0x80))
    {
        reg_0x1e |= (x_th&0x0f);
        reg_0x1e |= ((z_th<<4)|0x80);
        qma6100_writereg(0x35, reg_0x1e);
    }
    else
    {
        reg_0x1e |= (x_th&0x0f);
        reg_0x1e |= (z_th<<4);
        qma6100_writereg(0x35, reg_0x1e);
    }
}




void qma6100_hand_raise_down(int32_t int_map, int32_t enable)
{
    uint8_t reg_16,reg_19,reg_1b;

    qma6100_set_hand_up_down(0);
    
    qma6100_readreg(0x16, &reg_16, 1);
    qma6100_readreg(0x19, &reg_19, 1);
    qma6100_readreg(0x1b, &reg_1b, 1);

    // 0x24: RAISE_WAKE_TIMEOUT_TH<7:0>: Raise_wake_timeout_th[11:0] * ODR period = timeout count
    // 0x25: RAISE_WAKE_PERIOD<7:0>: Raise_wake_period[10:0] * ODR period = wake count
    // 0x26:
    // RAISE_MODE: 0:raise wake function, 1:ear-in function
    // RAISE_WAKE_PERIOD<10:8>: Raise_wake_period[10:0] * ODR period = wake count
    // RAISE_WAKE_TIMEOUT_TH<11:8>: Raise_wake_timeout_th[11:0] * ODR period = timeout count

    if(enable)
    {
        reg_16 |= (0x02);
        reg_19 |= (0x02);
        reg_1b |= (0x02);
        qma6100_writereg(0x16, reg_16);
        if(int_map == QMA6100_MAP_INT1)
            qma6100_writereg(0x19, reg_19);
        else if(int_map == QMA6100_MAP_INT2)
            qma6100_writereg(0x1b, reg_1b);
    }
    else
    {
        reg_16 &= ~((0x02|0x04));
        reg_19 &= ~((0x02|0x04));
        reg_1b &= ~((0x02|0x04));
        qma6100_writereg(0x16, reg_16);
        qma6100_writereg(0x19, reg_19);
        qma6100_writereg(0x1b, reg_1b);
    }
}
#endif

void qma6100_irq_hdlr(void)
{
    uint8_t ret = QMA6100_FAIL;
    uint8_t databuf[4] = {0};
    int32_t retry = 0;

    while((ret==QMA6100_FAIL)&&(retry++<10))
    {
        ret = qma6100_readreg(QMA6100_INT_STAT0, databuf, 4);
        if(ret == QMA6100_SUCCESS)
        {
            break;
        }
    }
    if(ret == QMA6100_FAIL)
    {
        QMA6100_LOG("qma6100_irq_hdlr read status fail!");
        return;
    }
    else
    {
        //QMA6100_LOG("irq [0x%x 0x%x 0x%x 0x%x]", databuf[0],databuf[1],databuf[2],databuf[3]);
    }

#if defined(QMA6100_DATA_READY)
    if(databuf[2]&0x10)
    {
        qma6100_read_raw_xyz(g_qma6100.raw);
        QMA6100_LOG("drdy    %d    %d    %d",g_qma6100.raw[0],g_qma6100.raw[1],g_qma6100.raw[2]);
    }
#endif
#if defined(QMA6100_FIFO_FUNC)
    if(databuf[2]&0x20)
    {
        QMA6100_LOG("FIFO FULL");
        qma6100_read_fifo(qma6100_fifo_reg);
        qma6100_exe_fifo(qma6100_fifo_reg);
    }
    if(databuf[2]&0x40)
    {
        //QMA6100_LOG("FIFO WMK");
        qma6100_read_fifo(qma6100_fifo_reg);
        qma6100_exe_fifo(qma6100_fifo_reg);
    }
#endif
#if defined(QMA6100_ANY_MOTION)
    if(databuf[0]&0x07)
    {
        QMA6100_LOG("any motion!");
    }
#if defined(QMA6100_SIGNIFICANT_MOTION)
    if(databuf[1]&0x01)
    {
        QMA6100_LOG("significant motion!");
    }
#endif
#endif
#if defined(QMA6100_NO_MOTION)
    if(databuf[0]&0x80)
    {
        QMA6100_LOG("no motion!");
    }
#endif
#if defined(QMA6100_STEP_INT)
    if(databuf[1]&0x08)
    {
        QMA6100_LOG("step int!");
    }
#endif
#if defined(QMA6100_SIGNIFICANT_STEP_INT)
    if(databuf[1]&0x40)
    {
        QMA6100_LOG("significant step int!");
    }
#endif
#if defined(QMA6100_TAP_FUNC)
    if(databuf[1]&0x80)
    {
        QMA6100_LOG("SINGLE tap int!");
    }
    if(databuf[1]&0x20)
    {
        QMA6100_LOG("DOUBLE tap int!");
    }
    if(databuf[1]&0x10)
    {
        QMA6100_LOG("TRIPLE tap int!");
    }    
    if(databuf[2]&0x01)
    {
        QMA6100_LOG("QUARTER tap int!");
    }
#endif
#if defined(QMA6100_HAND_RAISE_DOWN)
    if(databuf[1]&0x02)
    {
        QMA6100_LOG("hand raise!");
    }
    if(databuf[1]&0x04)
    {
        QMA6100_LOG("hand down!");
    }
#endif
}


uint8_t qma6100_drdy_read_raw_xyz(int32_t *data)
{
    uint8_t databuf[6] = {0};    
    int16_t raw_data[3];    
    uint8_t drdy = 0;
    int32_t ret;

    while(!drdy)
    {
        ret = qma6100_readreg(QMA6100_XOUTL, databuf, 6);
        if(ret == QMA6100_FAIL){
            QMA6100_ERR("read xyz error!!!");
            return 0;    
        }
        drdy = (databuf[0]&0x01) + (databuf[2]&0x01) + (databuf[4]&0x01);
        //QMA6100_LOG("drdy 0x%d ",drdy);
    }

    raw_data[0] = (int16_t)(((int16_t)(databuf[1]<<8))|(databuf[0]));
    raw_data[1] = (int16_t)(((int16_t)(databuf[3]<<8))|(databuf[2]));
    raw_data[2] = (int16_t)(((int16_t)(databuf[5]<<8))|(databuf[4]));
    data[0] = raw_data[0]>>2;
    data[1] = raw_data[1]>>2;
    data[2] = raw_data[2]>>2;

    return QMA6100_SUCCESS;
}

int32_t qma6100_read_raw_xyz(int32_t *data)
{
    uint8_t databuf[6] = {0};     
    int16_t raw_data[3];    
    int32_t ret;

    ret = qma6100_readreg(QMA6100_XOUTL, databuf, 6);
    if(ret == QMA6100_FAIL){
        QMA6100_ERR("read xyz error!!!");
        return QMA6100_FAIL;    
    }

    raw_data[0] = (int16_t)(((int16_t)(databuf[1]<<8))|(databuf[0]));
    raw_data[1] = (int16_t)(((int16_t)(databuf[3]<<8))|(databuf[2]));
    raw_data[2] = (int16_t)(((int16_t)(databuf[5]<<8))|(databuf[4]));
    data[0] = raw_data[0]>>2;
    data[1] = raw_data[1]>>2;
    data[2] = raw_data[2]>>2;

    return QMA6100_SUCCESS;
}

int32_t qma6100_read_acc_xyz(int32_t *accData)
{
    int32_t ret;
    int32_t rawData[3];

    ret = qma6100_read_raw_xyz(rawData);
    if(ret == QMA6100_SUCCESS)
    {
        accData[g_qma6100.cvt.map[0]] = g_qma6100.cvt.sign[0]*rawData[0];
        accData[g_qma6100.cvt.map[1]] = g_qma6100.cvt.sign[1]*rawData[1];
        accData[g_qma6100.cvt.map[2]] = g_qma6100.cvt.sign[2]*rawData[2];

        accData[0] = (accData[0]*9870)/(g_qma6100.lsb_1g);            //GRAVITY_EARTH_1000
        accData[1] = (accData[1]*9870)/(g_qma6100.lsb_1g);
        accData[2] = (accData[2]*9870)/(g_qma6100.lsb_1g);
    }
    return ret;
}


int32_t qma6100_soft_reset(void)
{
    uint8_t reg_0x11 = 0;
    uint8_t reg_0x33 = 0;
    uint32_t retry = 0;

//    QMA6100_LOG("qma6100_soft_reset");
    qma6100_writereg(0x36, 0xb6);
    qma6100_delay(5);
    qma6100_writereg(0x36, 0x00);
    qma6100_delay(100);

    retry = 0;
    while(reg_0x11 != 0x80)
    {
        qma6100_writereg(0x11, 0x80);
        qma6100_delay(2);
        qma6100_readreg(0x11,&reg_0x11,1);
//        QMA6100_LOG("confirm read 0x11 = 0x%x",reg_0x11);
        if(retry++ > 100)
            break;
    }
    
    // load otp
    qma6100_writereg(0x33, 0x08);
    qma6100_delay(5);
    
    retry = 0;
    while(reg_0x33 != 0x05)
    {
        qma6100_readreg(0x33,&reg_0x33,1);
//        QMA6100_LOG("confirm read 0x33 = 0x%x",reg_0x33);
        qma6100_delay(10);
        if(retry++ > 100)
            break;
    }
    
    return 0;
}




int32_t qma6100_initialize(enum qma6100_odr odr_config)
{
#if defined(QMA6100_INT_LATCH)
    uint8_t reg_data[4];
#endif

    qma6100_soft_reset();
    // config by peili
    //qma6100_set_mode_odr(QMA6100_MODE_ACTIVE, QMA6100_MCLK_500K, QMA6100_DIV_2048, QMA6100_LPF_0);
    //qma6100_set_range(QMA6100_RANGE_8G);
    qma6100_writereg(0x4a, 0x20);   //Must write 0x20 to this register after power on. Refer to section 6.3.
    qma6100_writereg(0x50, 0x51);   
    qma6100_writereg(0x56, 0x01);   //Must write 0x01 to this register after power on. Refer to section 6.3
    
    qma6100_set_range(QMA6100_RANGE_4G);
//#if defined(QMA6100_ODR_27)
//    qma6100_writereg(0x10, 0x06);
//#elif defined(QMA6100_ODR_55)
//    qma6100_writereg(0x10, 0x00);      //0x25 51K ODR50,0x20 25K ODR 50
//#elif defined(QMA6100_ODR_100)
//    qma6100_writereg(0x10, 0x00);
//#endif
//    qma6100_writereg(0x11, 0x85);//25.6kHz

     switch(odr_config)
	 {
		 case ODR_25:
		 {
			 qma6100_writereg(0x10, 0x05);      //25600/1024 25hz
			 qma6100_writereg(0x11, 0x85);//25.6kHz
			 break;
		 }
		 case ODR_50:
		 {
			 qma6100_writereg(0x10, 0x00);      //50hz   25600/512
			 qma6100_writereg(0x11, 0x85);//25.6kHz
			 break;
		 }
		 case ODR_100:
		 {
			 qma6100_writereg(0x10, 0x01);      //100hz   25600/256
			 qma6100_writereg(0x11, 0x85);//25.6kHz
			 break;
		 }
		 case ODR_150:
		 {
			 qma6100_writereg(0x10, 0x02);      //200hz   25600/256
			 qma6100_writereg(0x11, 0x85);//25.6kHz
			 break;
		 }
		 case ODR_200:
		 {
			 qma6100_writereg(0x10, 0x02);      //200hz   25600/128
			 qma6100_writereg(0x11, 0x85);//25.6kHz
			 break;
		 }
		 default :
		 {
			 break;
		 }
	 }
	
//    qma6100_writereg(0x11, 0x86);//12.8kHz
//    qma6100_writereg(0x11, 0x87);//6.4KHZ
//    qma6100_writereg(0x11, 0x04);//

#if defined(QMA6100_QST_USE_SPI)
    qma6100_writereg(0x4A, 0x20);  //force to SPI  interface
#else
    qma6100_writereg(0x4A, 0x28);  //force to  IIC interface
#endif

    //qma6100_writereg(0x46, 0x0f);    // for div mclk/2
    
#if defined(QMA6100_QST_USE_SPI)
    qma6100_writereg(0x11, 0x04);
    qma6100_delay(5);
    qma6100_writereg(0x20, 0xC5);  //   SPI interface , pull up  close
    qma6100_writereg(0x11, 0x84);
#else
    qma6100_writereg(0x20, 0x05);
#endif

    qma6100_delay(5);
    qma6100_writereg(0x5F, 0x80);
    qma6100_delay(5);
    qma6100_writereg(0x5F, 0x00);
    qma6100_delay(5);
    
    // config by peili
#if defined(QMA6100_DATA_READY)
    qma6100_drdy_config(QMA6100_MAP_INT1, QMA6100_ENABLE);
#endif

#if defined(QMA6100_FIFO_FUNC)
    qma6100_fifo_config(QMA6100_MAP_INT1, QMA6100_ENABLE);
#endif

#if defined(QMA6100_STEPCOUNTER)
    qma6100_stepcounter_config(QMA6100_ENABLE);
    #if defined(QMA6100_STEP_INT)
    qma6100_step_int_config(QMA6100_MAP_INT1, QMA6100_ENABLE);
    #endif    
    #if defined(QMA6100_SIGNIFICANT_STEP_INT)
    qma6100_sigstep_int_config(QMA6100_MAP_INT1, QMA6100_ENABLE);
    #endif
#endif

#if defined(QMA6100_ANY_MOTION)
    qma6100_anymotion_config(QMA6100_MAP_INT1, QMA6100_ENABLE);
#endif
#if defined(QMA6100_NO_MOTION)
    qma6100_nomotion_config(QMA6100_MAP_INT1, QMA6100_ENABLE);
#endif

#if defined(QMA6100_TAP_FUNC)
    qma6100_tap_config(QMA6100_TAP_DOUBLE|QMA6100_TAP_TRIPLE|QMA6100_TAP_QUARTER, QMA6100_MAP_INT1, QMA6100_ENABLE);
#endif

#if defined(QMA6100_HAND_RAISE_DOWN)
    qma6100_hand_raise_down(QMA6100_MAP_INT1, QMA6100_ENABLE);
#endif

#if defined(QMA6100_INT_LATCH)
    qma6100_writereg(0x21, 0x03);    // default 0x1c, step latch mode
    qma6100_readreg(0x09, reg_data, 4);
    QMA6100_LOG("read status=[0x%x 0x%x 0x%x 0x%x] ", reg_data[0],reg_data[1],reg_data[2],reg_data[3]);
#endif
    QMA6100_LOG("qma6100_initialize success \r\n");
    return QMA6100_SUCCESS;
}



int32_t qma6100_init(uint32_t odr_config)
{
    uint32_t ret = 0;
    uint8_t slave_addr[2] = {QMA6100_I2C_SLAVE_ADDR, QMA6100_I2C_SLAVE_ADDR2};
    uint8_t index=0;

    for(index=0; index<2; index++)
    {
        g_qma6100.chip_id = 0;
        g_qma6100.slave = slave_addr[index];
        g_qma6100.chip_id = qma6100_chip_id();
        if((g_qma6100.chip_id == 0xFA)||((g_qma6100.chip_id & 0xF0) == 0x90))
        {
            QMA6100_LOG("qma6100 find \n");
            break;
        }
    }
#if defined(QMA6100_FIX_IIC)
    qma6100_writereg(0x20, 0x45);
    g_qma6100.slave = QMA6100_I2C_SLAVE_ADDR;
#endif

    if((g_qma6100.chip_id == 0xFA)||((g_qma6100.chip_id & 0xF0) == 0x90)){
        ret = qma6100_initialize((enum qma6100_odr) odr_config);
        
    }
    g_qma6100.layout = 0;
    g_qma6100.cvt.map[0] = qst_map[g_qma6100.layout].map[0];
    g_qma6100.cvt.map[1] = qst_map[g_qma6100.layout].map[1];
    g_qma6100.cvt.map[2] = qst_map[g_qma6100.layout].map[2];
    g_qma6100.cvt.sign[0] = qst_map[g_qma6100.layout].sign[0];
    g_qma6100.cvt.sign[1] = qst_map[g_qma6100.layout].sign[1];
    g_qma6100.cvt.sign[2] = qst_map[g_qma6100.layout].sign[2];
    return ret;
}

void qma6100_standby_on(void)
{
    uint8_t reg_0x11 = 0;
//    uint8_t reg = 0;
    qma6100_readreg(0x11,&reg_0x11,1);
    reg_0x11 &= (~0x80); 
//    reg = reg_0x11;
    qma6100_writereg(0x11, reg_0x11);
//    qma6100_readreg(0x11,&reg_0x11,1);
//    if(reg != reg_0x11){
//        user_hal_led_write(LED_BLUE, 1);
//    }
}

void qma6100_standby_off(void)
{
    uint8_t reg_0x11 = 0;
//    uint8_t reg = 0;
    qma6100_readreg(0x11,&reg_0x11,1);
    reg_0x11 |= 0x80; 
//    reg = reg_0x11;
    qma6100_writereg(0x11, reg_0x11);
//    qma6100_readreg(0x11,&reg_0x11,1);
//    if(reg != reg_0x11){
//        user_hal_led_write(LED_RED, 1);
//    }
}

