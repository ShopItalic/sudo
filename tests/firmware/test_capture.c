#include "bc_capture.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_LEN(value) (sizeof(value) / sizeof((value)[0]))

static unsigned checks;
static unsigned failures;

static void check_condition(bool condition, const char *expression,
                            unsigned line)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        fprintf(stderr, "FAIL line %u: %s\n", line, expression);
    }
}

#define CHECK(condition) check_condition((condition), #condition, __LINE__)

static void reset_capture(bc_capture *capture)
{
    memset(capture, 0, sizeof(*capture));
}

static bool capture_clean(const bc_capture *capture, uint64_t id)
{
    return capture->error == BC_REC_OK &&
           bc_capture_drained(capture, id);
}

static void test_slot_ownership_and_reuse(void)
{
    bc_capture capture;
    uint8_t slots[BC_CAPTURE_BUFFERS];
    uint32_t sequence;
    uint8_t i;
    uint8_t reused;

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x1001U));
    for (i = 0U; i < BC_CAPTURE_BUFFERS; ++i)
    {
        slots[i] = bc_capture_acquire_dma(&capture, 0x1001U);
        CHECK(slots[i] == i);
        CHECK(capture.slots[slots[i]] == BC_CAP_DMA);
        CHECK(bc_capture_full(&capture, 0x1001U, slots[i]));
    }
    CHECK(capture.count == BC_CAPTURE_BUFFERS);
    CHECK(bc_capture_acquire_dma(&capture, 0x1001U) == BC_CAPTURE_NONE);
    CHECK(capture.error == BC_REC_CAPTURE_OVERFLOW);

    /* Reset the owner and fill every slot so an encoding slot is the only
     * possible candidate for reuse. It must remain owned until release. */
    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x1002U));
    for (i = 0U; i < BC_CAPTURE_BUFFERS; ++i)
    {
        slots[i] = bc_capture_acquire_dma(&capture, 0x1002U);
        CHECK(bc_capture_full(&capture, 0x1002U, slots[i]));
    }
    CHECK(bc_capture_take(&capture, 0x1002U, &sequence) == slots[0]);
    CHECK(sequence == 1U);
    CHECK(capture.slots[slots[0]] == BC_CAP_ENCODING);
    CHECK(bc_capture_acquire_dma(&capture, 0x1002U) == BC_CAPTURE_NONE);
    CHECK(capture.error == BC_REC_CAPTURE_OVERFLOW);
    CHECK(capture.stop_requested);
    CHECK(bc_capture_release(&capture, 0x1002U, slots[0]));
    CHECK(capture.slots[slots[0]] == BC_CAP_FREE);
    reused = bc_capture_acquire_dma(&capture, 0x1002U);
    CHECK(reused == slots[0]);
    CHECK(capture.slots[reused] == BC_CAP_DMA);
}

static void test_fifo_order_and_wrap(void)
{
    bc_capture capture;
    uint8_t slots[BC_CAPTURE_BUFFERS];
    uint32_t sequence;
    unsigned batch;
    uint8_t i;
    uint32_t expected = 1U;

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x2001U));

    /* Several complete FIFO turns exercise head/count modulo arithmetic and
     * retain the slot and sequence associated with every full buffer. */
    for (batch = 0U; batch < 12U; ++batch)
    {
        for (i = 0U; i < BC_CAPTURE_BUFFERS; ++i)
        {
            slots[i] = bc_capture_acquire_dma(&capture, 0x2001U);
            CHECK(slots[i] < BC_CAPTURE_BUFFERS);
            CHECK(bc_capture_full(&capture, 0x2001U, slots[i]));
        }
        for (i = 0U; i < BC_CAPTURE_BUFFERS; ++i)
        {
            uint8_t taken = bc_capture_take(&capture, 0x2001U, &sequence);
            CHECK(taken == slots[i]);
            CHECK(sequence == expected);
            ++expected;
            CHECK(capture.slots[taken] == BC_CAP_ENCODING);
            CHECK(bc_capture_release(&capture, 0x2001U, taken));
        }
        CHECK(capture.count == 0U);
    }
    CHECK(capture.error == BC_REC_OK);
    CHECK(capture.sequence == 12U * BC_CAPTURE_BUFFERS);
}

static void test_stop_boundary_and_drain_barrier(void)
{
    bc_capture capture;
    uint8_t ready_slot;
    uint8_t boundary_slot;
    uint8_t unfilled_slot;
    uint32_t sequence;

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x3001U));
    ready_slot = bc_capture_acquire_dma(&capture, 0x3001U);
    CHECK(ready_slot != BC_CAPTURE_NONE);
    CHECK(bc_capture_full(&capture, 0x3001U, ready_slot));
    CHECK(bc_capture_take(&capture, 0x3001U, &sequence) == ready_slot);
    CHECK(sequence == 1U);

    boundary_slot = bc_capture_acquire_dma(&capture, 0x3001U);
    CHECK(boundary_slot != BC_CAPTURE_NONE);
    CHECK(capture.slots[boundary_slot] == BC_CAP_DMA);
    unfilled_slot = bc_capture_acquire_dma(&capture, 0x3001U);
    CHECK(unfilled_slot != BC_CAPTURE_NONE);
    CHECK(capture.slots[unfilled_slot] == BC_CAP_DMA);
    CHECK(bc_capture_stop(&capture, 0x3001U));
    CHECK(capture.stop_requested);
    CHECK(!bc_capture_drained(&capture, 0x3001U));

    /* Stop waits for the current complete DMA boundary. */
    CHECK(bc_capture_full(&capture, 0x3001U, boundary_slot));
    CHECK(capture.slots[boundary_slot] == BC_CAP_READY);
    CHECK(capture.count == 1U);

    /* Before peripheral quiescence, even an empty queue cannot drain. */
    CHECK(!bc_capture_drained(&capture, 0x3001U));
    CHECK(bc_capture_quiesced(&capture, 0x3001U, false));
    CHECK(capture.quiescent);
    CHECK(!capture.running);
    CHECK(capture.slots[ready_slot] == BC_CAP_ENCODING);
    CHECK(capture.slots[boundary_slot] == BC_CAP_READY);
    CHECK(capture.slots[unfilled_slot] == BC_CAP_FREE);
    CHECK(capture.error == BC_REC_OK);
    CHECK(!bc_capture_drained(&capture, 0x3001U));

    /* Quiescence frees only the unfilled DMA slots. Ready and encoding
     * ownership survives until the encoder queue is explicitly drained. */
    CHECK(bc_capture_take(&capture, 0x3001U, &sequence) == boundary_slot);
    CHECK(sequence == 2U);
    CHECK(!bc_capture_drained(&capture, 0x3001U));
    CHECK(bc_capture_release(&capture, 0x3001U, boundary_slot));
    CHECK(!bc_capture_drained(&capture, 0x3001U));
    CHECK(bc_capture_release(&capture, 0x3001U, ready_slot));
    CHECK(capture_clean(&capture, 0x3001U));
}

static void test_restart_requires_all_ownership_released(void)
{
    bc_capture capture;
    uint8_t dma_slot;
    uint8_t ready_slot;
    uint8_t encoding_slot;
    uint32_t sequence;

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x4001U));

    dma_slot = bc_capture_acquire_dma(&capture, 0x4001U);
    CHECK(dma_slot != BC_CAPTURE_NONE);
    CHECK(!bc_capture_begin(&capture, 0x4002U));

    ready_slot = bc_capture_acquire_dma(&capture, 0x4001U);
    CHECK(ready_slot != BC_CAPTURE_NONE);
    CHECK(bc_capture_full(&capture, 0x4001U, ready_slot));
    CHECK(!bc_capture_begin(&capture, 0x4002U));

    encoding_slot = bc_capture_take(&capture, 0x4001U, &sequence);
    CHECK(encoding_slot == ready_slot);
    CHECK(sequence == 1U);
    CHECK(!bc_capture_begin(&capture, 0x4002U));

    CHECK(bc_capture_stop(&capture, 0x4001U));
    CHECK(bc_capture_quiesced(&capture, 0x4001U, false));
    CHECK(capture.slots[dma_slot] == BC_CAP_FREE);
    CHECK(capture.slots[encoding_slot] == BC_CAP_ENCODING);
    CHECK(!bc_capture_begin(&capture, 0x4002U));
    CHECK(bc_capture_release(&capture, 0x4001U, encoding_slot));
    CHECK(bc_capture_drained(&capture, 0x4001U));
    CHECK(bc_capture_begin(&capture, 0x4002U));
    CHECK(capture.id == 0x4002U);
    CHECK(capture.sequence == 0U);
}

static void test_stale_session_cannot_mutate_new_owner(void)
{
    bc_capture capture;
    bc_capture before;
    uint8_t slot;
    uint32_t sequence = 0xfeedfaceU;

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x5001U));
    slot = bc_capture_acquire_dma(&capture, 0x5001U);
    CHECK(slot != BC_CAPTURE_NONE);
    CHECK(bc_capture_stop(&capture, 0x5001U));
    CHECK(bc_capture_quiesced(&capture, 0x5001U, false));
    CHECK(bc_capture_drained(&capture, 0x5001U));
    CHECK(bc_capture_begin(&capture, 0x5002U));
    slot = bc_capture_acquire_dma(&capture, 0x5002U);
    CHECK(slot != BC_CAPTURE_NONE);
    before = capture;

    CHECK(bc_capture_acquire_dma(&capture, 0x5001U) == BC_CAPTURE_NONE);
    CHECK(!bc_capture_full(&capture, 0x5001U, slot));
    CHECK(!bc_capture_stop(&capture, 0x5001U));
    CHECK(!bc_capture_quiesced(&capture, 0x5001U, true));
    bc_capture_fault(&capture, 0x5001U, BC_REC_WRITE_ERROR);
    CHECK(bc_capture_take(&capture, 0x5001U, &sequence) == BC_CAPTURE_NONE);
    CHECK(sequence == 0xfeedfaceU);
    CHECK(!bc_capture_release(&capture, 0x5001U, slot));
    CHECK(!bc_capture_drained(&capture, 0x5001U));
    CHECK(memcmp(&capture, &before, sizeof(capture)) == 0);

    CHECK(capture.error == BC_REC_OK);
    CHECK(capture.id == 0x5002U);
    CHECK(capture.slots[slot] == BC_CAP_DMA);
    CHECK(bc_capture_full(&capture, 0x5002U, slot));
}

static void test_capture_faults_are_explicit(void)
{
    bc_capture capture;
    uint8_t slot;
    uint32_t sequence;

    /* Duplicate full callback is a fault and cannot append another FIFO item. */
    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x6001U));
    slot = bc_capture_acquire_dma(&capture, 0x6001U);
    CHECK(bc_capture_full(&capture, 0x6001U, slot));
    CHECK(!bc_capture_full(&capture, 0x6001U, slot));
    CHECK(capture.error == BC_REC_CAPTURE_OVERFLOW);
    CHECK(capture.stop_requested);
    CHECK(capture.count == 1U);

    /* Invalid indexes and releasing anything other than ENCODING are faults. */
    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x6002U));
    CHECK(!bc_capture_full(&capture, 0x6002U, BC_CAPTURE_BUFFERS));
    CHECK(capture.error == BC_REC_CAPTURE_OVERFLOW);

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x6003U));
    slot = bc_capture_acquire_dma(&capture, 0x6003U);
    CHECK(!bc_capture_release(&capture, 0x6003U, slot));
    CHECK(capture.error == BC_REC_CAPTURE_OVERFLOW);
    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x6004U));
    slot = bc_capture_acquire_dma(&capture, 0x6004U);
    CHECK(bc_capture_full(&capture, 0x6004U, slot));
    CHECK(!bc_capture_release(&capture, 0x6004U, slot));
    CHECK(capture.error == BC_REC_CAPTURE_OVERFLOW);
    CHECK(bc_capture_take(&capture, 0x6004U, &sequence) == slot);
    CHECK(bc_capture_release(&capture, 0x6004U, slot));
    CHECK(!bc_capture_release(&capture, 0x6004U, slot));
    CHECK(capture.error == BC_REC_CAPTURE_OVERFLOW);
}

static void test_unexpected_and_forced_quiescence(void)
{
    bc_capture capture;
    uint8_t dma_slot;
    uint8_t ready_slot;
    uint8_t encoding_slot;
    uint32_t sequence;

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x7001U));
    dma_slot = bc_capture_acquire_dma(&capture, 0x7001U);
    CHECK(dma_slot != BC_CAPTURE_NONE);
    CHECK(bc_capture_quiesced(&capture, 0x7001U, false));
    CHECK(capture.error == BC_REC_CAPTURE_ERROR);
    CHECK(capture.stop_requested);
    CHECK(capture.slots[dma_slot] == BC_CAP_FREE);
    CHECK(bc_capture_drained(&capture, 0x7001U));

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x7002U));
    ready_slot = bc_capture_acquire_dma(&capture, 0x7002U);
    CHECK(bc_capture_full(&capture, 0x7002U, ready_slot));
    encoding_slot = bc_capture_take(&capture, 0x7002U, &sequence);
    CHECK(encoding_slot == ready_slot);
    dma_slot = bc_capture_acquire_dma(&capture, 0x7002U);
    CHECK(dma_slot != BC_CAPTURE_NONE);
    CHECK(bc_capture_stop(&capture, 0x7002U));
    CHECK(bc_capture_quiesced(&capture, 0x7002U, true));
    CHECK(capture.error == BC_REC_CAPTURE_ERROR);
    CHECK(capture.slots[dma_slot] == BC_CAP_FREE);
    CHECK(capture.slots[encoding_slot] == BC_CAP_ENCODING);
    CHECK(!bc_capture_drained(&capture, 0x7002U));
    CHECK(bc_capture_release(&capture, 0x7002U, encoding_slot));
    CHECK(bc_capture_drained(&capture, 0x7002U));
}

static void test_sequence_uint32_max_fault(void)
{
    bc_capture capture;
    uint8_t slot;

    reset_capture(&capture);
    CHECK(bc_capture_begin(&capture, 0x8001U));
    capture.sequence = UINT32_MAX - 1U;
    slot = bc_capture_acquire_dma(&capture, 0x8001U);
    CHECK(slot != BC_CAPTURE_NONE);
    CHECK(bc_capture_full(&capture, 0x8001U, slot));
    CHECK(capture.sequence == UINT32_MAX);

    slot = bc_capture_acquire_dma(&capture, 0x8001U);
    CHECK(slot != BC_CAPTURE_NONE);
    CHECK(!bc_capture_full(&capture, 0x8001U, slot));
    CHECK(capture.error == BC_REC_CAPTURE_OVERFLOW);
    CHECK(capture.stop_requested);
    CHECK(capture.sequence == UINT32_MAX);
}

static void test_basic_invalid_inputs(void)
{
    bc_capture capture;
    bc_capture before;
    uint32_t sequence = 0x12345678U;

    reset_capture(&capture);
    CHECK(!bc_capture_begin(NULL, 0x9001U));
    CHECK(!bc_capture_begin(&capture, 0U));
    CHECK(bc_capture_acquire_dma(&capture, 0U) == BC_CAPTURE_NONE);
    CHECK(!bc_capture_full(&capture, 0U, 0U));
    CHECK(!bc_capture_stop(&capture, 0U));
    CHECK(!bc_capture_quiesced(&capture, 0U, false));
    CHECK(bc_capture_take(&capture, 0U, &sequence) == BC_CAPTURE_NONE);
    CHECK(!bc_capture_release(&capture, 0U, 0U));
    CHECK(!bc_capture_drained(&capture, 0U));
    CHECK(capture.error == BC_REC_OK);

    CHECK(bc_capture_begin(&capture, 0x9001U));
    before = capture;
    CHECK(bc_capture_take(&capture, 0x9001U, NULL) == BC_CAPTURE_NONE);
    bc_capture_fault(&capture, 0x9001U, BC_REC_OK);
    CHECK(memcmp(&capture, &before, sizeof(capture)) == 0);
}

int main(void)
{
    test_slot_ownership_and_reuse();
    test_fifo_order_and_wrap();
    test_stop_boundary_and_drain_barrier();
    test_restart_requires_all_ownership_released();
    test_stale_session_cannot_mutate_new_owner();
    test_capture_faults_are_explicit();
    test_unexpected_and_forced_quiescence();
    test_sequence_uint32_max_fault();
    test_basic_invalid_inputs();

    if (failures != 0U)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks (DMA ownership, FIFO ordering, stop/quiescence,\n"
           "      stale sessions, explicit faults and sequence wrap)\n",
           checks);
    return 0;
}
