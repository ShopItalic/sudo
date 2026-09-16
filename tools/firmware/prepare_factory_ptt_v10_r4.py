#!/usr/bin/env python3
"""Prepare P10-r4 with local recording preserved across BLE reconnects. Never flash."""
import argparse
import json
from pathlib import Path
import factory_recording_connection as recording_connection
import prepare_factory_ptt_v10_r3 as previous

prepare_base = previous.prepare_base


def apply_overlay(destination, parent, evidence):
    result = previous.apply_overlay(destination, parent, evidence)
    recording_connection.apply(destination, result["changes"])
    result.update(packageRevision=4, reconnectStopRemoved=True)
    (destination / "p10-r4-preparation.json").write_text(json.dumps(result, indent=2) + "\n")
    (destination / "NON-FLASHABLE.txt").write_text(
        "Unsigned P10 revision-4 development tree. Local recording survives Bluetooth reconnects. "
        "Use verify_factory_ptt_v10_r4.py; p10-r3-preparation.json records inherited inputs only. "
        "Physical qualification required.\n")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    destination = args.output.resolve()
    parent, evidence = prepare_base(destination)
    print(json.dumps(apply_overlay(destination, parent, evidence), indent=2))
