#!/usr/bin/env python3
"""Prepare staged P11 releases from locked P10-r2. Unsigned, never flash."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path
import xml.etree.ElementTree as ET
from factory_p11_base import ROOT, LOCK, prepare_base
import factory_audio_v11 as audio
import factory_charging_v11 as charging
import factory_disable_swipes as swipe_retirement
import factory_keep_short_recordings as keep_short
import factory_retire_diagnostics as retirement

VERSION = "6.0.3.3P11"
OVERLAY = ROOT / "firmware/factory_ptt_v11"
RECORDING = "firmware/bc_ros/bc_module/recording/"
OPUS = "firmware/bc_ros/bc_module/opus/opus-1.6.1/"
CODEC_DEFINES = "OPUS_BUILD FIXED_POINT DISABLE_FLOAT_API NONTHREADSAFE_PSEUDOSTACK CUSTOM_SUPPORT GLOBAL_STACK_SIZE=20480 ENABLE_HARDENING OVERRIDE_celt_fatal"


def committed(path):
    commit = json.loads(LOCK.read_text())["sourceCommit"]
    return subprocess.check_output(["git", "show", commit + ":" + path], cwd=ROOT)


def codec_inputs():
    """Pinned codec source/licenses and recording primitives, no S05 app."""
    commit = json.loads(LOCK.read_text())["sourceCommit"]
    paths = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", commit, OPUS], cwd=ROOT, text=True).splitlines()
    paths += [RECORDING + name for name in (
        "bc_capture.c", "bc_capture.h", "bc_recording.h", "bc_resampler.c", "bc_resampler.h",
        "bc_audio_format.c", "bc_audio_format.h", "bc_opus_encoder.c", "bc_opus_encoder.h",
        "bc_opus_profile.h", "opus_support/custom_support.h")]
    return paths


def prepare_codec(destination):
    changes = []
    for relative in codec_inputs():
        content = committed(relative)
        source_sha = hashlib.sha256(content).hexdigest()
        if relative.endswith("/bc_opus_profile.h"):
            content = audio.once(content.decode(), "#define BC_OPUS_PACKET_MAX 1275U", "#define BC_OPUS_PACKET_MAX 30U")
            content = audio.once(content, "#define BC_OPUS_FRAME_SAMPLES_MAX 960U", "#define BC_OPUS_FRAME_SAMPLES_MAX 320U").encode()
        if relative.endswith("/celt/stack_alloc.h"):
            # ArmCC5 correctly rejects a conditional mixing int and void.
            # Both branches are void; retain the exact pre-write fatal check.
            content = audio.once(content.decode(), '?0:CELT_FATAL("pseudostack overflow")',
                                 '?(void)0:CELT_FATAL("pseudostack overflow")').encode()
        path = destination / relative
        path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(content)
        changes.append({"path": relative, "sourceSha256": source_sha, "sha256": hashlib.sha256(content).hexdigest()})
    return changes


def opus_units(destination):
    result = []
    for filename, name in (("opus_sources.mk", "OPUS_SOURCES"), ("celt_sources.mk", "CELT_SOURCES"),
                           ("silk_sources.mk", "SILK_SOURCES"), ("silk_sources.mk", "SILK_SOURCES_FIXED")):
        text = (destination / OPUS / filename).read_text()
        match = re.search(r"^" + name + r" = (.*?)(?:\n\n|\Z)", text, re.M | re.S)
        if not match: raise ValueError("Missing pinned Opus source list")
        for path in match[1].replace("\\", " ").split():
            if path.endswith(".c") and not any(part in path for part in ("multistream", "projection", "mapping_matrix")):
                result.append(OPUS + path)
    if len(result) != 123 or len(set(result)) != len(result):
        raise ValueError("Unexpected fixed-point Opus subset")
    return result


def apply_keep_short(destination, changes):
    for relative, patch in keep_short.PATCHES.items():
        path = destination / relative; before = path.read_bytes()
        after = patch(before.decode("latin1").replace("\r\n", "\n")).replace("\n", "\r\n").encode("latin1")
        path.write_bytes(after)
        item = next((c for c in changes if c["path"] == relative), None)
        if item is None:
            item = {"path": relative, "beforeSha256": hashlib.sha256(before).hexdigest()}
            changes.append(item)
        item["sha256"] = hashlib.sha256(after).hexdigest()


def apply_charging_only(destination, parent, parent_evidence):
    """R1 retains the P10 capture/codec workers and adds only charging output."""
    changes = []
    for relative, patch in {**charging.PATCHES, **swipe_retirement.PATCHES}.items():
        path = destination / relative; before = path.read_bytes()
        after = patch(before.decode("latin1").replace("\r\n", "\n")).replace("\n", "\r\n").encode("latin1")
        path.write_bytes(after)
        changes.append({"path": relative, "beforeSha256": hashlib.sha256(before).hexdigest(),
                        "sha256": hashlib.sha256(after).hexdigest()})
    apply_keep_short(destination, changes)
    config = destination / "firmware/bc_ros/bc_config/ring_config.h"
    before = config.read_bytes()
    config.write_bytes(audio.once(before.decode("latin1"), '"6.0.3.3P10"', '"6.0.3.3P11"').encode("latin1"))
    changes.append({"path": str(config.relative_to(destination)), "beforeSha256": hashlib.sha256(before).hexdigest(),
                    "sha256": hashlib.sha256(config.read_bytes()).hexdigest()})
    header = destination / audio.APP / "app_factory_charging_p11.h"
    header.write_bytes((OVERLAY / header.name).read_bytes())
    changes.append({"path": str(header.relative_to(destination)), "sha256": hashlib.sha256(header.read_bytes()).hexdigest()})
    old_project = destination / parent["project"]
    project = old_project.with_name("factory_ptt_p11_build_only.uvprojx")
    tree = ET.parse(old_project)
    retirement.apply(destination, changes, tree)
    tree.write(project, encoding="utf-8", xml_declaration=True)
    result = {"version": VERSION, "packageRevision": 1, "releaseName": "P11-r1 — Charging lights",
        "status": "development-not-qualified", "mode": "charging-lights", "parent": parent_evidence,
        "project": str(project.relative_to(destination)), "projectSha256": hashlib.sha256(project.read_bytes()).hexdigest(),
        "changes": changes, "opusUnits": 0, "chargingLights": True, "firmwareShortDeletion": False,
        "signed": False, "flashed": False, "physicallyQualified": False,
        "bootloaderModified": False, "swipesDisabled": True, "speedTestDisabled": True, "motionDisabled": True}
    (destination / "p11-preparation.json").write_text(json.dumps(result, indent=2) + "\n")
    (destination / "NON-FLASHABLE.txt").write_text("Unsigned P11-r1 charging-light candidate, original P10 ADPCM capture. Use factory_ptt_p11_build_only.uvprojx. Physical qualification required.\n")
    return result


def apply_overlay(destination, parent, parent_evidence, adpcm_control=False, revision=1):
    if revision not in (1, 2): raise ValueError("P11 release revision must be 1 or 2")
    if adpcm_control and revision != 2: raise ValueError("The pipeline control belongs only to P11-r2")
    if revision == 1: return apply_charging_only(destination, parent, parent_evidence)
    changes = []
    for relative, patch in {**audio.PATCHES, **charging.PATCHES, **swipe_retirement.PATCHES}.items():
        path = destination / relative; before = path.read_bytes()
        after = patch(before.decode("latin1").replace("\r\n", "\n"))
        if adpcm_control:
            if relative.endswith("/app_pdm_handler.c"):
                after = after.replace("PPG_FILE_TYPE_16K_2_MIC_OPUS", "PPG_FILE_TYPE_16K_2_MIC_ADPCM")
            if relative.endswith("/app_ppg_file_data_handler.c"):
                marker = after.index("\nbool p11_file_write(")
                after = after[:marker] + after[marker:].replace("PPG_FILE_TYPE_16K_2_MIC_OPUS", "PPG_FILE_TYPE_16K_2_MIC_ADPCM")
                after = audio.once(after,
                    "if (app_ppg_file_hardle.file_type == PPG_FILE_TYPE_16K_2_MIC_OPUS) {",
                    "if (app_ppg_file_hardle.file_type == PPG_FILE_TYPE_16K_2_MIC_ADPCM) {")
            if relative.endswith("/app_factory_short.h"):
                after = audio.once(after, "FACTORY_SHORT_SAMPLES_PER_STEP 8000U", "FACTORY_SHORT_SAMPLES_PER_STEP 4000U")
        after = after.replace("\n", "\r\n").encode("latin1")
        path.write_bytes(after)
        changes.append({"path": relative, "beforeSha256": hashlib.sha256(before).hexdigest(), "sha256": hashlib.sha256(after).hexdigest()})
    apply_keep_short(destination, changes)
    config = destination / "firmware/bc_ros/bc_config/ring_config.h"
    before = config.read_bytes()
    config.write_bytes(audio.once(before.decode("latin1"), '"6.0.3.3P10"', '"6.0.3.3P11"').encode("latin1"))
    changes.append({"path": str(config.relative_to(destination)), "beforeSha256": hashlib.sha256(before).hexdigest(),
                    "sha256": hashlib.sha256(config.read_bytes()).hexdigest()})
    additions = []
    for path in sorted(OVERLAY.glob("*")):
        if path.suffix not in (".c", ".h"): continue
        if path.name == ("app_factory_audio.c" if adpcm_control else "app_factory_audio_control.c"): continue
        target = destination / audio.APP / path.name
        target.write_bytes(path.read_bytes())
        additions.append(audio.APP + path.name)
        changes.append({"path": audio.APP + path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    changes += prepare_codec(destination)
    old_project = destination / parent["project"]
    tree = ET.parse(old_project); target = tree.find("./Targets/Target")
    groups = target.find("Groups")
    for group in list(groups):
        if group.findtext("GroupName", "").lower().startswith("opus/"): groups.remove(group)
        else:
            files = group.find("Files")
            if files is not None:
                for entry in list(files):
                    if entry.findtext("FileName") == "app_opus.c": files.remove(entry)
    controls = target.find("TargetOption/TargetArmAds/Cads/VariousControls")
    define = controls.find("Define")
    inherited = [d for d in (define.text or "").split()
                 if d not in {"VAR_ARRAYS", "USE_ALLOCA", "REMOVE_FOR_MALLOC"} and d not in CODEC_DEFINES.split()]
    define.text = " ".join(inherited) + " " + CODEC_DEFINES
    if adpcm_control: define.text += " P11_ADPCM_CONTROL"
    # Put 1.6.1 before any supplier include path with a same-named opus.h.
    includes = [OPUS + d for d in ("include", "celt", "silk", "silk/fixed", "src")]
    includes += [RECORDING.rstrip("/"), RECORDING + "opus_support"]
    def project_path(relative):
        return "..\\..\\..\\..\\" + relative.removeprefix("firmware/").replace("/", "\\")
    inc = controls.find("IncludePath")
    inc.text = ";".join(project_path(p) for p in includes) + ";" + (inc.text or "")
    group = ET.SubElement(groups, "Group"); ET.SubElement(group, "GroupName").text = "P11 bounded Opus"
    files = ET.SubElement(group, "Files")
    units = [p for p in additions if p.endswith(".c")]
    units += [RECORDING + p for p in (("bc_capture.c",) if adpcm_control else
              ("bc_capture.c", "bc_resampler.c", "bc_audio_format.c", "bc_opus_encoder.c"))]
    if not adpcm_control: units += opus_units(destination)
    for relative in units:
        entry = ET.SubElement(files, "File")
        ET.SubElement(entry, "FileName").text = Path(relative).name
        ET.SubElement(entry, "FileType").text = "1"
        ET.SubElement(entry, "FilePath").text = project_path(relative)
    project = old_project.with_name("factory_ptt_p11_build_only.uvprojx")
    retirement.apply(destination, changes, tree)
    tree.write(project, encoding="utf-8", xml_declaration=True)
    # Keep the authenticated P10 project as evidence, but it no longer builds
    # an authenticated parent after the P11 delta. NON-FLASHABLE says so.
    result = {"version": VERSION, "packageRevision": 2, "releaseName": "P11-r2 — Opus + charging lights",
        "status": "development-not-qualified", "chargingLights": True, "firmwareShortDeletion": False,
        "mode": "adpcm-control-not-for-release" if adpcm_control else "opus", "parent": parent_evidence,
        "project": str(project.relative_to(destination)), "projectSha256": hashlib.sha256(project.read_bytes()).hexdigest(),
        "changes": changes, "opusUnits": 0 if adpcm_control else len(opus_units(destination)), "signed": False, "flashed": False,
        "physicallyQualified": False, "bootloaderModified": False, "swipesDisabled": True,
        "speedTestDisabled": True, "motionDisabled": True}
    (destination / "p11-preparation.json").write_text(json.dumps(result, indent=2) + "\n")
    (destination / "NON-FLASHABLE.txt").write_text("Unsigned P11 development tree. Use ONLY factory_ptt_p11_build_only.uvprojx. Parent projects are historical evidence, not current build entry points. Physical qualification required.\n")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--revision", type=int, choices=(1, 2), default=1,
                        help="1: charging lights with factory ADPCM; 2: Opus plus charging lights")
    parser.add_argument("--adpcm-control", action="store_true", help="P11-r2 build-only pipeline control, never a release")
    args = parser.parse_args(); destination = args.output.resolve()
    parent, evidence = prepare_base(destination)
    print(json.dumps(apply_overlay(destination, parent, evidence, args.adpcm_control, args.revision), indent=2))
