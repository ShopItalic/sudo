"""Keep local recording independent of BLE connection on new factory candidates."""
import hashlib
from pathlib import Path
from factory_local_recording_v8 import function_span

BLE = "firmware/bc_ros/bc_application/app_ble_handler.c"
CONNECT = "static void app_ble_connect_callback(void)"
STOP_ON_CONNECT = "    app_package_mic_recording_stop_isr();"
KEEP_RECORDING = "    /* Bluetooth connection changes do not stop local recording. */"


def patch_ble(source):
    start, end = function_span(source, CONNECT)
    callback = source[start:end]
    if callback.count(STOP_ON_CONNECT) != 1:
        raise ValueError("Expected one factory offline-recording stop in the BLE connected callback")
    callback = callback.replace(STOP_ON_CONNECT, KEEP_RECORDING, 1)
    return source[:start] + callback + source[end:]


def apply(destination, changes):
    """Apply after other BLE patches and retain one final manifest hash."""
    path = Path(destination) / BLE
    before = path.read_bytes()
    after = patch_ble(before.decode("latin1").replace("\r\n", "\n"))
    after = after.replace("\n", "\r\n").encode("latin1")
    path.write_bytes(after)
    item = next((item for item in changes if item["path"] == BLE), None)
    if item is None:
        item = {"path": BLE, "beforeSha256": hashlib.sha256(before).hexdigest()}
        changes.append(item)
    item["sha256"] = hashlib.sha256(after).hexdigest()
