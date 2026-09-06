
#include <stdint.h>

typedef enum{
    RING_INIT,
    RING_IDLE,
    RING_SHIPMODE,
    RING_MEASURE,
    RING_UPLOAD,
} ring_status_t;

ring_status_t bc_ring_get_ringStatus(void);
void bc_ring_state_change(ring_status_t new_state);
