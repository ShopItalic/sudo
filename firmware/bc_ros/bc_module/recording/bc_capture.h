#ifndef BC_CAPTURE_H
#define BC_CAPTURE_H

#include "bc_recording.h"

/* One 880-sample mono PDM block produces 440 samples after the supplier's
 * decimation, then 220 ADPCM bytes. Only full DMA buffers enter the FIFO.
 * Eight blocks use 14,080 bytes, less than the supplier's two 4,400-sample
 * buffers (17,600 bytes). State calls are serialized by the adapter's short
 * PDM interrupt critical sections; encoding and storage run outside them. */
#define BC_CAPTURE_BUFFERS 8U
#define BC_CAPTURE_SAMPLES 880U
#define BC_CAPTURE_NONE 0xffU

typedef enum { BC_CAP_FREE, BC_CAP_DMA, BC_CAP_READY, BC_CAP_ENCODING } bc_cap_slot;

typedef struct {
    uint64_t id;
    uint32_t sequence;
    uint32_t sequences[BC_CAPTURE_BUFFERS];
    bc_cap_slot slots[BC_CAPTURE_BUFFERS];
    uint8_t ready[BC_CAPTURE_BUFFERS];
    uint8_t head;
    uint8_t count;
    bc_rec_result error;
    bool running;
    bool stop_requested;
    bool quiescent;
} bc_capture;

/* Begin is legal only with no DMA, queued, or encoding buffers still owned.
 * Allocation failure is a capture overflow, not permission to reuse a slot. */
bool bc_capture_begin(bc_capture *cap, uint64_t id);
uint8_t bc_capture_acquire_dma(bc_capture *cap, uint64_t id);
bool bc_capture_full(bc_capture *cap, uint64_t id, uint8_t slot);
bool bc_capture_stop(bc_capture *cap, uint64_t id);
/* Called only after the peripheral is disabled. DMA buffers returned by
 * STOPPED have no valid-sample count and must never be treated as full. */
bool bc_capture_quiesced(bc_capture *cap, uint64_t id, bool forced);
void bc_capture_fault(bc_capture *cap, uint64_t id, bc_rec_result error);
uint8_t bc_capture_take(bc_capture *cap, uint64_t id, uint32_t *sequence);
bool bc_capture_release(bc_capture *cap, uint64_t id, uint8_t slot);
bool bc_capture_drained(const bc_capture *cap, uint64_t id);

#endif
