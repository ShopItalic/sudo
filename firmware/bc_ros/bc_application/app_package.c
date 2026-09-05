#include "app_package.h"


#include "app_cmd_handler.h"
//#include "app_ppg_handler.h"
#include "app_ble_handler.h"
//#include "app_authentication_handler.h"
#include "bc_device_info.h"
#include "customer.h"

#include "bc_queue.h"
#include "bc_ble_modu_interface.h"
#include "stdio.h"

#include "bc_logger.h"

#include <string.h>

struct bc_ble_calss ble_calss;

#if ( defined(HANDWARE_1_19_1) )
#include "bc_wifi.h"
#endif 


/*******************************************************************************
 * Function Name     : app_package_send_enqueue
 * Description       : 数据包入队
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 *******************************************************************************/	
void app_package_send_enqueue(struct app_cmd_package * cmd_package,uint8_t length)
{
	struct bc_ble_data_package ble_package;
	memcpy(ble_package.data,(uint8_t*)cmd_package,length);
	ble_package.data_length = length;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);
}


/*******************************************************************************
 * Function Name     : app_package_ppg
 * Description       : ppg相关数据包组包
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
void app_package_ppg(struct app_cmd_package *package,void const *pack_pyload,uint16_t pyload_length,uint8_t pack_type)
{
	if(!ble_calss.ble_connect_status())
	{
		BC_LOG_INFO("ble null\r\n");
		return ;		
	}
	package->subcmd = pack_type;
	memcpy(package->data,(uint8_t*)pack_pyload,pyload_length);
	ble_calss.ble_send((uint8_t*)package,4+pyload_length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
	BC_LOG_HEX_P("ble send ppg:",(uint8_t*)package,4+pyload_length);
	BC_LOG_INFO("pyload_length:%d  \r\n",4+pyload_length);
}

void app_package_ppg_file_uplaod(struct app_cmd_package *package, uint16_t length)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}

  
  ble_calss.ble_send((uint8_t*)package,4+length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);

 
  
	
//	app_package_send_enqueue(package,4+pyload_length);
//	app_ble_task_event(BLE_TASK_TYPE_SEND);
}

void app_package_file_spi_uplaod(struct app_cmd_package *package, uint16_t length)
{

#if ( defined(HANDWARE_1_19_1) )
 bc_wifi_send((uint8_t*)package,4+length);
#endif 

  
//	app_package_send_enqueue(package,4+pyload_length);
//	app_ble_task_event(BLE_TASK_TYPE_SEND);
}

/*******************************************************************************
 * Function Name     : app_package_history_record_up
 * Description       : 历史记录相关数据包组包
 * Input             : 
 * Output            : 
 * Return            : 
 * Author            : 邱成凯
 * Modified Date:    : 2023年12月28日
 *******************************************************************************/	
void app_package_history_record_up(struct app_cmd_package *package,void const *pack_pyload,uint16_t pyload_length,uint32_t total_num,uint32_t seq)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	if(package->subcmd == 0xFF)
	{
		memcpy(&package->data[0],(uint8_t*)pack_pyload,pyload_length);
//		app_package_send_enqueue(package,4+pyload_length);
		
		ble_calss.ble_send((uint8_t*)package,4+pyload_length);
		app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
	}
	else
	{
		*(uint32_t*)&package->data = total_num;
		*(uint32_t*)&package->data[4] = seq;

		memcpy(&package->data[8],(uint8_t*)pack_pyload,pyload_length);
//		app_package_send_enqueue(package,4+8+pyload_length);
		ble_calss.ble_send((uint8_t*)package,4+8+pyload_length);
		app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
	}
//	app_ble_task_event(BLE_TASK_TYPE_SEND);
}

void app_package_pdm_switch_online_to_offline(void)
{
    struct bc_ble_data_package ble_package = {0};
    struct app_cmd_package *p_cmd = (struct app_cmd_package *)ble_package.data;
    p_cmd->frame_type = 0;
    p_cmd->frame_id = 0x09;
    p_cmd->cmd = 0x71;       // CMD_PDM
    p_cmd->subcmd = 0xFD;    // 内部子命令：在线转离线
    ble_package.data_length = 4;
    bc_queue_isr_enqueue(BC_QUEUE_TYPE_BLE_RECV, &ble_package);
}

void app_package_pdm_key_flag_clear(void)
{
    struct bc_ble_data_package ble_package = {0};
    struct app_cmd_package *p_cmd = (struct app_cmd_package *)ble_package.data;
    p_cmd->frame_type = 0;
    p_cmd->frame_id = 0x09;
    p_cmd->cmd = 0x71;       // CMD_PDM
    p_cmd->subcmd = 0xFC;    // 内部子命令：清除触摸PDM按键标志
    ble_package.data_length = 4;
    bc_queue_isr_enqueue(BC_QUEUE_TYPE_BLE_RECV, &ble_package);
  }

void app_package_authentication_req_code_up(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0x81;
	package.frame_id = 0x9;
	package.subcmd = 0x02;
	
//	app_authentcation_req_code_get(&package.data[1],&package.data[0]);
	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 + 1 + package.data[0]);
	ble_package.data_length = 4 + 1 + package.data[0];
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);

}

void app_package_sports_stop(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =0x38;
	package.frame_id = 0x9;
	package.subcmd = 0x03;
	
	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 );
	ble_package.data_length = 4 ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}
void app_package_mic_recording_start(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =0x71;
	package.frame_id = 0x9;
	package.subcmd = 0x05;
	package.data[0] = 1;
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +1);
	ble_package.data_length = 4 + 1 ;
	ble_package.data_length = 4 ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_active_upload_file_name(uint8_t *file_name,uint8_t name_length)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =0x36;
	package.frame_id = 0x9;
	package.subcmd = 0x1E;
	package.data[0] = name_length;
  memcpy(&package.data[1],file_name,name_length);
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +1+name_length);
	ble_package.data_length = 4 + 1+name_length ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);
}

void app_package_active_upload_check(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =CMD_IPC;
	package.frame_id = 0x9;
	package.subcmd = 0x00;
	memcpy(ble_package.data,(uint8_t*)&package,4 );
	ble_package.data_length = 4  ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_mic_recording_stop_isr(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =0x71;
	package.frame_id = 0x9;
	package.subcmd = 0xF9;  // 内部子命令：停止离线录音
	package.data[0] = 0;
	
	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +1);
	ble_package.data_length = 4 + 1 ;
	//bc_queue_isr_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
    bc_queue_isr_enqueue_not_yield(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_mic_recording_stop(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =0x71;
	package.frame_id = 0x9;
	package.subcmd = 0x05;
	package.data[0] = 0;
	
	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +1);
	ble_package.data_length = 4 + 1 ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_mic_capture_recording_start(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =0x71;
	package.frame_id = 0x9;
	package.subcmd = 0xFE;
	package.data[0] = 1;
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +1);
	ble_package.data_length = 4 + 1 ;
	ble_package.data_length = 4 ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_mic_capture_recording_stop(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =0x71;
	package.frame_id = 0x9;
	package.subcmd = 0xFE;
	package.data[0] = 0;
	
	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +1);
	ble_package.data_length = 4 + 1 ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_button_up(enum app_button_type button_type)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = CMD_BUTTON_UP;
	package.frame_id = 0x9;
	package.subcmd = 0x00;
	
	package.data[0] = (uint8_t)button_type;

	memcpy(ble_package.data,(uint8_t*)&package,4 + 1 );
	ble_package.data_length = 4 + 1 ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);

}

void app_package_button_rawdata_up(uint8_t *rawdata,uint8_t rawdata_length)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = CMD_BUTTON_UP;
	package.frame_id = 0x9;
	package.subcmd = 0x01;
	
	package.data[0] = rawdata_length;
	memcpy(&package.data[1],rawdata,rawdata_length);

	memcpy(ble_package.data,(uint8_t*)&package,4 + 1+ package.data[0] );
	ble_package.data_length = 4 + 1 + package.data[0] ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);

}

void app_package_button_rawdata_check_up(uint8_t *rawdata,uint8_t rawdata_length)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0xF2;
	package.frame_id = 0x9;
	package.subcmd = 0x25;
	
	package.data[0] = rawdata_length;
	memcpy(&package.data[1],rawdata,rawdata_length);

	memcpy(ble_package.data,(uint8_t*)&package,4 + 1+ package.data[0] );
	ble_package.data_length = 4 + 1 + package.data[0] ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);

}


void app_package_ppg_ir_midvalue_up(uint8_t seq,uint8_t midvalue_num,uint8_t *midvalue_data,uint8_t midvalue_length)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0x32;
	package.frame_id = 0x9;
	package.subcmd = 0x05;
	
	package.data[0] = midvalue_num;
	package.data[1] = seq;
	
	memcpy(&package.data[2],midvalue_data,midvalue_length);
	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 + 2 + midvalue_length);
	ble_package.data_length = 4 + 1 + midvalue_length;
	ble_calss.ble_send(ble_package.data,ble_package.data_length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
	 printf("ir_midvalue_up:");
	  for(uint8_t i = 0;i < ble_package.data_length;i++)
	  {
		printf("%02x ",ble_package.data[i]);
	  }
	  printf("\r\n");
//	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);
//	app_ble_task_event(BLE_TASK_TYPE_SEND);
}

void app_package_temper_up(uint8_t id,uint8_t status,uint16_t temper,uint8_t subcmd)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0x34;
	package.frame_id = id;
	package.subcmd = 0x00;
	
	package.data[0] = status;
	*(uint16_t*)&package.data[1] = temper;
	

	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 + 3);
	ble_package.data_length = 4 + 3;
	ble_calss.ble_send(ble_package.data,ble_package.data_length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
}

void app_package_ble_log_up(uint8_t *send_data,uint8_t length)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0xF2;
	package.frame_id = 0x2A;
	package.subcmd = 0x2A;
	
	package.data[0] = 1;
	package.data[1] = length;
	memcpy(&package.data[2],send_data,length);

//	app_package_send_enqueue(struct app_cmd_package * cmd_package,uint8_t length);
	
	memcpy(ble_package.data,(uint8_t*)&package,4 + 1+1+length);
	ble_package.data_length = 4 + 1+1+length;
	ble_calss.ble_send(ble_package.data,ble_package.data_length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
}

void app_package_mouse_event_up(uint8_t *send_data,uint8_t length)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0xF2;
	package.frame_id = 0x2A;
	package.subcmd = 0x33;
	
	package.data[0] = length;
	memcpy(&package.data[1],send_data,length);

//	app_package_send_enqueue(struct app_cmd_package * cmd_package,uint8_t length);
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +1+length);
	ble_package.data_length = 4 +1+length;
	ble_calss.ble_send(ble_package.data,ble_package.data_length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
}

#if 1
void app_package_precent_up(uint8_t data)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	if(!app_ble_notify_allowed())
	{
		return ;		
	}
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0x12;
	package.frame_id = 0x2A;
#if defined(FACTORY_USE)
	package.subcmd = 0x02;
#else
    package.subcmd = 0x00;
#endif
	
    package.data[0] = data;

	
	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +1);
	ble_package.data_length = 4 + 1;
	ble_calss.ble_send(ble_package.data,ble_package.data_length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);

}
#else
void app_package_precent_up(uint16_t data)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	if(!app_ble_notify_allowed())
	{
		return ;		
	}
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0x12;
	package.frame_id = 0x2A;
	package.subcmd = 0x00;
	
    *(uint16_t*)package.data = data;



	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +2);
	ble_package.data_length = 4 + 2;
	ble_calss.ble_send(ble_package.data,ble_package.data_length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);

}
#endif

void app_package_precent_status_up(uint16_t data)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	if(!app_ble_notify_allowed())
	{
		return ;		
	}
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0x12;
	package.frame_id = 0x2A;
	package.subcmd = 0x01;
	
    *(uint16_t*)package.data = data;



	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +2);
	ble_package.data_length = 4 + 2;
	ble_calss.ble_send(ble_package.data,ble_package.data_length);
	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);

}

 void app_ble_recv_enent(uint8_t *recv_data,uint16_t recv_length)
{
#if (HARDWARE_ARCH_TYPE_NORDIC == 1)	
	struct bc_ble_data_package ble_recv_pack;
	memcpy(ble_recv_pack.data,recv_data,recv_length);
    ble_recv_pack.data_length = recv_length;	
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_recv_pack);
	BC_LOG_HEX("ble recv:",ble_recv_pack.data,ble_recv_pack.data_length);
#endif	
	
}

void app_package_pdm_upload_over(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0x71;
	package.frame_id = 0x9;
	package.subcmd = 0x04;
	

	
	
	memcpy(ble_package.data,(uint8_t*)&package,4);
	ble_package.data_length = 4;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);
}

void app_package_speed_test_up(uint32_t seq,uint8_t leng)
{
	if(!ble_calss.ble_connect_status())
	{
		return ;		
	}
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0xF2;
	package.frame_id = 0x2A;
	package.subcmd = 0x3B;
	
    *(uint32_t*)package.data = seq;

    for(uint8_t i = 0; i < leng;i++)
	{
		package.data[4+i] = i;
	}

	
	
	memcpy(ble_package.data,(uint8_t*)&package,4 +4+leng);
	ble_package.data_length = 4 + 4 + leng;
	app_ble_send(ble_package.data,ble_package.data_length);
//	app_connect_idie_timer_start(BLE_CONNECT_IDIE_TIMEOUT_TIMER);
//	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_SEND,&ble_package);
//	app_ble_task_event(BLE_TASK_TYPE_SEND);
}


void app_package_mic_touch_start(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =CMD_IPC;
	package.frame_id = 0x9;
	package.subcmd = 0x01;
	memcpy(ble_package.data,(uint8_t*)&package,4 );
	ble_package.data_length = 4  ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_mic_touch_stop(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =CMD_IPC;
	package.frame_id = 0x9;
	package.subcmd = 0x02;
	memcpy(ble_package.data,(uint8_t*)&package,4 );
	ble_package.data_length = 4  ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_ipc_ic_led_ble_connect(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =CMD_IPC;
	package.frame_id = 0x9;
	package.subcmd = 0x03;
	memcpy(ble_package.data,(uint8_t*)&package,4 );
	ble_package.data_length = 4  ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_ipc_ic_led_ble_disconnect(void)
{
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd =CMD_IPC;
	package.frame_id = 0x9;
	package.subcmd = 0x04;
	memcpy(ble_package.data,(uint8_t*)&package,4 );
	ble_package.data_length = 4  ;
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

void app_package_enqueue(void *package_head,uint8_t package_head_length,uint8_t *package_pyload,uint16_t package_pyload_length)
{
	
}

void app_package_init(void)
{
	ble_calss = bc_ble_new();
}


void app_package_authentication_test_recv(void)
{
//	struct app_cmd_package package = {0};
//	struct bc_ble_data_package ble_package = {0};
//	package.cmd = 0x82;
//	package.frame_id = 0x9;
//	package.subcmd = 0x01;
//	static char temp[] = "test.aicaring.com/api/conweb/";
//	package.data[0] = strlen(temp);
//	memcpy(&package.data[1],temp,strlen(temp));
//	
//	memcpy(ble_package.data,(uint8_t*)&package,4 + 1 + package.data[0]);
//	ble_package.data_length = 4 + 1 + package.data[0];
//	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
//	app_ble_task_event(BLE_TASK_TYPE_RECV);
	
	struct app_cmd_package package = {0};
	struct bc_ble_data_package ble_package = {0};
	package.cmd = 0x62;
	package.frame_id = 0x9;
	package.subcmd = 0x00;
	static uint8_t temp[] = {0,1,0,1,0,1,0,1};;
	package.data[0] = 85;
	package.data[1] = sizeof(temp);
	memcpy(&package.data[2],temp,sizeof(temp));
	
	memcpy(ble_package.data,(uint8_t*)&package,4 + 2 + package.data[1]);
	ble_package.data_length = 4 + 2 + package.data[1];
	bc_queue_enqueue(BC_QUEUE_TYPE_BLE_RECV,&ble_package);
}

