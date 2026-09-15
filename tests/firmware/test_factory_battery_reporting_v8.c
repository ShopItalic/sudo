#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BLE_CONNECT_IDIE_TIMEOUT_TIMER 0xA5U
#include "factory_packet_types.inc"

static unsigned checks;
static bool connected, allowed, disconnect_after_guards, nested;
static bool revised;
static unsigned send_depth;
static const uint8_t *outer_buffer;
typedef struct {
    uint8_t wire[2][6];
    uint8_t lengths[2];
    unsigned sends, connection_checks, notify_checks, timer_starts;
    unsigned timer_ids[2];
    char events[32];
    unsigned event_count;
} observation;
static observation observed;

static void require(bool value, const char *message)
{
    ++checks;
    if (!value) { fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static void event(char value)
{
    require(observed.event_count < sizeof(observed.events), "event bounds");
    observed.events[observed.event_count++] = value;
}

static bool connected_now(void)
{
    event('C');
    ++observed.connection_checks;
    return connected;
}

static void radio_copy(uint8_t *data, uint16_t length);
static struct {
    bool (*ble_connect_status)(void);
    void (*ble_send)(uint8_t *, uint16_t);
} ble_calss = {connected_now, radio_copy};

static bool app_ble_notify_allowed(void)
{
    event('N'); ++observed.notify_checks;
    if (disconnect_after_guards) connected = false;
    return allowed;
}

static void app_connect_idie_timer_start(unsigned timer)
{
    event('T');
    require(observed.timer_starts < 2, "timer bounds");
    observed.timer_ids[observed.timer_starts++] = timer;
}

/* Both versions retain the factory's direct radio callback and guards. */
#include "original_notifications.inc"
#include "compact_notifications.inc"

static void radio_copy(uint8_t *data, uint16_t length)
{
    uint8_t saved[6];
    require(length == 5 || length == 6, "wire length");
    require(observed.sends < 2, "send bounds");
    memcpy(saved, data, length);
    event('S');
    unsigned index = observed.sends++;
    observed.lengths[index] = (uint8_t)length;
    memcpy(observed.wire[index], data, length);
    if (send_depth) require(data != outer_buffer, "nested buffer ownership");
    if (nested && !send_depth) {
        outer_buffer = data;
        ++send_depth;
        if (length == 5) {
            if (revised) compact_status(0xA17E);
            else original_status(0xA17E);
        } else {
            if (revised) compact_percent(0x5B);
            else original_percent(0x5B);
        }
        --send_depth;
        require(memcmp(saved, data, length) == 0, "outer packet survives nested call");
    }
}

static observation run(bool is_revised, bool status, uint16_t value,
                       unsigned scenario, bool reenter)
{
    memset(&observed, 0, sizeof(observed));
    revised = is_revised;
    connected = scenario != 0;
    allowed = scenario != 1;
    disconnect_after_guards = scenario == 3;
    nested = reenter;
    send_depth = 0;
    outer_buffer = NULL;
    if (status) {
        if (is_revised) compact_status(value); else original_status(value);
    } else {
        if (is_revised) compact_percent((uint8_t)value);
        else original_percent((uint8_t)value);
    }
    return observed;
}

static bool same_observation(const observation *a, const observation *b)
{
    /* Compare every field, never unspecified structure padding. */
    return memcmp(a->wire, b->wire, sizeof(a->wire)) == 0 &&
        memcmp(a->lengths, b->lengths, sizeof(a->lengths)) == 0 &&
        a->sends == b->sends && a->connection_checks == b->connection_checks &&
        a->notify_checks == b->notify_checks && a->timer_starts == b->timer_starts &&
        memcmp(a->timer_ids, b->timer_ids, sizeof(a->timer_ids)) == 0 &&
        memcmp(a->events, b->events, sizeof(a->events)) == 0 &&
        a->event_count == b->event_count;
}

int main(void)
{
    unsigned pairs = 0;
    for (unsigned kind = 0; kind < 2; ++kind) {
        unsigned limit = kind ? 65536 : 256;
        for (unsigned value = 0; value < limit; ++value) {
            for (unsigned scenario = 0; scenario < 4; ++scenario) {
                for (unsigned reenter = 0; reenter < 2; ++reenter) {
                    observation original = run(false, kind, (uint16_t)value, scenario, reenter);
                    observation compact = run(true, kind, (uint16_t)value, scenario, reenter);
                    if (!same_observation(&original, &compact))
                        fprintf(stderr, "kind=%u value=%u scenario=%u nested=%u\n",
                                kind, value, scenario, reenter);
                    require(same_observation(&original, &compact),
                            "original/compact payload or side-effect mismatch");
                    ++pairs;
                }
            }
        }
    }
    printf("PASS compact battery notifications: %u comparison pairs; %u assertions; "
           "all uint8/uint16 inputs, connection guards, disconnect and nested sends\n",
           pairs, checks);
    return 0;
}
