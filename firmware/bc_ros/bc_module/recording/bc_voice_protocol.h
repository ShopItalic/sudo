#ifndef BC_VOICE_PROTOCOL_H
#define BC_VOICE_PROTOCOL_H

#include "bc_voice_wire.h"

/* Candidate protocol v1. All integers are unsigned little endian. Every
 * request begins with a nonzero u32 request ID. Responses echo it followed
 * by one bc_rec_result byte. IDs identify operations; recording IDs remain
 * the persistent Start idempotency key across reconnects and reboots. */
enum bc_voice_kind {
    BC_VOICE_HELLO = 1,       /* request ID only */
    BC_VOICE_START = 2,       /* + recording u64, trigger u8, limit_ms u32 */
    BC_VOICE_STOP = 3,        /* + recording u64; response after finalization */
    BC_VOICE_QUERY = 4,       /* + recording u64; 0 queries current owner */
    BC_VOICE_READY = 5,       /* + enabled u8; renew every <=5 seconds */
    BC_VOICE_LIVE_ACK = 6,    /* + stream token u32, next sequence u32 */
    BC_VOICE_SETTINGS_SET = 7,/* + ptt_ms u32,memo_ms u32,memo/LED/haptic u8 */
    BC_VOICE_SETTINGS_GET = 8,
    BC_VOICE_CATALOG = 9,     /* + after recording u64; one next entry */
    BC_VOICE_RESUME = 10,     /* + recording u64, offset u32, token u32 */
    BC_VOICE_TRANSFER_ACK = 11,/* + transfer token u32, next byte offset u32 */
    BC_VOICE_RECEIPT = 12,    /* + recording u64, bytes u32, CRC u32, delete u8 */
    BC_VOICE_CANCEL = 13,    /* + transfer token u32 */
    BC_VOICE_TUNING_SET = 14,/* + touch set/clear/strength u8, start/stop ms u16 */
    BC_VOICE_TUNING_GET = 15,
    BC_VOICE_PHONE_OUTCOME = 16, /* + recording u64, live token u32, outcome u8 */
    BC_VOICE_STATE = 0x40,   /* snapshot; request ID is 0 */
    BC_VOICE_LIVE = 0x41,    /* stream token u32, sequence u32, 220 raw bytes */
    BC_VOICE_FILE = 0x42     /* transfer token u32, absolute offset u32, raw */
};

#define BC_VOICE_READY_MS 10000U
#define BC_VOICE_LIVE_STALL_MS 2000U
#define BC_VOICE_RETRY_MS 1500U
#define BC_VOICE_ARCHIVE_STALL_MS 30000U
#define BC_VOICE_TRANSFER_WINDOW 6U
#define BC_VOICE_CONTROL_SLOTS 4U
#define BC_VOICE_TX_BURST 4U
#define BC_VOICE_PHONE_OUTCOME_WINDOW_MS 10000U

/* Phone outcome values. Unknown values are rejected. */
#define BC_VOICE_PHONE_OUTCOME_KEYBOARD_INSERTED 1U

enum bc_voice_capability {
    BC_VOICE_CAP_LOCAL = 1U << 0,
    BC_VOICE_CAP_PTT = 1U << 1,
    BC_VOICE_CAP_MEMO = 1U << 2,
    BC_VOICE_CAP_LIVE = 1U << 3,
    BC_VOICE_CAP_RESUME = 1U << 4,
    BC_VOICE_CAP_CUSTODY = 1U << 5,
    BC_VOICE_CAP_SETTINGS = 1U << 6,
    BC_VOICE_CAP_TUNING = 1U << 7,
    BC_VOICE_CAP_PHONE_OUTCOME = 1U << 8
};

/* Snapshot response payload:
 * 0 request u32, 4 result u8, 5 recording u64, 13 trigger u8,
 * 14 phase u8, 15 recording error u8, 16 flags u8 (complete=1,
 * recovered=2, delivered=4), 17 limit_ms u32, 21 revision u32,
 * 25 accepted bytes u32, 29 accepted frames u32, 33 durable bytes u32,
 * 37 file bytes u32, 41 file frames u32, 45 raw CRC u32,
 * 49 live queued frames u32, 53 live dropped frames u32,
 * 57 live token u32, 61 name length u8, 62 name bytes (no NUL).
 * Stored catalog/query metadata is not a fresh scan of raw content. The
 * resumable reader verifies the complete committed prefix before delivery.
 * CATALOG returns this snapshot with NOT_FOUND and recording=0 at the end.
 * HELLO extends the normal 5-byte response with capabilities u32, sample
 * rate u16, samples/block u16, bytes/block u16, checkpoint_ms u16,
 * release bound_ms u16, transfer window u8 (20 bytes total).
 * READY extends it with live token u32 and current recording u64 (17 bytes).
 * SETTINGS responses extend it with the same 11 settings bytes as SET.
 * TUNING responses add touch set/clear/strength u8, start/stop active_ms u16,
 * then touch apply status u8: 0 pending, 1 verified on sensor, 2 I/O error.
 * SET success confirms durable desired values; it does not claim that the
 * sensor has applied them. Sensor writes wait for a valid release report.
 * RESUME extends it with recording u64, transfer token u32, file bytes u32,
 * CRC u32, requested offset u32 (29 bytes), only after raw verification.
 * ACK offsets mean bytes durably retained by the client. The separate exact
 * whole-file RECEIPT is required before deletion; queue acceptance is never
 * custody. A terminal partial recording is retrievable with its flags.
 */

static inline uint32_t bc_voice_get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline uint64_t bc_voice_get64(const uint8_t *p)
{
    return (uint64_t)bc_voice_get32(p) | ((uint64_t)bc_voice_get32(p + 4) << 32);
}
static inline void bc_voice_put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static inline void bc_voice_put32(uint8_t *p, uint32_t v)
{
    unsigned i;
    for (i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (i * 8));
}
static inline void bc_voice_put64(uint8_t *p, uint64_t v)
{
    bc_voice_put32(p, (uint32_t)v); bc_voice_put32(p + 4, (uint32_t)(v >> 32));
}

#endif
