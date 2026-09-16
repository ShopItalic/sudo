#!/usr/bin/env python3
"""Authenticate P11 inputs against regenerated P10-r2 + reviewed overlay.

Verify compiled bytes/load bounds and report real linked reservations. This is
not signing, flashing, real-time measurement, or physical qualification.
"""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile

import prepare_factory_ptt_v11 as recipe
import factory_retire_diagnostics as retirement
from armcc5.build_windows import input_manifest
from qualify_armcc5_runtime import Artifact, RAM_START


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_build_inputs(source, evidence, build):
    """Bind regenerated source to the recorded compiler inputs and objects."""
    current = {p["path"]: p["sha256"] for p in input_manifest(source / "firmware")}
    recorded = {p["path"]: p["sha256"] for p in json.loads((evidence / "inputs-after.json").read_text())}
    if current != recorded:
        raise ValueError("Source differs from the completed build input manifest")
    receipt = build["actual_compiler_inputs"]
    path = Path(receipt["path"])
    if sha(path) != receipt["sha256"]:
        raise ValueError("Compiler dependency receipt changed")
    actual = json.loads(path.read_text())
    for item in actual["inputs"]:
        if sha(Path(item["path"])) != item["sha256"]:
            raise ValueError("Compiler dependency changed: " + item["path"])
    for unit in actual["compilation_units"]:
        if sha(Path(unit["object"])) != unit["object_sha256"]:
            raise ValueError("Compiled object changed: " + unit["object"])


def task_descriptors(artifact):
    found = []
    for offset in range(RAM_START - 0x20000000, artifact.ram_end - 0x20000000 - 63, 4):
        entry = struct.unpack_from("<I", artifact.ram, offset + 56)[0]
        names = artifact.functions.get(entry & ~1, []) if entry & 1 else []
        if not any("thread" in n for n in names):
            continue
        name = artifact.ram[offset:offset + 40].split(b"\0")[0]
        words = struct.unpack_from("<H", artifact.ram, offset + 40)[0]
        priority = struct.unpack_from("<I", artifact.ram, offset + 48)[0]
        if not name or any(c < 32 or c > 126 for c in name):
            continue
        if not 0 < words < 16384 or priority >= 32:
            raise ValueError("Invalid task descriptor")
        found.append({"address": hex(0x20000000 + offset), "name": name.decode(),
                      "entry": hex(entry & ~1), "function": names[0],
                      "stack_bytes": words * 4, "priority": priority})
    return found


def verify(source, evidence):
    receipt = json.loads((source / "p11-preparation.json").read_text())
    control = receipt["mode"] == "adpcm-control-not-for-release"
    # Receipt alone is not authority: reproduce the active overlay on a
    # freshly authenticated immutable parent and compare the entire tree.
    with tempfile.TemporaryDirectory(prefix="p11-verify-", dir=recipe.ROOT / "build") as folder:
        regenerated = Path(folder) / "source"
        parent, parent_evidence = recipe.prepare_base(regenerated)
        prior_pdm = (regenerated / recipe.audio.APP / "app_pdm_handler.c").read_text(encoding="latin1")
        expected = recipe.apply_overlay(regenerated, parent, parent_evidence, control, receipt["packageRevision"])
        actual_files = {p["path"]: p["sha256"] for p in input_manifest(source / "firmware")}
        wanted_files = {p["path"]: p["sha256"] for p in input_manifest(regenerated / "firmware")}
        generated = []
        for path in sorted(actual_files.keys() | wanted_files.keys()):
            if actual_files.get(path) == wanted_files.get(path):
                continue
            if ".base@8.35.0" in path and path not in wanted_files:
                original = path.split(".base@")[0]
                if sha(source / "firmware" / path) == wanted_files.get(original):
                    generated.append(path); continue
            if path.endswith("RTE/_1.23.2/RTE_Components.h"):
                a = (source / "firmware" / path).read_text().strip()
                b = (regenerated / "firmware" / path).read_text().strip()
                if a.startswith("/*") and b.startswith("/*") and a.split("*/", 1)[1] == b.split("*/", 1)[1]:
                    generated.append(path); continue
            difference = ""
            if path.endswith((".c", ".h")) and path in actual_files and path in wanted_files:
                difference = "\n".join(list(difflib.unified_diff(
                    (source / "firmware" / path).read_text(encoding="latin1").splitlines(),
                    (regenerated / "firmware" / path).read_text(encoding="latin1").splitlines()))[:40])
            raise ValueError("Unreviewed compiler input: " + path + " actual=" + str(actual_files.get(path)) +
                             " expected=" + str(wanted_files.get(path)) + "\n" + difference)
        for field in ("version", "packageRevision", "mode", "project", "projectSha256", "changes", "opusUnits",
                      "chargingLights", "firmwareShortDeletion", "swipesDisabled", "speedTestDisabled", "motionDisabled"):
            if receipt[field] != expected[field]:
                raise ValueError("Stale preparation field: " + field)
        current_pdm = (source / recipe.audio.APP / "app_pdm_handler.c").read_text(encoding="latin1")
        # Match the supplier's global declaration exactly.
        pattern = r"(?m)^nrfx_pdm_config_t pdm_config\s*=\s*\{.*?\};"
        old = re.search(pattern, prior_pdm, re.S)
        new = re.search(pattern, current_pdm, re.S)
        if not old or not new or old[0] != new[0]:
            raise ValueError("Supplier microphone configuration changed")
    build = json.loads((evidence / "result.json").read_text())
    if build["errors"] != 0 or build["target"] != "1.23.2" or build["signed"] or build["flashed"]:
        raise ValueError("Incorrect build receipt")
    verify_build_inputs(source, evidence, build)
    for item in build["outputs"]:
        if sha(Path(item["path"])) != item["sha256"]:
            raise ValueError("Build output changed: " + item["path"])
    project = source / receipt["project"]
    stem = project.parent / "Objects/app"
    artifact = Artifact(stem.with_suffix(".axf"), stem.with_suffix(".bin"))
    retired = retirement.verify(artifact)
    binary = stem.with_suffix(".bin").read_bytes()
    if binary.count(b"6.0.3.3P11\0") != 2 or b"6.0.3.3P10\0" in binary:
        raise ValueError("Wrong firmware identity")
    descriptors = task_descriptors(artifact)
    stacks = {}
    stack_failures = []
    report = stem.with_suffix(".htm").read_text()
    startup_budget = retirement.startup_stack_budget(artifact, report)
    for descriptor in descriptors:
        name = descriptor["function"]
        match = re.search(r"</a>" + re.escape(name) + r"</STRONG>.*?Max Depth = (\d+)", report, re.S)
        if not match:
            raise ValueError("Missing stack chain: " + name)
        used = int(match[1])
        # Linker excludes indirect callees. Include the codec's callback chain
        # and allocation/init path explicitly in the owning encoder budget.
        indirect = 0
        if name == "app_pdm_irq_handler_thread":
            for callback in ("sink", "discard_test", "cycles"):
                matches = re.findall(r"</a>" + callback + r"</STRONG>.*?Max Depth = (\d+)", report, re.S)
                indirect = max(indirect, *(int(v) for v in matches), 0)
        remaining = descriptor["stack_bytes"] - used - indirect - 256
        stacks[name] = {**descriptor, "static_call_chain": used, "indirect_extra": indirect,
                        "exception_reserve": 256, "remaining_bytes": remaining}
        if remaining < 0:
            stack_failures.append(stacks[name])
    def object_bytes(name):
        entries = artifact.symbols.get(name, [])
        return sum(size for address, size in entries if RAM_START <= address < artifact.ram_end)
    return {"status": "blocked-stack-budgets" if stack_failures else "pass-source-and-binary-checks",
            "sourceAndLoadChecksPassed": True, **retired,
            "mode": receipt["mode"], "packageRevision": receipt["packageRevision"],
            "firmwareShortDeletion": receipt["firmwareShortDeletion"],
            "stackBudgetFailures": stack_failures,
            "startupStackBudget": startup_budget,
            "parent": "v6.0.3.3P10-r2", "sourceCommit": receipt["parent"]["lock"]["sourceCommit"],
            "source_files": len(actual_files), "generated_ide_files": generated,
            "project_sha256": sha(project), **artifact.identity, "layout": artifact.layout,
            "startup": artifact.startup, "task_stacks": stacks,
            "reservations": {"rtos_heap_bytes": object_bytes("ucHeap"), "pcm_bytes": object_bytes("pcm"),
                             "packet_queue_bytes": object_bytes("records"),
                             "audio_core_bytes": object_bytes("audio"),
                             "interrupt_stack_bytes": artifact.stack_size},
            "limits": ["Linker call chains and CPU startup are not physical or real-time qualification.",
                       "Heap contains dynamic task/queue/codec allocations; do not add them twice.",
                       "Physical heap/stack/scratch high-water and recovery remain required."],
            "signed": False, "flashed": False, "published": False, "physicallyQualified": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Use a new evidence output")
    result = verify(args.source.resolve(), args.evidence.resolve())
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    if result["stackBudgetFailures"]:
        raise SystemExit(1)
