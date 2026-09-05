#ifndef __APP_HARDLINE_TSDB_HANDLER_H__
#define __APP_HARDLINE_TSDB_HANDLER_H__


#include "stdint.h"

#include "app_cmd_handler.h"

#pragma pack (1)
struct app_hardline_struct
{
  uint32_t timer;
  uint8_t type;
};
#pragma pack ()


void app_hardline_upload_tsdb(struct app_cmd_package *package);

void app_hardline_clear(void);
void app_hardline_tsdb_create(void);

void app_hardline_mark(void);








#endif




