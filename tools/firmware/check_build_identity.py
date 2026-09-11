#!/usr/bin/env python3
"""Verify the firmware version string embedded in a linked image.

The Sudo profile defines RING_1232_SOFTWARE_VERSION as a ten-character string
plus NUL, copied into the protocol version reply. If SUDO_VOICE_ONLY is lost the
image silently falls back to the factory "6.0.3.3Z62" identity. No existing
build or release step reads the linked bytes, so a wrong-profile image could be
flashed or published unnoticed.

Usage:
    python3 tools/firmware/check_build_identity.py \
        --bin build/firmware/sudo_voice/sudo_voice_candidate.bin \
        --expect 6.0.3.3S04
"""
import argparse
import sys
from pathlib import Path

# Known identities this repository has produced. A linked image must contain
# exactly the expected one and must not contain a conflicting one.
KNOWN_VERSIONS = (
    "6.0.3.3Z62",  # factory baseline
    "6.0.3.3S01",
    "6.0.3.3S02",
    "6.0.3.3S03",
    "6.0.3.3S04",
    "6.0.3.3S05",
)


def version_offsets(data, version):
    needle = version.encode("ascii") + b"\x00"
    offsets = []
    start = 0
    while True:
        found = data.find(needle, start)
        if found < 0:
            return offsets
        offsets.append(found)
        start = found + 1


def check_identity(data, expected):
    expected = expected.rstrip("\x00")
    if expected not in KNOWN_VERSIONS:
        raise ValueError(f"expected version {expected!r} is not a known Sudo identity")
    present = {v: version_offsets(data, v) for v in KNOWN_VERSIONS}
    present = {v: o for v, o in present.items() if o}
    if not present.get(expected):
        found = ", ".join(sorted(present)) or "none"
        raise SystemExit(
            f"identity failure: expected {expected} not found in image; found {found}"
        )
    conflicting = sorted(v for v in present if v != expected)
    if conflicting:
        raise SystemExit(
            f"identity failure: expected {expected} but image also contains {conflicting}"
        )
    return present[expected]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin", dest="binary", required=True, help="linked application BIN")
    parser.add_argument("--expect", required=True, help="expected version string")
    args = parser.parse_args(argv)

    data = Path(args.binary).read_bytes()
    offsets = check_identity(data, args.expect)
    print(
        f"build identity: {args.expect} at {', '.join(hex(o) for o in offsets)} "
        f"in {args.binary} ({len(data)} bytes)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
