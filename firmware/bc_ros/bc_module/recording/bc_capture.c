#include "bc_capture.h"

#include <limits.h>
#include <string.h>

static bool current(const bc_capture *cap, uint64_t id)
{
    return cap != NULL && id != 0U && cap->id == id;
}

void bc_capture_fault(bc_capture *cap, uint64_t id, bc_rec_result error)
{
    if (!current(cap, id) || error == BC_REC_OK)
        return;
    if (cap->error == BC_REC_OK)
        cap->error = error;
    cap->stop_requested = true;
}

bool bc_capture_drained(const bc_capture *cap, uint64_t id)
{
    unsigned i;
    if (!current(cap, id) || !cap->quiescent || cap->count != 0U)
        return false;
    for (i = 0; i < BC_CAPTURE_BUFFERS; ++i)
        if (cap->slots[i] != BC_CAP_FREE)
            return false;
    return true;
}

bool bc_capture_begin(bc_capture *cap, uint64_t id)
{
    unsigned i;
    if (cap == NULL || id == 0U || cap->running || cap->count != 0U)
        return false;
    for (i = 0; i < BC_CAPTURE_BUFFERS; ++i)
        if (cap->slots[i] != BC_CAP_FREE)
            return false;
    memset(cap, 0, sizeof(*cap));
    cap->id = id;
    cap->running = true;
    return true;
}

uint8_t bc_capture_acquire_dma(bc_capture *cap, uint64_t id)
{
    uint8_t i;
    if (!current(cap, id) || !cap->running || cap->quiescent)
        return BC_CAPTURE_NONE;
    /* A pending Stop still needs the first/full buffer boundary. The device
     * adapter issues Stop immediately after full(), and does not acquire
     * another DMA buffer on that event. */
    for (i = 0; i < BC_CAPTURE_BUFFERS; ++i) {
        if (cap->slots[i] == BC_CAP_FREE) {
            cap->slots[i] = BC_CAP_DMA;
            return i;
        }
    }
    bc_capture_fault(cap, id, BC_REC_CAPTURE_OVERFLOW);
    return BC_CAPTURE_NONE;
}

bool bc_capture_full(bc_capture *cap, uint64_t id, uint8_t slot)
{
    if (!current(cap, id))
        return false;
    if (!cap->running || cap->quiescent || slot >= BC_CAPTURE_BUFFERS ||
        cap->slots[slot] != BC_CAP_DMA || cap->count >= BC_CAPTURE_BUFFERS ||
        cap->sequence == UINT32_MAX) {
        bc_capture_fault(cap, id, BC_REC_CAPTURE_OVERFLOW);
        return false;
    }
    cap->slots[slot] = BC_CAP_READY;
    cap->sequences[slot] = ++cap->sequence;
    cap->ready[(cap->head + cap->count) % BC_CAPTURE_BUFFERS] = slot;
    ++cap->count;
    return true;
}

bool bc_capture_stop(bc_capture *cap, uint64_t id)
{
    if (!current(cap, id))
        return false;
    cap->stop_requested = true;
    return true;
}

bool bc_capture_quiesced(bc_capture *cap, uint64_t id, bool forced)
{
    unsigned i;
    if (!current(cap, id))
        return false;
    if (forced || !cap->stop_requested)
        bc_capture_fault(cap, id, BC_REC_CAPTURE_ERROR);
    cap->quiescent = true;
    cap->running = false;
    for (i = 0; i < BC_CAPTURE_BUFFERS; ++i)
        if (cap->slots[i] == BC_CAP_DMA)
            cap->slots[i] = BC_CAP_FREE;
    return true;
}

uint8_t bc_capture_take(bc_capture *cap, uint64_t id, uint32_t *sequence)
{
    uint8_t slot;
    if (!current(cap, id) || sequence == NULL || cap->count == 0U)
        return BC_CAPTURE_NONE;
    slot = cap->ready[cap->head];
    if (slot >= BC_CAPTURE_BUFFERS || cap->slots[slot] != BC_CAP_READY) {
        bc_capture_fault(cap, id, BC_REC_CAPTURE_OVERFLOW);
        return BC_CAPTURE_NONE;
    }
    cap->head = (cap->head + 1U) % BC_CAPTURE_BUFFERS;
    --cap->count;
    cap->slots[slot] = BC_CAP_ENCODING;
    *sequence = cap->sequences[slot];
    return slot;
}

bool bc_capture_release(bc_capture *cap, uint64_t id, uint8_t slot)
{
    if (!current(cap, id))
        return false;
    if (slot >= BC_CAPTURE_BUFFERS || cap->slots[slot] != BC_CAP_ENCODING) {
        bc_capture_fault(cap, id, BC_REC_CAPTURE_OVERFLOW);
        return false;
    }
    cap->slots[slot] = BC_CAP_FREE;
    return true;
}
