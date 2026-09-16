#!/usr/bin/env python3
"""Verify P11-r4 inputs and compiled image against the recording-connection fix."""
import argparse
import json
from pathlib import Path
import prepare_factory_ptt_v11_r4 as recipe
import verify_factory_ptt_v11 as previous


def verify(source, evidence):
    return previous.verify(source, evidence, recipe_module=recipe)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists(): parser.error("Use a new evidence output")
    result = verify(args.source.resolve(), args.evidence.resolve())
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    if result["stackBudgetFailures"]: raise SystemExit(1)
