#!/usr/bin/env python3
"""Guard the public firmware manifest against advertising unsafe artifacts.

The S05 GNU build that bricked a dev Ring was hosted as a directly downloadable
BIN. This check encodes the release policy: a public manifest may only point at
a binary built with the authorized Arm Compiler 5 toolchain, and a withdrawn
manifest must not offer a download or OTA package at all.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = ROOT / "tools/firmware-hosting/s05-manifest.json"

AUTHORIZED_TOOLCHAIN_PREFIX = "Arm Compiler"


def fail(message):
    raise SystemExit(f"release manifest policy failure: {message}")


def check_manifest(path):
    manifest = json.loads(Path(path).read_text())
    label = manifest.get("version", path)

    if manifest.get("otaAvailable"):
        if not manifest.get("otaPackageURL"):
            fail(f"{label}: otaAvailable is true without an otaPackageURL")
        if not manifest.get("toolchain", "").startswith(AUTHORIZED_TOOLCHAIN_PREFIX):
            fail(f"{label}: OTA package was not built with {AUTHORIZED_TOOLCHAIN_PREFIX} 5")

    binary = manifest.get("binary") or {}
    url = binary.get("url")

    if manifest.get("withdrawn"):
        if url:
            fail(f"{label}: withdrawn manifest still advertises binary.url")
        if binary.get("flashable"):
            fail(f"{label}: withdrawn manifest is marked flashable")
        if manifest.get("otaPackageURL"):
            fail(f"{label}: withdrawn manifest still advertises an OTA package")
        return "withdrawn"

    if not url:
        return "no-download"

    if not manifest.get("toolchain", "").startswith(AUTHORIZED_TOOLCHAIN_PREFIX):
        fail(f"{label}: downloadable binary was not built with {AUTHORIZED_TOOLCHAIN_PREFIX} 5")
    if binary.get("flashable") is not True:
        fail(f"{label}: downloadable binary is not marked flashable: true")
    return "downloadable"


def main(argv):
    manifests = argv[1:] or [str(DEFAULT_MANIFEST)]
    results = [f"{Path(m).name}: {check_manifest(m)}" for m in manifests]
    print("release manifest policy: " + "; ".join(results))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
