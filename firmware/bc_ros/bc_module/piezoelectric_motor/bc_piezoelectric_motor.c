#include "bc_piezoelectric_motor.h"

#include "bc_piezoelectric_motor_port.h"
#include "bc_ldo_switch.h"

#include <stdint.h>

#include "bc_logger.h"
#include "bc_delay.h"

#include "bos1921.h"
#include "string.h"

#define piezoelectric_motor_addr   (0x44 << 1)

static struct bos1921_slice_parameters slice_parameters = {0};

void bc_piezoelectric_motor_chip_id(void)
{
	uint8_t temp[2] = {0};
	bc_ldo_motor_power_on();
	bc_piezoelectric_motor_i2c_open();
	bc_delay_ms(200);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x1E,(uint8_t*)temp,2);
	
    bc_piezoelectric_motor_i2c_close();
	bc_ldo_motor_power_off();
	BC_LOG_INFO("piezoelectric_motor_chip_id:%02x %02x\r\n",temp[0],temp[1]);
}


uint8_t bc_piezoelectric_motor_chip_id_get(void)
{
	uint8_t temp[2] = {0};
	bc_ldo_motor_power_on();
	bc_piezoelectric_motor_i2c_open();
    bc_delay_ms(200);
	temp[0] = 0x1E;
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x1E,(uint8_t*)temp,1);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x1E,(uint8_t*)temp,2);
	
    bc_piezoelectric_motor_i2c_close();
	bc_ldo_motor_power_off();
	BC_LOG_INFO("piezoelectric_motor_chip_id:%02x %02x\r\n",temp[0],temp[1]);
	return  temp[1];
}

bool bc_piezoelectric_motor_chip_id_check(void)
{
	if(bc_piezoelectric_motor_chip_id_get() != 0x40)
	{
		return false;
	}
	return true;
}

static uint8_t count = 0;
void bc_piezoelectric_motor_test_play(void)
{
	bc_ldo_motor_power_on();
	bc_piezoelectric_motor_i2c_open();
    bc_delay_ms(20);
	
	uint8_t write_temp[12] = {0};
	uint8_t read_temp[3] = {0};
	slice_parameters.amplitude = 0xFFF;
	slice_parameters.cycles = 20;
	slice_parameters.frequency = 0x33;
	slice_parameters.mode = 1;
	
	if(count == 0)
	{
		count = 1;
		
		write_temp[0] = 0x06;
		write_temp[1] = 0x10;		
		bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
			
		write_temp[0] = 0x00;
		write_temp[1] = 0x01;	
		write_temp[2] = 0x00;
		write_temp[3] = 0x2D;
//		write_temp[4] = 0xFF;  //振幅
//		write_temp[5] = 0x0F;  //振幅
//		write_temp[6] = 0x0A;
//		write_temp[7] = 0x1a;	
//		write_temp[8] = 0x04;
//		write_temp[9] = 0x00;
//        slice_parameters.amplitude &= 0xfff;		
		memcpy(&write_temp[4],(uint8_t*)&slice_parameters,6);
		bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,10);
		
		write_temp[0] = 0x00;
		write_temp[1] = 0x01;	
		write_temp[2] = 0x00;
		write_temp[3] = 0x00;
		write_temp[4] = 0x00;
		write_temp[5] = 0x2D;     //电压
		write_temp[6] = 0x00;
		write_temp[7] = 0x2F;	
		write_temp[8] = 0x00;
		write_temp[9] = 0x60;		  //周期次数
		bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,10);
		
	}
	
	
	
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x12;
	write_temp[2] = 0x00;	
	write_temp[3] = 0x00;		
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,4);
	
	write_temp[0] = 0x06;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x06;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	 bc_piezoelectric_motor_i2c_close();
	bc_ldo_motor_power_off();
}

void bc_piezoelectric_motor_play_tdk(void)
{
//	bc_ldo_motor_power_on();
//	 bc_delay_ms(100);
	bc_piezoelectric_motor_i2c_open();
    bc_delay_ms(100);
	
	uint8_t write_temp[102] = {0};
	uint8_t read_temp[3] = {0};
	slice_parameters.amplitude = 0xFFE;
	slice_parameters.cycles = 0x20;
	slice_parameters.frequency = 0x1A;
	slice_parameters.mode = 1;
	

	
	write_temp[0] = 0x06;
	write_temp[1] = 0x00;		
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x14;
	write_temp[2] = 0x00;	
	write_temp[3] = 0x00;
	write_temp[4] = 0x00;
write_temp[5] = 0x30;
write_temp[6] = 0x00;
write_temp[7] = 0x2D;
write_temp[8] = 0x00;
write_temp[9] = 0x2F;
write_temp[10] = 0x00;   //count
write_temp[11] = 05;  //count
write_temp[11+84+1] = 0x0A;  //AMPLITUDE
write_temp[11+84+2] = 0x1A;
write_temp[11+84+3] = 0x58;  //CYCLES 
write_temp[11+84+4] = 0x33; //FREQUENCY
write_temp[11+84+5] = 0x04;
write_temp[11+84+6] = 0x00;

	memcpy(&write_temp[11+84+1],(uint8_t*)&slice_parameters,6);

	
	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,102);
	bc_delay_ms(20);

	write_temp[0] = 0x00;
	write_temp[1] = 0x12;
	write_temp[2] = 0x00;	
	write_temp[3] = 0x00;
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,4);
	

	
	write_temp[0] = 0x06;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);


	
	 bc_piezoelectric_motor_i2c_close();
//	bc_ldo_motor_power_off();
}


void bc_piezoelectric_motor_init(void)
{
//	bc_ldo_motor_power_on();
//	 bc_delay_ms(100);
	bc_piezoelectric_motor_i2c_open();
    bc_delay_ms(100);
	
	uint8_t write_temp[104] = {0};
	uint8_t read_temp[3] = {0};
	
//	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,104);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x1E;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	bc_delay_us(150);
	write_temp[0] = 0x00;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x1E,(uint8_t*)write_temp,2);
	bc_delay_us(60);
	write_temp[0] = 0x00;
	write_temp[1] = 0x05;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	write_temp[0] = 0x00;
	write_temp[1] = 0x05;
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	bc_delay_us(150);
	write_temp[0] = 0x30;
//	write_temp[0] = 0x06;
	write_temp[1] = 0x40;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	bc_delay_us(1000*10);
	write_temp[0] = 0x00;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x1E,(uint8_t*)write_temp,2);
	bc_delay_us(255);
	write_temp[0] = 0x00;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x01;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x02;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x03;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x04;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x05;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x06;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x07;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x08;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x09;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x0A;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x0B;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x0F;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x11;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x18;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x1B;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x1E;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x1F;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	bc_delay_us(2000);
	
	write_temp[0] = 0x10;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x0F;
	write_temp[1] = 0x5D;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x01,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x0B;
	write_temp[1] = 0x6A;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x02,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x80;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x03,(uint8_t*)write_temp,2);
	
	
	write_temp[0] = 0x02;
	write_temp[1] = 0x60;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x04,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x06,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x0F;
	write_temp[1] = 0xC1;
//	write_temp[1] = 0x1C;
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x07,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x8B;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x85;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	
	write_temp[0] = 0x30;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);

	

	bc_piezoelectric_motor_i2c_close();
//	bc_ldo_motor_power_off();
}



void bc_piezoelectric_motor_test(void)
{
	bc_ldo_motor_power_on();
	bc_piezoelectric_motor_i2c_open();
    bc_delay_ms(100);
	
	uint8_t write_temp[3] = {0};
	uint8_t read_temp[3] = {0};
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x1E;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	bc_delay_us(150);
	write_temp[0] = 0x00;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x1E,(uint8_t*)write_temp,2);
	bc_delay_us(60);
	write_temp[0] = 0x00;
	write_temp[1] = 0x05;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	bc_delay_us(150);
	write_temp[0] = 0x10;
//	write_temp[0] = 0x06;
	write_temp[1] = 0x40;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	bc_delay_us(1000*10);
	write_temp[0] = 0x00;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x1E,(uint8_t*)write_temp,2);
	bc_delay_us(255);
	write_temp[0] = 0x00;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x01;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x02;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x03;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x04;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x05;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x06;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x07;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x08;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x09;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x0A;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x0B;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x0F;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x11;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x18;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x1B;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x1E;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x1F;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	bc_delay_us(2000);
	
	write_temp[0] = 0x10;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x0F;
	write_temp[1] = 0x5D;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x01,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x0B;
	write_temp[1] = 0x6A;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x02,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x80;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x03,(uint8_t*)write_temp,2);
	
	
	write_temp[0] = 0x02;
	write_temp[1] = 0x60;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x04,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x06,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x0F;
	write_temp[1] = 0xC1;
//	write_temp[1] = 0x1C;
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x07,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);
	bc_piezoelectric_motor_i2c_read(piezoelectric_motor_addr,0x17,(uint8_t*)read_temp,2);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x8B;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x0B,(uint8_t*)write_temp,2);

	

	bc_piezoelectric_motor_i2c_close();
	bc_ldo_motor_power_off();
}



void bc_piezoelectric_motor_play(struct bc_slice_parameters *slice_parameters)
{
	bc_ldo_motor_power_on();
	bc_piezoelectric_motor_i2c_open();
    bc_delay_ms(20);
	
	
	
	uint8_t write_temp[12] = {0};
	uint8_t read_temp[3] = {0};
	
	bc_piezoelectric_motor_test();
	bc_delay_ms(200);
		
		write_temp[0] = 0x06;
		write_temp[1] = 0x10;		
		bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
			
		write_temp[0] = 0x00;
		write_temp[1] = 0x01;	
		write_temp[2] = 0x00;
		write_temp[3] = 0x2D;
	
		memcpy(&write_temp[4],(uint8_t*)&slice_parameters->slice_parameters,6);
		bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,10);
		
		write_temp[0] = 0x00;
		write_temp[1] = 0x01;	
		write_temp[2] = 0x00;
		write_temp[3] = 0x00;
		write_temp[4] = 0x00;
		write_temp[5] = 0x2D;     //电压
		write_temp[6] = 0x00;
		write_temp[7] = 0x2F;	
		write_temp[8] = 0x00;
		write_temp[9] = slice_parameters->slice_count;		  //周期次数
		bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,10);
		
	bc_delay_ms(200);
	
	
	
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x12;
	write_temp[2] = 0x00;	
	write_temp[3] = 0x00;		
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,4);
	
	write_temp[0] = 0x06;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x06;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	bc_delay_ms(200);
	
	write_temp[0] = 0x00;
	write_temp[1] = 0x12;
	write_temp[2] = 0x00;	
	write_temp[3] = 0x00;		
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x00,(uint8_t*)write_temp,4);
	
	write_temp[0] = 0x06;
	write_temp[1] = 0x10;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	write_temp[0] = 0x06;
	write_temp[1] = 0x00;	
	bc_piezoelectric_motor_i2c_write(piezoelectric_motor_addr,0x05,(uint8_t*)write_temp,2);
	
	 bc_piezoelectric_motor_i2c_close();
}

static bool motor_check = false;
void bc_piezoelectric_motor_check(void)
{
	bc_ldo_motor_power_on();
	bc_piezoelectric_motor_i2c_open();
    bc_delay_ms(20);
	if(!motor_check)
	{
		bc_piezoelectric_motor_test();
		bc_delay_ms(200);
	}
	bc_piezoelectric_motor_test_play();
	
	bc_piezoelectric_motor_i2c_close();
	bc_ldo_motor_power_off();
}

