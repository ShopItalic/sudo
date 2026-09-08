#!/usr/bin/env python3
"""Export the shared Opus fixtures produced by the firmware codec suite.

The C suite (tests/firmware/test_opus_codec.c) encodes synthetic signals with
the real fixed-point libopus 1.6.1 encoder, decodes them with the upstream
decoder and writes, per scenario, the container stream (.sopus), the input
PCM, the trimmed decoded PCM and a metadata JSON.  This tool runs that suite
into a directory, adds SHA-256 digests, and optionally mirrors the result into
the iOS test bundle so both sides verify identical bytes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUT = ROOT / "tests/fixtures/opus"
BINARY = ROOT / "build/firmware/tests/test_opus_codec"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--mirror", type=Path, help="copy the fixture set into this directory (e.g. the app repo)")
    parser.add_argument("--verify", action="store_true", help="check an existing fixture set against its manifest")
    parser.add_argument("--check", action="store_true",
                        help="regenerate into a temporary directory and fail if the committed set drifted")
    args = parser.parse_args()
    out = args.out if args.out.is_absolute() else ROOT / args.out
    manifest_path = out / "manifest.json"
    if args.verify:
        manifest = json.loads(manifest_path.read_text())
        bad = [f for f, digest in manifest["files"].items() if not (out / f).is_file() or sha256(out / f) != digest]
        if bad:
            print("fixture mismatch: " + ", ".join(bad), file=sys.stderr)
            return 1
        print(f"PASS: {len(manifest['files'])} Opus fixture files match {manifest_path.relative_to(ROOT)}")
        return 0
    if args.check:
        import tempfile
        manifest = json.loads(manifest_path.read_text())
        with tempfile.TemporaryDirectory() as temp:
            subprocess.run([str(BINARY)], check=True, env={"OPUS_FIXTURE_DIR": temp}, stdout=subprocess.DEVNULL)
            fresh = {p.name: sha256(p) for p in Path(temp).iterdir() if p.is_file()}
        if fresh != manifest["files"]:
            changed = sorted(set(fresh) ^ set(manifest["files"]) |
                             {f for f in fresh if f in manifest["files"] and fresh[f] != manifest["files"][f]})
            print("committed Opus fixtures drifted from the encoder output; re-run export: " + ", ".join(changed), file=sys.stderr)
            return 1
        print(f"PASS: encoder output matches the {len(fresh)} committed Opus fixture files")
        return 0
    if not BINARY.is_file():
        print(f"build the codec suite first (tools/firmware/test.sh); missing {BINARY}", file=sys.stderr)
        return 1
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    subprocess.run([str(BINARY)], check=True, env={"OPUS_FIXTURE_DIR": str(out)}, stdout=subprocess.DEVNULL)
    files = {p.name: sha256(p) for p in sorted(out.iterdir()) if p.is_file()}
    scenarios = {}
    for meta in sorted(out.glob("*.meta.json")):
        scenarios[meta.name.replace(".meta.json", "")] = json.loads(meta.read_text())
    manifest = {
        "generator": "tests/firmware/test_opus_codec.c via tools/firmware/export_opus_fixtures.py",
        "libopus": "1.6.1 fixed-point, DISABLE_FLOAT_API, complexity 0",
        "container": "Sudo Opus container v1 (bc_opus_stream.h)",
        "scenarios": scenarios,
        "files": files,
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    if args.mirror:
        mirror = args.mirror if args.mirror.is_absolute() else ROOT / args.mirror
        if mirror.exists():
            shutil.rmtree(mirror)
        shutil.copytree(out, mirror)
        print(f"mirrored {len(files)} files to {mirror}")
    total = sum((out / f).stat().st_size for f in files)
    print(f"exported {len(files)} fixture files ({total} bytes) to {out.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
