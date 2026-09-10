#!/usr/bin/env python3
"""Record an unsigned ArmCC5 µVision build through an installed CrossOver bottle.

This uses an already installed, licensed compiler; it never installs a license,
changes TOOLS.INI, signs, packages, flashes or publishes firmware. Compiler
selection and disabled user-program hooks are checked before invoking µVision.
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import sys
import time
import xml.etree.ElementTree as ET


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path):
    return {"path": str(path.resolve()), "bytes": path.stat().st_size, "sha256": sha(path)}


def save(path, data):
    path.write_text(json.dumps(data, indent=2) + "\n")


def windows(path):
    return "Z:" + str(path.resolve()).replace("/", "\\")


def validate_build_identity(log, expected_compiler_folder):
    match = re.search(r"\*\*\* Using Compiler '([^']+)', folder: '([^']+)'", log)
    if (match is None or "5.06" not in match[1] or "build 960" not in match[1] or
            match[2].lower().rstrip("\\/") != expected_compiler_folder.lower().rstrip("\\/")):
        raise RuntimeError("µVision did not report the expected installed build 960 compiler")
    return {"version": match[1], "folder": match[2]}


def input_manifest(root):
    """Hash source/config inputs, excluding generated outputs and local IDE state."""
    skip_dirs = {"Objects", "Listings", ".git", "__pycache__"}
    skip_suffixes = {".o", ".d", ".axf", ".hex", ".map", ".htm", ".lnp", ".dep", ".lst"}
    result = []
    for p in sorted(root.rglob("*")):
        rel = p.relative_to(root)
        if not p.is_file() or any(part in skip_dirs for part in rel.parts):
            continue
        if p.suffix.lower() in skip_suffixes or ".uvguix." in p.name or p.name.endswith(".__i"):
            continue
        # Vendor .bin/.lib/.sct files are retained: they can be genuine inputs.
        result.append({"path": str(rel), "bytes": p.stat().st_size, "sha256": sha(p)})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--target", required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output-stem", type=Path, required=True)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--bottle", default="Bravechip-ArmCC5-Trial")
    parser.add_argument("--rebuild", action="store_true")
    parser.add_argument("--keil-relative", default=r"users\crossover\AppData\Local\Keil_v5")
    args = parser.parse_args()
    project = args.project.resolve()
    target = next((t for t in ET.parse(project).getroot().findall("./Targets/Target")
                   if t.findtext("TargetName") == args.target), None)
    if target is None or not target.findtext("pCCUsed", "").startswith("5060960::"):
        raise RuntimeError("Target must select Arm Compiler 5.06 update 7 build 960")
    for field in target.iter():
        if field.tag.startswith("RunUserProg") and field.text not in (None, "0"):
            raise RuntimeError(f"Enabled user-program hook: {field.tag}; use a build-only project")
    out = args.evidence.resolve()
    out.mkdir(parents=True, exist_ok=True)
    if (out / "result.json").exists():
        raise RuntimeError("Evidence directory already contains a result; use a new directory")
    bottle = Path.home() / "Library/Application Support/CrossOver/Bottles" / args.bottle
    keil = bottle / "drive_c" / Path(args.keil_relative.replace("\\", "/"))
    compiler = keil / "ARM/ARM_Compiler_5.06u7"
    wine = Path("/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine")
    for p in (wine, keil / "UV4/UV4.exe", compiler / "bin/armcc.exe", compiler / "bin/fromelf.exe"):
        if not p.is_file():
            raise RuntimeError(f"Missing installed tool: {p}")
    prefix = [str(wine), "--bottle", args.bottle, "--cx-app"]
    winroot = "C:\\" + args.keil_relative
    env = {**os.environ, "ARM_TOOL_VARIANT": "mdk_pro"}
    version = subprocess.run(prefix + [winroot + r"\ARM\ARM_Compiler_5.06u7\bin\armcc.exe", "--vsn"],
                             env=env, text=True, capture_output=True)
    version_text = version.stdout + version.stderr
    (out / "compiler-version.txt").write_text(version_text)
    if version.returncode != 0 or "build 960" not in version_text or "5.06" not in version_text:
        raise RuntimeError("Exact licensed compiler version check failed; see compiler-version.txt")
    save(out / "compiler-files.json", [record(p) for p in sorted(compiler.rglob("*")) if p.is_file()])
    inputs = input_manifest(args.source_root)
    save(out / "inputs-before.json", inputs)
    log = out / "build.log"
    command = prefix + [winroot + r"\UV4\UV4.exe", "-r" if args.rebuild else "-b",
                        windows(project), "-t", args.target, "-j0", "-sg", "-o", windows(log)]
    save(out / "command.json", {"argv": command, "environment": {"ARM_TOOL_VARIANT": "mdk_pro"},
                                "project_sha256": sha(project), "host": socket.gethostname()})
    started = time.monotonic()
    with (out / "launcher.log").open("w") as launcher:
        code = subprocess.call(command, env=env, stdout=launcher, stderr=subprocess.STDOUT)
    text = log.read_text(errors="replace") if log.exists() else ""
    summary = re.findall(r"(\d+) Error\(s\), (\d+) Warning\(s\)", text)
    result = {"recorded_at": datetime.now(timezone.utc).isoformat(), "host": socket.gethostname(),
              "project": str(project), "target": args.target, "uv4_exit_code": code,
              "elapsed_seconds": round(time.monotonic() - started, 2),
              "errors": int(summary[-1][0]) if summary else None,
              "warnings": int(summary[-1][1]) if summary else None,
              "signed": False, "flashed": False, "physically_qualified": False}
    sizes = re.findall(r"Program Size: Code=(\d+) RO-data=(\d+) RW-data=(\d+) ZI-data=(\d+)", text)
    if sizes:
        result["program_size"] = dict(zip(("code", "ro_data", "rw_data", "zi_data"), map(int, sizes[-1])))
    after = input_manifest(args.source_root)
    save(out / "inputs-after.json", after)
    prior = {p["path"]: p["sha256"] for p in inputs}
    result["changed_inputs"] = [p["path"] for p in after if prior.get(p["path"]) != p["sha256"]]
    result["removed_inputs"] = sorted(set(prior) - {p["path"] for p in after})
    if code not in (0, 1) or result["errors"] != 0:
        save(out / "result.json", result)
        raise RuntimeError(f"µVision build failed: {result}")
    result["compiler_used"] = validate_build_identity(text, winroot + r"\ARM\ARM_Compiler_5.06u7\Bin")
    stem = args.output_stem.resolve()
    binary = stem.with_suffix(".bin")
    convert = prefix + [winroot + r"\ARM\ARM_Compiler_5.06u7\bin\fromelf.exe", "--bin",
                        "--output", windows(binary), windows(stem.with_suffix(".axf"))]
    run = subprocess.run(convert, env=env, text=True, capture_output=True)
    save(out / "fromelf.json", {"argv": convert, "exit_code": run.returncode,
                                 "stdout": run.stdout, "stderr": run.stderr})
    if run.returncode != 0:
        save(out / "result.json", result)
        raise RuntimeError("fromelf conversion failed")
    files = [stem.with_suffix(s) for s in (".axf", ".bin", ".hex", ".lnp", ".sct", ".htm")]
    files += [args.map.resolve(), log, out / "compiler-version.txt"]
    result["outputs"] = [record(p) for p in files]
    result["compiled_objects"] = len(list(stem.parent.glob("*.o")))
    result["flash_end"] = hex(0x27000 + binary.stat().st_size)
    if 0x27000 + binary.stat().st_size > 0xE0000:
        raise RuntimeError("Application BIN exceeds reserved flash boundary")
    subprocess.run([sys.executable, str(Path(__file__).with_name("record_inputs.py")),
                    "--project-dir", str(project.parent), "--objects", str(stem.parent),
                    "--output", str(out / "actual-compiler-inputs.json"), "--bottle", args.bottle], check=True)
    result["actual_compiler_inputs"] = record(out / "actual-compiler-inputs.json")
    save(out / "result.json", result)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
