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
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = ROOT / "tools/firmware-hosting/s05-manifest.json"
CATALOG = ROOT / "tools/firmware-hosting/releases.json"
REVISION_CATALOG = ROOT / "tools/firmware-hosting/releases-v2.json"

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
    parsed = urlparse(url)
    parts = parsed.path.split("/")
    if parsed.query or parsed.fragment or len(parts) != 7 or any(p in (".", "..") for p in parts):
        fail(f"{label}: malformed release asset URL")


def check_manifest(path):
    text = Path(path).read_text()
    if RETIRED_HOST in text:
        fail(f"{path}: retired host {RETIRED_HOST} must not appear in the catalog")
    manifest = json.loads(text)
    if "releases" in manifest:
        releases = manifest["releases"]
        if manifest.get("schemaVersion") not in (1, 2) or not isinstance(releases, list) or len(releases) > 100:
            fail(f"{path}: invalid release inventory")
        if any(not isinstance(r, dict) or not isinstance(r.get("version"), str) for r in releases):
            fail(f"{path}: invalid release entry")
        for release in releases:
            revision = release.get("packageRevision", 1)
            if type(revision) is not int or revision < 1:
                fail(f"{path}: invalid package revision")
        versions = [r["version"] if manifest["schemaVersion"] == 1 else
                    (r["version"], r.get("packageRevision", 1)) for r in releases]
        if len(set(versions)) != len(versions):
            fail(f"{path}: duplicate firmware package revisions")
        return ", ".join(check_entry(r, path) for r in releases) or "empty"
    return check_entry(manifest, path)


def check_entry(manifest, path):
    label = manifest.get("version", str(path))
    revision = manifest.get("packageRevision", 1)
    if type(revision) is not int or revision < 1:
        fail(f"{label}: invalid package revision")
    if label == "6.0.3.3P11" and manifest.get("packageRevision") not in (1, 2, 3, 4):
        fail(f"{label}: unknown P11 package revision")

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
        if manifest.get("schemaVersion") != 1 or manifest.get("hardware") != "603V1.23.2":
            fail(f"{label}: unsupported OTA schema or hardware")
        if manifest.get("status") != "supplier-signed-ota":
            fail(f"{label}: otaAvailable requires status supplier-signed-ota")
        if not manifest.get("toolchain", "").startswith(AUTHORIZED_TOOLCHAIN_PREFIX):
            fail(f"{label}: OTA package was not built with {AUTHORIZED_TOOLCHAIN_PREFIX} 5")
        package = manifest.get("otaPackage") or {}
        check_trusted_url(f"{label}.otaPackage.url", package.get("url"))
        if package.get("apiURL") is not None and not re.fullmatch(
            r"https://api\.github\.com/repos/ShopItalic/sudo/releases/assets/[0-9]+", package["apiURL"]
        ):
            fail(f"{label}: apiURL must identify an asset in ShopItalic/sudo")
        if urlparse(package["url"]).path.rsplit("/", 1)[-1] != f"BCL603S2P_{label}.zip":
            fail(f"{label}: OTA filename does not match version")
        if label == "6.0.3.3P11" and urlparse(package["url"]).path.split("/")[-2] != f"v{label}-r{revision}":
            fail(f"{label}: OTA release tag does not match package revision")
        if manifest.get("otaPackageURL") != package.get("url"):
            fail(f"{label}: otaPackageURL and otaPackage.url disagree")
        if not SHA256.match(str(package.get("sha256", ""))):
            fail(f"{label}: otaPackage.sha256 is not a SHA-256 digest")
        if not isinstance(package.get("bytes"), int) or package["bytes"] <= 0:
            fail(f"{label}: otaPackage.bytes must be a positive integer")

    url = binary.get("url")
    if not url:
        return "ota-downloadable" if manifest.get("otaAvailable") else "no-download"

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
    manifests = argv[1:] or [str(DEFAULT_MANIFEST), str(CATALOG)] + (
        [str(REVISION_CATALOG)] if REVISION_CATALOG.exists() else [])
    results = [f"{Path(m).name}: {check_manifest(m)}" for m in manifests]
    print("release manifest policy: " + "; ".join(results))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
