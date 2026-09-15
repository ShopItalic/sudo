#include "app_factory_short.h"
#include "bc_rtos.h"

static volatile struct {
    bool active, accepting, failed, producer, sender;
    uint16_t generation;
    unsigned queued, steps, samples;
    uint32_t ready, consumed;
} capture;

bool factory_capture_begin(unsigned steps)
{
    bool ok;
    taskENTER_CRITICAL();
    ok = !capture.active && !capture.producer && !capture.sender && !capture.queued;
    if (ok) {
        capture.active = true; capture.accepting = false; capture.failed = false;
        capture.steps = steps <= FACTORY_SHORT_MAX_STEPS ? steps : 0U;
        capture.samples = 0; capture.ready = capture.consumed = 0;
        if (++capture.generation == 0U) ++capture.generation;
    }
    taskEXIT_CRITICAL();
    return ok;
}
void factory_capture_enable(void)
{
    taskENTER_CRITICAL();
    if (capture.active && !capture.failed) capture.accepting = true;
    taskEXIT_CRITICAL();
}
void factory_capture_ready(void)
{
    /* PDM ISR: aligned counters only. Task readers mask this IRQ. */
    if (capture.active && capture.accepting) ++capture.ready;
}
void factory_capture_isr_fault(void) { if (capture.active) capture.failed = true; }
bool factory_capture_producer_enter(void)
{
    bool ok;
    taskENTER_CRITICAL();
    ok = capture.active && capture.accepting && !capture.producer &&
        capture.ready != capture.consumed;
    if (ok) {
        if (capture.ready - capture.consumed != 1U) capture.failed = true;
        capture.consumed = capture.ready; capture.producer = true;
    }
    taskEXIT_CRITICAL();
    return ok;
}
void factory_capture_producer_leave(void)
{
    taskENTER_CRITICAL(); capture.producer = false; taskEXIT_CRITICAL();
}
bool factory_capture_queue_begin(uint8_t *header)
{
    bool ok;
    taskENTER_CRITICAL();
    ok = capture.active && capture.producer && header && capture.queued < 255U;
    if (ok) {
        header[0] = (uint8_t)capture.generation;
        header[1] = (uint8_t)(capture.generation >> 8);
        ++capture.queued;
    } else capture.failed = true;
    taskEXIT_CRITICAL();
    return ok;
}
void factory_capture_queue_failed(void)
{
    taskENTER_CRITICAL();
    capture.failed = true; if (capture.queued) --capture.queued;
    taskEXIT_CRITICAL();
}
bool factory_capture_sender_enter(const uint8_t *header)
{
    bool ok;
    taskENTER_CRITICAL();
    ok = capture.active && !capture.sender && capture.queued && header &&
        ((uint16_t)header[0] | ((uint16_t)header[1] << 8)) == capture.generation;
    if (ok) capture.sender = true;
    taskEXIT_CRITICAL();
    return ok;
}
void factory_capture_sender_leave(void)
{
    taskENTER_CRITICAL();
    if (capture.sender) {
        capture.sender = false;
        if (capture.queued) --capture.queued; else capture.failed = true;
    }
    taskEXIT_CRITICAL();
}
void factory_capture_written(unsigned bytes, bool success)
{
    const unsigned maximum = FACTORY_SHORT_MAX_STEPS * FACTORY_SHORT_SAMPLES_PER_STEP;
    taskENTER_CRITICAL();
    if (capture.active && capture.sender) {
        if (!success) capture.failed = true;
        else if (bytes >= (maximum - capture.samples + 1U) / 2U) capture.samples = maximum;
        else capture.samples += bytes * 2U; /* factory mono IMA ADPCM: two samples/byte */
    }
    taskEXIT_CRITICAL();
}
void factory_capture_fault(void)
{
    taskENTER_CRITICAL(); if (capture.active) capture.failed = true; taskEXIT_CRITICAL();
}
void factory_capture_stop_accepting(void)
{
    taskENTER_CRITICAL();
    capture.accepting = false;
    if (capture.ready != capture.consumed) capture.failed = true;
    taskEXIT_CRITICAL();
}
bool factory_capture_quiet(void)
{
    bool quiet;
    taskENTER_CRITICAL();
    quiet = !capture.accepting && !capture.producer && !capture.sender && !capture.queued;
    taskEXIT_CRITICAL();
    return quiet;
}
bool factory_capture_should_discard(void)
{
    bool discard;
    taskENTER_CRITICAL();
    discard = capture.active && !capture.accepting && !capture.producer &&
        !capture.sender && !capture.queued && !capture.failed && capture.steps &&
        capture.samples < capture.steps * FACTORY_SHORT_SAMPLES_PER_STEP;
    taskEXIT_CRITICAL();
    return discard;
}
void factory_capture_end(void)
{
    taskENTER_CRITICAL();
    if (!capture.producer && !capture.sender && !capture.queued) {
        capture.active = false; capture.accepting = false;
    }
    taskEXIT_CRITICAL();
}
bool factory_capture_active(void)
{
    bool active;
    taskENTER_CRITICAL(); active = capture.active; taskEXIT_CRITICAL();
    return active;
}
bool factory_capture_succeeded(void)
{
    bool success;
    taskENTER_CRITICAL(); success = capture.active && !capture.failed; taskEXIT_CRITICAL();
    return success;
}
