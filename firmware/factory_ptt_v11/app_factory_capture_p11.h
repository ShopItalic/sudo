#ifndef APP_FACTORY_CAPTURE_P11_H
#define APP_FACTORY_CAPTURE_P11_H
#include "bc_rtos.h"
#include "nrfx_pdm.h"
bool p11_capture_start(const nrfx_pdm_config_t *config);
void p11_capture_stop(void);
void p11_capture_encoder_thread(void *argument);
void p11_capture_writer_thread(void *argument);
bool p11_capture_idle(void);

/* Debugger-visible, monotonic high-water evidence; no new BLE protocol. */
typedef struct {
    uint32_t workspace_bytes, pcm_bytes, queue_bytes, scratch_high_water;
    uint32_t heap_before, heap_after, max_encode_cycles, max_queue_records;
    uint32_t faults, allocations, completed_blocks;
} p11_capture_diagnostics;
extern volatile p11_capture_diagnostics p11_audio_diagnostics;

/* Storage adapter: only the existing storage worker calls these. */
bool p11_file_write(const uint8_t *data, unsigned length);
bool p11_file_rollover(void);
/* Short-recording ledger uses real 16 kHz sample counts, never codec bytes. */
bool factory_capture_p11_hold(void);
void factory_capture_p11_samples(unsigned samples, bool written);
#endif
