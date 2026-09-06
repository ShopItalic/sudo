#ifndef __APP_HID_HANDLER_H__
#define __APP_HID_HANDLER_H__



struct __attribute__((__packed__)) touch_hid_info
{
	unsigned int  photograph: 1;
	unsigned int short_video : 1;
	unsigned int music : 1;
	unsigned int ppt : 1;
	unsigned int up_audio : 1;
	unsigned int : 27;
	unsigned int : 32;
};

struct __attribute__((__packed__)) gesture_hid_info
{
	unsigned int  photograph: 1;
	unsigned int short_video : 1;
	unsigned int music : 1;
	unsigned int ppt : 1;
	unsigned int snap : 1;
	unsigned int : 28;
	unsigned int : 32;
};






void app_hid_photograth_start(void);

void app_hid_photograth_stop(void);


void app_hid_handler_init(void);







#endif


