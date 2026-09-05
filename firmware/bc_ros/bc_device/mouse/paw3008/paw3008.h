#ifndef __PAW3008_H__
#define __PAW3008_H__




#include <stdbool.h>

bool OFN_Init(void);


void  OFN_ReportXY_handler(void);


bool mouse_x_y_callback_register_callback(void *callback);

bool mouse_event_data_callback_register_callback(void *callback);



#endif



