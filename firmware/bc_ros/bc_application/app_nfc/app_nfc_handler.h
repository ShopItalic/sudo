#ifndef __APP_NFC_HANDLER_H__
#define __APP_NFC_HANDLER_H__





enum app_nfc_event_type
{
	APP_NFC_MONITOR_EVENT = 0,
	APP_NFC_IO_IRQ_HANDLER_EVENT,
	APP_NFC_EVENT_TYPE_NUM,
};

enum app_nfc_timer_type
{
	APP_NFC_POLL_MONITOR_TIMER = 0,
	APP_NFC_START_TIMER,
	APP_NFC_TIMER_TYPE_NUM,
};












void app_nfc_resoure_init(void);




#endif

















