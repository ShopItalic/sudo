#!/usr/bin/env python3
"""Hash the actual µVision dependency inputs and compiled objects after a build.

The generated .d files identify actual compiler/header inputs, including the
installed CMSIS pack and Arm compiler headers outside the source tree. No
license configuration or bottle registry is read.
"""

import argparse
import hashlib
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-dir", type=Path, required=True)
    parser.add_argument("--objects", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--bottle", default="Bravechip-ArmCC5-Trial")
    args = parser.parse_args()
    cdrive = Path.home() / "Library/Application Support/CrossOver/Bottles" / args.bottle / "drive_c"
    def native(raw):
        value = raw.strip().replace("\\", "/")
        if value.lower().startswith("c:/"):
            return (cdrive / value[3:]).resolve()
        if value.lower().startswith("z:/"):
            return Path(value[2:]).resolve()
        return (args.project_dir / value).resolve()
    units, inputs = [], {}
    for dep in sorted(args.objects.glob("*.d")):
        dependencies = []
        for line in dep.read_text(errors="strict").splitlines():
            if ": " not in line:
                raise RuntimeError(f"Unrecognized dependency line in {dep}")
            raw = line.split(": ", 1)[1]
            path = native(raw)
            if not path.is_file():
                raise RuntimeError(f"Unresolved dependency: {raw}")
            key = str(path)
            if key not in inputs:
                inputs[key] = {"path": key, "bytes": path.stat().st_size, "sha256": sha(path)}
            dependencies.append(key)
        obj = dep.with_suffix(".o")
        if not obj.is_file() or not dependencies:
            raise RuntimeError(f"Missing object or source for {dep}")
        units.append({"object": str(obj.resolve()), "object_sha256": sha(obj),
                      "dependency_file_sha256": sha(dep), "source": dependencies[0],
                      "dependencies": dependencies})
    objects = list(args.objects.glob("*.o"))
    if len(units) != len(objects):
        raise RuntimeError(f"Dependency coverage {len(units)} does not match {len(objects)} objects")
    result = {"compilation_units": units, "inputs": list(inputs.values()),
              "object_count": len(units), "dependency_input_count": len(inputs),
              "opus_1_6_1_units": sum("/opus-1.6.1/" in u["source"] for u in units)}
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(f"Recorded {len(units)} compiled objects, {len(inputs)} actual inputs, "
          f"{result['opus_1_6_1_units']} Opus 1.6.1 sources")


if __name__ == "__main__":
    main()
