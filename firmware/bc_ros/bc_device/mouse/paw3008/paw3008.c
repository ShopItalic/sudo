#include "paw3008.h"

#include "bc_mouse_device_port.h"
#include "bc_delay.h"

#include "stdio.h"



#define I2C_ADDRESS			0x53

/* control bits */
bool mcupolling_enh = 1;
bool data2uart_enh = 0;
bool OFN_HRD_enh = 0;
bool OFN_XYD_enh = 1;
bool OFN_CAP_enh = 0;
bool OFN_Init_enh = 1;

/* OFN Variables */  
unsigned char OFN_Init_OK = 0;
unsigned char OFN_Init_Retry = 0;
unsigned char OFN_HRD_Data[13] = {0};
unsigned char OFN_HRD_RptCnt = 0;
unsigned char OFN_HRD_ValidFrameCnt = 0;
unsigned char OFN_HRD_ET_LB = 0;
unsigned char OFN_HRD_ET_HB = 0;
unsigned char OFN_XYD_RptCnt = 0;
unsigned char OFN_CAP_RAW_LB = 0;
unsigned char OFN_CAP_RAW_HB = 0;

typedef struct
{
  	unsigned char motion;//bit7	
	int8_t dx;
	int8_t dy;
	unsigned char dir;//bit3~0	
	unsigned char touch;//bit7,4
	unsigned char click;//bit5,4
} OFN_XYD_t;
OFN_XYD_t OFN_Data;

#define AP_OPT_TOUCH	0x80
#define AP_CAP_TOUCH	0x10

static char hexChar[3];               // Hex characters with leading zero

typedef void (*mouse_x_y_callback)(int8_t x,int8_t y);
typedef void (*mouse_event_data_callback)(uint8_t* data,uint8_t length);

static mouse_x_y_callback x_y_callback = NULL;
static mouse_event_data_callback event_data_callback = NULL;

static void delay(uint16_t ms)
{
	bc_delay_ms(ms);
}

static void OFN_WriteReg(unsigned char address, unsigned char value) 
{
	uint8_t addr = (uint8_t)address;
	uint8_t val = (uint8_t)value;
	bc_mouse_i2c_write(I2C_ADDRESS,addr,&val,1);
}

static void OFN_MultiReadReg(unsigned char address, unsigned char num, unsigned char *buf)
{
	uint8_t addr = (uint8_t)address  & 0x7F;
	bc_mouse_i2c_read(I2C_ADDRESS,addr,buf,num);  
}

static unsigned char OFN_ReadReg(unsigned char address)
{
	unsigned char rdata = 0;   
	uint8_t addr = (uint8_t)address  & 0x7F;
	bc_mouse_i2c_read(I2C_ADDRESS,addr,(uint8_t*)&rdata,1); 

	return(rdata);  
}

static void OFN_WriteReadReg(unsigned char address, unsigned char wdata) 
{
	unsigned char rdata;

	do
	{
		OFN_WriteReg(address, wdata);// write data
		rdata = OFN_ReadReg(address);// read back
	} while(rdata != wdata);// check
}


void Hex8(unsigned char data)
{
  unsigned char first ;
  first = (data >> 4) | 48;
  if (first > 57) hexChar[0] = first + (unsigned char)39;
  else hexChar[0] = first ;
  
  first = (data & 0x0F) | 48;
  if (first > 57) hexChar[1] = first + (unsigned char)39;
  else hexChar[1] = first;
}

// Print in hex with leading zero
void PrintHex8(unsigned char data)
{
  Hex8(data);
  
//  printf("%d %d %d",hexChar[0],hexChar[1],hexChar[2]);
	printf("%s",hexChar);
}


bool OFN_Init(void)
{
	unsigned char OFN_pid=0;
	unsigned char try_cnt=0;//try 3 times
	bool read_id_ok=0;
	
	do
	{
		// Read OFN_pid in address 0x00 to check if the serial link is valid, it should be 0x33
		OFN_pid = OFN_ReadReg(0x00);
		if(OFN_pid == 0x33)
		{
			OFN_WriteReadReg(0x7F, 0x00);//bank0

			OFN_WriteReg(0x0A, 0x90);//software reset (i.e. set bit7=1). It will be reset to 0 automatically
			delay(20);//delay 20ms

//			delay(20);//delay 20ms
			
		    OFN_WriteReadReg(0x7F, 0x00);//bank0
			OFN_WriteReadReg(0x09, 0x5A);//disable write protect
			OFN_WriteReadReg(0x0C, 0x20);//set sensor orientation (depends on application)(added for EVK)
			OFN_WriteReadReg(0x0D, 0x10);//set X-asix=1000 CPI (depends on application)
			OFN_WriteReadReg(0x0E, 0x10);//set Y-asix=1000 CPI (depends on application)			
			OFN_WriteReadReg(0x14, 0x5F);
			OFN_WriteReadReg(0x55, 0xFF);//enable click
			
			OFN_WriteReadReg(0x7F, 0x01);//bank1
			OFN_WriteReadReg(0x08, 0x12);
			OFN_WriteReadReg(0x0A, 0xB6);
			OFN_WriteReadReg(0x0D, 0x40);
			OFN_WriteReadReg(0x11, 0x84);
			OFN_WriteReadReg(0x12, 0x7E);
//OFN_WriteReadReg(0x12, 0x42);
			OFN_WriteReadReg(0x13, 0x08);
			OFN_WriteReadReg(0x14, 0x60);
			OFN_WriteReadReg(0x17, 0x2B);		
			OFN_WriteReadReg(0x1B, 0xF3);			
			OFN_WriteReadReg(0x20, 0x10);
			OFN_WriteReadReg(0x45, 0x04);
			
			OFN_WriteReadReg(0x7F, 0x02);//bank2
			OFN_WriteReadReg(0x42, 0x12);
			OFN_WriteReadReg(0x45, 0x9C);
			OFN_WriteReadReg(0x47, 0x08);


			
			//Demo AP will write these Cap Settings
			if(OFN_CAP_enh)
			{
				OFN_WriteReadReg(0x7F, 0x00);
				OFN_WriteReadReg(0x37, 0x19);
				OFN_WriteReadReg(0x7F, 0x03);
				OFN_WriteReadReg(0x1E, 0x15);
//				OFN_WriteReadReg(0x12, 0x15);
				OFN_WriteReadReg(0x22, 0x03);
				OFN_WriteReadReg(0x23, 0xFF);
			}
			

			OFN_WriteReadReg(0x7F, 0x00);//switch to bank0	
			printf(" WriteReadReg:%x  OFN_ReadReg:%x   \r\n",0x00,OFN_ReadReg(0x7F));
			OFN_WriteReadReg(0x09, 0x00);//enable write protect
			printf(" WriteReadReg:%x  OFN_ReadReg:%x   \r\n",0x00,OFN_ReadReg(0x09));
			read_id_ok = 1;	
		}
		else
		{
			printf("Read OFN_pid = 0x%x \r\n",OFN_pid);
			try_cnt++;
			delay(1);
		}
	}while(read_id_ok==0 && try_cnt<=2);

	
	return read_id_ok;
} 

unsigned char OFN_CheckReg(void)//check 2 Registers of Bank0
{
	unsigned char reg0x00=0;
	unsigned char reg0x14=0;
	unsigned char result=0;
	
	reg0x00 = OFN_ReadReg(0x00);
	reg0x14 = OFN_ReadReg(0x14);

	if(reg0x00 != 0x33)
	{
		result=3;
	}	
	else if(reg0x00 == 0x33 && reg0x14 != 0x5F)
	{
		result=2;
	}	
	else if(reg0x00 == 0x33 && reg0x14 == 0x5F)
	{
		result=1;
	}	

	return result;
}


bool OFN_ReadXYD(void)// Read OFN data: motion,dx,dy,dir,opt_touch,click,cap_touch
{
	//register STATUS bit define
	#define OFN_STATUS_MOT			0x80
	#define OFN_STATUS_DIR			0x40
	#define OFN_STATUS_OPT_TOUCH	0x20
	#define OFN_STATUS_CAP_TOUCH	0x10
	#define OFN_STATUS_CLICK		0x02

	static unsigned char status_pre=0;
	unsigned char sensor_status=0;
	bool data_update_flag=0;
	
	//read Status
	if(OFN_CAP_enh)
	{
		sensor_status = OFN_ReadReg(0x02) & 0xF2;
	}
	else
	{
		sensor_status = OFN_ReadReg(0x02) & 0xE2;
	}
//	printf("sensor_status:%d \r\n",sensor_status);
	//check Motion bit
	if(sensor_status & OFN_STATUS_MOT)
	{
		OFN_Data.motion = sensor_status & OFN_STATUS_MOT;//mask Motion bit
		
		//read Movement
		OFN_Data.dx = OFN_ReadReg(0x03);
		OFN_Data.dy = OFN_ReadReg(0x04);
	}
	else
	{
		OFN_Data.motion=0;
		OFN_Data.dx=0;
		OFN_Data.dy=0;
	}

	//check Direction bit
	if(sensor_status & OFN_STATUS_DIR)
	{
		//read Direction
		OFN_Data.dir = OFN_ReadReg(0x05) & 0xF0;//mask Direction bits only
		OFN_Data.dir = OFN_Data.dir >> 4;//align report format with Demo AP 
	}
	else
	{
		OFN_Data.dir=0;
	}

	//check Opt_Touch bit
	if(sensor_status & OFN_STATUS_OPT_TOUCH)
	{	
		//read Opt_Touch
		OFN_Data.touch |= AP_OPT_TOUCH;//align report format with Demo AP
	}
	else
	{
		OFN_Data.touch &= ~AP_OPT_TOUCH;
	}
	
	//check Click bit
	if(sensor_status & OFN_STATUS_CLICK)
	{	
		//read Click
		OFN_Data.click = OFN_ReadReg(0x06) & 0x03;//mask Click bits only
		OFN_Data.click = OFN_Data.click << 4;//align report format with Demo AP 
	}
	else
	{
		OFN_Data.click=0;
	}

	if(OFN_CAP_enh)
	{
		//check Cap_Touch bit
		if(sensor_status & OFN_STATUS_CAP_TOUCH)
		{	
			OFN_Data.touch |= AP_CAP_TOUCH;//align report format with Demo AP
		}
		else
		{
			OFN_Data.touch &= ~AP_CAP_TOUCH;
		}

		//read Cap_Raw_Data (for testing only)
		OFN_CAP_RAW_LB = OFN_ReadReg(0x15);
		OFN_CAP_RAW_HB = OFN_ReadReg(0x16);
	}

	if(sensor_status || (sensor_status != status_pre) || OFN_CAP_enh)
	{
		data_update_flag=1;
	}	
	status_pre = sensor_status;

	return data_update_flag;

}

bool OFN_ReadHRD(void) 
{
	static unsigned long time_pre = 0, time_cur = 0;	
	unsigned char reg2 = 0;

	OFN_HRD_Data[0] = OFN_ReadReg(0x48) & 0x07;//check status: 0 is not ready, 1 is ready, 2 is loss one data
//    printf("OFN_HRD_Data[0] %d  \r\n",OFN_HRD_Data[0]);
	if(OFN_HRD_Data[0] == 0) 
	{
		return false;
	}
	else if(OFN_HRD_Data[0] >= 2) 
	{
		printf("Speed up polling time/check task loading \r\n");
		return false;
	}
	else
	{
		OFN_MultiReadReg(0x49,4,&OFN_HRD_Data[1]);
		OFN_MultiReadReg(0x4D,3,&OFN_HRD_Data[5]);

		OFN_HRD_Data[8] = OFN_HRD_ValidFrameCnt++;
		time_cur = bc_systick_get();//get system tick
		OFN_HRD_Data[9] = (unsigned char) (time_cur - time_pre);//time(uint:ms) in between each valid data ready.
		time_pre = time_cur;		
		OFN_HRD_Data[10] = 0;	
		OFN_HRD_Data[12] = OFN_ReadReg(0x3F);
		OFN_HRD_ET_LB = OFN_ReadReg(0x73);
		OFN_HRD_ET_HB = OFN_ReadReg(0x74);

		reg2 = OFN_ReadReg(0x02);
		if(reg2 & 0x20)
		{	
			OFN_HRD_Data[11] |= AP_OPT_TOUCH;//align report format with Demo AP
		}
		else
		{	
			OFN_HRD_Data[11] &= ~AP_OPT_TOUCH;
		}

		if(OFN_CAP_enh)
		{
			if(reg2 & 0x10)
			{	
				OFN_HRD_Data[11]  |= AP_CAP_TOUCH;//align report format with Demo AP
			}
			else
			{	
				OFN_HRD_Data[11] &= ~AP_CAP_TOUCH;
			}
			
			OFN_CAP_RAW_LB = OFN_ReadReg(0x15);
			OFN_CAP_RAW_HB = OFN_ReadReg(0x16);
		}
		
		return true;
	}
  
}


void OFN_StatusLED(void)//control LEFT_LED & MIDDLE_LED
{
	if(OFN_Data.dir >= 1 && OFN_Data.dir <= 8)//Dir event
	{	
//		DIR_LED_ON;
	}
	else
	{	
//		DIR_LED_OFF;
	}

	if(OFN_Data.touch & AP_OPT_TOUCH)//Opt_Touch event
	{	
//		OPT_TOUCH_LED_ON;
	}
	else
	{	
//		OPT_TOUCH_LED_OFF;
	}

	if(OFN_CAP_enh){
	if(OFN_Data.touch & AP_CAP_TOUCH)//Cap_Touch event
	{	
//		CAP_TOUCH_LED_ON;
	}
	else
	{	
//		CAP_TOUCH_LED_OFF;
	}
	}
	
	if(OFN_Data.click & 0x30)//Click event
	{	
//		CLICK_LED_ON;
	}
	else
	{	
//		CLICK_LED_OFF;
	}

}

void PrintDirection(unsigned char DIR)
{
	//align report format with Demo AP

	switch (DIR)// 4-direction mode: 1~4,   8-direction mode: 1~8
	{
	case 1:
		printf("UU\r\n");//up
		break;
	case 2:
		printf("DD\r\n");//down
		break;
	case 3:
		printf("LL\r\n");//left
		break;
	case 4:
		printf("RR\r\n");//right
		break;
	case 5:
		printf("UL\r\n");//up left
		break;
	case 6:
		printf("DL\r\n");//down left
		break;
	case 7:
		printf("UR\r\n");//up right
		break;
	case 8:
		printf("DR\r\n");//down right
		break;
	default:
		printf("NN\r\n");//none
		break;
	}
}

void PrintTouch(unsigned char Touch)
{
	printf("Touch:0x%x \r\n",Touch);
}



void OFN_ReportXYD(void)//report to Demo AP
{
	
	
	if(OFN_Data.dx == 0 && OFN_Data.dy == 0)
	{
		return ;
	}
//	printf("OFN-XYD : x:%02x  Y:%02x",OFN_Data.dx,OFN_Data.dy);
//	printf("OFN-XYD :\n");
//	PrintHex8(OFN_XYD_RptCnt++);
//	printf("\n");
//	PrintHex8(OFN_Data.motion);
//	printf("\n");
	printf("OFN-X");
	PrintHex8(OFN_Data.dx);
	printf("    ");
	printf("OFN-Y");
	PrintHex8(OFN_Data.dy);
	printf("\n");
//	PrintDirection(OFN_Data.dir);
//	printf("\n");
//	printf("touch ");
//	PrintTouch(OFN_Data.touch);
//	printf("\n");
//	PrintHex8(OFN_Data.click);
//	printf("\n");
//	PrintHex8(0);
//	printf("\n");
//	PrintHex8(OFN_CAP_RAW_LB);
//	printf("\n");
//	PrintHex8(OFN_CAP_RAW_HB);
//	printf("\n");
	if(x_y_callback != NULL)
	{
		x_y_callback(OFN_Data.dx,OFN_Data.dy);
	}
	if(event_data_callback != NULL)
	{
		event_data_callback((uint8_t*)&OFN_Data,sizeof(OFN_Data));
	}
}



void  OFN_ReportXY_handler(void)
{
	unsigned char check_result = 0;
	bool report_update_flag = 0;
	
	check_result = OFN_CheckReg();
//	printf("check_result:%d \n",check_result);
	if(check_result == 1)//Go on if check reg ok
	{
		report_update_flag = OFN_ReadXYD();		
		OFN_StatusLED();
//		printf("report_update_flag:%d \n",report_update_flag);
		if( report_update_flag)//report to UART
		{				
			OFN_ReportXYD();
		}	
		
		OFN_Init_Retry = 0;	
	}
	else if(check_result == 2)//OFN init again if check reg failed
	{
		OFN_Init_enh = 1;
		OFN_Init_Retry++;
		printf("OFN_Init Retry ");
//		Serial.println(OFN_Init_Retry);
	}	
	else if(check_result == 3)//OFN init again if check reg failed
	{
		//power off/on OFN
//		OFN_PWR_OFF;
//		delay(5);
//		OFN_PWR_ON;
//		delay(5);

//		OFN_Init_enh = 1;
//		OFN_Init_Retry++;
//		Serial.print("OFN_Init Retry ");
//		Serial.println(OFN_Init_Retry);
	}
}





bool mouse_x_y_callback_register_callback(void *callback)
{
	if(callback != NULL)
	{
		x_y_callback = callback;
		return true;
	}
	return false;
}

bool mouse_event_data_callback_register_callback(void *callback)
{
	if(callback != NULL)
	{
		event_data_callback = callback;
		return true;
	}
	return false;
}













