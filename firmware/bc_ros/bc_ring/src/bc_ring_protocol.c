#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "bc_queue.h"
#include "bc_logger.h"
#include "bc_ble_modu_interface.h"
#include "bc_ring_protocol.h"
#include "bc_rtc.h"
#include "bc_pmic.h"
#include "bc_ring_power.h"
#include "bc_ppg.h"
#include "bc_gsensor.h"
#include "bc_power.h"

static uint8_t cmd_resData[24] = {0};
static uint8_t cmd_resData_len = 0;

//时间同步
static void cmd_set_time(const uint8* data, uint16 len)
{
    if(data[3] == 0)  //写时间
    {
        uint8_t time_respose_data[4] = {0};
        uint8_t unix_time[8] = {0};
        cmd_resData_len = 4;
        
        memcpy(unix_time, &data[4], 8);
        bc_rtc_time_set_uinx_time((*(uint64_t*)unix_time)/1000,0);
    }
    else if(data[3] == 1)  //读时间
    {
        uint64_t unix_time = bg_rtc_time_get_uinx_time();
        cmd_resData_len = 13;
        unix_time *= 1000;
        
        memcpy(&cmd_resData[4], (uint8_t *)&unix_time, 8);
        cmd_resData[12] = 0x8;
    }
    
    cmd_resData[0] = 0x0;
    cmd_resData[1] = data[1];
    cmd_resData[2] = data[2];
    cmd_resData[3] = data[3];
    
    bc_ring_ble_send(cmd_resData,cmd_resData_len);
}

//读版本号
static void cmd_get_version(const uint8* data, uint16 len)
{
    cmd_resData_len = 14;
    if(data[3] == 0)  //读软件版本
    {
        memcpy(&cmd_resData[4], SOFT_VERSION, 10);
    }
    else if(data[3] == 1)  //读硬件版本
    {
        memcpy(&cmd_resData[4], HARD_VERSION, 10);
    }
    
    cmd_resData[0] = 0x0;
    cmd_resData[1] = data[1];
    cmd_resData[2] = data[2];
    cmd_resData[3] = data[3];
    
    bc_ring_ble_send(cmd_resData,cmd_resData_len);
}

//电池管理
static void cmd_get_bat(const uint8* data, uint16 len)
{
    cmd_resData_len = 5;
    if(data[3] == 0)  //读取电池电量
    {
        charge_state_e charge_status = bc_pmic_get_charge_status(); 
        if(charge_status == 0)
        {
            cmd_resData[4] = bc_ring_get_batPercent();
        }
        else if(charge_status == 1)
        {
            cmd_resData[4] = 101; //充电中，电量无效
        }
        else
        {
            cmd_resData[4] = 102; //充电完成
        }
    }
    else if(data[3] == 1)  //读取电池充电状态
    {
        cmd_resData[4] = bc_pmic_get_charge_status();
    }
    
    cmd_resData[0] = 0x0;
    cmd_resData[1] = data[1];
    cmd_resData[2] = data[2];
    cmd_resData[3] = data[3];
    
    bc_ring_ble_send(cmd_resData,cmd_resData_len);
}

//工装测试
static void cmd_tool_test(const uint8* data, uint16 len)
{

    
    switch (data[3])
    {
        case 0x0: //读ppg id
        {
            cmd_resData_len = 5;
            cmd_resData[4] = bc_ppg_get_chipId();
        }
        break;
        
        case 0x1: //读加速度计 id
        {
            cmd_resData_len = 5;
            cmd_resData[4] = bc_gsensor_getId();
        }
        break;
        
        case 0x2: //读PMIC id
        {
            cmd_resData_len = 5;
            cmd_resData[4] = bc_pmic_get_id();
        }
        break;
        
        case 0x3: //读电压AD值
        {
            uint16_t vbat_tmp;
            cmd_resData_len = 6;
            vbat_tmp = bc_ring_get_batAdc();
            
            LOG("vbat_adc:%d \r\n",vbat_tmp);
            memcpy(&cmd_resData[4],&vbat_tmp,2);
        }
        break;
        
        case 0x8:   //单独点灯
        {
            cmd_resData_len = 4;
            bc_power_vled_on();
            if(data[4] == 1)
            {
                bc_ppg_ledOn(PPG_LED_GREEN);
            }
            else if(data[5] == 1)
            {
                bc_ppg_ledOn(PPG_LED_IRED);
            }
            else if(data[6] == 1)
            {
                bc_ppg_ledOn(PPG_LED_RED);
            }
            else
            {
                bc_power_vled_off();
                bc_ppg_ledOn(PPG_LED__OFF);
            }
        }
        break;
        
        case 0x9:   //重启
        {
            NVIC_SystemReset();
        }
        break;
        
        case 0x12:   //灰卡测试
        {
//            user_hal_ppg_init();
//            user_hal_vled_enable();  //led使能
//            hx3605_init(FT_GRAY_CARD_MODE,0);
//            get_GrayCardBuf((uint32_t *)&test_respose_data[4]);
//            test_respose_len = 24;
//            user_hal_vled_disable();  //led使能
        }
        break;
        
    }
    
    cmd_resData[0] = 0x0;
    cmd_resData[1] = data[1];
    cmd_resData[2] = data[2];
    cmd_resData[3] = data[3];
    
    bc_ring_ble_send(cmd_resData,cmd_resData_len);
}

//命令处理
void ble_recieved_cmd_packet(void)
{
    struct bc_ble_data_package ble_data_package = {0};
    
    while(bc_queue_dequeue(BC_QUEUE_TYPE_BLE_RECV,&ble_data_package) == true)
    {
        BC_LOG_HEX("re : ",ble_data_package.data,ble_data_package.data_length);
        switch(ble_data_package.data[2])
        {
            //时间同步
            case  CMD_SET_TIME:
                cmd_set_time(ble_data_package.data, ble_data_package.data_length);
            break;
            
            //读版本号
            case CMD_GET_VERSION:
                cmd_get_version(ble_data_package.data, ble_data_package.data_length);
            break;
            
            //电池管理
            case CMD_GET_BAT:
                cmd_get_bat(ble_data_package.data, ble_data_package.data_length);
            break;
//            
//            //实时温度采集
//            case CMD_GET_TEMP:
//                cmd_get_temp(req_buf[read_index % 5], req_buf_len[read_index % 5]);
//            break;
//            
//            //实时心率血氧测量
//            case CMD_GET_SPO:
//                cmd_get_spo(req_buf[read_index % 5], req_buf_len[read_index % 5]);
//            break;
//                     
//            //实时心率和心率变异性测量
//            case CMD_GET_HRV:
//                cmd_get_hrv(req_buf[read_index % 5], req_buf_len[read_index % 5]);
//            break;
//            
//            //实时加速度计数据
//            case CMD_GET_ACC:
//                cmd_get_acc(req_buf[read_index % 5], req_buf_len[read_index % 5]);
//            break;
//            
//            //实时步数
//            case CMD_GET_SPORT:
//                cmd_get_sport(req_buf[read_index % 5], req_buf_len[read_index % 5]);
//            break;
//            
//            //心率血氧测量历史记录
//            case CMD_GET_HISTORY:
//                cmd_get_history(req_buf[read_index % 5], req_buf_len[read_index % 5]);
//            break;
//            
            //工装测试
            case CMD_TOOL_TEST:
                cmd_tool_test(ble_data_package.data, ble_data_package.data_length);
            break;
//            
//            //系统设置
//            case CMD_SYS_SET:
//                cmd_sys_set(req_buf[read_index % 5], req_buf_len[read_index % 5]);
//            break;
//            default:break;
        }
    }
}

//ble发送
void bc_ring_ble_send(uint8_t* data,uint8_t data_len)
{
    struct bc_ble_calss  ble_calss = bc_ble_new();
    
    if(ble_calss.ble_connect_status())
    {
        ble_calss.ble_send(data,data_len);
    }
}
