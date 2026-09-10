#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "bc_logger.h"

#define __MODULE__ "runtime-test"
uint32_t test_ipsr;
static unsigned scheduler_depth, writes, ticks, argument_calls;
static char output[1024];
static unsigned output_length;

uint32_t test_task_ticks(void)
{
    assert(test_ipsr == 0U);
    ++ticks;
    return 123U;
}

void vTaskSuspendAll(void)
{
    assert(test_ipsr == 0U);
    ++scheduler_depth;
}

BaseType_t xTaskResumeAll(void)
{
    assert(scheduler_depth != 0U);
    --scheduler_depth;
    return pdFALSE;
}

static int argument(void) { ++argument_calls; return 7; }

unsigned SEGGER_RTT_Write(unsigned index, const void *data, unsigned length)
{
    assert(index == 0U && scheduler_depth == 1U && test_ipsr == 0U);
    assert(length < sizeof(output));
    memcpy(output, data, length);
    output[length] = '\0';
    output_length = length;
    ++writes;
    /* Model a radio callback preempting the task during output. Neither a
     * direct logger call nor its macro may overwrite the task's buffer. */
    test_ipsr = 38U;
    bc_log_task_printf("nested ISR");
    BC_LOG_INFO("nested %d", argument());
    test_ipsr = 0U;
    assert(memcmp(output, data, length) == 0);
    return length;
}

int main(void)
{
    uint8_t bytes[100];
    char long_text[600];
    unsigned before, saved_arguments, saved_ticks;
    (void)argument; /* The DEBUG_INFO=0 build intentionally removes every call. */
    memset(bytes, 0xab, sizeof(bytes));
    memset(long_text, 'x', sizeof(long_text) - 1U);
    long_text[sizeof(long_text) - 1U] = '\0';

    for (test_ipsr = 1U; test_ipsr < 80U; ++test_ipsr) {
        BC_LOG_INFO("%d", argument());
        BC_LOG_DEBUG("%d", argument());
        BC_LOG_ERROR("%d", argument());
        BC_LOG_WARN("%d", argument());
        BC_LOG_PRINTF("%d", argument());
        BC_LOG_HEX("hex", bytes, (size_t)argument());
        bc_log_task_printf("direct ISR");
        bc_log_task_hex("direct ISR", bytes, sizeof(bytes));
    }
    assert(writes == 0U && ticks == 0U && argument_calls == 0U);
    test_ipsr = 0U;

    BC_LOG_INFO("value=%d", argument());
#if DEBUG_INFO == 1
    assert(writes == 1U && ticks == 1U && argument_calls == 1U);
    assert(strstr(output, "INFO:123;runtime-test") != NULL);
    assert(strstr(output, "value=7") != NULL);
#else
    assert(writes == 0U && ticks == 0U && argument_calls == 0U);
#endif
    saved_arguments = argument_calls;
    saved_ticks = ticks;
    bc_log_task_printf("%s", long_text);
    assert(output_length == 239U && output[238] == 'x');
    assert(argument_calls == saved_arguments && ticks == saved_ticks);
    bc_log_task_printf("integer=%d float=%.2f", -7, 1.25);
    assert(strcmp(output, "integer=-7 float=1.25") == 0);
    bc_log_task_hex("data:", bytes, 2U);
    assert(strcmp(output, "data:ab ab \r\n") == 0);
    bc_log_task_hex(long_text, bytes, sizeof(bytes));
    assert(output_length == 213U && strstr(output, "...\r\n") != NULL);
    bc_log_task_hex("empty", NULL, 0U);
    assert(strcmp(output, "empty\r\n") == 0);
    before = writes;
    bc_log_task_printf(NULL);
    bc_log_task_hex(NULL, bytes, 1U);
    bc_log_task_hex("bad", NULL, 1U);
    assert(writes == before && scheduler_depth == 0U);
    puts("Task logging: ISR exclusion, argument evaluation, preemption, bounds and output passed");
    return 0;
}
