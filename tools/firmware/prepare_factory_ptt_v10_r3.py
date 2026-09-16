#!/usr/bin/env python3
"""Prepare P10 revision 3 from immutable released revision 2. Never flash."""
import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import factory_retire_diagnostics as retirement
from factory_p11_base import prepare_base
from factory_disable_swipes import PATCHES
from factory_keep_short_recordings import PATCHES as KEEP_SHORT_PATCHES


def apply_overlay(destination, parent, evidence):
    changes = []
    for relative, patch in {**PATCHES, **KEEP_SHORT_PATCHES}.items():
        path = destination / relative
        before = path.read_bytes()
        after = patch(before.decode("latin1").replace("\r\n", "\n")).replace("\n", "\r\n").encode("latin1")
        path.write_bytes(after)
        changes.append({"path": relative, "beforeSha256": hashlib.sha256(before).hexdigest(),
                        "sha256": hashlib.sha256(after).hexdigest()})
    project = destination / parent["project"]
    tree = ET.parse(project)
    retirement.apply(destination, changes, tree)
    tree.write(project, encoding="utf-8", xml_declaration=True)
    result = {"version": "6.0.3.3P10", "packageRevision": 3,
              "status": "development-not-qualified", "parent": evidence,
              "project": parent["project"], "projectSha256": hashlib.sha256(project.read_bytes()).hexdigest(),
              "changes": changes, "swipesDisabled": True, "firmwareShortDeletion": False,
              "bluetoothServicesUnchanged": True, "sensorConfigurationUnchanged": False,
              "speedTestDisabled": True, "motionDisabled": True,
              "signed": False, "flashed": False, "physicallyQualified": False}
    (destination / "p10-r3-preparation.json").write_text(json.dumps(result, indent=2) + "\n")
    (destination / "NON-FLASHABLE.txt").write_text("Unsigned P10 revision-3 development tree. Swipes and duration-based deletion retired. Physical qualification required.\n")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    destination = args.output.resolve()
    parent, evidence = prepare_base(destination)
    print(json.dumps(apply_overlay(destination, parent, evidence), indent=2))
