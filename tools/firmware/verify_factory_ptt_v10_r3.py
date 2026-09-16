#!/usr/bin/env python3
"""Verify immutable-parent P10-r3 source, compiled load bytes and stack bounds."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile

import prepare_factory_ptt_v10_r3 as recipe
import factory_retire_diagnostics as retirement
from factory_p11_base import ROOT
from armcc5.build_windows import input_manifest
from qualify_armcc5_runtime import Artifact
from verify_factory_ptt_v11 import task_descriptors, verify_build_inputs
from verify_factory_ptt_v8 import battery_timer_budget


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(source, evidence):
    receipt = json.loads((source / "p10-r3-preparation.json").read_text())
    with tempfile.TemporaryDirectory(prefix="p10-r3-verify-", dir=ROOT / "build") as folder:
        regenerated = Path(folder) / "source"
        parent, parent_evidence = recipe.prepare_base(regenerated)
        expected = recipe.apply_overlay(regenerated, parent, parent_evidence)
        actual_files = {p["path"]: p["sha256"] for p in input_manifest(source / "firmware")}
        wanted_files = {p["path"]: p["sha256"] for p in input_manifest(regenerated / "firmware")}
        for path in sorted(actual_files.keys() | wanted_files.keys()):
            if actual_files.get(path) == wanted_files.get(path): continue
            if ".base@8.35.0" in path and path not in wanted_files:
                if sha(source / "firmware" / path) == wanted_files.get(path.split(".base@")[0]): continue
            if path.endswith("RTE/_1.23.2/RTE_Components.h"):
                a = (source / "firmware" / path).read_text().strip()
                b = (regenerated / "firmware" / path).read_text().strip()
                if a.startswith("/*") and b.startswith("/*") and a.split("*/", 1)[1] == b.split("*/", 1)[1]: continue
            raise ValueError("Unexpected compiler input: " + path)
        for field in ("version", "packageRevision", "project", "projectSha256", "changes", "swipesDisabled", "firmwareShortDeletion", "speedTestDisabled", "motionDisabled"):
            if receipt[field] != expected[field]: raise ValueError("Stale preparation: " + field)
    build = json.loads((evidence / "result.json").read_text())
    if build["errors"] != 0 or build["target"] != "1.23.2" or build["signed"] or build["flashed"]:
        raise ValueError("Incorrect build receipt")
    verify_build_inputs(source, evidence, build)
    for item in build["outputs"]:
        if sha(Path(item["path"])) != item["sha256"]: raise ValueError("Build output changed")
    stem = (source / receipt["project"]).parent / "Objects/app"
    artifact = Artifact(stem.with_suffix(".axf"), stem.with_suffix(".bin"))
    retired = retirement.verify(artifact)
    binary = stem.with_suffix(".bin").read_bytes()
    if binary.count(b"6.0.3.3P10\0") != 2 or b"6.0.3.3Z62\0" in binary:
        raise ValueError("Incorrect firmware identity")
    stacks = {}
    report = stem.with_suffix(".htm").read_text()
    startup_budget = retirement.startup_stack_budget(artifact, report)
    for descriptor in task_descriptors(artifact):
        name = descriptor["function"]
        match = re.search(r"</a>" + re.escape(name) + r"</STRONG>.*?Max Depth = (\d+)", report, re.S)
        if not match: raise ValueError("Missing stack chain: " + name)
        used = int(match[1])
        remaining = descriptor["stack_bytes"] - used - 256
        stacks[name] = {**descriptor, "static_call_chain": used, "exception_reserve": 256, "remaining_bytes": remaining}
    if not stacks: raise ValueError("No linked task descriptors found")
    failures = [name for name, stack in stacks.items() if stack["remaining_bytes"] < 0]
    # Preserve all findings in the evidence while still failing the CLI gate.
    # Inherited budgets are not exempt from qualification.
    return {"status": "blocked-stack-budgets" if failures else "pass-source-and-binary-checks",
            "stackBudgetFailures": failures, "sourceAndLoadChecksPassed": True,
            "startupStackBudget": startup_budget,
            "version": "6.0.3.3P10", "packageRevision": 3,
            "parent": "v6.0.3.3P10-r2", "source_files": len(actual_files),
            **artifact.identity, "layout": artifact.layout, "startup": artifact.startup, "task_stacks": stacks,
            "batteryTimerStack": battery_timer_budget(report),
            "swipesDisabled": True, "firmwareShortDeletion": False, **retired,
            "bluetoothServicesUnchanged": True, "sensorConfigurationUnchanged": False,
            "limits": ["Static call chains and CPU startup are not physical runtime, pairing or recovery qualification."],
            "signed": False, "flashed": False, "published": False, "physicallyQualified": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists(): parser.error("Use a new evidence output")
    result = verify(args.source.resolve(), args.evidence.resolve())
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    if result["stackBudgetFailures"]: raise SystemExit(1)
