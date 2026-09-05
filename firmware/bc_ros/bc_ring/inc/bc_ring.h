
#include <stdint.h>

typedef enum{
    RING_INIT,
    RING_IDLE,
    RING_SHIPMODE,
    RING_MEASURE,
    RING_UPLOAD,
} ring_status_t;

uint8_t bc_ring_get_ringStatus(void);
