#ifndef BC_VOICE_WIRE_H
#define BC_VOICE_WIRE_H

#include <stdbool.h>
#include <stdint.h>

/* Candidate-only extension on the existing BCL characteristic. The factory
 * image and stock SDK do not implement these messages. All integers are LE.
 * Logical payloads exclude the 4-byte CRC added/checked by this module. */
#define BC_VOICE_COMMAND 0x7eU
#define BC_VOICE_VERSION 1U
#define BC_VOICE_HEADER 12U
#define BC_VOICE_BODY_MAX 232U
#define BC_VOICE_PAYLOAD_MAX (BC_VOICE_BODY_MAX - 4U)
#define BC_VOICE_PACKET_MAX 250U
#define BC_VOICE_ASSEMBLY_MS 1000U

typedef enum { BC_VOICE_REQUEST = 0, BC_VOICE_RESPONSE = 1, BC_VOICE_EVENT = 2 } bc_voice_direction;
typedef enum { BC_WIRE_INVALID = 0, BC_WIRE_MORE, BC_WIRE_MESSAGE } bc_wire_result;

typedef struct {
    uint16_t message_id;
    uint8_t kind;
    bc_voice_direction direction;
    uint16_t length;
    uint8_t payload[BC_VOICE_PAYLOAD_MAX];
} bc_voice_message;

typedef struct {
    uint32_t epoch;
    uint32_t started_ms;
    uint16_t message_id;
    uint16_t total;
    uint16_t received;
    uint8_t kind;
    bc_voice_direction direction;
    bool active;
    uint8_t body[BC_VOICE_BODY_MAX];
} bc_voice_receiver;

/* Zero-initialize receiver. A changed connection epoch discards an incomplete
 * message. Ordered fragments and exact duplicate ranges are accepted; gaps,
 * overlap changes, bad CRC, excessive length and expired assemblies cannot
 * produce a command. A new offset-zero message replaces an incomplete one. */
bc_wire_result bc_voice_receive(bc_voice_receiver *receiver, uint32_t epoch,
                                uint32_t now_ms, const uint8_t *packet,
                                uint16_t length, bc_voice_message *message);
/* Returns packet length, or zero for invalid arguments. Offset addresses the
 * payload plus its CRC. Keep the same message immutable until all fragments
 * are accepted by TX. Retry the same offset when its queue is full. */
uint16_t bc_voice_fragment(const bc_voice_message *message, uint16_t offset,
                            uint16_t att_payload_limit, uint8_t *packet,
                            uint16_t capacity, uint16_t *next_offset);
uint32_t bc_voice_crc32(const uint8_t *bytes, uint32_t length);

#endif
