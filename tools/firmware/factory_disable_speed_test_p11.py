"""P11-only retirement of the supplier BLE/TCP traffic generator.

Apply to a fresh P11 tree, after the existing overlays. Preserve the vendor
source for diagnostic builds; exclude its task from production projects and
explicitly reject both command routes. No audio, transfer, or sensor changes.
"""
import hashlib
from pathlib import Path

from factory_local_recording_v8 import function_span

APP = "firmware/bc_ros/bc_application/"
UNIT = "app_ble_speed_handler.c"
REJECT_SIGNATURE = "static uint8_t app_test_ble_speed_callback(struct app_cmd_package * cmd_package)"
REJECT_BODY = """    uint8_t reply[5] = {0};
    /* P11: traffic-generator commands are unsupported, including stop.
     * Do not read payload bytes or modify the request/recorder state. */
    if (!cmd_package || cmd_package->length < 4U) return 0;
    memcpy(reply, cmd_package, 4U);
    app_package_send_enqueue((struct app_cmd_package *)reply, sizeof(reply));
    return 0;"""


def once(source, old, new):
    if source.count(old) != 1:
        raise ValueError("Unexpected P11 speed-test patch input: " + old[:90])
    return source.replace(old, new, 1)


def patch_app(source):
    source = once(source, '#include "app_ble_speed_handler.h"', '')
    return once(source, "  app_ble_speed_time_create();",
                "  /* P11: no production traffic-generator task. */")


def patch_commands(source):
    source = once(source, '#include "app_ble_speed_handler.h"', '')
    start, end = function_span(source, REJECT_SIGNATURE)
    source = source[:start] + REJECT_SIGNATURE + "\n{\n" + REJECT_BODY + "\n}" + source[end:]
    # The shared task also had a legacy TCP/Wi-Fi diagnostic entry point.
    # Removing only the BLE entry point would leave dangling references.
    signature = "static uint8_t app_cmd_wifi_event(struct app_cmd_package * cmd_package)"
    start, end = function_span(source, signature)
    body = source[start:end]
    first, last = body.index("    case 0x00:"), body.index("    case 0x01:")
    body = body[:first] + """    case 0x00: /* P11: retired TCP traffic generator */
      app_test_ble_speed_callback(cmd_package);
      break;
""" + body[last:]
    return source[:start] + body + source[end:]


def patch_package(source):
    start, end = function_span(source, "void app_package_speed_test_up(uint32_t seq,uint8_t leng)")
    return source[:start] + "/* P11: synthetic traffic sender removed. */" + source[end:]


def patch_package_header(source):
    return once(source, "void app_package_speed_test_up(uint32_t seq,uint8_t leng);", "")


PATCHES = {APP + "app.c": patch_app,
           APP + "app_cmd_handler.c": patch_commands,
           APP + "app_package.c": patch_package,
           APP + "app_package.h": patch_package_header}


def apply_source_patches(destination, changes):
    """Merge into the P11 preparation ledger, retaining each parent digest."""
    destination = Path(destination)
    for relative, patch in PATCHES.items():
        path = destination / relative
        before = path.read_bytes()
        after = patch(before.decode("latin1").replace("\r\n", "\n"))
        after = after.replace("\n", "\r\n").encode("latin1")
        path.write_bytes(after)
        item = next((c for c in changes if c["path"] == relative), None)
        if item is None:
            item = {"path": relative, "beforeSha256": hashlib.sha256(before).hexdigest()}
            changes.append(item)
        item["sha256"] = hashlib.sha256(after).hexdigest()


def patch_project(tree):
    """Exclude exactly one vendor task unit; keep its source on disk."""
    entries = [(files, entry) for files in tree.findall(".//Files") for entry in files
               if entry.findtext("FileName") == UNIT]
    if len(entries) != 1:
        raise ValueError("Expected exactly one supplier speed-test compilation unit")
    files, entry = entries[0]
    files.remove(entry)


def verify_retirement(artifact):
    """Fail closed if an old worker, control API, or generator is still linked."""
    forbidden = ("app_ble_speed_send_handler_thread", "app_ble_speed_time_create",
                 "app_ble_speed_test_start", "app_ble_speed_test_stop",
                 "app_wifi_speed_test_start", "app_wifi_speed_test_stop",
                 "app_speed_mode_set", "app_speed_mode_get", "app_package_speed_test_up")
    present = [name for name in forbidden if artifact.symbols.get(name)]
    if present:
        raise ValueError("Retired speed-test code remains linked: " + ", ".join(present))
    return {"speedTestDisabled": True, "retiredSymbolsAbsent": list(forbidden)}
