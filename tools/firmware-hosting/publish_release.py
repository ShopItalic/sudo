#!/usr/bin/env python3
"""Publish a qualified firmware release to GitHub Releases.

This helper never builds, signs, flashes or republishes firmware on its own. It
re-runs the catalog policy, checks the linked version identity of any BIN,
stages a `manifest.json` copy plus `SHA256SUMS`, and then creates (or updates) a
GitHub release with the supplied assets.

    python3 tools/firmware-hosting/publish_release.py \
        --tag v6.0.3.3S05 --assets-dir build/release

Use --dry-run to print the commands without touching GitHub. Withdrawing a
release is a separate, deliberately manual operation; see the hosting README.
"""
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
POLICY = ROOT / "tests/firmware/test_release_manifest.py"
IDENTITY = ROOT / "tools/firmware/check_build_identity.py"
DEFAULT_MANIFEST = ROOT / "tools/firmware-hosting/s05-manifest.json"
DEFAULT_REPO = "ShopItalic/sudo"


def fail(message):
    raise SystemExit(f"publish release: {message}")


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run(command, dry_run):
    print("+ " + " ".join(command))
    if not dry_run:
        subprocess.run(command, check=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True, help="release tag, e.g. v6.0.3.3S05")
    parser.add_argument("--assets-dir", required=True, type=Path, help="directory of files to attach")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--repo", default=DEFAULT_REPO)
    parser.add_argument("--title", default=None)
    parser.add_argument("--notes-file", type=Path, default=None)
    parser.add_argument("--update", action="store_true", help="upload to an existing release")
    parser.add_argument("--latest", action="store_true", help="mark the release latest")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args(argv)

    manifest = json.loads(Path(args.manifest).read_text())
    version = manifest.get("version")
    if manifest.get("withdrawn"):
        fail(f"catalog {args.manifest} is withdrawn; there is nothing to publish")
    if not version or version not in args.tag:
        fail(f"tag {args.tag!r} must contain the catalog version {version!r}")

    assets_dir = args.assets_dir.resolve()
    if not assets_dir.is_dir():
        fail(f"assets directory {assets_dir} does not exist")
    assets = sorted(p for p in assets_dir.iterdir() if p.is_file())
    if not assets:
        fail(f"assets directory {assets_dir} is empty")

    run([sys.executable, str(POLICY), str(args.manifest)], dry_run=False)

    for binary in (p for p in assets if p.suffix.lower() == ".bin"):
        run([sys.executable, str(IDENTITY), "--bin", str(binary), "--expect", version], dry_run=False)

    if manifest.get("otaAvailable"):
        expected = f"BCL603S2P_{version}.zip"
        if not (assets_dir / expected).is_file():
            fail(f"otaAvailable is true but {expected} is not in {assets_dir}")

    staging = Path(tempfile.mkdtemp(prefix="sudo-release-"))
    try:
        for asset in assets:
            shutil.copy2(asset, staging / asset.name)
        shutil.copy2(Path(args.manifest), staging / "manifest.json")
        sums = staging / "SHA256SUMS"
        sums.write_text(
            "".join(f"{sha256(p)}  {p.name}\n" for p in sorted(staging.iterdir()) if p != sums)
        )

        files = [str(p) for p in sorted(staging.iterdir())]
        title = args.title or f"Italic Ring {version}"
        if args.update:
            run(["gh", "release", "upload", args.tag, "--repo", args.repo, "--clobber", *files], args.dry_run)
            if args.latest:
                run(["gh", "release", "edit", args.tag, "--repo", args.repo, "--latest"], args.dry_run)
        else:
            command = [
                "gh", "release", "create", args.tag, "--repo", args.repo,
                "--title", title, *files,
            ]
            if args.latest:
                command.append("--latest")
            else:
                command.append("--prerelease")
            if args.notes_file:
                command += ["--notes-file", str(args.notes_file)]
            run(command, args.dry_run)
        print(f"publish release: staged {len(files)} files from {assets_dir}")
    finally:
        shutil.rmtree(staging, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
