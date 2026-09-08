#!/usr/bin/env python3
"""Import or verify the pinned upstream libopus source subset for Sudo Voice.

The Sudo Opus profile compiles the fixed-point encoder/decoder from an exact
upstream release tarball.  This script materializes a reviewed subset of that
tarball under firmware/bc_ros/bc_module/opus/<version> and records every file
hash in an import manifest.  ``--verify`` recomputes the hashes and fails if a
listed file changed, is missing, or an unlisted source file appeared.

Only unmodified upstream bytes are imported.  Licensing text (COPYING, AUTHORS,
README) is preserved.  The optional neural-network directory (dnn/), platform
intrinsics, demos, tests, documentation and build-system files are excluded
because the firmware selects the portable C fixed-point paths only.  The
supplier's separate opus-1.5.2 drop is never touched.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[2]
OPUS_VERSION = "1.6.1"
TARBALL_NAME = f"opus-{OPUS_VERSION}.tar.gz"
TARBALL_SHA256 = "6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1"
TARBALL_URL = f"https://downloads.xiph.org/releases/opus/{TARBALL_NAME}"
DROP = ROOT / "firmware/bc_ros/bc_module/opus" / f"opus-{OPUS_VERSION}"
MANIFEST = DROP / "sudo-import-manifest.json"
NOTES = DROP / "SUDO-IMPORT.md"

# Directories whose *direct* .c/.h members are imported.  Subdirectories are
# imported only when listed themselves.
SOURCE_DIRECTORIES = ("include", "celt", "silk", "silk/fixed", "src")
# Files copied verbatim from the tarball root.
ROOT_FILES = (
    "COPYING",
    "AUTHORS",
    "README",
    "NEWS",
    "opus_sources.mk",
    "celt_sources.mk",
    "silk_sources.mk",
)
# Demo/test programs that live beside the library sources.
EXCLUDED_FILES = {
    "celt/opus_custom_demo.c",
    "src/opus_demo.c",
    "src/opus_compare.c",
    "src/repacketizer_demo.c",
}


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def selected(member_name: str) -> bool:
    prefix = f"opus-{OPUS_VERSION}/"
    if not member_name.startswith(prefix):
        return False
    relative = member_name[len(prefix):]
    if relative in ROOT_FILES:
        return True
    if relative in EXCLUDED_FILES:
        return False
    parent = str(Path(relative).parent)
    if parent not in SOURCE_DIRECTORIES:
        return False
    return relative.endswith((".c", ".h"))


def import_tarball(tarball: Path) -> int:
    actual = sha256_path(tarball)
    if actual != TARBALL_SHA256:
        print(f"tarball hash mismatch: {actual}", file=sys.stderr)
        return 1
    if DROP.exists():
        shutil.rmtree(DROP)
    DROP.mkdir(parents=True)
    files = []
    with tarfile.open(tarball, "r:gz") as archive:
        for member in archive:
            if not member.isfile() or not selected(member.name):
                continue
            relative = member.name.split("/", 1)[1]
            data = archive.extractfile(member).read()
            destination = DROP / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            files.append({"path": relative, "bytes": len(data), "sha256": sha256_bytes(data)})
    files.sort(key=lambda item: item["path"])
    manifest = {
        "opus_version": OPUS_VERSION,
        "tarball": TARBALL_NAME,
        "tarball_sha256": TARBALL_SHA256,
        "tarball_url": TARBALL_URL,
        "selection": {
            "source_directories": list(SOURCE_DIRECTORIES),
            "root_files": list(ROOT_FILES),
            "excluded_files": sorted(EXCLUDED_FILES),
            "excluded_trees": ["dnn", "doc", "tests", "cmake", "meson", "m4",
                               "celt/arm", "celt/x86", "celt/mips", "celt/tests",
                               "silk/arm", "silk/x86", "silk/mips", "silk/float",
                               "silk/tests", "silk/fixed/arm", "silk/fixed/x86",
                               "silk/fixed/mips"],
        },
        "file_count": len(files),
        "total_bytes": sum(item["bytes"] for item in files),
        "files": files,
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    NOTES.write_text(
        f"# Upstream libopus {OPUS_VERSION} import\n\n"
        f"This directory holds unmodified files from `{TARBALL_NAME}`\n"
        f"(SHA-256 `{TARBALL_SHA256}`, published at {TARBALL_URL}).\n"
        "It is the pinned codec source for the Sudo Voice Opus recording profile.\n\n"
        "- Every imported file and its SHA-256 is listed in `sudo-import-manifest.json`.\n"
        "- `python3 tools/firmware/import_opus.py --verify` checks the files against that manifest.\n"
        "- Only the portable C encoder/decoder subset is imported: `include/`, `celt/`, `silk/`,\n"
        "  `silk/fixed/` and `src/` top-level sources plus `COPYING`, `AUTHORS`, `README`, `NEWS`\n"
        "  and the upstream source lists. The optional neural-network code (`dnn/`), platform\n"
        "  intrinsics, float SILK, demos, tests and build-system files are not imported and the\n"
        "  firmware never enables DRED, OSCE or deep PLC.\n"
        "- The separate supplier `opus-1.5.2` drop is preserved as imported and is not selected\n"
        "  by the Sudo Voice profile.\n"
        "- Do not edit files in this directory; re-run the import from the exact tarball instead.\n",
        encoding="utf-8",
    )
    print(f"imported {len(files)} files ({manifest['total_bytes']} bytes) into {DROP.relative_to(ROOT)}")
    return 0


def verify() -> int:
    if not MANIFEST.is_file():
        print(f"missing manifest {MANIFEST}", file=sys.stderr)
        return 1
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if manifest.get("tarball_sha256") != TARBALL_SHA256 or manifest.get("opus_version") != OPUS_VERSION:
        print("manifest identity does not match the pinned release", file=sys.stderr)
        return 1
    expected = {item["path"]: item for item in manifest["files"]}
    failures = 0
    for relative, item in expected.items():
        path = DROP / relative
        if not path.is_file():
            print(f"missing {relative}", file=sys.stderr)
            failures += 1
            continue
        if path.stat().st_size != item["bytes"] or sha256_path(path) != item["sha256"]:
            print(f"modified {relative}", file=sys.stderr)
            failures += 1
    for path in DROP.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(DROP).as_posix()
        if relative in expected or path in (MANIFEST, NOTES):
            continue
        print(f"unlisted file {relative}", file=sys.stderr)
        failures += 1
    if failures:
        return 1
    print(f"PASS: {len(expected)} libopus {OPUS_VERSION} files match the import manifest")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tarball", type=Path, help="exact upstream release tarball to import")
    parser.add_argument("--verify", action="store_true", help="verify the checked-in drop")
    args = parser.parse_args()
    if args.verify:
        return verify()
    if args.tarball is None:
        parser.error("--tarball or --verify is required")
    return import_tarball(args.tarball)


if __name__ == "__main__":
    sys.exit(main())
