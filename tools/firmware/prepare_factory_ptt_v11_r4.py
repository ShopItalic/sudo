#!/usr/bin/env python3
"""Prepare P11-r4: Opus, charging lights, and reconnect fix. Never flash."""
import argparse
import json
from pathlib import Path
import factory_recording_connection as recording_connection
import prepare_factory_ptt_v11 as previous

ROOT, OVERLAY, audio = previous.ROOT, previous.OVERLAY, previous.audio
prepare_base = previous.prepare_base


def apply_overlay(destination, parent, evidence, adpcm_control=False, revision=4):
    if revision != 4:
        raise ValueError("This recipe prepares P11 revision 4 only")
    result = previous.apply_overlay(destination, parent, evidence,
                                    adpcm_control=adpcm_control, revision=2)
    recording_connection.apply(destination, result["changes"])
    result.update(packageRevision=4, reconnectStopRemoved=True,
                  releaseName="P11-r4 — Opus + charging lights + reconnect fix")
    (destination / "p11-preparation.json").write_text(json.dumps(result, indent=2) + "\n")
    (destination / "NON-FLASHABLE.txt").write_text(
        "Unsigned P11 revision-4 development tree. Local recording survives Bluetooth reconnects. "
        "Use verify_factory_ptt_v11_r4.py. The ADPCM control is diagnostic only. "
        "Physical qualification required.\n")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--adpcm-control", action="store_true", help="Build-only diagnostic; never release")
    args = parser.parse_args()
    destination = args.output.resolve()
    parent, evidence = prepare_base(destination)
    print(json.dumps(apply_overlay(destination, parent, evidence, args.adpcm_control), indent=2))
