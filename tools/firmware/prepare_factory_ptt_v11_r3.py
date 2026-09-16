#!/usr/bin/env python3
"""Prepare P11-r3: factory ADPCM, charging lights, and reconnect fix. Never flash."""
import argparse
import json
from pathlib import Path
import factory_recording_connection as recording_connection
import prepare_factory_ptt_v11 as previous

ROOT, OVERLAY, audio = previous.ROOT, previous.OVERLAY, previous.audio
prepare_base = previous.prepare_base


def apply_overlay(destination, parent, evidence, adpcm_control=False, revision=3):
    if revision != 3:
        raise ValueError("This recipe prepares P11 revision 3 only")
    if adpcm_control:
        raise ValueError("P11-r3 uses original factory ADPCM; the diagnostic control belongs to P11-r4")
    result = previous.apply_overlay(destination, parent, evidence,
                                    revision=1)
    recording_connection.apply(destination, result["changes"])
    result.update(packageRevision=3, reconnectStopRemoved=True,
                  releaseName="P11-r3 — Charging lights + reconnect fix")
    (destination / "p11-preparation.json").write_text(json.dumps(result, indent=2) + "\n")
    (destination / "NON-FLASHABLE.txt").write_text(
        "Unsigned P11 revision-3 development tree. Local recording survives Bluetooth reconnects. "
        "Original factory ADPCM recorder with charging lights. Use verify_factory_ptt_v11_r3.py. "
        "Physical qualification required.\n")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    destination = args.output.resolve()
    parent, evidence = prepare_base(destination)
    print(json.dumps(apply_overlay(destination, parent, evidence), indent=2))
