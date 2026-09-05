#include "gxt310.h"


#include "bc_temp_port.h"
#include "bc_logger.h"
#include "bc_util.h"


#define GXT310X0     0x91
#define GXT310X1     0x93
#define GXT310X2     0x95
#define GXT310X3     0x97


static bool gxt310_i2c_write(uint8_t slave_addr,uint8_t reg_addr,uint8_t *write_data,uint8_t write_length)
{
	return bc_temper_i2c_write(slave_addr >> 1,reg_addr,write_data,write_length);
}

static bool gxt310_i2c_read(uint8_t slave_addr,uint8_t reg_addr,uint8_t *read_data,uint8_t read_length)
{
	return bc_temper_i2c_read(slave_addr >> 1 ,reg_addr,read_data,read_length);
}

void gxt310_temper_get_config_all_reg(uint8_t slave_addr)
{
	uint16_t config_data[4] = {0};
	
	BC_LOG_INFO("temper %d config data:",slave_addr);
	for(uint8_t i = 0; i < 4;i++)
	{
		gxt310_i2c_read(slave_addr,i,(uint8_t*)&config_data[i],2);
		printf(" %04x ",config_data[i]);
	}
	printf("\r\n");
}

void gxt310_temper_set_frequency_reg(uint8_t number)
{
	uint16_t temp = 0;
	uint8_t slave_addr = GXT310X0;
	
	for(uint8_t i = 0; i < number;i++)
	{
		temp = 0;
		gxt310_i2c_read(slave_addr,0x01,(uint8_t*)&temp,2);
		temp = 0xc200;
		gxt310_i2c_write(slave_addr,0x01,(uint8_t*)&temp,2);
		slave_addr+=2;
	}
}

void gxt310_temper_get_config_all_device(void)
{
	gxt310_temper_get_config_all_reg(GXT310X0);
	gxt310_temper_get_config_all_reg(GXT310X1);
	gxt310_temper_get_config_all_reg(GXT310X2);
	gxt310_temper_get_config_all_reg(GXT310X3);
}

float gxt310x0_temper_get(void)
{
//	bc_temper_device_i2c_open();
	 uint8_t buff[6] = {0};
	 int tem = 0;
	 float Temperature=0;
	 gxt310_i2c_read(GXT310X0,0x00,buff,2);
	 if(buff[0]&0x80)
    {
        tem= 0x10000 - ((buff[0] << 8) | buff[1] );       	
        Temperature = -(float)(tem * 0.0078125);
    }
    else
    {
        tem =(buff[0] << 8) | buff[1] ;
        Temperature=(float)(tem * 0.0078125);
    }
//	bc_temper_device_i2c_close();
    BC_LOG_INFO(" gxt310x0 temperature:%3.6f",Temperature);//111.01*C 100.01%（保留2位小数）	
    return Temperature;
}

float gxt310x1_temper_get(void)
{
//	bc_temper_device_i2c_open();
	 uint8_t buff[6] = {0};
	 int tem = 0;
	 float Temperature=0;
	 gxt310_i2c_read(GXT310X1,0x00,buff,2);
	 if(buff[0]&0x80)
    {
        tem= 0x10000 - ((buff[0] << 8) | buff[1] );       	
        Temperature = -(float)(tem * 0.0078125);
    }
    else
    {
        tem =(buff[0] << 8) | buff[1] ;
        Temperature=(float)(tem * 0.0078125);
    }
//	bc_temper_device_i2c_close();
    BC_LOG_INFO("gxt310x1 temperature:%3.6f",Temperature);//111.01*C 100.01%（保留2位小数）	
    return Temperature;
}

float gxt310x2_temper_get(void)
{
//	bc_temper_device_i2c_open();
	 uint8_t buff[6] = {0};
	 int tem = 0;
	 float Temperature=0;
	 gxt310_i2c_read(GXT310X2,0x00,buff,2);	
	 if(buff[0]&0x80)
    {
        tem= 0x10000 - ((buff[0] << 8) | buff[1] );       	
        Temperature = -(float)(tem * 0.0078125);
    }
    else
    {
        tem =(buff[0] << 8) | buff[1] ;
        Temperature=(float)(tem * 0.0078125);
    }
//	bc_temper_device_i2c_close();
    BC_LOG_INFO("gxt310x2 temperature:%3.6f",Temperature);//111.01*C 100.01%（保留2位小数）	
    return Temperature;
}

float gxt310x3_temper_get(void)
{
//	bc_temper_device_i2c_open();
	 uint8_t buff[6] = {0};
	 int tem = 0;
	 float Temperature=0;
	 gxt310_i2c_read(GXT310X3,0x00,buff,2);
	 if(buff[0]&0x80)
    {
        tem= 0x10000 - ((buff[0] << 8) | buff[1] );       	
        Temperature = -(float)(tem * 0.0078125);
    }
    else
    {
        tem =(buff[0] << 8) | buff[1] ;
        Temperature=(float)(tem * 0.0078125);
    }
//	bc_temper_device_i2c_close();
    BC_LOG_INFO("gxt310x3 temperature:%3.6f",Temperature);//111.01*C 100.01%（保留2位小数）	
    return Temperature;
}

uint8_t gxt310x0_temper_get_id(void)
{
	 uint16_t id = 0;
	uint8_t buff[2] = {0};
	 gxt310_i2c_read(GXT310X0,0x03,buff,2);
	id  =(buff[0] << 8) | buff[1] ;     //0x5000
    BC_LOG_INFO("gxt310x0 id:%04x",buff[0]);  // 0x5000
	return buff[0];
}

uint8_t gxt310x1_temper_get_id(void)
{
	 uint16_t id = 0;
	uint8_t buff[2] = {0};
	 gxt310_i2c_read(GXT310X1,0x03,buff,2);
	id  =(buff[0] << 8) | buff[1] ;     //0x5000
    BC_LOG_INFO("gxt310x1 id:%04x",buff[0]);  // 0x5000
	return buff[0];
}

uint8_t gxt310x2_temper_get_id(void)
{
	 uint16_t id = 0;
	uint8_t buff[2] = {0};
	 gxt310_i2c_read(GXT310X2,0x03,buff,2);
	id  =(buff[0] << 8) | buff[1] ;     //0x5000
    BC_LOG_INFO("gxt310x2 id:%04x",buff[0]);  // 0x5000
	return buff[0];
}

uint8_t gxt310x3_temper_get_id(void)
{
	 uint16_t id = 0;
	uint8_t buff[2] = {0};
	 gxt310_i2c_read(GXT310X3,0x03,buff,2);
	id  =(buff[0] << 8) | buff[1] ;     //0x5000
    BC_LOG_INFO("gxt310x3 id:%04x",buff[0]);  // 0x5000
	return buff[0];
}



//模式切换 0关断，1工作
void gxt310x0_switch_mode(bool mode)
{
    uint8_t buff[2] ={0,0xc0};
    gxt310_i2c_read(GXT310X0,0x01,buff,2);
    if(!mode)
    {
        buff[0] = 0x01;
        gxt310_i2c_write(GXT310X0,0x01,buff,2);
    }
    else
    {
        buff[0] = 0x00 ;
        gxt310_i2c_write(GXT310X0,0x01,buff,2);
    }
}


//模式切换 0关断，1工作
void gxt310x1_switch_mode(bool mode)
{
    uint8_t buff[2] ={0,0xc0};
    gxt310_i2c_read(GXT310X1,0x01,buff,2);
    if(!mode)
    {
        buff[0] = 0x01 ;
        gxt310_i2c_write(GXT310X1,0x01,buff,2);
    }
    else
    {
        buff[0] = 0x00;
        gxt310_i2c_write(GXT310X1,0x01,buff,2);
    }
}


//模式切换 0关断，1工作
void gxt310x2_switch_mode(bool mode)
{
    uint8_t buff[2] ={0,0xc0};
    gxt310_i2c_read(GXT310X2,0x01,buff,2);
    if(!mode)
    {
        buff[0] = 0x01 ;
        gxt310_i2c_write(GXT310X2,0x01,buff,2);
    }
    else
    {
        buff[0] = 0x00 ;
        gxt310_i2c_write(GXT310X2,0x01,buff,2);
    }
}


//模式切换 0关断，1工作
void gxt310x3_switch_mode(bool mode)
{
    uint8_t buff[2] ={0,0xc0};
    gxt310_i2c_read(GXT310X3,0x01,buff,2);
    if(!mode)
    {
        buff[0] = 0x01 ;
        gxt310_i2c_write(GXT310X3,0x01,buff,2);
    }
    else
    {
        buff[0] = 0x00;
        gxt310_i2c_write(GXT310X3,0x01,buff,2);
    }
}




































