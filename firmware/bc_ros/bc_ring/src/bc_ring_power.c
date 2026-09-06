#include "bc_power.h"
#include "bc_ring_status.h"
#include "bc_delay.h"
#include "bc_logger.h"

uint16_t bc_ring_get_batAdc(void)
{
    uint16_t ring_batAdc = 0;
    if(bc_ring_get_ringStatus() == RING_MEASURE)
    {
        ring_batAdc = bc_power_get_batAdc() * 1000;
    }
    else
    {
        bc_power_vled_on();
        bc_delay_ms(10);
        ring_batAdc = bc_power_get_batAdc() * 1000;
        bc_power_vled_off();
    }
    
    BC_LOG_INFO("ring_batAdc = %d",ring_batAdc);
    return ring_batAdc;
}

//¶ÁµçÁ¿
//Vbat = V * 9.76 / 59.66
uint8_t bc_ring_get_batPercent(void)
{
    uint16_t bat_adc = bc_ring_get_batAdc();
    BC_LOG_INFO("vbat_adc:%d \r\n",(int)bat_adc);
#if (BATT_TYPE == 1)
    if(bat_adc > 654)
    {
        bat_adc = 654;
    }
    else if(bat_adc < 555)
    {
        bat_adc = 555;
    }
    
    if(bat_adc >= 631) //3.855V 60%
    {
        bat_adc = ((bat_adc - 631)*40) / 23 + 60;
    }
    else if(bat_adc >= 605 && bat_adc < 631) //3.698V 20%
    {
        bat_adc = ((bat_adc - 605)*40) / 26 + 20;
    }
    else if(bat_adc >= 598 && bat_adc < 605) //3.657V 10%
    {
        bat_adc = ((bat_adc - 598)*10) / 7 + 10;
    }
    else //3.4V 0%
    {
        bat_adc = ((bat_adc - 555)*10) / 43;
    }
#elif (BATT_TYPE == 0)
    if(bat_adc > 679)
    {
        bat_adc = 679;
    }
    else if(bat_adc < 530)
    {
        bat_adc = 530;
    }
    
    if(bat_adc >= 652)  //3.985V 80%
    {
        bat_adc = ((bat_adc - 652)*20) / 27 + 80;
    }
    else if(bat_adc >= 631 && bat_adc < 652) //3.855V 60%
    {
        bat_adc = ((bat_adc - 631)*20) / 21 + 60;
    }
    else if(bat_adc >= 605 && bat_adc < 631) //3.698V 20%
    {
        bat_adc = ((bat_adc - 605)*40) / 26 + 20;
    }
    else if(bat_adc >= 598 && bat_adc < 605) //3.657V 10%
    {
        bat_adc = ((bat_adc - 598)*10) / 7 + 10;
    }
    else //3.2V 0%
    {
        bat_adc = ((bat_adc - 530)*10) / 68;
    }
#endif
    
    return (uint8_t)bat_adc;
}
