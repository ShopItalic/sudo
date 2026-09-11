#!/usr/bin/env python3
"""Guard the public firmware catalog against advertising unsafe artifacts.

The S05 GNU build that bricked a dev Ring was hosted as a directly downloadable
BIN on a custom domain. This check encodes the release policy now that firmware
is distributed through GitHub Releases: a public manifest may only point at a
binary built with the authorized Arm Compiler 5 toolchain and published as a
GitHub Release asset, and a withdrawn manifest must not offer a download or OTA
package at all.
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = ROOT / "tools/firmware-hosting/s05-manifest.json"

AUTHORIZED_TOOLCHAIN_PREFIX = "Arm Compiler"
TRUSTED_ASSET_PREFIX = "https://github.com/ShopItalic/sudo/releases/download/"
RETIRED_HOST = "firmware.italic.com"
SHA256 = re.compile(r"^[0-9a-fA-F]{64}$")
COMMIT = re.compile(r"^[0-9a-f]{40}$")


def fail(message):
    raise SystemExit(f"release manifest policy failure: {message}")


def check_trusted_url(label, url):
    if not isinstance(url, str) or not url.startswith(TRUSTED_ASSET_PREFIX):
        fail(f"{label}: {url!r} is not a trusted GitHub Release asset URL")
    if RETIRED_HOST in url:
        fail(f"{label}: {url!r} still points at the retired {RETIRED_HOST} host")


def check_manifest(path):
    text = Path(path).read_text()
    if RETIRED_HOST in text:
        fail(f"{path}: retired host {RETIRED_HOST} must not appear in the catalog")
    manifest = json.loads(text)
    label = manifest.get("version", str(path))

    if manifest.get("sourceCommit") and not COMMIT.match(manifest["sourceCommit"]):
        fail(f"{label}: sourceCommit is not a 40-character lowercase commit")
    ci_run = manifest.get("ciRunURL")
    if ci_run and not ci_run.startswith("https://github.com/ShopItalic/sudo/actions/runs/"):
        fail(f"{label}: ciRunURL is not a ShopItalic/sudo CI run")

    binary = manifest.get("binary") or {}

    if manifest.get("withdrawn"):
        if binary.get("url"):
            fail(f"{label}: withdrawn manifest still advertises binary.url")
        if binary.get("flashable"):
            fail(f"{label}: withdrawn manifest is marked flashable")
        if manifest.get("otaPackageURL") or manifest.get("otaPackage"):
            fail(f"{label}: withdrawn manifest still advertises an OTA package")
        if manifest.get("otaAvailable"):
            fail(f"{label}: withdrawn manifest still has otaAvailable: true")
        return "withdrawn"

    if manifest.get("otaAvailable"):
        if manifest.get("status") != "supplier-signed-ota":
            fail(f"{label}: otaAvailable requires status supplier-signed-ota")
        if not manifest.get("toolchain", "").startswith(AUTHORIZED_TOOLCHAIN_PREFIX):
            fail(f"{label}: OTA package was not built with {AUTHORIZED_TOOLCHAIN_PREFIX} 5")
        package = manifest.get("otaPackage") or {}
        check_trusted_url(f"{label}.otaPackage.url", package.get("url"))
        if manifest.get("otaPackageURL") != package.get("url"):
            fail(f"{label}: otaPackageURL and otaPackage.url disagree")
        if not SHA256.match(str(package.get("sha256", ""))):
            fail(f"{label}: otaPackage.sha256 is not a SHA-256 digest")
        if not isinstance(package.get("bytes"), int) or package["bytes"] <= 0:
            fail(f"{label}: otaPackage.bytes must be a positive integer")

    url = binary.get("url")
    if not url:
        return "no-download"

    if not manifest.get("toolchain", "").startswith(AUTHORIZED_TOOLCHAIN_PREFIX):
        fail(f"{label}: downloadable binary was not built with {AUTHORIZED_TOOLCHAIN_PREFIX} 5")
    if binary.get("flashable") is not True:
        fail(f"{label}: downloadable binary is not marked flashable: true")
    check_trusted_url(f"{label}.binary.url", url)
    if not SHA256.match(str(binary.get("sha256", ""))):
        fail(f"{label}: binary.sha256 is not a SHA-256 digest")
    if not isinstance(binary.get("bytes"), int) or binary["bytes"] <= 0:
        fail(f"{label}: binary.bytes must be a positive integer")
    return "downloadable"


def main(argv):
    manifests = argv[1:] or [str(DEFAULT_MANIFEST)]
    results = [f"{Path(m).name}: {check_manifest(m)}" for m in manifests]
    print("release manifest policy: " + "; ".join(results))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
