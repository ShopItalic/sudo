"""Retire swipe output while preserving the P10 Bluetooth service contract."""
from factory_local_recording_v8 import function_span

CONTROLS = "firmware/bc_ros/bc_application/app_factory_controls.c"


def once(source, old, new):
    if source.count(old) != 1:
        raise ValueError("Unexpected swipe retirement input: " + old[:90])
    return source.replace(old, new)


def patch_controls(source):
    start, end = function_span(source, "bool app_factory_controls_swipe_enabled(unsigned direction)")
    source = source[:start] + """bool app_factory_controls_swipe_enabled(unsigned direction)
{
    /* P10-r3/P11: retained HID service for pairing compatibility, no input. */
    (void)direction;
    return false;
}""" + source[end:]
    source = once(source, "active_values = current[2];", "active_values = current[2] | SWIPE_MUTED_MASK;")
    source = once(source,
        "if (r[4] >= 4U) packet[19] |= (uint8_t)((~current[2]) & SWIPE_MUTED_MASK);",
        "/* P10-r3/P11: high nibble is zero in every reply, including errors. */")
    source = once(source, "if (current[0] != MAGIC) {",
        "/* Retire prior saved opt-ins in RAM after validating their CRC.\n"
        "         * No boot-time flash write; the next explicit save persists Off. */\n"
        "        if (current[0] != MAGIC || (current[2] & SWIPE_MUTED_MASK) != SWIPE_MUTED_MASK) {")
    source = once(source, "if (d[4] == 4U && ((~(uint32_t)d[18]) & SWIPE_MUTED_MASK) != (current[2] & SWIPE_MUTED_MASK)) {\n"
        "        reply(d, INVALID); return true; /* A scroll-aware app must use schema 5. */\n"
        "    }\n"
        "    values |= d[4] == 5U ? ((~(uint32_t)d[18]) & SWIPE_MUTED_MASK) : (current[2] & SWIPE_MUTED_MASK);",
        "/* Old and new apps may save recording controls, never enable swipes. */\n"
        "    if (d[4] >= 4U && (d[18] & 0xf0U) != 0U) { reply(d, INVALID); return true; }\n"
        "    values |= SWIPE_MUTED_MASK;")
    return source


PATCHES = {CONTROLS: patch_controls}
