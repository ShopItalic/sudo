
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "types.h"
#include "error.h"
#include "qma6100p.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define qma6100pP_USE_IIC               true//true: I2C, false: SPI
#define qma6100pP_SPI_DMA               false//true: SPI_DMA, false: SPI_IT

/******************************************************
 *                 Static Variables
 ******************************************************/
static uint8_t g_qmi6100p_address;
static void* g_pi2c;
int16_t rdata[64][3];
/* Private function prototypes -----------------------------------------------*/

#if (qma6100pP_USE_IIC)
/**
  * @brief i2c1 Initialization Function
  * @param None
  * @retval None
  */
static void qma6100p_i2c_init(const qma6100p_if_handle_t *p_if)
{

}

static int qma6100p_i2c_deinit(const qma6100p_if_handle_t *p_if)
{
  int ret;

  return ret;
}
#else
/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void qma6100p_spi_init(const qma6100p_if_handle_t *p_if)
{

}
#endif

static void qma6100p_int_event(GPIO_Pin_e pin,IO_Wakeup_Pol_e type)
{
//    LOG("qma6100p_int_event \r\n");
  qma6100p_device_t* p_dev = get_qma6100p_handle();

  if(pin == p_dev->hw_if->pin.int1)
  {

  }
  else if(pin == p_dev->hw_if->pin.int2)
  {

  }
}

static void qma6100p_if_init(const qma6100p_if_handle_t *p_if)
{
#if (qma6100pP_USE_IIC)
  g_qmi6100p_address = p_if->i2c_address;
  /* add this deinit before init to fix the first i2c read timeout issue. */
  qma6100p_i2c_deinit(p_if);
  qma6100p_i2c_init(p_if);
#else
  qma6100p_spi_init(p_if);
#endif

  if (p_if->pin.int1 != GPIO_DUMMY) 
        {
    //hal_gpio_init();
    hal_gpio_pin_init(p_if->pin.int1,IE);
    hal_gpio_pull_set(p_if->pin.int1,PULL_DOWN);
    hal_gpioin_register(p_if->pin.int1, qma6100p_int_event, NULL);
  }
  if (p_if->pin.int2 != GPIO_DUMMY) {
    //hal_gpio_init();
    hal_gpio_pin_init(p_if->pin.int2,IE);
    hal_gpio_pull_set(p_if->pin.int2,PULL_DOWN);
    hal_gpioin_register(p_if->pin.int2, qma6100p_int_event, NULL);
  }
}

#if (qma6100pP_USE_IIC)
static int qma6100p_i2c_read(uint8_t reg, uint8_t *pData, uint8_t size)
{
  if (g_pi2c == NULL) {
    qma6100p_LOG("Err(read): I2C has not been initialized.\n");
    return PPlus_ERR_INVALID_PARAM;
  }
}

static int qma6100p_i2c_write(uint8_t reg, uint8_t *pData, uint8_t size)
{
  if (g_pi2c == NULL) {
    qma6100p_LOG("Err(write): I2C has not been initialized.\n");
    return PPlus_ERR_INVALID_PARAM;
  }
}

#else

static int qma6100p_spi_read(uint8_t reg, uint8_t *pData, uint16_t size)
{
  int ret;


  return ret;
}

static int qma6100p_spi_write(uint8_t reg, uint8_t *pData, uint16_t size)
{
  int ret;

  return ret;
}
#endif

static ret_code_t qma6100p_read_byte(uint8_t reg, uint8_t *pData)
{
  int ret;
#if (qma6100pP_USE_IIC)
  ret = qma6100p_i2c_read(reg, pData, 1);
#else
  ret = qma6100p_spi_read(reg, pData, 1);
#endif
  return ret == PPlus_SUCCESS ? QMA_SUCCESS : QMA_ERROR;
}

static ret_code_t qma6100p_read_multi_byte(uint8_t reg, uint8_t *pData, uint8_t size)
{
  int ret;
#if (qma6100pP_USE_IIC)
  ret = qma6100p_i2c_read(reg, pData, (uint16_t)size);
#else
  ret = qma6100p_spi_read(reg, pData, (uint16_t)size);
#endif
  return ret == PPlus_SUCCESS ? QMA_SUCCESS : QMA_ERROR;
}

static ret_code_t qma6100p_write_byte(uint8_t reg, uint8_t val)
{
  int ret;
#if (qma6100pP_USE_IIC)
  ret = qma6100p_i2c_write(reg, &val, 1);
#else
  ret = qma6100p_spi_write(reg, &val, 1);
#endif
  return ret == PPlus_SUCCESS ? QMA_SUCCESS : QMA_ERROR;
}


/*
After power up , sensor will reload OTP automatically.
and if send 0xB6 to Reg 0x36 , it will trigger softwarereset,
and if you send 0x00 to Reg 0x36 a few cycles later ,  it will trigger OTP_LOADING too.

POWERUP  -  otpload

SOFTWARERESET (0x36 [0xb6 ,0x00])  - otpload

OTPLOAD (0x33 bit3 set '1') -otpload

*/
void softwarereset(void)
{
  uint8_t reg_read,cnt=0;
  qma6100p_LOG("SOFTWARE RESET \n");
  qma6100p_write_byte(0x36, 0xb6);
  WaitMs(1); // delay time can be very short  , such as the time between two i2c cmd.
  qma6100p_write_byte(0x36, 0x00);
  WaitMs(10);

  qma6100p_read_multi_byte(0x33,&reg_read,1);
  qma6100p_LOG("tim 0x33 = 0x%x\n",reg_read);
  while((reg_read&0x05)!=0x05)
  {
    qma6100p_read_multi_byte(0x33,&reg_read,1);
    WaitMs(5);
    cnt++;
    if((reg_read&0x05)==0x05)
    qma6100p_LOG("timeout cnt=%d, 0x33 = 0x%x\n",cnt,reg_read);
    if(cnt>=100)
    {
      qma6100p_LOG("Read 0x33 status timeout\n");
      break;
    }
  }
  WaitMs(10);

  qma6100p_LOG("SOFTWARE RESET END\n");
}

/*

it will compare all the resigers value with their default value.

*/
void ComparetoDefaultRegvalue(void)
{
  uint8_t i,reg;
  uint8_t cnt = 1;
  /*just for reference , some of it may be changed in anytime*/
  uint8_t reg_default_value[0x60]=
  {
  0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,/*0x00~0x0f*/
  /*Id-DX------------------------DZ,STCNT[~15],INTSTATE 1~~~~~~~4,STCNT[8],FIFCNT,RangeLPF*/
  0x00,0x00,0x14,0x7f,0x19,0x16,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0xA9,/*0x10~0x1f*/
  /*BW,PMcl,STEPCONFIG 0------3,INT_EN 0-----3,INT_MAP 0---------3,STEP_TAP_CONFIG*/
  0x05,0x00,0xD8,0x7C,0x00,0x81,0x02,0x00,0x00,0x00,0x05,0xCD,0x00,0x00,0x00,0x00,/*0x20~0x2f*/
  0x3F,0x00,0x00,0x05,0x9D,0x66,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x07,0x00,/*0x30-0x3F*/
                    /*0x37--------------NVM-------------0x3D*/
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,/*0x40-0x4F*/
  /*0x40------------NVM-------------0x48*/
  0x00,0x00,0x00,0xFF,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00/*0x50-0x5F*/
  /*0x58---------NVM---------0x5D*/
  };

  for(i=0;i<0x60;i++)
    {    
      if(i==0x37)
        i=0x3E;
      if(i==0x40)
        i=0x49;
      if(i==0x58)
        i=0x5F;
      qma6100p_read_multi_byte(i,&reg,1);
      WaitMs(1);
      if(reg!=reg_default_value[i]||i==0x33)
      qma6100p_LOG("Reg[0x%02x]=0x%02x%s",i,reg,(cnt++)%2?", ":"\n");
    }
  qma6100p_LOG("\n6100P Regvalue CMP end \r\n");
}

/*

change the mode of the chip  between STANDMODE and WAKEMODE
0x11 bit[7]

*/
void set_chip_mode(qma6100p_mode_t state)
{
  uint8_t reg;
  qma6100p_read_multi_byte(0x11,&reg,1);
  qma6100p_LOG("Last mode state=0x%x, ",reg);
  if(state == WAKEMODE)
    reg= (reg&0x7f)|0x80;
  else if(state == STANDBYMODE)
    reg= (reg&0x7f);
  else
  {
    qma6100p_LOG("Incorrect mode input =0x%x,readback state=0x%x\n",state,reg);
    return ;
  }
  qma6100p_LOG("Set mode to 0x%x\n",reg);
  qma6100p_write_byte(0x11, reg);

  // operate reg[0x5f] is mandatory when device was woken from standby mode.
  if(state == WAKEMODE)
  {
    qma6100p_write_byte(0x5f, 0x80);// enable testmode ,take control FSM
    WaitMs(1);
    qma6100p_write_byte(0x5f, 0x00);// normal mode
    WaitMs(1);
  }
}

/*

change the mainclock frequency of chip

512k ,307k,205,100k,51k,25k,12k,6k

0x11 bit[3:0]

*/
void set_Mclk(qma6100p_mclk_t mclk)
{
  uint8_t reg;
  qma6100p_read_multi_byte(0x11,&reg,1);
  //qma6100p_LOG("Last mainclock is 0x%x,",reg);
  if(mclk<8)
  {
    reg= (reg&0xf0) | mclk;
    qma6100p_write_byte(0x11, reg);
        qma6100p_LOG("new mainclock is 0x%x \r\n",reg);
  }
  else
  {
    qma6100p_LOG("mainclock input =0x%x is out of range!\n",mclk);
    return ;
  }
}

/*

change the range and filter
bit[3:0] range setting   2,4,8,16,32G
bit[6]   0 use LPF  ,  1 use HPF
LPF  : moveavg 1,2,4,8
HPF  : odr/10 /25 /50 /100 /200 /400 /800
*/
void set_range(qma6100p_range_t range, uint8_t movingavgiir, lpfcf_t lpfcf, hpfcf_t hpfcf)
{
  uint8_t reg,reg_lpfrange;
  if(((range&(range-1))!=0) || range>RANGE_32G)
  {
    range =RANGE_2G;
    qma6100p_LOG("Range is invalid,use default 2G \n");
  }

  reg = range;
  qma6100p_read_multi_byte(0x0F,&reg_lpfrange,1);

  reg = reg|(reg_lpfrange&0x30)|movingavgiir;
  qma6100p_write_byte(0x0F, reg);
  qma6100p_LOG("Range is %dG,0x0f=0x%x\n",range*2,reg);

  if(movingavgiir==HPF)
  {
    qma6100p_read_multi_byte(0x10,&reg,1);
    reg = (reg&0x1F)|hpfcf;
    qma6100p_write_byte(0x10, reg);
    qma6100p_LOG("FILTER 0x10=0x%x\n",reg);
  }
  else if(movingavgiir==LPF)
  {
    qma6100p_read_multi_byte(0x10,&reg,1);
    reg = (reg&0x1F)|lpfcf;
    qma6100p_write_byte(0x10, reg);
    qma6100p_LOG("FILTER 0x10=0x%x\n",reg);
  }
}

/*
0x3f  get fifo data from it
0x3e  fifo-mode ,fifo-enz-eny-enx
0x31 set watermark
0x1a 0x1c map to int1 int2
*/
void set_map_fifo(uint8_t wm,uint8_t mode,uint8_t fifo_int_type,int_port_t port,axis_sel_t axis_sel)
{
  uint8_t reg;

  qma6100p_read_multi_byte(0x3E,&reg,1);
  reg=(mode)|(axis_sel)|(reg&0x38);
  qma6100p_write_byte(0x3E, reg);
  qma6100p_LOG("FIFO CFG 0x3E=0x%x,",reg);

  if(mode==0)
    wm=0;
  qma6100p_write_byte(0x31, wm);
  qma6100p_LOG("FIFO WM 0x31=0x%x\n",reg);

  //if(wm!=0)
  //{
    if(FIFO_INT_FULL==fifo_int_type)
      reg=0x20;//fifo wmk int
    else if(FIFO_INT_WATERMARK==fifo_int_type)
      reg=0x40;
    qma6100p_write_byte(0x17, reg);
  //}
    
#if 1
  qma6100p_read_multi_byte(0x16,&reg,1);  
  reg=(1<<3)|(reg&0xF7);
  qma6100p_write_byte(0x16, reg);        //使能计步中断

  qma6100p_read_multi_byte(0x12,&reg,1); //0x89   
  reg=(1<<7)|(reg&0x7F);
  qma6100p_write_byte(0x12, reg);        //使能计步

  //qma6100p_read_multi_byte(0x13,&reg,1);
  qma6100p_write_byte(0x13, 0x80);  //清计步器
  WaitMs(1);
  qma6100p_write_byte(0x13, 0x7F);  //设计步精度
  qma6100p_write_byte(0x14, 0x0B);  //设计步最小窗口时间
  qma6100p_write_byte(0x15, 0x0C);//0x32  //设离开计步最大窗口时间
  qma6100p_write_byte(0x1F, 0x40); //this is for counting animal steps from every step. //计步算法参数
#endif
    
  if(port==1)
  {
    qma6100p_read_multi_byte(0x1a,&reg,1); 
    if(fifo_int_type == FIFO_INT_WATERMARK)
      reg=(0x40)|(reg&0x9f);
    else if(fifo_int_type == FIFO_INT_FULL)
      reg=(0x20)|(reg&0x9f);
    qma6100p_write_byte(0x1a, reg); 
  }
  else if(port==2)
  {
    qma6100p_read_multi_byte(0x1c,&reg,1); 
    if(fifo_int_type == FIFO_INT_WATERMARK)
      reg=(0x40)|(reg&0x9f);
    else if(fifo_int_type == FIFO_INT_FULL)
      reg=(0x20)|(reg&0x9f);
    qma6100p_write_byte(0x1c, reg); 
  }
  else
  {
    qma6100p_LOG("Wrong FIFO port input %d!\n",port);
  }
}

/*
 2G 244uG/LSB  
 SEL=0 , for anythr 1LSB = 0.244mg*16=3.9mg/LSB
 SEL=1 , for anythr 1LSB = 0.244mg*32=7.8mg/LSB

 4G 488ug/LSB
 SEL=0 , 1LSB =7.8mg/LSB
 SEL=1 , 1LSB =15.6mg/LSB
 
 thr:  N mg   1G=1000mg
 duration : 0x2C bit[0~1]
*/
void set_anymotion(uint32_t thr,uint8_t duration,am_type_t slope,uint8_t port)
{
  uint8_t reg,reg_lpfrange;

  qma6100p_read_multi_byte(0x0F,&reg_lpfrange,1);   //读量程
  reg_lpfrange=reg_lpfrange&0x0F;
  if(reg_lpfrange==RANGE_2G)
  {
    thr = thr*10/39;
  }
  else if(reg_lpfrange==RANGE_4G)
  {
    thr = thr*10/78;
  }
  else if(reg_lpfrange==RANGE_8G)
  {
    thr = thr*10/156;
  }
  else if(reg_lpfrange==RANGE_16G)
  {
    thr = thr*10/312;
  }
  else if(reg_lpfrange==RANGE_32G)
  {
    thr = thr*10/625;
  }
  thr = thr +1 ; // close to the next value
  qma6100p_read_multi_byte(0x2F,&reg,1);    
  if( (reg&0x40)==AM_GRAVITY)  //中断触发是斜率or加速度
    thr=thr/2;
  else
    thr=thr/1;
  if(thr>255)
    thr=255;

  reg = (thr&0xff); // thr     1/G * 16 * 1e
  qma6100p_write_byte(0x2E, reg);      //设阈值

  qma6100p_read_multi_byte(0x2c,&reg,1);
  reg=(duration&0x03)|(reg&0xfc); 
  qma6100p_write_byte(0x2c, reg);    //设采样点个数

  qma6100p_read_multi_byte(0x18,&reg,1);
  reg = 0x07|(reg&0xe0); 
  qma6100p_write_byte(0x18, reg);    //使能xyz三轴中断
  qma6100p_LOG("Anymotion thr=0x%x,quiet=%d,reg_lpfrange=0x%x\n",thr,duration,reg_lpfrange);

  qma6100p_read_multi_byte(0x2f,&reg,1);
  reg = (slope)|(reg&0xBF); //    slope or highG
  qma6100p_write_byte(0x2f, reg);   //slope触发
  qma6100p_LOG("Anymotion thr=0x%x,quiet=%d\n",thr,duration);

#if 1
  qma6100p_read_multi_byte(0x16,&reg,1);  
  reg=(1<<3)|(reg&0xF7);
  qma6100p_write_byte(0x16, reg);        //使能计步中断

  qma6100p_read_multi_byte(0x12,&reg,1); //0x89   
  reg=(1<<7)|(reg&0x7F);
  qma6100p_write_byte(0x12, reg);        //使能计步

  //qma6100p_read_multi_byte(0x13,&reg,1);
  qma6100p_write_byte(0x13, 0x80);  //清计步器
  WaitMs(1);
  qma6100p_write_byte(0x13, 0x7F);  //设计步精度
  qma6100p_write_byte(0x14, 0x0B);  //设计步最小窗口时间
  qma6100p_write_byte(0x15, 0x0C);//0x32  //设离开计步最大窗口时间
  qma6100p_write_byte(0x1F, 0x40); //this is for counting animal steps from every step. //计步算法参数
#endif

  if(port==1)
  {    
    qma6100p_read_multi_byte(0x1a,&reg,1); 
    reg=(0x01)|(reg&0xfe);
    qma6100p_write_byte(0x1a, reg); 
    qma6100p_LOG(" Anymotion write to 1A =%x",reg); 
    qma6100p_read_multi_byte(0x1a,&reg,1);
    qma6100p_LOG(" Anymotion read  1A =%x\n",reg); 
    
  }
  else if(port==2)
  {
    qma6100p_read_multi_byte(0x1C,&reg,1); 
    reg=(0x01)|(reg&0xfe);
    qma6100p_write_byte(0x1C, reg); 
    qma6100p_LOG(" Anymotion write to 1C =%x",reg);
    qma6100p_read_multi_byte(0x1C,&reg,1);
    qma6100p_LOG(" Anymotion read  1C =%x\n",reg); 
  }
  else
  {
    qma6100p_LOG("Wrong Anymotion port input %d!\n",port);
  }
}

//使能/失能anymotion中断
void enable_disable_anymotion(uint8_t enable,uint8_t port)
{
    uint8_t reg_0x18 = 0;
    uint8_t reg_0x1a = 0;
    uint8_t reg_0x1c = 0;
    uint8_t int_status = 0;
    qma6100p_device_t* p_dev = get_qma6100p_handle();
    
  if (!p_dev->initialized) {
      qma6100p_LOG("\nqma6100pP has not been initialized.\n");
      return;
  }

#if (qma6100pP_USE_IIC)
  qma6100p_i2c_init(p_dev->hw_if);
#endif
    
    qma6100p_read_multi_byte(0x18, &reg_0x18, 1);
    qma6100p_read_multi_byte(0x1a, &reg_0x1a, 1);
    qma6100p_read_multi_byte(0x1c, &reg_0x1c, 1);
    
    if(enable)
    {
        reg_0x18 |= 0x07;
        reg_0x1a |= 0x01;
        reg_0x1c |= 0x01;

        qma6100p_write_byte(0x18, reg_0x18);    // enable any motion
        if(port==1)
            qma6100p_write_byte(0x1a, reg_0x1a);
        else if(port==2)
            qma6100p_write_byte(0x1c, reg_0x1c);
    }
    else
    {
        reg_0x18 &= (~0x07);
        reg_0x1a &= (~0x01);
        reg_0x1c &= (~0x01);
        
        qma6100p_read_multi_byte(0x09, &int_status,1); //清中断标志
        qma6100p_write_byte(0x18, reg_0x18);
        qma6100p_write_byte(0x1a, reg_0x1a);
        qma6100p_write_byte(0x1c, reg_0x1c);
    }
    
    qma6100p_i2c_deinit(p_dev->hw_if);
}

/*
time: Reg 0x2c bit[2-7]
time range : 1second~16seconds  , 20seconds~95seconds,100seconds~250seconds
LSB : 1 / [2^bit/ (2*Range)]
THR :  thr = CNT *16 * (LSB)
*/
void set_nomotion(uint8_t thr,uint8_t time,int_port_t port)
{
  uint8_t reg;

  qma6100p_write_byte(0x2d, thr); //thr  1/G * 16 * 15   2g -3.91mg/cnt  4g 7.82mg/cnt

  if(time>16&&time<20) time =16 ;
  if(time>95&&time<100) time =95 ;
  if(time>250) time =250;

  qma6100p_read_multi_byte(0x2c,&reg,1);
  if(time<=16)
    reg=((time-1)<<2)|(reg&0x03); //duration bit[7-3]
  else if (time <=95)
    reg=0x40|((time/5-4)<<2 )|(reg&0x03);
  else //if(time<250)
    reg=0x80|((time/10-10)<<2 )|(reg&0x03);

  qma6100p_LOG("no motion durtime=0x%x\n",reg);

  qma6100p_write_byte(0x2c, reg);

  qma6100p_read_multi_byte(0x18,&reg,1);
  reg = 0xe0|(reg&0x07);  
  qma6100p_write_byte(0x18, reg);

  if(port==1)
  {
    qma6100p_read_multi_byte(0x1a,&reg,1);
    reg=(0x80)|(reg&0x7f);
    qma6100p_write_byte(0x1a, reg);
  }
  else if(port==2)
  {
    qma6100p_read_multi_byte(0x1C,&reg,1);
    reg=(0x80)|(reg&0x7f);
    qma6100p_write_byte(0x1C, reg);
  }
  else
  {
    qma6100p_LOG("Wrong DreadyInt port input %d!\n",port);
  }
}

void set_dataReadyInt(uint8_t enable,int_port_t port)
{
  uint8_t reg;

  qma6100p_LOG("set_dataReadyInt Enable=%d \n",enable);
  qma6100p_read_multi_byte(0x17,&reg,1);
  if( ((reg&0x10)>>4) ==enable)
    return ;
  else if(enable==1)
  {
    reg=(reg&0xef)|0x10;
  }
  else if(enable==0)
  {
    reg=(reg&0xef)|0x00;
  }
  else
  {
    qma6100p_LOG("incorrect parameters,it shoule be 0 or 1 \n");
    return;
  }
  qma6100p_write_byte(0x17, reg);

  if(port==1)
  {    
    qma6100p_read_multi_byte(0x1a,&reg,1);
    reg=(0x10)|(reg&0xef);
    qma6100p_write_byte(0x1a, reg); 
  }
  else if(port==2)
  {
    qma6100p_read_multi_byte(0x1C,&reg,1);
    reg=(0x10)|(reg&0xef);
    qma6100p_write_byte(0x1C, reg); 
  }
  else
  {
    qma6100p_LOG("Wrong DreadyInt port input %d!\n",port);
  }
}

/*
type  :  N  =  N tap  N <=3

0x1e bit[0-5]  quiet thr,  31.25mg/cnt
0x2A bit7  tap quiet time  0:20ms,1:30ms
    bit6  tap shock time  0:75ms,1:50ms
    bit5  tap delay     0: 3tap not wait for 4tap  1: 3tap will wait for 4tap
    bit4  tap-ear-in    0: tap enable by 0x16  1: tap enable by 0x08[bit1]
    bit0-3 tap duration 0:100ms-150ms-200ms-250ms-300ms-400ms-500ms-700ms
0x2B bit7-6 tap-axis-sel  x,y,z,sqrt(x2+y2+z2)
    bit5-0 tap-shock-thr    31.25mg*CNT
*/
void set_map_tapInt(qma6100pP_tap_t type,uint8_t quietthr,uint8_t shockthr,int_port_t port)
{
  uint8_t reg;

  qma6100p_read_multi_byte(0x1e,&reg,1); 
  reg=quietthr|(reg&0xc0);;//quietthr|(reg&0xc0);
  qma6100p_write_byte(0x1e, reg); 

  /*  bit7 bit6 bit5 bit4*/
  reg=0x80|0x00|0x00|0x00|0x06;
  qma6100p_write_byte(0x2a, reg);

  reg=0xC0|shockthr; //  sqrt(xyz^2)|tap  shock  threshold
  qma6100p_write_byte(0x2b, reg);

  /*enable tap 1-2-3-4*/
  qma6100p_read_multi_byte(0x16,&reg,1);
  reg=type|(reg&0x4E);
  qma6100p_write_byte(0x16, reg);

  if(port==1)
  {
    qma6100p_read_multi_byte(0x1a,&reg,1);
    reg=((type&0x01)<<1)|(reg&0xfd);
    qma6100p_write_byte(0x1a, reg);
    qma6100p_read_multi_byte(0x19,&reg,1);
    reg=(type&0xB0)|(reg&0x4E);
    qma6100p_write_byte(0x19, reg);
  }
  else if(port==2)
  {
    qma6100p_read_multi_byte(0x1C,&reg,1);
    reg=((type&0x01)<<1)|(reg&0xfd);
    qma6100p_write_byte(0x1C, reg);
    qma6100p_read_multi_byte(0x1B,&reg,1);
    reg=(type&0xB0)|(reg&0x4E);
    qma6100p_write_byte(0x1B, reg);
  }
  else
  {
    qma6100p_LOG("Wrong tapInt port input %d!\n",port);
  }
}

uint16_t odr[8] ={13,25,50,100,200,400,800,1600};
uint8_t rateregs[8]={0x07,0x06,0x05,0x00,0x01,0x02,0x03,0x04};
/*set odr after set mCLK*/
void set_odr(uint16_t rate, qma6100p_mclk_t mclk)
{
  uint8_t reg,i;
  qma6100p_read_multi_byte(0x10,&reg,1); 
  reg=(reg&0xE0);

  for(i=0;i<8;i++)
  {
    if(rate<=odr[i])
    {
      break;
    }
  }
  if(i>7)
    i=7;
  reg = (reg&0xE0) |rateregs[i];
  qma6100p_write_byte(0x10, reg);
  qma6100p_LOG("Set desired ODR = %d ,real ODR = %d, 0x10 = 0x%x\n",rate,odr[i],reg);
}


void map_int_port(int_port_t port)
{
  if((port!=PORT_1) && (port!=PORT_2))
  {
    qma6100p_LOG("Invalid port %d ,Map to Port 1 !!\n",port);
    port = PORT_1;
  }
  if(port == PORT_1)
  {
    qma6100p_write_byte(0x19, 0xff);
    qma6100p_write_byte(0x1A, 0xf3);
    qma6100p_LOG("MAP ALL INT to Port %d\n",port);
    
  }
  else if(port == PORT_2)
  {
    qma6100p_write_byte(0x1B, 0xff);
    qma6100p_write_byte(0x1C, 0xf3);
    qma6100p_LOG("MAP ALL INT to Port %d\n",port);
  }
}

/*now use */
void set_map_stepInt(uint8_t enable,int_port_t port)
{
  uint8_t reg;

  if(enable>0)
    enable=1;
  qma6100p_read_multi_byte(0x16,&reg,1);  
  reg=(enable<<3)|(reg&0xF7);
  qma6100p_write_byte(0x16, reg);        //??????ж?

  qma6100p_read_multi_byte(0x12,&reg,1); //0x89   
  reg=(enable<<7)|(reg&0x7F);
  qma6100p_write_byte(0x12, reg);        //?????

  //qma6100p_read_multi_byte(0x13,&reg,1);
  qma6100p_write_byte(0x13, 0x80);  //??????
  WaitMs(1);
  qma6100p_write_byte(0x13, 0x7F);  //????????
  qma6100p_write_byte(0x14, 0x0B);  //??????С???????
  qma6100p_write_byte(0x15, 0x0C);//0x32  //???????????????
  qma6100p_write_byte(0x1F, 0x09); //this is for counting animal steps from every step. //?????????

  if(port==PORT_1)
  {
    qma6100p_read_multi_byte(0x19,&reg,1);
    reg=0x08|(reg&0xF7);;//quietthr|(reg&0xc0);
    qma6100p_write_byte(0x19, reg);
  }
  else if(port==PORT_2)
  {
    qma6100p_read_multi_byte(0x1B,&reg,1);
    reg=0x08|(reg&0xF7);;//quietthr|(reg&0xc0);
    qma6100p_write_byte(0x1B, reg);
  }
  else
  {
    qma6100p_LOG("Wrong StepInt port input %d!\n",port);
  }
}

/*
 set qma6100pP step count to zero ,step counter will recalculate the step from zero .
*/
void clear_step_cnt(void)
{
    qma6100p_device_t* p_dev = get_qma6100p_handle();
#if (qma6100pP_USE_IIC)
  qma6100p_i2c_init(p_dev->hw_if);
#endif    
    
  qma6100p_write_byte(0x13, 0x80);
  WaitMs(1);
  qma6100p_write_byte(0x13, 0x7F);
  qma6100p_LOG("RESET Current Stepcnt \n");
}

void set_map_raiseINT(uint8_t upenable,uint8_t downenable,int_port_t port)
{
  uint8_t reg;

  if(upenable>0)
    upenable=1;
  if(downenable>0)
    downenable=1;
  qma6100p_read_multi_byte(0x16,&reg,1);
  reg=(upenable<<1)|(downenable<<2)|(reg&0xF7);
  qma6100p_write_byte(0x16, reg);

  if(port==PORT_1)
  {
    qma6100p_read_multi_byte(0x19,&reg,1);
    reg=(upenable<<1)|(downenable<<2)|(reg&0xF9);
    qma6100p_write_byte(0x19, reg);
  }
  else if(port==PORT_2)
  {
    qma6100p_read_multi_byte(0x1B,&reg,1);
    reg=(upenable<<1)|(downenable<<2)|(reg&0xF9);
    qma6100p_write_byte(0x1B, reg);
  }
  else
  {
    qma6100p_LOG("Wrong StepInt port input %d!\n",port);
  }
}

void justFOR6100(void)
{
  //qma6100p_write_byte(0x50, 0x51); // 1 just for 6100
  qma6100p_write_byte(0x4A, 0x20); // enable  DWA of AFE
  qma6100p_write_byte(0x56, 0x01); // AFE_ATBP connected to pin 10, AFE_ATBM to pin 11
}

void get_dieID_WaferID(void)
{
  uint16_t dieid;
  uint8_t waferid;
  uint8_t reg[2];

  qma6100p_read_multi_byte(0x47,reg,2);
  dieid = (reg[1]<<8)|reg[0];
  qma6100p_LOG("dieID=0x%x,", dieid);
  qma6100p_read_multi_byte(0x5A,reg,1);
  waferid = reg[0]&0x3F;
  qma6100p_LOG("waferID=0x%x\n", waferid);
}

static bool qma6100p_int_is_disabled(void)
{
  uint8_t regVal[3] = {0,};
  uint16_t intEnReg = 0;
  bool intIsDisabled = false;
  qma6100p_read_multi_byte(0x16,regVal,3);
  intEnReg = regVal[0] + regVal[1] +regVal[2];
  qma6100p_LOG("intEn:%d \r\n",intEnReg);
  intIsDisabled = intEnReg>0?false:true;
  return intIsDisabled;
}

static ret_code_t qma6100pp_reg_init(const qma6100p_if_handle_t* p_if)
{
  uint8_t reg;

  qma6100p_LOG("%s \r\n", __FUNCTION__);
  qma6100p_write_byte(0x11, 0x80);
  softwarereset();

  // qma6100p_write_byte(0x11, 0x80);

  /*special setting*/
  justFOR6100();
  /*special setting end*/

  set_Mclk(MCLK_6KHZ);
    //set_Mclk(MCLK_51KHZ);
  set_range(RANGE_4G,LPF,LPCF_AVG4,HPCF_ODRDIV10);
  //set_odr(800,MCLK_6KHZ);   //6k,93.8
    set_odr(400,MCLK_6KHZ);   //6k,50
    //set_odr(50,MCLK_51KHZ); //51k,50
    //set_odr(1600,MCLK_51KHZ); //51k,1600

  set_anymotion(117,0,AM_SLOPE,PORT_1); //???0x10,INT1?ж?

  //set_nomotion(0x1e,5,PORT_2);
  //set_map_raiseINT(1,1,PORT_2);
  //set_map_stepInt(1,PORT_2);

  //set_map_fifo(10,FIFO_MODE_STREAM,FIFO_INT_FULL,PORT_2,(AXIS_X|AXIS_Y|AXIS_Z));

  //set_dataReadyInt(1,PORT_1);

  //set_map_tapInt(qma6100p_TAP_SINGLE|qma6100p_TAP_DOUBLE,3,5,PORT_2);


#ifdef qma6100pP_USE_IIC
  reg = 0x01; //bit0  STEP INT in non-latch mode, INT latched,spi 0x21 ,iic 0x01
#else
  reg = 0x21;
#endif
  qma6100p_write_byte(0x21, reg);

  reg=0x85;//|0x40; // active low~INT 1-2   //1 Why 10pin can be configured, for what ?
  qma6100p_write_byte(0x20, reg);// 10pin SENB dis or enable pullup resitor,SPI3-4,INT1-2 OD-PP Default Level 
  WaitMs(5);

  qma6100p_write_byte(0x46, 0x0f); // ultra low power(<4uA) setting
  qma6100p_LOG("qma6100pP ultra low power setting: reg[0x46]=0x0f\n");

  get_dieID_WaferID();
  set_chip_mode(WAKEMODE);

  ComparetoDefaultRegvalue();
  return QMA_SUCCESS;
}

static ret_code_t qma6100pp_probe(qma6100p_device_t* p_dev)
{
  uint8_t chip_id = 0;
  ret_code_t ret;

  ret = p_dev->hw_if->read_byte(0x00, &chip_id);
  qma6100p_LOG("chipID 0x%X \r\n", chip_id);
  if (ret == QMA_SUCCESS) {
    p_dev->dev_id = chip_id;
    if ((chip_id&0xF0) == 0x90) {
      return QMA_SUCCESS;
    }
  }
  return QMA_ERROR;
}

uint8_t qma6100pp_get_id(void)
{
    uint8_t chip_id = 0;
    qma6100p_device_t* p_dev = get_qma6100p_handle();
    qma6100p_i2c_init(p_dev->hw_if);

    qma6100p_read_byte(0x00, &chip_id);
    qma6100p_LOG("chipID 0x%X \r\n", chip_id);
    
    qma6100p_i2c_deinit(p_dev->hw_if);

  return chip_id;
}

/*qma6100p初始化*/
ret_code_t qma6100p_init(qma6100p_device_t* p_dev)
{
  ret_code_t ret = QMA_ERROR;

  qma6100p_if_init(p_dev->hw_if);

  if (qma6100pp_probe(p_dev) == QMA_SUCCESS) {
    qma6100pp_reg_init(p_dev->hw_if);
    p_dev->initialized = true;
    ret = QMA_SUCCESS;
  }

#if (qma6100pP_USE_IIC)
  if(qma6100p_int_is_disabled() || \
    ((p_dev->hw_if->pin.int1 == GPIO_DUMMY)&&(p_dev->hw_if->pin.int2 == GPIO_DUMMY))){
    qma6100p_i2c_deinit(p_dev->hw_if);
  }
#endif
  return ret;
}

ret_code_t qma6100p_data_read(int16_t *pdata, uint8_t size)
{
  qma6100p_device_t* p_dev = get_qma6100p_handle();
  uint8_t raw[6] = {0x00,};
  ret_code_t ret = QMA_ERROR;

  if (size<sizeof(raw)) {
      qma6100p_LOG("buffer overflow, size:%d, expected:%d\n", size, sizeof(raw));
      return ret;
  }

  if (!p_dev->initialized) {
      qma6100p_LOG("\nInt1: qma6100pP has not been initialized.\n");
      return ret;
  }

#if (qma6100pP_USE_IIC)
  qma6100p_i2c_init(p_dev->hw_if);
#endif

  ret = qma6100p_read_multi_byte(0x01, raw, sizeof(raw));
  *pdata = (int16_t)((raw[1]<<8) + raw[0])>>2;
  *(pdata+1) = (int16_t)((raw[3]<<8) + raw[2])>>2;
  *(pdata+2) = (int16_t)((raw[5]<<8) + raw[4])>>2;

#if (qma6100pP_USE_IIC)
  qma6100p_i2c_deinit(p_dev->hw_if);
#endif

  return ret;
}
uint32_t qma6100pp_get_steps(void)
{
    uint8_t int_status[6]={0,};
    uint8_t raw[6]={0,};
    uint8_t ret;
    uint32_t step_tmp = 0;
    qma6100p_device_t* p_dev = get_qma6100p_handle();

    if (!p_dev->initialized) {
      qma6100p_LOG("\nqma6100pP has not been initialized.\n");
      return 0;
    }
    qma6100p_i2c_init(p_dev->hw_if);
    
    qma6100p_read_multi_byte(0x09, int_status,6);
    ret = qma6100p_read_multi_byte(0x07, raw, 2);
    if(ret == QMA_SUCCESS) 
    {
      step_tmp = raw[0]|(raw[1]<<8)|int_status[4];
      qma6100p_LOG("stepCnt=%d \r\n", step_tmp);
    }
    
    qma6100p_i2c_deinit(p_dev->hw_if);
    
    return step_tmp;
}

void qma6100p_read_fifo(int16_t rdata[][3])
{
    uint8_t fifo_buf[384] = {0};
    uint8_t int_status[2]={0,};
    uint8_t reg,ret;
    uint8_t cntforint;
    uint8_t nbytes=0;
    uint16_t readbytes;
    uint8_t cnt1=0;
    uint16_t i,j =0;
    qma6100p_device_t* p_dev = get_qma6100p_handle();

    if (!p_dev->initialized) {
        qma6100p_LOG("\nqma6100pP has not been initialized.\n");
        return;
    }
    qma6100p_i2c_init(p_dev->hw_if);

    qma6100p_read_multi_byte(0x0b, int_status,1);
    qma6100p_read_multi_byte(0x0e, &int_status[1],1);
//    qma6100p_LOG("Int2 cnt,State[0x%x,0x%x,0x%x],framecnt=%d\n",int_status[0],int_status[1],int_status[2],int_status[5]);

#if 1
    if((int_status[0]&0x60)!=0x0)
    {
        cntforint = int_status[1];

        // check x y z enabled-axis //
        qma6100p_read_multi_byte(0x3E, &reg,1);
        reg = reg&0x07;
        if((reg&0x01)==0x01)
            nbytes+=1;
        if((reg&0x02)==0x02)
            nbytes+=1;
        if((reg&0x04)==0x04)
            nbytes+=1;
        // fifo-read-bytes  0~6 bytes

        readbytes=nbytes*2*cntforint;//6*cntforint;

        // platform don't support 384 bytes one time , so read it twice . Per your platform , you can read it in one cycle.
        if(readbytes>255)
        {
            cnt1 = readbytes-240;
            readbytes = 240;
        }
//        qma6100p_LOG("Port1-:cnt=%d,cnt1=%d,Datarow=%d\n",readbytes,cnt1,nbytes);

        qma6100p_read_multi_byte(0x3f,fifo_buf,readbytes);
        if(cnt1!=0) {
            qma6100p_read_multi_byte(0x3f,&fifo_buf[240],cnt1);
            // platform don't support 384 bytes one time , so read it twice . Per your platform , you can read it in one cycle.
        }

        for(i=0;i<(cntforint);i++)//(cnt+cnt1)/6
        {
            for(j=0;j<nbytes;j++)
            {
                rdata[i][j] = (int16_t)((fifo_buf[(1+j*2)+nbytes*2*i]<<8) | (fifo_buf[(0+j*2)+nbytes*2*i]))>>2;
            }
        }

        qma6100p_read_multi_byte(0x0E,&reg,1);
//        qma6100p_LOG("Port1: data left in fifo: %d\n",reg);

        /*clear framcnt when need to clear fifocnt quickly*/
//        qma6100p_read_multi_byte(0x31,&reg,1);
//        qma6100p_write_byte(0x31,reg);
        /*clear framcnt quickly if needed*/
    }
    else
    {
        qma6100p_LOG("Int2 cnt,State[0x%x],framecnt=%d\n",int_status[0],int_status[1]);
        qma6100p_LOG("Port1: clr state\n");
    }
#endif
}

ret_code_t qma6100p_demo(qma6100p_evt_hdl_t evt_hdl)
{
  qma6100p_device_t* p_dev = get_qma6100p_handle();
  //int16_t Acc[3] = {0x00,};

  if(!p_dev->initialized)
  {
    qma6100p_LOG("qma6100pP demo \r\n");
    if (qma6100p_init(p_dev) == QMA_SUCCESS) {
      p_dev->evt_hdl = evt_hdl;
      qma6100p_LOG("qma6100pP is initialized! \r\n");
      return QMA_SUCCESS;
    }
  }
  else
  {
     //qma6100p_data_read(Acc, sizeof(Acc));
     //qma6100p_LOG("X %d, Y %d, Z %d\n", Acc[0], Acc[1], Acc[2]);
     return QMA_SUCCESS;
  }
  return QMA_ERROR;
}

const qma6100p_if_handle_t qma6100p_if_cfg = {
  .type = if_i2c,
  .speed = i2c_speed_100K,
  .i2c_address = 0x12,  //0x12:AD0 ---GND; 0x13:AD0 ---VDD
  .pin = {
      .scl = P27, //P24,P25 cannot be pull-up
      .sda = P26,
      .int1 = P2,
      .int2 = GPIO_DUMMY,
  },
  .read_byte = qma6100p_read_byte,
  .read_multi_byte = qma6100p_read_multi_byte,
  .write_byte = qma6100p_write_byte,
};

static qma6100p_device_t qma6100p_dev = {
    .dev_id = 0x00,
    .initialized = false,
    .hw_if = &qma6100p_if_cfg,
};

qma6100p_device_t* get_qma6100p_handle(void)
{
  return &qma6100p_dev;
}
