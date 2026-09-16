"""Remove optional traffic/motion workers from P10-r3 and staged P11 only."""
import hashlib
import re
from pathlib import Path
import xml.etree.ElementTree as ET
import factory_disable_speed_test_p11 as speed
from factory_local_recording_v8 import function_span, replace_function

ROOT = Path(__file__).resolve().parents[2]
APP = speed.APP
GSENSOR = "firmware/bc_ros/bc_module/gsensor/bc_gsensor.c"
QUEUE = "firmware/bc_ros/bc_module/queue/bc_queue.c"


def patch_app(source):
    source = speed.once(source, '#include "app_g_sensor_handler.h"', '#include "app_factory_motion_off.h"')
    source = speed.once(source, '\tapp_g_sensor_time_create();', '\tapp_factory_motion_off(); /* no motion task */')
    return speed.once(source, '\tapp_six_axis_sensor_time_create();', '    /* No IMU sampling timer. */')


def patch_commands(source):
    for signature in ("static uint8_t app_cmd_get_step_count(struct app_cmd_package * cmd_package)",
                      "static uint8_t app_cmd_six_axis_sensor(struct app_cmd_package * cmd_package)"):
        source = replace_function(source, signature, "    return app_test_ble_speed_callback(cmd_package); /* retired diagnostic */")
    # Reject attempts to persist an enabled motion-HID mode. Touch-mode handling
    # and ordinary HID service/pairing remain unchanged.
    signature = "static uint8_t app_cmd_hid(struct app_cmd_package * cmd_package)"
    start, end = function_span(source, signature)
    body = source[start:end]
    body = speed.once(body, '\t\t\tbc_device_hid_info hid_info = {0};',
        '\t\t\tbc_device_hid_info hid_info = {0};\n'
        '            if (cmd_package->length < 7U || cmd_package->data[1] != 0xFFU)\n'
        '                return app_test_ble_speed_callback(cmd_package);')
    return source[:start] + body + source[end:]


def patch_worker(source):
    if source.count("static void app_g_sensor_storage_handler_thread(void *thread_handler)\n{") != 1:
        raise ValueError("Unexpected motion worker source")
    return '''/* P10-r3/P11: no motion task, timer, sampling or persistent writes.
 * Full supplier implementation is retained in the immutable parent source. */
#include "app_g_sensor_handler.h"
#include "bc_device_info.h"
uint16_t app_g_sensor_sport_step_count_get(void) { return bc_device_info_get_sport_count(); }
void app_g_sensor_sport_step_count_clear(void) {}
'''


def patch_sensor(source):
    # Referencing sport_num retained the supplier's whole static data section,
    # including timer callback pointers. Retire the table, not only its callers.
    for signature in ("enum g_sensor_result bc_gsensor_init(void)", "uint8_t bc_gsensor_getId(void)",
                      "bool bc_gsensor_id_hardware_check(void)", "bool bc_gsensor_hardware_check(void)",
                      "uint8_t bc_gsensor_sport_num_get(void)", "void bc_gsensor_sport_num_clear(void)"):
        function_span(source, signature)
    return '''/* Standard 1232 P10-r3/P11: sensor identity only, no motion engine.
 * The complete supplier implementation remains in the immutable parent. */
#include "bc_gsensor.h"
#include "bc_g_sensor_device_port.h"
#include "app_factory_motion_off.h"
void bc_g_sensor_device_find(void) { bc_g_sensor_device_i2c_find(); }
enum g_sensor_result bc_gsensor_init_status(void)
{
    return app_factory_motion_status == FACTORY_MOTION_OFF ? G_SENSOR_SUCCESS : G_SENSOR_FAILD;
}
enum g_sensor_result bc_gsensor_init(void)
{
    app_factory_motion_off();
    return bc_gsensor_init_status();
}
uint8_t bc_gsensor_getId(void) { return app_factory_motion_id(); }
bool bc_gsensor_id_hardware_check(void) { return app_factory_motion_id() == 0x6CU; }
bool bc_gsensor_hardware_check(void)
{
    return app_factory_motion_status == FACTORY_MOTION_OFF && app_factory_motion_id() == 0x6CU;
}
uint8_t bc_gsensor_sport_num_get(void) { return 0; }
void bc_gsensor_sport_num_clear(void) {}
'''


def patch_queue(source):
    start = source.index('.queue_name = "imu data"')
    end = source.index('},', start)
    body = source[start:end]
    body = speed.once(body, '"imu data"', '"imu disabled"')
    body = speed.once(body, '.queue_depth = 10,', '.queue_depth = 1,')
    body = speed.once(body, '.queue_buff_length = sizeof(struct imu_sensor_package),', '.queue_buff_length = 1,')
    return source[:start] + body + source[end:]


PATCHES = {APP + "app.c": patch_app, APP + "app_cmd_handler.c": patch_commands,
           APP + "app_g_sensor_handler.c": patch_worker, GSENSOR: patch_sensor, QUEUE: patch_queue}


def apply(destination, changes, tree):
    destination = Path(destination)
    speed.apply_source_patches(destination, changes)
    for relative, patch in PATCHES.items():
        path = destination / relative
        before = path.read_bytes()
        after = patch(before.decode("latin1").replace("\r\n", "\n")).replace("\n", "\r\n").encode("latin1")
        path.write_bytes(after)
        item = next((c for c in changes if c["path"] == relative), None)
        if item is None:
            item = {"path": relative, "beforeSha256": hashlib.sha256(before).hexdigest()}
            changes.append(item)
        item["sha256"] = hashlib.sha256(after).hexdigest()
    speed.patch_project(tree)
    found = [(files, entry) for files in tree.findall(".//Files") for entry in files
             if entry.findtext("FileName") == "app_six_axis_sensor_handler.c"]
    if len(found) != 1: raise ValueError("Expected one IMU streaming unit")
    found[0][0].remove(found[0][1])
    group = ET.SubElement(tree.find("./Targets/Target/Groups"), "Group")
    ET.SubElement(group, "GroupName").text = "Retired diagnostics - safe sensor off"
    files = ET.SubElement(group, "Files")
    for name in ("app_factory_motion_off.c", "app_factory_motion_off.h"):
        content = (ROOT / "firmware/factory_retirement" / name).read_bytes()
        (destination / APP / name).write_bytes(content)
        changes.append({"path": APP + name, "sha256": hashlib.sha256(content).hexdigest()})
        if name.endswith(".c"):
            entry = ET.SubElement(files, "File")
            ET.SubElement(entry, "FileName").text = name
            ET.SubElement(entry, "FileType").text = "1"
            ET.SubElement(entry, "FilePath").text = "..\\..\\..\\..\\bc_ros\\bc_application\\" + name


def verify(artifact):
    result = speed.verify_retirement(artifact)
    forbidden = ("app_g_sensor_storage_handler_thread", "app_g_sensor_time_create",
                 "app_gsensor_sample_timer_callback", "app_six_axis_sensor_time_create",
                 "app_six_axis_sensor_event", "app_six_axis_sensor_start", "app_imu_poll",
                 "gsensor_int_timer_create", "bc_gsensor_int_init", "sport_count_timer_callback")
    forbidden += ("bc_gsensor_set_sport_state", "bc_gsensor_dataRead", "bc_gsensor_Gyroscope_dataRead",
                  "bc_gsensor_RawData_dataRead", "bc_gsensor_irqOn", "bc_gsensor_fifoRead",
                  "lsm6sdo_init", "lsm6sdo_enable_anymotion", "lsm6sdo_change_acc_odr",
                  "lsm6sdo_on_and_off", "app_hid_photograth_start")
    present = [name for name in forbidden if artifact.symbols.get(name)]
    if present: raise ValueError("Retired motion code remains linked: " + ", ".join(present))
    if not artifact.symbols.get("app_factory_motion_off"):
        raise ValueError("Required sensor shutdown is missing")
    result.update(motionDisabled=True, motionSymbolsAbsent=list(forbidden))
    return result


def startup_stack_budget(artifact, report):
    """Conservatively include the new startup path's indirect board IO calls."""
    depths = {}
    for block in report.split("<P><STRONG>")[1:]:
        name = re.search(r"</a>([^<]+)</STRONG>", block)
        depth = re.search(r"Max Depth = (\d+)", block)
        if not depth and "[Calls]" not in block:
            depth = re.search(r"Stack size (\d+) bytes", block)
        if name and depth:
            depths[name[1]] = max(depths.get(name[1], 0), int(depth[1]))
    for name in ("main", "app_factory_motion_off", "motion_read", "motion_write"):
        if name not in depths: raise ValueError("Missing startup stack chain: " + name)
    # main's direct chain already includes ST setters. Add rather than replace
    # the callback and every deepest board/pin layer omitted by indirect calls.
    board = [v for n, v in depths.items() if n.startswith("bsp_i2c_")]
    gpio = [v for n, v in depths.items() if n.startswith("nrf_gpio_pin_")]
    if not board or not gpio: raise ValueError("Missing indirect board IO stack evidence")
    extras = max(depths["motion_read"], depths["motion_write"]) + max(board) + max(gpio)
    remaining = artifact.stack_size - depths["main"] - extras - 256
    if remaining < 0: raise ValueError("New sensor shutdown exceeds startup stack budget")
    return {"allocationBytes": artifact.stack_size, "mainStaticChainBytes": depths["main"],
            "indirectIOExtraBytes": extras, "exceptionReserveBytes": 256,
            "remainingBytes": remaining,
            "limit": "Conservative static bound for added startup IO, not physical interrupt high-water."}
