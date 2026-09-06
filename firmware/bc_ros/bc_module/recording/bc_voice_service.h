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
    bool (*get)(void *, bc_voice_inputs *, uint8_t *touch_status);
    bc_rec_result (*set)(void *, const bc_voice_inputs *);
} bc_voice_inputs_port;

typedef struct {
    uint32_t sequence, at_ms;
    uint8_t input, phase;
} bc_voice_input_event;
#define BC_VOICE_INPUT_EVENT_SLOTS 4U
#define BC_VOICE_INPUT_EVENT_TTL_MS 1000U

typedef struct {
    void *ctx;
    bool (*send)(void *ctx, const uint8_t *data, uint16_t length, uint32_t epoch);
    /* Commit settings atomically before reporting success. */
    bc_rec_result (*settings)(void *ctx, const bc_voice_settings *settings);
} bc_voice_service_port;

typedef struct {
    void *ctx;
    /* Queue application feedback only. This callback runs on the voice
     * worker and must not perform hardware I/O or reenter the service. */
    void (*set_outcome)(void *ctx, uint64_t recording_id, uint8_t outcome);
} bc_voice_outcome_port;

#define BC_VOICE_LIVE_PREFIX_SLOTS 32U

typedef struct {
    uint32_t sequence;
    uint8_t data[BC_REC_FRAME_MAX];
} bc_voice_live_frame;

typedef struct {
    bc_recording *recording;
    bc_rec_store *store;
    bc_voice_gesture *gesture;
    bc_voice_service_port port;
    bc_voice_outcome_port outcome_port;
    bc_voice_tuning_port tuning_port;
    bc_voice_inputs_port inputs_port;
    bc_voice_input_event input_events[BC_VOICE_INPUT_EVENT_SLOTS];
    uint8_t input_read, input_count;
    uint32_t input_sequence, input_tx_ms;
    bool input_hold_accepted;
    bc_voice_settings settings;
    bc_voice_receiver receiver;
    bc_voice_message controls[BC_VOICE_CONTROL_SLOTS];
    uint8_t control_read, control_count;
    bc_voice_message tx;
    uint16_t tx_offset, next_message;
    bool tx_active;
    bc_rec_snapshot latest;
    bool state_pending, state_urgent;
    uint32_t state_sent_ms;
    uint32_t epoch, now_ms, ready_ms, live_progress_ms;
    bool connected, ready;
    uint64_t token_recording;
    uint32_t token_counter, live_token, live_ack;
    uint32_t live_ends[4];
    uint8_t live_count;
    /* Raw local-accepted frames remain here until READY and the normal four
     * message ACK window can drain them. The service worker is the sole owner. */
    bc_voice_live_frame live_prefix[BC_VOICE_LIVE_PREFIX_SLOTS];
    uint8_t live_prefix_read, live_prefix_count;
    uint32_t live_sequence;
    bool live_disabled;
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
    /* Retry scheduling is separate from actual verification/durable ACK
     * progress; retransmits and duplicate requests cannot renew the lease. */
    uint32_t transfer_progress_ms;
    uint32_t archive_progress_ms;
    bool verifying, transferring;
    /* Keyboard insertion confirmation is a one-link, one-record lease. */
    uint64_t outcome_recording_id;
    uint64_t outcome_ready_recording_id;
    uint32_t outcome_live_token;
    uint32_t outcome_epoch;
    uint32_t outcome_terminal_ms;
    bool outcome_ready_accepted;
    bool outcome_terminal_ready;
    bool outcome_reported;
} bc_voice_service;

bool bc_voice_service_init(bc_voice_service *service, bc_recording *recording,
                            bc_rec_store *store, bc_voice_gesture *gesture,
                            const bc_voice_service_port *port,
                            const bc_voice_settings *settings);
bool bc_voice_tuning_valid(const bc_voice_tuning *tuning);
bool bc_voice_service_set_inputs_port(bc_voice_service *service,
                                       const bc_voice_inputs_port *port);
bool bc_voice_service_input(bc_voice_service *service, uint8_t input, uint8_t phase);
bool bc_voice_service_set_tuning_port(bc_voice_service *service,
                                      const bc_voice_tuning_port *port);
bool bc_voice_service_set_outcome_port(bc_voice_service *service,
                                       const bc_voice_outcome_port *port);
/* Worker context only. Connection changes revoke live readiness and discard
 * transport state, never the local recording. */
void bc_voice_service_link(bc_voice_service *service, uint32_t epoch, bool connected);
void bc_voice_service_receive(bc_voice_service *service, uint32_t epoch,
                               const uint8_t *packet, uint16_t length, uint32_t now_ms);
/* At most one bounded archive verification/read and four fragment enqueues
 * from one message; stop immediately on backpressure or message completion. */
bool bc_voice_service_poll(bc_voice_service *service, uint32_t now_ms, uint16_t att_limit);
/* Recording-port callbacks: no I/O, reentry, or waits. */
void bc_voice_service_changed(bc_voice_service *service, const bc_rec_snapshot *snapshot);
bool bc_voice_service_live(bc_voice_service *service, uint64_t id, uint32_t sequence,
                            const uint8_t *data, uint16_t length);
/* Before a new capture, close the archive reader and preserve its committed
 * client offset. The client may Resume after the recording finishes. */
bc_rec_result bc_voice_service_cancel_archive(bc_voice_service *service);

#endif
