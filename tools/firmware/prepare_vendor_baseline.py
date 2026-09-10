#!/usr/bin/env python3
"""Prepare the preserved supplier target for compilation only; never build or flash."""
import argparse
import copy
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import tarfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
BASELINE = "102bfd2"
PROJECT = "firmware/BCL603S2X/app/project/mdk5/bc_ring_app.uvprojx"
COMPILER = "5060960::V5.06 update 7 (build 960)::.\\ARMCC"


def build_only_project(data):
    root = ET.fromstring(data)
    targets = root.find("Targets")
    matches = [t for t in targets if t.findtext("TargetName") == "1.23.2"]
    if len(matches) != 1 or matches[0].findtext("pCCUsed") != COMPILER:
        raise ValueError("Expected the original 1.23.2 / ARMCC 5.06u7 build 960 target")
    original = matches[0]
    selected = copy.deepcopy(original)
    changes = []
    for phase in ("BeforeCompile", "BeforeMake", "AfterMake"):
        node = selected.find(f"TargetOption/TargetCommonOption/{phase}")
        for number in (1, 2):
            flag = node.find(f"RunUserProg{number}")
            if flag.text != "0":
                changes.append({"phase": phase, "hook": number,
                                "command": node.findtext(f"UserProg{number}Name")})
                flag.text = "0"
    # Prove that only execution flags differ within the retained target.
    restored = copy.deepcopy(selected)
    for phase in ("BeforeCompile", "BeforeMake", "AfterMake"):
        for number in (1, 2):
            path = f"TargetOption/TargetCommonOption/{phase}/RunUserProg{number}"
            restored.find(path).text = original.findtext(path)
    if ET.tostring(restored) != ET.tostring(original):
        raise ValueError("Unexpected compiler/source setting change")
    for target in list(targets):
        targets.remove(target)
    targets.append(selected)
    return ET.tostring(root, encoding="utf-8", xml_declaration=True), changes


def prepare(destination):
    destination = destination.resolve()
    build_root = (ROOT / "build").resolve()
    if not destination.is_relative_to(build_root) or destination == build_root:
        raise ValueError("Output must be a new subdirectory of this repository's build directory")
    if destination.exists():
        raise ValueError("Output already exists; preserve it and choose a new output directory")
    commit = subprocess.check_output(
        ["git", "rev-parse", f"{BASELINE}^{{commit}}"], cwd=ROOT, text=True).strip()
    manifest_bytes = subprocess.check_output(
        ["git", "show", f"{commit}:firmware/source-manifest.json"], cwd=ROOT)
    expected = {"firmware/" + f["path"]: f for f in json.loads(manifest_bytes)["files"]}
    destination.mkdir(parents=True)
    seen = set()
    process = subprocess.Popen(["git", "archive", commit, "firmware"],
                               cwd=ROOT, stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout, mode="r|") as archive:
            for member in archive:
                if member.isdir():
                    continue
                path = PurePosixPath(member.name)
                if not member.isfile() or path.is_absolute() or ".." in path.parts:
                    raise ValueError("Unexpected archive member")
                data = archive.extractfile(member).read()
                if member.name == "firmware/source-manifest.json":
                    if data != manifest_bytes:
                        raise ValueError("Manifest changed during extraction")
                else:
                    item = expected[member.name]
                    if len(data) != item["bytes"] or hashlib.sha256(data).hexdigest() != item["sha256"]:
                        raise ValueError(f"Baseline hash mismatch: {member.name}")
                    seen.add(member.name)
                output = destination.joinpath(*path.parts)
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(data)
        if process.wait() or seen != set(expected):
            raise ValueError("Incomplete factory source extraction")
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait()
    original = destination / PROJECT
    generated, changes = build_only_project(original.read_bytes())
    project = original.with_name("vendor_baseline_build_only.uvprojx")
    project.write_bytes(generated)
    result = {
        "status": "prepared-not-built-not-qualified",
        "sourceCommit": commit, "verifiedOriginalFiles": len(seen),
        "target": "1.23.2", "requiredCompiler": COMPILER,
        "devicePack": "NordicSemiconductor.nRF_DeviceFamilyPack.8.35.0",
        "project": str(project.relative_to(destination)),
        "projectSha256": hashlib.sha256(generated).hexdigest(),
        "changes": {"removedOtherTargets": True, "disabledUserHooks": changes},
        "sourceFilesModified": False, "compiled": False,
        "signed": False, "flashed": False, "physicallyQualified": False,
        "commandArguments": ["UV4.exe", "-r", str(project.relative_to(destination)),
                             "-t", "1.23.2", "-o", "vendor-build.log"],
    }
    (destination / "baseline-preparation.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path,
                        default=ROOT / "build/firmware/vendor-baseline-102bfd2")
    args = parser.parse_args()
    try:
        print(json.dumps(prepare(args.output), indent=2))
    except (ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Baseline preparation failed: {error}\n")
