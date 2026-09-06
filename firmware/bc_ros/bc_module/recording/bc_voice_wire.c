#include "bc_voice_wire.h"

#include <string.h>

static uint16_t read16(const uint8_t *in)
{
    return (uint16_t)(in[0] | (uint16_t)in[1] << 8);
}

static uint32_t read32(const uint8_t *in)
{
    return (uint32_t)in[0] | (uint32_t)in[1] << 8 |
           (uint32_t)in[2] << 16 | (uint32_t)in[3] << 24;
}

static void write16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static uint32_t update_crc(uint32_t crc, const uint8_t *bytes, uint32_t length)
{
    uint32_t i;
    for (i = 0; i < length; ++i) {
        unsigned bit;
        crc ^= bytes[i];
        for (bit = 0; bit < 8U; ++bit)
            crc = (crc >> 1) ^ (0xedb88320UL & (0U - (crc & 1U)));
    }
    return crc;
}

uint32_t bc_voice_crc32(const uint8_t *bytes, uint32_t length)
{
    if (bytes == NULL && length != 0U)
        return 0;
    return update_crc(0xffffffffUL, bytes, length) ^ 0xffffffffUL;
}

static uint32_t message_crc(uint16_t id, uint8_t kind,
                            bc_voice_direction direction,
                            const uint8_t *payload, uint16_t length)
{
    uint8_t prefix[5];
    uint32_t crc;
    prefix[0] = BC_VOICE_VERSION;
    prefix[1] = kind;
    prefix[2] = (uint8_t)direction;
    write16(prefix + 3, id);
    crc = update_crc(0xffffffffUL, prefix, sizeof(prefix));
    return update_crc(crc, payload, length) ^ 0xffffffffUL;
}

uint16_t bc_voice_fragment(const bc_voice_message *message, uint16_t offset,
                            uint16_t att_payload_limit, uint8_t *packet,
                            uint16_t capacity, uint16_t *next_offset)
{
    uint16_t total, count, i;
    uint32_t crc;
    if (message == NULL || packet == NULL || next_offset == NULL ||
        message->length > BC_VOICE_PAYLOAD_MAX ||
        message->direction < BC_VOICE_REQUEST || message->direction > BC_VOICE_EVENT ||
        att_payload_limit < 20U || att_payload_limit > BC_VOICE_PACKET_MAX)
        return 0;
    total = message->length + 4U;
    if (offset >= total || capacity <= BC_VOICE_HEADER)
        return 0;
    count = total - offset;
    if (count > att_payload_limit - BC_VOICE_HEADER)
        count = att_payload_limit - BC_VOICE_HEADER;
    if (count > capacity - BC_VOICE_HEADER)
        count = capacity - BC_VOICE_HEADER;
    packet[0] = 0; /* Existing BCL frame type. */
    packet[1] = (uint8_t)message->message_id;
    packet[2] = BC_VOICE_COMMAND;
    packet[3] = message->kind;
    packet[4] = BC_VOICE_VERSION;
    packet[5] = (uint8_t)message->direction;
    write16(packet + 6, message->message_id);
    write16(packet + 8, offset);
    write16(packet + 10, total);
    crc = message_crc(message->message_id, message->kind, message->direction,
                      message->payload, message->length);
    for (i = 0; i < count; ++i) {
        uint16_t at = offset + i;
        packet[BC_VOICE_HEADER + i] = at < message->length ? message->payload[at] :
            (uint8_t)(crc >> (8U * (at - message->length)));
    }
    *next_offset = offset + count;
    return BC_VOICE_HEADER + count;
}

bc_wire_result bc_voice_receive(bc_voice_receiver *receiver, uint32_t epoch,
                                uint32_t now_ms, const uint8_t *packet,
                                uint16_t length, bc_voice_message *message)
{
    uint16_t id, offset, total, count;
    uint32_t crc;
    bool same;
    if (receiver == NULL || message == NULL)
        return BC_WIRE_INVALID;
    if (receiver->epoch != epoch ||
        (receiver->active && (uint32_t)(now_ms - receiver->started_ms) >= BC_VOICE_ASSEMBLY_MS)) {
        receiver->active = false;
        receiver->epoch = epoch;
    }
    if (packet == NULL || length <= BC_VOICE_HEADER || length > BC_VOICE_PACKET_MAX ||
        packet[0] != 0U || packet[2] != BC_VOICE_COMMAND || packet[4] != BC_VOICE_VERSION ||
        packet[5] > BC_VOICE_EVENT || packet[1] != packet[6])
        return BC_WIRE_INVALID;
    id = read16(packet + 6);
    offset = read16(packet + 8);
    total = read16(packet + 10);
    count = length - BC_VOICE_HEADER;
    if (total < 4U || total > BC_VOICE_BODY_MAX || offset >= total || count > total - offset)
        return BC_WIRE_INVALID;
    same = receiver->active && receiver->message_id == id &&
           receiver->kind == packet[3] && receiver->direction == packet[5] &&
           receiver->total == total;
    if (!same) {
        if (offset != 0U)
            return BC_WIRE_INVALID;
        receiver->active = true;
        receiver->started_ms = now_ms;
        receiver->message_id = id;
        receiver->kind = packet[3];
        receiver->direction = (bc_voice_direction)packet[5];
        receiver->total = total;
        receiver->received = 0;
    }
    if (offset < receiver->received) {
        if (count <= receiver->received - offset &&
            memcmp(receiver->body + offset, packet + BC_VOICE_HEADER, count) == 0)
            return BC_WIRE_MORE;
        receiver->active = false;
        return BC_WIRE_INVALID;
    }
    if (offset != receiver->received) {
        receiver->active = false;
        return BC_WIRE_INVALID;
    }
    memcpy(receiver->body + offset, packet + BC_VOICE_HEADER, count);
    receiver->received += count;
    if (receiver->received < total)
        return BC_WIRE_MORE;
    receiver->active = false;
    crc = message_crc(id, receiver->kind, receiver->direction, receiver->body, total - 4U);
    if (crc != read32(receiver->body + total - 4U))
        return BC_WIRE_INVALID;
    message->message_id = id;
    message->kind = receiver->kind;
    message->direction = receiver->direction;
    message->length = total - 4U;
    memcpy(message->payload, receiver->body, message->length);
    return BC_WIRE_MESSAGE;
}
