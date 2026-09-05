#include "bc_fuel_gauge.h"
#include "bc_fuel_gauge_port.h"
#include "ring_config.h"
#include "bc_logger.h"
//#include "am_bsp_i2c.h"
#include "bc_delay.h"
#include "bc_device_info.h"

#define FUEL_GAUGE_DEVICE_CW2215EHBC

#if (defined FUEL_GAUGE_DEVICE_CW2215EHBC)
#include "Cellwise_CW221X_Driver_V1.2.1.h"

#define FUEL_GAUGE_ID 0xA0

#else
#define FUEL_GAUGE_ID 0xFF
#endif

static void bc_fuel_gauge_i2c_open(void)
{
    cw221x_i2c_open();
}


static void bc_fuel_gauge_i2c_close(void)
{
    cw221x_i2c_close();
}

bool bc_fuel_gauge_i2c_write(uint8_t slave_addr,uint8_t reg_add ,uint8_t *data,uint8_t length)
{
    if(cw221x_i2c_write(slave_addr,reg_add,(uint8_t*)data,length) == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool bc_fuel_gauge_i2c_read(uint8_t slave_addr,uint8_t reg_add ,uint8_t *data,uint8_t length)
{
    if(cw221x_i2c_read(slave_addr,reg_add,(uint8_t*)data,length) == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void bc_fuel_gauge_sleep(void)
{
//    int ret = 0, icnt = 5;
//    //bc_fuel_gauge_i2c_open();
//    do{
//        ret = cw221x_sleep();
//        if(!ret)
//            break;
//        else {
//            BC_LOG_INFO("bc_fuel_gauge_sleep ret: %d\r\n", ret);
//        }
//    } while(icnt--);
//    //bc_fuel_gauge_i2c_close();
//    if(!icnt)
//        BC_LOG_ERROR("bc_fuel_gauge_sleep fail!!!\r\n");
}

//void bc_fuel_gauge_active(void)
//{
//    int ret = 0, icnt = 5;
//    //bc_fuel_gauge_i2c_open();
//    do{
//        ret = cw221x_active();
//        if(!ret)
//            break;
//        else {
//            BC_LOG_INFO("bc_fuel_gauge_active ret: %d\r\n", ret);
//        }
//    } while(icnt--);
//    //bc_fuel_gauge_i2c_close();
//    if(!icnt)
//        BC_LOG_ERROR("bc_fuel_gauge_active fail!!!\r\n");
//}

//初始化
bool bc_fuel_gauge_init(void)
{
    bool res = 0;
#if (defined FUEL_GAUGE_DEVICE_CW2215EHBC)
    BC_LOG_INFO("bc_fuel_gauge_i2c_open\r\n");
    bc_fuel_gauge_i2c_open();
    bc_delay_ms(20);
    uint8_t vbattype = bc_device_info_get_vbat_type();
    if(vbattype >= BATT_TYPE_NUM)
        vbattype = 2;
    res = cw221x_bat_init(vbattype);
    bc_fuel_gauge_sleep();
    bc_delay_ms(5);
    bc_fuel_gauge_i2c_close();
    BC_LOG_INFO("bc_fuel_gauge_i2c_close\r\n");
#endif
    
    return res;
}

//读id
uint8_t bc_fuel_gauge_getId(void)
{
    uint8_t id = 0xFF;
#if (defined FUEL_GAUGE_DEVICE_CW2215EHBC)
    bc_fuel_gauge_i2c_open();
    bc_delay_ms(1);
    cw221x_get_chip_id((int*)&id);
    bc_fuel_gauge_sleep();
    bc_delay_ms(1);
    bc_fuel_gauge_i2c_close();
#endif
    
    return id;
}

bool bc_fuel_gauge_checkId(void)
{
    bool res;
    
    if(bc_fuel_gauge_getId() == FUEL_GAUGE_ID)
    {
        res = true;
    }
    else
    {
        res = false;
    }
    
    return res;
}

//读电量
uint8_t bc_fuel_gauge_getBattPer(void)
{
    uint8_t percent = 0;
    bc_fuel_gauge_i2c_open();
    bc_delay_ms(1);
    cw221x_get_capacity((int*)&percent);
    bc_fuel_gauge_sleep();
    bc_delay_ms(1);
    bc_fuel_gauge_i2c_close();
    
    return percent;
}
                             
void bc_fuel_gauge_getParameter(uint8_t *data)
{
    fuel_parameter_t fuel_parameter = {0};
    
    bc_fuel_gauge_i2c_open();
    bc_delay_ms(1);
    cw221x_get_capacity(&fuel_parameter.batt_per);
    cw221x_get_vol(&fuel_parameter.batt_vol);
    cw221x_get_current(&fuel_parameter.batt_cur);
    cw221x_get_temp(&fuel_parameter.batt_temp);
    cw221x_get_cycle_count(&fuel_parameter.batt_cycle_cnt);
    cw221x_get_soh(&fuel_parameter.batt_health);
    bc_fuel_gauge_sleep();
    bc_delay_ms(1);
    bc_fuel_gauge_i2c_close();
    
//    am_util_debug_printf("bc_fuel_gauge_getParameter : %d %d %d %d %d %d \r\n",fuel_parameter.batt_per,fuel_parameter.batt_vol,fuel_parameter.batt_cur, \
//                                                                                fuel_parameter.batt_temp,fuel_parameter.batt_cycle_cnt,fuel_parameter.batt_health);
    BC_LOG_INFO("per %d,vol %d,cur %ld,temp %d,cycle %d,health %d \r\n",fuel_parameter.batt_per,fuel_parameter.batt_vol,fuel_parameter.batt_cur, \
                                                                                fuel_parameter.batt_temp,fuel_parameter.batt_cycle_cnt,fuel_parameter.batt_health);
    memcpy(data,&fuel_parameter,sizeof(fuel_parameter_t));
}




