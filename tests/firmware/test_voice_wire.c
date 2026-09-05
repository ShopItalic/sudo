#include "bc_voice_wire.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_LEN(value) (sizeof(value) / sizeof((value)[0]))
#define GUARD_SIZE 16U
#define MAX_FRAGMENTS 32U

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

typedef struct {
    uint8_t pre[GUARD_SIZE];
    bc_voice_receiver receiver;
    uint8_t post[GUARD_SIZE];
} receiver_box;

typedef struct {
    uint8_t pre[GUARD_SIZE];
    bc_voice_message message;
    uint8_t post[GUARD_SIZE];
} message_box;

typedef struct {
    uint8_t pre[GUARD_SIZE];
    uint8_t packet[BC_VOICE_PACKET_MAX];
    uint8_t post[GUARD_SIZE];
} packet_box;

typedef struct {
    uint8_t packets[MAX_FRAGMENTS][BC_VOICE_PACKET_MAX];
    uint16_t lengths[MAX_FRAGMENTS];
    uint16_t offsets[MAX_FRAGMENTS];
    unsigned count;
} fragment_set;

static void fill_guards(uint8_t *bytes, uint8_t value)
{
    memset(bytes, value, GUARD_SIZE);
}

static void init_receiver(receiver_box *box)
{
    memset(box, 0, sizeof(*box));
    fill_guards(box->pre, 0xc1U);
    fill_guards(box->post, 0xc2U);
}

static void init_message(message_box *box, uint8_t value)
{
    memset(box, value, sizeof(*box));
}

static void init_packet(packet_box *box, uint8_t value)
{
    memset(box, value, sizeof(*box));
    fill_guards(box->pre, 0xd1U);
    fill_guards(box->post, 0xd2U);
}

static bool guards_ok(const uint8_t *pre, const uint8_t *post,
                      uint8_t pre_value, uint8_t post_value)
{
    unsigned i;
    for (i = 0U; i < GUARD_SIZE; ++i)
    {
        if (pre[i] != pre_value || post[i] != post_value)
            return false;
    }
    return true;
}

static bool receiver_guards_ok(const receiver_box *box)
{
    return guards_ok(box->pre, box->post, 0xc1U, 0xc2U);
}

static bool packet_guards_ok(const packet_box *box)
{
    return guards_ok(box->pre, box->post, 0xd1U, 0xd2U);
}

static bool message_guards_ok(const message_box *box)
{
    unsigned i;
    for (i = 0U; i < GUARD_SIZE; ++i)
    {
        if (box->pre[i] != 0xa5U || box->post[i] != 0xa5U)
            return false;
    }
    return true;
}

static void write16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static uint16_t read16(const uint8_t *bytes)
{
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static bc_voice_message make_message(uint16_t id, uint8_t kind,
                                     bc_voice_direction direction,
                                     uint16_t length)
{
    bc_voice_message message;
    uint16_t i;
    memset(&message, 0, sizeof(message));
    message.message_id = id;
    message.kind = kind;
    message.direction = direction;
    message.length = length;
    for (i = 0U; i < length; ++i)
        message.payload[i] = (uint8_t)(i * 37U + id + kind);
    return message;
}

static bool fragment_all(const bc_voice_message *message,
                         uint16_t att_payload_limit, fragment_set *set)
{
    uint16_t offset = 0U;
    uint16_t total = message->length + 4U;

    memset(set, 0, sizeof(*set));
    while (offset < total)
    {
        uint16_t next;
        uint16_t length;
        if (set->count >= ARRAY_LEN(set->packets))
        {
            CHECK(false);
            return false;
        }
        length = bc_voice_fragment(message, offset, att_payload_limit,
                                   set->packets[set->count],
                                   BC_VOICE_PACKET_MAX, &next);
        CHECK(length > BC_VOICE_HEADER);
        CHECK(next > offset);
        CHECK(next <= total);
        if (length <= BC_VOICE_HEADER || next <= offset || next > total)
            return false;
        set->offsets[set->count] = offset;
        set->lengths[set->count] = length;
        ++set->count;
        offset = next;
    }
    CHECK(offset == total);
    return offset == total;
}

static bool same_message(const bc_voice_message *left,
                         const bc_voice_message *right)
{
    return left->message_id == right->message_id &&
           left->kind == right->kind && left->direction == right->direction &&
           left->length == right->length &&
           memcmp(left->payload, right->payload, left->length) == 0;
}

static bool receive_all(const bc_voice_message *expected,
                        uint16_t att_payload_limit, uint32_t epoch,
                        uint32_t now_ms)
{
    fragment_set set;
    receiver_box receiver;
    message_box output;
    unsigned i;

    CHECK(fragment_all(expected, att_payload_limit, &set));
    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    for (i = 0U; i < set.count; ++i)
    {
        bc_wire_result result = bc_voice_receive(
            &receiver.receiver, epoch, now_ms + i, set.packets[i],
            set.lengths[i], &output.message);
        if (i + 1U == set.count)
            CHECK(result == BC_WIRE_MESSAGE);
        else
            CHECK(result == BC_WIRE_MORE);
        CHECK(receiver_guards_ok(&receiver));
        CHECK(message_guards_ok(&output));
    }
    CHECK(!receiver.receiver.active);
    CHECK(same_message(&output.message, expected));
    CHECK(message_guards_ok(&output));
    return same_message(&output.message, expected) &&
           receiver_guards_ok(&receiver) && message_guards_ok(&output);
}

static bc_wire_result receive_checked(receiver_box *receiver,
                                      message_box *output, uint32_t epoch,
                                      uint32_t now_ms, const uint8_t *packet,
                                      uint16_t length);

static void test_crc_vector(void)
{
    static const uint8_t vector[] = "123456789";
    CHECK(bc_voice_crc32(vector, (uint32_t)strlen((const char *)vector)) ==
          0xcbf43926U);
    CHECK(bc_voice_crc32(NULL, 0U) == 0U);
    CHECK(bc_voice_crc32(NULL, 1U) == 0U);
}

static void test_interoperability_vector(void)
{
    /* Hand-authored little-endian wire vector for id=0x1234, kind=0x42,
     * response direction and payload {10 20 30}. The CRC is over
     * {version, kind, direction, full id, payload}, not the transport
     * header, and is serialized little-endian. */
    static const uint8_t expected_packet[] = {
        0x00U, 0x34U, 0x7eU, 0x42U, 0x01U, 0x01U, 0x34U, 0x12U,
        0x00U, 0x00U, 0x07U, 0x00U, 0x10U, 0x20U, 0x30U, 0x2eU,
        0x5eU, 0x8aU, 0xb0U};
    static const uint8_t payload[] = {0x10U, 0x20U, 0x30U};
    bc_voice_message source = make_message(0x1234U, 0x42U,
                                            BC_VOICE_RESPONSE, 3U);
    bc_voice_message expected = source;
    packet_box packet;
    receiver_box receiver;
    message_box output;
    uint16_t next = 0xbeefU;
    uint16_t length;

    memcpy(source.payload, payload, sizeof(payload));
    expected = source;
    init_packet(&packet, 0xa5U);
    length = bc_voice_fragment(&source, 0U, 250U, packet.packet,
                               BC_VOICE_PACKET_MAX, &next);
    CHECK(length == sizeof(expected_packet));
    CHECK(next == sizeof(expected_packet) - BC_VOICE_HEADER);
    CHECK(length <= BC_VOICE_PACKET_MAX);
    CHECK(memcmp(packet.packet, expected_packet, sizeof(expected_packet)) ==
          0);
    CHECK(packet_guards_ok(&packet));

    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(receive_checked(&receiver, &output, 100U, 500U,
                          expected_packet, sizeof(expected_packet)) ==
          BC_WIRE_MESSAGE);
    CHECK(same_message(&output.message, &expected));
    CHECK(receiver_guards_ok(&receiver));
    CHECK(message_guards_ok(&output));
}

static void test_att_sizes_and_payload_bounds(void)
{
    static const uint16_t limits[] = {20U, 64U, 244U, 250U};
    bc_voice_message empty = make_message(0x0000U, 0x11U, BC_VOICE_REQUEST, 0U);
    bc_voice_message small = make_message(0x1234U, 0x22U, BC_VOICE_RESPONSE, 9U);
    bc_voice_message maximum = make_message(0xffffU, 0x33U, BC_VOICE_EVENT,
                                            BC_VOICE_PAYLOAD_MAX);
    unsigned i;

    for (i = 0U; i < ARRAY_LEN(limits); ++i)
    {
        CHECK(receive_all(&empty, limits[i], 1U + i, 10U));
        CHECK(receive_all(&small, limits[i], 20U + i, 20U));
        CHECK(receive_all(&maximum, limits[i], 40U + i, 30U));
    }
}

static void test_fragment_canaries_and_capacity(void)
{
    bc_voice_message message = make_message(0x3456U, 0x44U, BC_VOICE_REQUEST,
                                             20U);
    packet_box box;
    bc_voice_message original = message;
    uint16_t next = 0xaaaaU;
    uint16_t length;
    uint16_t offset;

    init_packet(&box, 0xa5U);
    length = bc_voice_fragment(&message, 0U, 250U, box.packet,
                               BC_VOICE_PACKET_MAX, &next);
    CHECK(length == message.length + 4U + BC_VOICE_HEADER);
    CHECK(next == message.length + 4U);
    CHECK(packet_guards_ok(&box));
    CHECK(box.packet[length] == 0xa5U);
    CHECK(box.packet[BC_VOICE_PACKET_MAX - 1U] == 0xa5U);
    CHECK(box.packet[0] == 0U);
    CHECK(box.packet[2] == BC_VOICE_COMMAND);
    CHECK(read16(box.packet + 6U) == message.message_id);
    CHECK(read16(box.packet + 8U) == 0U);
    CHECK(read16(box.packet + 10U) == message.length + 4U);
    CHECK(memcmp(&message, &original, sizeof(message)) == 0);

    init_packet(&box, 0xa5U);
    next = 0xbeefU;
    length = bc_voice_fragment(&message, 0U, 250U, box.packet,
                               BC_VOICE_HEADER + 1U, &next);
    CHECK(length == BC_VOICE_HEADER + 1U);
    CHECK(next == 1U);
    CHECK(packet_guards_ok(&box));
    CHECK(box.packet[BC_VOICE_HEADER + 1U] == 0xa5U);

    init_packet(&box, 0xa5U);
    next = 0xbeefU;
    CHECK(bc_voice_fragment(&message, 0U, 250U, box.packet,
                            BC_VOICE_HEADER, &next) == 0U);
    CHECK(next == 0xbeefU);
    CHECK(packet_guards_ok(&box));

    init_packet(&box, 0xa5U);
    offset = message.length + 4U;
    next = 0xbeefU;
    CHECK(bc_voice_fragment(&message, offset, 250U, box.packet,
                            BC_VOICE_PACKET_MAX, &next) == 0U);
    CHECK(next == 0xbeefU);
    CHECK(packet_guards_ok(&box));
}

static void test_fragment_invalid_arguments(void)
{
    bc_voice_message message = make_message(0x4567U, 0x55U, BC_VOICE_REQUEST,
                                             1U);
    packet_box box;
    uint16_t next;
    bc_voice_message invalid;

    init_packet(&box, 0xa5U);
    next = 0xcafeU;
    CHECK(bc_voice_fragment(NULL, 0U, 250U, box.packet,
                            BC_VOICE_PACKET_MAX, &next) == 0U);
    CHECK(next == 0xcafeU);
    CHECK(packet_guards_ok(&box));

    init_packet(&box, 0xa5U);
    next = 0xcafeU;
    CHECK(bc_voice_fragment(&message, 0U, 250U, NULL,
                            BC_VOICE_PACKET_MAX, &next) == 0U);
    CHECK(next == 0xcafeU);
    CHECK(packet_guards_ok(&box));

    init_packet(&box, 0xa5U);
    CHECK(bc_voice_fragment(&message, 0U, 250U, box.packet,
                            BC_VOICE_PACKET_MAX, NULL) == 0U);
    CHECK(packet_guards_ok(&box));

    invalid = message;
    invalid.length = BC_VOICE_PAYLOAD_MAX + 1U;
    init_packet(&box, 0xa5U);
    next = 0xcafeU;
    CHECK(bc_voice_fragment(&invalid, 0U, 250U, box.packet,
                            BC_VOICE_PACKET_MAX, &next) == 0U);
    CHECK(next == 0xcafeU);
    CHECK(packet_guards_ok(&box));

    invalid = message;
    invalid.direction = (bc_voice_direction)3U;
    init_packet(&box, 0xa5U);
    next = 0xcafeU;
    CHECK(bc_voice_fragment(&invalid, 0U, 250U, box.packet,
                            BC_VOICE_PACKET_MAX, &next) == 0U);
    CHECK(next == 0xcafeU);
    CHECK(packet_guards_ok(&box));

    init_packet(&box, 0xa5U);
    next = 0xcafeU;
    CHECK(bc_voice_fragment(&message, 0U, BC_VOICE_HEADER, box.packet,
                            BC_VOICE_PACKET_MAX, &next) == 0U);
    CHECK(next == 0xcafeU);
    CHECK(packet_guards_ok(&box));

    init_packet(&box, 0xa5U);
    next = 0xcafeU;
    CHECK(bc_voice_fragment(&message, 0U, BC_VOICE_HEADER + 7U,
                            box.packet, BC_VOICE_PACKET_MAX, &next) == 0U);
    CHECK(next == 0xcafeU);
    CHECK(packet_guards_ok(&box));

    init_packet(&box, 0xa5U);
    next = 0xcafeU;
    CHECK(bc_voice_fragment(&message, 0U, BC_VOICE_PACKET_MAX + 1U,
                            box.packet, BC_VOICE_PACKET_MAX, &next) == 0U);
    CHECK(next == 0xcafeU);
    CHECK(packet_guards_ok(&box));
}

static bc_wire_result receive_checked(receiver_box *receiver,
                                      message_box *output, uint32_t epoch,
                                      uint32_t now_ms, const uint8_t *packet,
                                      uint16_t length)
{
    bc_wire_result result = bc_voice_receive(&receiver->receiver, epoch,
                                             now_ms, packet, length,
                                             &output->message);
    CHECK(receiver_guards_ok(receiver));
    CHECK(message_guards_ok(output));
    return result;
}

static void test_receiver_truncation_and_bounds(void)
{
    bc_voice_message message = make_message(0x5678U, 0x66U, BC_VOICE_REQUEST,
                                             9U);
    fragment_set set;
    unsigned cut;
    uint8_t malformed[BC_VOICE_PACKET_MAX];

    CHECK(fragment_all(&message, 250U, &set));
    CHECK(set.count == 1U);
    for (cut = 0U; cut < set.lengths[0]; ++cut)
    {
        receiver_box receiver;
        message_box output;
        bc_wire_result result;
        init_receiver(&receiver);
        init_message(&output, 0xa5U);
        result = receive_checked(&receiver, &output, 7U, 100U,
                                 set.packets[0], (uint16_t)cut);
        CHECK(result != BC_WIRE_MESSAGE);
        if (cut <= BC_VOICE_HEADER)
            CHECK(result == BC_WIRE_INVALID);
        else
            CHECK(result == BC_WIRE_MORE);
        CHECK(!receiver.receiver.active || cut > BC_VOICE_HEADER);
    }

    /* A missing fragment can never complete the command. Omitting each
     * position also covers a gap after an otherwise valid prefix. */
    {
        bc_voice_message long_message = make_message(0x5679U, 0x67U,
                                                      BC_VOICE_REQUEST, 40U);
        fragment_set long_set;
        unsigned omit;
        CHECK(fragment_all(&long_message, 20U, &long_set));
        for (omit = 0U; omit < long_set.count; ++omit)
        {
            receiver_box receiver;
            message_box output;
            unsigned i;
            bc_wire_result result = BC_WIRE_MORE;
            init_receiver(&receiver);
            init_message(&output, 0xa5U);
            for (i = 0U; i < long_set.count; ++i)
            {
                if (i != omit)
                    result = receive_checked(&receiver, &output, 8U, 110U + i,
                                             long_set.packets[i],
                                             long_set.lengths[i]);
            }
            CHECK(result != BC_WIRE_MESSAGE);
            CHECK(message_guards_ok(&output));
        }
    }

    memcpy(malformed, set.packets[0], set.lengths[0]);
    write16(malformed + 10U, 3U);
    {
        receiver_box receiver;
        message_box output;
        init_receiver(&receiver);
        init_message(&output, 0xa5U);
        CHECK(receive_checked(&receiver, &output, 9U, 120U, malformed,
                              set.lengths[0]) == BC_WIRE_INVALID);
    }

    memcpy(malformed, set.packets[0], set.lengths[0]);
    write16(malformed + 10U, BC_VOICE_BODY_MAX + 1U);
    {
        receiver_box receiver;
        message_box output;
        init_receiver(&receiver);
        init_message(&output, 0xa5U);
        CHECK(receive_checked(&receiver, &output, 9U, 120U, malformed,
                              set.lengths[0]) == BC_WIRE_INVALID);
    }

    memcpy(malformed, set.packets[0], set.lengths[0]);
    write16(malformed + 8U, read16(malformed + 10U));
    {
        receiver_box receiver;
        message_box output;
        init_receiver(&receiver);
        init_message(&output, 0xa5U);
        CHECK(receive_checked(&receiver, &output, 9U, 120U, malformed,
                              set.lengths[0]) == BC_WIRE_INVALID);
    }

    memcpy(malformed, set.packets[0], set.lengths[0]);
    write16(malformed + 8U, 1U);
    {
        receiver_box receiver;
        message_box output;
        init_receiver(&receiver);
        init_message(&output, 0xa5U);
        CHECK(receive_checked(&receiver, &output, 9U, 120U, malformed,
                              set.lengths[0]) == BC_WIRE_INVALID);
    }

    {
        receiver_box receiver;
        message_box output;
        uint8_t oversized[BC_VOICE_PACKET_MAX + 1U];
        init_receiver(&receiver);
        init_message(&output, 0xa5U);
        memset(oversized, 0xa5U, sizeof(oversized));
        memcpy(oversized, set.packets[0], set.lengths[0]);
        CHECK(receive_checked(&receiver, &output, 9U, 120U, oversized,
                              BC_VOICE_PACKET_MAX + 1U) == BC_WIRE_INVALID);
        CHECK(receive_checked(&receiver, &output, 9U, 120U, set.packets[0],
                              BC_VOICE_PACKET_MAX) == BC_WIRE_INVALID);
        CHECK(receive_checked(&receiver, &output, 9U, 120U, NULL, 20U) ==
              BC_WIRE_INVALID);
    }
}

static void test_frame_headers_and_corruption(void)
{
    bc_voice_message message = make_message(0x6789U, 0x78U, BC_VOICE_RESPONSE,
                                             20U);
    fragment_set set;
    unsigned i;

    CHECK(fragment_all(&message, 20U, &set));
    CHECK(set.count >= 2U);

    /* Every fixed header discriminator is checked independently. */
    {
        static const uint8_t indexes[] = {0U, 2U, 4U, 5U};
        static const uint8_t values[] = {1U, 0U, 0U, 3U};
        unsigned h;
        for (h = 0U; h < ARRAY_LEN(indexes); ++h)
        {
            uint8_t packet[BC_VOICE_PACKET_MAX];
            receiver_box receiver;
            message_box output;
            memcpy(packet, set.packets[0], set.lengths[0]);
            packet[indexes[h]] = values[h];
            init_receiver(&receiver);
            init_message(&output, 0xa5U);
            CHECK(receive_checked(&receiver, &output, 10U, 130U, packet,
                                  set.lengths[0]) != BC_WIRE_MESSAGE);
        }
    }

    {
        uint8_t packet[BC_VOICE_PACKET_MAX];
        receiver_box receiver;
        message_box output;
        memcpy(packet, set.packets[0], set.lengths[0]);
        packet[1] ^= 1U;
        init_receiver(&receiver);
        init_message(&output, 0xa5U);
        CHECK(receive_checked(&receiver, &output, 10U, 130U, packet,
                              set.lengths[0]) == BC_WIRE_INVALID);
    }

    /* Mutating any byte of any fragment must prevent a message. Header
     * mutations may be rejected immediately; body/CRC mutations are checked
     * at completion. */
    for (i = 0U; i < set.count; ++i)
    {
        unsigned byte;
        for (byte = 0U; byte < set.lengths[i]; ++byte)
        {
            receiver_box receiver;
            message_box output;
            unsigned j;
            bc_wire_result result = BC_WIRE_MORE;
            init_receiver(&receiver);
            init_message(&output, 0xa5U);
            for (j = 0U; j < set.count; ++j)
            {
                uint8_t packet[BC_VOICE_PACKET_MAX];
                memcpy(packet, set.packets[j], set.lengths[j]);
                if (j == i)
                    packet[byte] ^= 1U;
                result = receive_checked(&receiver, &output, 11U, 140U + j,
                                         packet, set.lengths[j]);
            }
            CHECK(result != BC_WIRE_MESSAGE);
            CHECK(message_guards_ok(&output));
        }
    }
}

static void test_order_duplicates_and_overlap(void)
{
    bc_voice_message message = make_message(0x789aU, 0x89U, BC_VOICE_REQUEST,
                                             40U);
    fragment_set set;
    receiver_box receiver;
    message_box output;
    unsigned i;

    CHECK(fragment_all(&message, 20U, &set));
    CHECK(set.count >= 4U);

    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(receive_checked(&receiver, &output, 20U, 200U, set.packets[1],
                          set.lengths[1]) == BC_WIRE_INVALID);
    CHECK(!receiver.receiver.active);

    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(receive_checked(&receiver, &output, 20U, 200U, set.packets[0],
                          set.lengths[0]) == BC_WIRE_MORE);
    CHECK(receive_checked(&receiver, &output, 20U, 201U, set.packets[2],
                          set.lengths[2]) == BC_WIRE_INVALID);
    CHECK(!receiver.receiver.active);

    /* Exact duplicate ranges are idempotent and do not emit a duplicate
     * message. The final output appears only when the remaining ranges arrive. */
    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(receive_checked(&receiver, &output, 21U, 210U, set.packets[0],
                          set.lengths[0]) == BC_WIRE_MORE);
    CHECK(receive_checked(&receiver, &output, 21U, 211U, set.packets[0],
                          set.lengths[0]) == BC_WIRE_MORE);
    for (i = 1U; i < set.count; ++i)
    {
        bc_wire_result result = receive_checked(&receiver, &output, 21U,
                                                211U + i, set.packets[i],
                                                set.lengths[i]);
        if (i + 1U == set.count)
            CHECK(result == BC_WIRE_MESSAGE);
        else
            CHECK(result == BC_WIRE_MORE);
    }
    CHECK(same_message(&output.message, &message));

    /* Same range with changed bytes is an overlap fault. */
    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(receive_checked(&receiver, &output, 22U, 220U, set.packets[0],
                          set.lengths[0]) == BC_WIRE_MORE);
    {
        uint8_t overlap[BC_VOICE_PACKET_MAX];
        uint16_t overlap_length = BC_VOICE_HEADER + 4U;
        memcpy(overlap, set.packets[0], set.lengths[0]);
        write16(overlap + 8U, 4U);
        overlap[BC_VOICE_HEADER] ^= 0x80U;
        CHECK(receive_checked(&receiver, &output, 22U, 221U, overlap,
                              overlap_length) == BC_WIRE_INVALID);
        CHECK(!receiver.receiver.active);
    }
}

static void test_interleaved_starts_and_ids(void)
{
    bc_voice_message first = make_message(0xfffeU, 0x9aU, BC_VOICE_REQUEST,
                                           17U);
    bc_voice_message second = make_message(0xffffU, 0x9bU, BC_VOICE_RESPONSE,
                                            17U);
    bc_voice_message zero = make_message(0x0000U, 0x9cU, BC_VOICE_EVENT, 17U);
    bc_voice_message one = make_message(0x0001U, 0x9dU, BC_VOICE_REQUEST, 17U);
    fragment_set first_set;
    fragment_set second_set;
    receiver_box receiver;
    message_box output;
    const bc_voice_message *messages[] = {&first, &second, &zero, &one};
    unsigned m;

    CHECK(fragment_all(&first, 20U, &first_set));
    CHECK(fragment_all(&second, 20U, &second_set));
    CHECK(first_set.count >= 2U);
    CHECK(second_set.count >= 2U);

    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(receive_checked(&receiver, &output, 30U, 300U, first_set.packets[0],
                          first_set.lengths[0]) == BC_WIRE_MORE);
    CHECK(receive_checked(&receiver, &output, 30U, 301U,
                          second_set.packets[0], second_set.lengths[0]) ==
          BC_WIRE_MORE);
    {
        unsigned i;
        for (i = 1U; i < second_set.count; ++i)
        {
            bc_wire_result result = receive_checked(
                &receiver, &output, 30U, 301U + i, second_set.packets[i],
                second_set.lengths[i]);
            if (i + 1U == second_set.count)
                CHECK(result == BC_WIRE_MESSAGE);
            else
                CHECK(result == BC_WIRE_MORE);
        }
    }
    CHECK(same_message(&output.message, &second));
    CHECK(receive_checked(&receiver, &output, 30U, 310U, first_set.packets[1],
                          first_set.lengths[1]) == BC_WIRE_INVALID);

    /* IDs are compared as full uint16 values: two active starts with the
     * same low byte must still replace one another as distinct messages. */
    {
        bc_voice_message same_low_first = make_message(
            0x12ffU, 0xa0U, BC_VOICE_REQUEST, 17U);
        bc_voice_message same_low_second = make_message(
            0x13ffU, 0xa1U, BC_VOICE_RESPONSE, 17U);
        fragment_set same_low_first_set;
        fragment_set same_low_second_set;
        receiver_box same_low_receiver;
        message_box same_low_output;
        unsigned i;

        CHECK(fragment_all(&same_low_first, 20U, &same_low_first_set));
        CHECK(fragment_all(&same_low_second, 20U, &same_low_second_set));
        CHECK(same_low_first_set.count >= 2U);
        CHECK(same_low_second_set.count >= 2U);
        init_receiver(&same_low_receiver);
        init_message(&same_low_output, 0xa5U);
        CHECK(receive_checked(&same_low_receiver, &same_low_output, 30U,
                              330U, same_low_first_set.packets[0],
                              same_low_first_set.lengths[0]) == BC_WIRE_MORE);
        CHECK(receive_checked(&same_low_receiver, &same_low_output, 30U,
                              331U, same_low_second_set.packets[0],
                              same_low_second_set.lengths[0]) == BC_WIRE_MORE);
        for (i = 1U; i < same_low_second_set.count; ++i)
        {
            bc_wire_result result = receive_checked(
                &same_low_receiver, &same_low_output, 30U, 331U + i,
                same_low_second_set.packets[i], same_low_second_set.lengths[i]);
            if (i + 1U == same_low_second_set.count)
                CHECK(result == BC_WIRE_MESSAGE);
            else
                CHECK(result == BC_WIRE_MORE);
        }
        CHECK(same_message(&same_low_output.message, &same_low_second));
        CHECK(receive_checked(&same_low_receiver, &same_low_output, 30U,
                              340U, same_low_first_set.packets[1],
                              same_low_first_set.lengths[1]) == BC_WIRE_INVALID);
    }

    /* The complete 16-bit ID space, including the 0xffff -> 0x0000 wrap,
     * uses the full ID in the reassembler despite the legacy low-byte field. */
    for (m = 0U; m < ARRAY_LEN(messages); ++m)
        CHECK(receive_all(messages[m], 244U, 31U + m, 320U + m));
}

static void test_epoch_and_timeout_wrap(void)
{
    bc_voice_message old = make_message(0xaaaaU, 0xa1U, BC_VOICE_REQUEST, 20U);
    bc_voice_message newer = make_message(0xbbbbU, 0xb2U, BC_VOICE_RESPONSE,
                                          20U);
    fragment_set old_set;
    fragment_set new_set;
    receiver_box receiver;
    message_box output;
    unsigned i;

    CHECK(fragment_all(&old, 20U, &old_set));
    CHECK(fragment_all(&newer, 20U, &new_set));
    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(receive_checked(&receiver, &output, 40U, 400U, old_set.packets[0],
                          old_set.lengths[0]) == BC_WIRE_MORE);
    CHECK(receive_checked(&receiver, &output, 41U, 401U, old_set.packets[1],
                          old_set.lengths[1]) == BC_WIRE_INVALID);
    CHECK(!receiver.receiver.active);
    CHECK(receive_checked(&receiver, &output, 41U, 402U, new_set.packets[0],
                          new_set.lengths[0]) == BC_WIRE_MORE);
    for (i = 1U; i < new_set.count; ++i)
    {
        bc_wire_result result = receive_checked(&receiver, &output, 41U,
                                                402U + i, new_set.packets[i],
                                                new_set.lengths[i]);
        if (i + 1U == new_set.count)
            CHECK(result == BC_WIRE_MESSAGE);
        else
            CHECK(result == BC_WIRE_MORE);
    }
    CHECK(same_message(&output.message, &newer));

    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(receive_checked(&receiver, &output, 42U, UINT32_MAX - 10U,
                          old_set.packets[0], old_set.lengths[0]) ==
          BC_WIRE_MORE);
    /* UINT32_MAX - 10 to 988 is 999 ms after wrap and remains live. */
    CHECK(receive_checked(&receiver, &output, 42U, 988U,
                          old_set.packets[1], old_set.lengths[1]) ==
          BC_WIRE_MORE);
    /* UINT32_MAX - 10 to 989 is exactly 1000 ms after wrap. */
    CHECK(receive_checked(&receiver, &output, 42U, 989U, old_set.packets[1],
                          old_set.lengths[1]) == BC_WIRE_INVALID);
    CHECK(!receiver.receiver.active);
    CHECK(receive_checked(&receiver, &output, 42U, 990U, new_set.packets[0],
                          new_set.lengths[0]) == BC_WIRE_MORE);
    for (i = 1U; i < new_set.count; ++i)
    {
        bc_wire_result result = receive_checked(&receiver, &output, 42U,
                                                990U + i, new_set.packets[i],
                                                new_set.lengths[i]);
        if (i + 1U == new_set.count)
            CHECK(result == BC_WIRE_MESSAGE);
        else
            CHECK(result == BC_WIRE_MORE);
    }
    CHECK(same_message(&output.message, &newer));
}

static void test_receiver_invalid_arguments(void)
{
    bc_voice_message message = make_message(0xccccU, 0xc1U, BC_VOICE_REQUEST,
                                             2U);
    fragment_set set;
    receiver_box receiver;
    message_box output;

    CHECK(fragment_all(&message, 250U, &set));
    init_receiver(&receiver);
    init_message(&output, 0xa5U);
    CHECK(bc_voice_receive(NULL, 1U, 1U, set.packets[0], set.lengths[0],
                           &output.message) == BC_WIRE_INVALID);
    CHECK(bc_voice_receive(&receiver.receiver, 1U, 1U, set.packets[0],
                           set.lengths[0], NULL) == BC_WIRE_INVALID);
    CHECK(receiver_guards_ok(&receiver));
    CHECK(message_guards_ok(&output));
}

int main(void)
{
    test_crc_vector();
    test_interoperability_vector();
    test_att_sizes_and_payload_bounds();
    test_fragment_canaries_and_capacity();
    test_fragment_invalid_arguments();
    test_receiver_truncation_and_bounds();
    test_frame_headers_and_corruption();
    test_order_duplicates_and_overlap();
    test_interleaved_starts_and_ids();
    test_epoch_and_timeout_wrap();
    test_receiver_invalid_arguments();

    if (failures != 0U)
    {
        fprintf(stderr, "FAIL: %u of %u checks\n", failures, checks);
        return 1;
    }
    printf("PASS: %u checks (fragmentation, CRC, bounds, corruption,\n"
           "      ordering, duplicate ranges, epochs, timeout wrap and canaries)\n",
           checks);
    return 0;
}
