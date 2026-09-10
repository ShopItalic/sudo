#!/usr/bin/env python3
"""Exercise x86 Linux executables; this does not run an Arm firmware compiler."""

import hashlib
import json
from pathlib import Path
import platform
import subprocess


def probe(path, elf_class, machine):
    binary = Path(path)
    data = binary.read_bytes()
    actual_class = data[4] if data[:4] == b"\x7fELF" else None
    actual_machine = int.from_bytes(data[18:20], "little")
    if (actual_class, actual_machine) != (elf_class, machine):
        raise RuntimeError("Unexpected ELF architecture: " + path)
    result = subprocess.run([path], capture_output=True, text=True, timeout=20)
    return {
        "path": path,
        "sha256": hashlib.sha256(data).hexdigest(),
        "elfClass": actual_class,
        "elfMachine": actual_machine,
        "exitCode": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }


def main():
    probes = [probe("/bin/true", 2, 62), probe("/lib32/libc.so.6", 1, 3)]
    packages = Path("/opt/armcc5-host-packages.txt").read_bytes()
    passed = all(p["exitCode"] == 0 for p in probes)
    result = {
        "status": "host-execution-passed" if passed else "host-execution-failed",
        "reportedMachine": platform.machine(),
        "pythonVersion": platform.python_version(),
        "packagesSha256": hashlib.sha256(packages).hexdigest(),
        "probes": probes,
        "compilerExecuted": False,
        "licenseValidated": False,
        "firmwareCompiled": False,
        "physicallyQualified": False,
    }
    print(json.dumps(result, indent=2))
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
