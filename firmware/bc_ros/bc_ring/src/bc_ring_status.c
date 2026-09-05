#include "bc_ring.h"
#include "bc_pmic.h"
#include "bc_gsensor.h"

static ring_status_t ring_status = RING_INIT;

uint8_t bc_ring_get_ringStatus(void)
{
    return ring_status;
}

void bc_ring_init(void)
{
    bc_gsensor_init();
    bc_pmic_init();
}

void bc_ring_state_change(void)
{
    switch (ring_status)
    {
        case RING_INIT:
        {
            bc_ring_init();
            ring_status = RING_IDLE;
        }
        break;
        
        default:
            break;
    }
}
