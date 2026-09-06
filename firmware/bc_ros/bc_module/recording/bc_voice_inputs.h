#ifndef BC_VOICE_INPUTS_H
#define BC_VOICE_INPUTS_H

#include <stdbool.h>
#include <stdint.h>

enum bc_voice_input {
    BC_VOICE_INPUT_HOLD = 1,
    BC_VOICE_INPUT_DOUBLE = 2,
    BC_VOICE_INPUT_TRIPLE = 3
};
enum bc_voice_input_action {
    BC_VOICE_INPUT_DISABLED = 0,
    BC_VOICE_INPUT_PTT = 1,
    BC_VOICE_INPUT_MEMO = 2,
    BC_VOICE_INPUT_APP = 3
};
enum bc_voice_input_phase {
    BC_VOICE_INPUT_ACTIVATED = 1,
    BC_VOICE_INPUT_RELEASED = 2,
    BC_VOICE_INPUT_CANCELLED = 3
};
#define BC_VOICE_INPUT_HOLD_MIN_MS 500U
#define BC_VOICE_INPUT_HOLD_MAX_MS 10000U
#define BC_VOICE_INPUT_HOLD_DEFAULT_MS 1000U

typedef struct {
    uint16_t hold_ms;
    uint8_t hold_action, double_action, triple_action;
} bc_voice_inputs;

static inline bool bc_voice_inputs_valid(const bc_voice_inputs *v)
{
    return v && v->hold_ms >= BC_VOICE_INPUT_HOLD_MIN_MS &&
        v->hold_ms <= BC_VOICE_INPUT_HOLD_MAX_MS &&
        v->hold_action <= BC_VOICE_INPUT_APP &&
        v->double_action <= BC_VOICE_INPUT_APP &&
        v->triple_action <= BC_VOICE_INPUT_APP &&
        v->double_action != BC_VOICE_INPUT_PTT &&
        v->triple_action != BC_VOICE_INPUT_PTT;
}
static inline uint16_t bc_voice_inputs_mask(const bc_voice_inputs *v)
{
    return (uint16_t)((v->hold_action ? 0x08U : 0U) |
        (v->double_action ? 0x02U : 0U) | (v->triple_action ? 0x04U : 0U));
}
static inline bool bc_voice_inputs_memo(const bc_voice_inputs *v)
{
    return v->hold_action == BC_VOICE_INPUT_MEMO ||
        v->double_action == BC_VOICE_INPUT_MEMO ||
        v->triple_action == BC_VOICE_INPUT_MEMO;
}
#endif
