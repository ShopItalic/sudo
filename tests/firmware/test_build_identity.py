#!/usr/bin/env python3
"""Unit checks for the linked-image identity verifier."""
import importlib.util
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "check_build_identity", ROOT / "tools/firmware/check_build_identity.py"
)
checker = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(checker)


def expect_failure(data, expected):
    try:
        checker.check_identity(data, expected)
    except SystemExit:
        return
    raise SystemExit(f"expected identity failure for {expected!r}")


def main():
    sudo = b"\x00\x00" + b"6.0.3.3S04\x00" + b"\xff" * 8
    factory = b"\x00\x00" + b"6.0.3.3Z62\x00" + b"\xff" * 8

    offsets = checker.check_identity(sudo, "6.0.3.3S04")
    assert offsets == [2], offsets

    # A factory fallback must not satisfy a Sudo expectation.
    expect_failure(factory, "6.0.3.3S04")
    # Two identities in one image is ambiguous and must fail.
    expect_failure(sudo + factory, "6.0.3.3S04")
    # An unknown expectation is a configuration error, not a pass.
    try:
        checker.check_identity(sudo, "6.0.3.3S99")
    except ValueError:
        pass
    else:
        raise SystemExit("expected unknown-version rejection")

    print("build identity verifier: expected, fallback, conflict and unknown cases passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
