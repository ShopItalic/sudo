#include "bc_ring.h"
#include "bc_ble_modu_interface.h"
#include "bc_gsensor.h"
#include "bc_pmic.h"
#include "bc_queue.h"
#include "bc_rtc.h"
#include "bc_power.h"
#include "bc_ppg.h"

void bc_init(void)
{
    struct bc_ble_calss ble_calss = bc_ble_new();
    ble_calss.ble_init();
    bc_rtc_device_find();
    bc_queue_init();
    bc_ppg_device_find();
    bc_g_sensor_device_find();
    bc_pmic_device_find();
    bc_power_batAdc_find();    
}