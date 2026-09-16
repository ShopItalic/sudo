"""Remove firmware duration filtering; retain safe capture close and receipt cleanup."""
from factory_local_recording_v8 import function_span
from factory_disable_swipes import once

APP = "firmware/bc_ros/bc_application/"


def replace_function(source, signature, body):
    start, end = function_span(source, signature)
    return source[:start] + signature + "\n{\n" + body + "\n}" + source[end:]


def patch_capture(source):
    source = once(source, "unsigned queued, steps, samples;", "unsigned queued;")
    source = once(source,
        "        capture.steps = steps <= FACTORY_SHORT_MAX_STEPS ? steps : 0U;\n"
        "        capture.samples = 0; capture.ready = capture.consumed = 0;",
        "        capture.ready = capture.consumed = 0;")
    source = once(source, "bool factory_capture_begin(unsigned steps)\n{\n    bool ok;",
        "bool factory_capture_begin(unsigned steps)\n{\n    bool ok;\n    (void)steps; /* Duration filtering belongs to the app. */")
    source = replace_function(source, "void factory_capture_written(unsigned bytes, bool success)",
        "    (void)bytes;\n    taskENTER_CRITICAL();\n"
        "    if (capture.active && capture.sender && !success) capture.failed = true;\n    taskEXIT_CRITICAL();")
    source = replace_function(source, "bool factory_capture_should_discard(void)",
        "    return false; /* Every duration is retained on the Ring. */")
    if "void factory_capture_p11_samples(unsigned samples, bool written)" in source:
        source = replace_function(source, "void factory_capture_p11_samples(unsigned samples, bool written)",
            "    (void)samples;\n    taskENTER_CRITICAL();\n"
            "    if (capture.active && capture.sender && !written) capture.failed = true;\n    taskEXIT_CRITICAL();")
    return source


def patch_file(source):
    source = once(source,
        "/* A short capture cannot roll over (the slice is much longer than 5 s).\n"
        " * Bind its initial path only for this capture, so a foreign handle/name change\n"
        " * can never redirect automatic deletion to an older recording. */\n"
        "static char factory_capture_path[FACTORY_DELETE_PATH_SIZE];",
        "/* Capture ownership is retained through drain and close. No duration-based deletion. */")
    source = replace_function(source, "bool app_factory_capture_bind_file(void)",
        "    return factory_capture_active() && app_ppg_file_hardle.ppg_file_status &&\n"
        "        app_ppg_file_hardle.fls_status == PPG_FLS_WRITE;")
    source = replace_function(source, "bool app_factory_capture_finish_file(void)",
        "    bool ok, captured;\n"
        "    if (!factory_capture_active() || !factory_capture_quiet() ||\n"
        "        app_ppg_file_hardle.fls_status != PPG_FLS_WRITE ||\n"
        "        !app_ppg_file_hardle.ppg_file_status) return false;\n"
        "    captured = factory_capture_succeeded();\n"
        "    ok = lk_ppg_file_close(&app_ppg_file_hardle) == PPG_FILE_SUCCESS;\n"
        "    bc_spi_flash_device_close();\n"
        "    app_ppg_file_hardle.fls_status = ok ? PPG_FLS_IDIE : PPG_FLS_BUSY;\n"
        "    factory_capture_end();\n"
        "    return ok && captured;")
    return source


def patch_settings(source):
    if source.count("#define SHORT_MAGIC 0x01524353UL") != 1:
        raise ValueError("Unexpected short-settings parent")
    return '''#include "app_factory_short.h"
#include "app_cmd_handler.h"
#include "app_package.h"
#include <string.h>

/* Legacy wire compatibility only. No FDS registration, reads or writes.
 * Existing duration records are ignored and remain untouched in flash. */
enum { OK, INVALID, BUSY, STORAGE, CONFLICT, NOT_READY };
void app_factory_short_init(void) {}
void app_factory_short_service(void) {}
unsigned app_factory_short_steps(void) { return 0U; }
static void reply(const uint8_t *request, unsigned status)
{
    uint8_t packet[17] = {0};
    memcpy(packet, request, 9); packet[9] = (uint8_t)status;
    app_package_send_enqueue((struct app_cmd_package *)packet, sizeof(packet));
}
bool app_factory_short_command(const uint8_t *d, unsigned n)
{
    if (!d || n < 4U || d[2] != 0x84U || (d[3] != 0x13U && d[3] != 0x14U)) return false;
    if (n < 9U) return true;
    if (d[0] != 0U || d[4] != 1U || n != (d[3] == 0x13U ? 9U : 14U)) {
        reply(d, INVALID); return true;
    }
    if (d[3] == 0x14U) {
        if (d[13] != 0U) { reply(d, INVALID); return true; }
        if (d[9] || d[10] || d[11] || d[12]) { reply(d, CONFLICT); return true; }
    }
    reply(d, OK); return true;
}
'''


PATCHES = {APP + "app_factory_short.c": patch_capture,
           APP + "app_ppg_file_data_handler.c": patch_file,
           APP + "app_factory_short_settings.c": patch_settings}
