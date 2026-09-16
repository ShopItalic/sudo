#!/usr/bin/env python3
"""Materialize P11's immutable P10 revision-2 parent. Never build or flash.

Use committed recipes and overlays, not the dirty checkout or a moving tag.
Reproduce and verify the released source delta BEFORE applying P11 changes.
An internal build-attempt number is not a release revision.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[2]
LOCK = ROOT / "firmware/factory_ptt_v11/base-lock.json"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def firmware_path(value):
    path = PurePosixPath(value)
    if path.is_absolute() or ".." in path.parts or not path.parts or path.parts[0] != "firmware":
        raise ValueError("Invalid parent manifest path")
    return path


def validate_base(destination, result, lock):
    baseline = json.loads((destination / "baseline-preparation.json").read_text())
    if baseline["target"] != lock["target"] or baseline["requiredCompiler"] != lock["requiredCompiler"]:
        raise ValueError("P11 parent board/compiler mismatch")
    if result["version"] != lock["version"] or result["sourceCommit"] != lock["vendorCommit"]:
        raise ValueError("P11 parent version/vendor mismatch")
    changes = {}
    for item in result["changes"]:
        path = firmware_path(item["path"])
        if str(path) in changes:
            raise ValueError("Duplicate parent manifest path")
        expected = item.get("afterSha256", item.get("sha256"))
        if digest((destination / path).read_bytes()) != expected:
            raise ValueError("P11 parent source mismatch: " + str(path))
        changes[str(path)] = expected
    delta = digest(json.dumps(changes, sort_keys=True, separators=(",", ":")).encode())
    if len(changes) != lock["generatedChangedFiles"] or delta != lock["generatedDeltaSha256"]:
        raise ValueError("P11 parent differs from released P10 revision 2")
    project = firmware_path(result["project"])
    if digest((destination / project).read_bytes()) != lock["projectSha256"]:
        raise ValueError("P11 parent project/compiler settings mismatch")
    # Check the ENTIRE generated parent, not only its declared delta. The
    # immutable vendor manifest authenticates every unchanged file as well.
    manifest = subprocess.check_output(
        ["git", "show", lock["vendorCommit"] + ":firmware/source-manifest.json"], cwd=ROOT)
    expected = {str(firmware_path("firmware/" + f["path"])): f["sha256"]
                for f in json.loads(manifest)["files"]}
    expected.update(changes)
    expected["firmware/source-manifest.json"] = digest(manifest)
    expected[str(project)] = lock["projectSha256"]
    expected[str(firmware_path(baseline["project"]))] = baseline["projectSha256"]
    actual = {}
    for path in (destination / "firmware").rglob("*"):
        if path.is_symlink():
            raise ValueError("Unexpected symlink in parent source")
        if path.is_file():
            actual[str(path.relative_to(destination))] = digest(path.read_bytes())
    if actual != expected:
        differing = sorted(k for k in actual.keys() | expected.keys() if actual.get(k) != expected.get(k))
        raise ValueError("Unexpected parent file changes: " + ", ".join(differing[:8]))
    return delta


def prepare_base(destination):
    destination = destination.resolve()
    build_root = (ROOT / "build").resolve()
    if not destination.is_relative_to(build_root) or destination == build_root or destination.exists():
        raise ValueError("Use a fresh subdirectory inside this repository's build directory")
    lock = json.loads(LOCK.read_text())
    if lock["schema"] != 1 or lock["revision"] != 2 or lock["release"] != "v6.0.3.3P10-r2":
        raise ValueError("Unexpected P11 parent lock")
    commit = subprocess.check_output(
        ["git", "rev-parse", lock["sourceCommit"] + "^{commit}"], cwd=ROOT, text=True).strip()
    if commit != lock["sourceCommit"]:
        raise ValueError("P11 parent must be a full immutable commit")
    build_root.mkdir(exist_ok=True)
    # Preserve this small input snapshot beside other build evidence. This is
    # not a worktree/branch and does not modify or retarget the user's checkout.
    snapshot = Path(tempfile.mkdtemp(prefix="p11-parent-r2-", dir=build_root))
    process = subprocess.Popen(
        ["git", "archive", commit, "tools/firmware", "firmware/factory_ptt_v10"],
        cwd=ROOT, stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout, mode="r|") as archive:
            for member in archive:
                if member.isdir():
                    continue
                relative = PurePosixPath(member.name)
                if not member.isfile() or relative.is_absolute() or ".." in relative.parts:
                    raise ValueError("Unexpected pinned recipe archive member")
                output = snapshot / relative
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(archive.extractfile(member).read())
        if process.wait():
            raise ValueError("Pinned recipe extraction failed")
    finally:
        process.stdout.close()
        if process.poll() is None:
            process.terminate()
        process.wait()
    # Isolated interpreter: no cached checkout imports/PYTHONPATH can replace
    # the pinned recipes. Only the vendor extractor's git/build root is moved
    # back to this repository; all overlay paths stay in the pinned snapshot.
    bootstrap = """
import json, sys
from pathlib import Path
sys.path.insert(0, sys.argv[1])
import prepare_factory_ptt_v10 as p10
import prepare_vendor_baseline as vendor
vendor.ROOT = Path(sys.argv[2])
vendor.BASELINE = sys.argv[4]
destination = Path(sys.argv[3])
print(json.dumps(p10.apply_overlay(destination, vendor.prepare(destination))))
"""
    result = json.loads(subprocess.check_output(
        [sys.executable, "-I", "-B", "-c", bootstrap, str(snapshot / "tools/firmware"),
         str(ROOT), str(destination), lock["vendorCommit"]], cwd=ROOT, text=True))
    delta = validate_base(destination, result, lock)
    evidence = {"status": "verified-p10-revision-2-parent-source",
                "lock": lock, "lockSha256": digest(LOCK.read_bytes()),
                "recipeSnapshot": str(snapshot.relative_to(ROOT)),
                "generatedDeltaSha256": delta,
                "compiled": False, "signed": False, "flashed": False,
                "physicalQualification": False}
    (destination / "p11-parent-verification.json").write_text(json.dumps(evidence, indent=2) + "\n")
    return result, evidence


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    result, evidence = prepare_base(args.output)
    print(json.dumps(evidence, indent=2))
