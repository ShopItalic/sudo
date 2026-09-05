#ifndef BC_VOICE_SERVICE_H
#define BC_VOICE_SERVICE_H

#include "bc_voice_gesture.h"
#include "bc_rec_store.h"
#include "bc_voice_protocol.h"

typedef struct {
    uint32_t ptt_limit_ms;
    uint32_t memo_limit_ms;
    bool memo_enabled;
    bool led_enabled;
    bool haptic_enabled;
} bc_voice_settings;

typedef struct {
    uint8_t touch_set, touch_clear, haptic_strength;
    uint16_t start_active_ms, stop_active_ms;
} bc_voice_tuning;

typedef struct {
    void *ctx;
    bool (*get)(void *ctx, bc_voice_tuning *tuning, uint8_t *touch_status);
    /* Commit desired values before publishing them to the sensor worker. */
    bc_rec_result (*set)(void *ctx, const bc_voice_tuning *tuning);
} bc_voice_tuning_port;

typedef struct {
    void *ctx;
    bool (*send)(void *ctx, const uint8_t *data, uint16_t length, uint32_t epoch);
    /* Commit settings atomically before reporting success. */
    bc_rec_result (*settings)(void *ctx, const bc_voice_settings *settings);
} bc_voice_service_port;

typedef struct {
    bc_recording *recording;
    bc_rec_store *store;
    bc_voice_gesture *gesture;
    bc_voice_service_port port;
    bc_voice_tuning_port tuning_port;
    bc_voice_settings settings;
    bc_voice_receiver receiver;
    bc_voice_message controls[BC_VOICE_CONTROL_SLOTS];
    uint8_t control_read, control_count;
    bc_voice_message tx;
    uint16_t tx_offset, next_message;
    bool tx_active;
    bc_voice_message live;
    bool live_pending;
    bc_rec_snapshot latest;
    bool state_pending, state_urgent;
    uint32_t state_sent_ms;
    uint32_t epoch, now_ms, ready_ms, live_progress_ms;
    bool connected, ready;
    uint64_t token_recording;
    uint32_t token_counter, live_token, live_ack;
    uint32_t live_ends[4];
    uint8_t live_count;
    bc_voice_message stop_request;
    uint64_t stop_id;
    bool stop_pending;
    bc_rec_snapshot stop_snapshot;
    bool stop_ready;
    bc_rec_store_reader reader;
    bc_rec_start archive_start;
    bc_rec_file archive_file;
    bc_voice_message resume_request;
    uint32_t transfer_token, transfer_offset, transfer_next, transfer_ack;
    uint32_t transfer_ends[BC_VOICE_TRANSFER_WINDOW];
    uint8_t transfer_count;
    uint32_t transfer_progress_ms;
    bool verifying, transferring;
} bc_voice_service;

bool bc_voice_service_init(bc_voice_service *service, bc_recording *recording,
                            bc_rec_store *store, bc_voice_gesture *gesture,
                            const bc_voice_service_port *port,
                            const bc_voice_settings *settings);
bool bc_voice_tuning_valid(const bc_voice_tuning *tuning);
bool bc_voice_service_set_tuning_port(bc_voice_service *service,
                                      const bc_voice_tuning_port *port);
/* Worker context only. Connection changes revoke live readiness and discard
 * transport state, never the local recording. */
void bc_voice_service_link(bc_voice_service *service, uint32_t epoch, bool connected);
void bc_voice_service_receive(bc_voice_service *service, uint32_t epoch,
                               const uint8_t *packet, uint16_t length, uint32_t now_ms);
/* At most one bounded archive verification/read and one fragment enqueue. */
bool bc_voice_service_poll(bc_voice_service *service, uint32_t now_ms, uint16_t att_limit);
/* Recording-port callbacks: no I/O, reentry, or waits. */
void bc_voice_service_changed(bc_voice_service *service, const bc_rec_snapshot *snapshot);
bool bc_voice_service_live(bc_voice_service *service, uint64_t id, uint32_t sequence,
                            const uint8_t *data, uint16_t length);
/* Before a new capture, close the archive reader and preserve its committed
 * client offset. The client may Resume after the recording finishes. */
bc_rec_result bc_voice_service_cancel_archive(bc_voice_service *service);

#endif
