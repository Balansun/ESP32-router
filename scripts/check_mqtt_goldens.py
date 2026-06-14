#!/usr/bin/env python3
"""Sanity-check MQTT golden JSON files (IDEA-H11)."""
from __future__ import annotations

import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
GOLDEN = REPO / "firmware" / "test" / "golden" / "mqtt"


def main() -> int:
    errors = []
    if not GOLDEN.is_dir():
        print(f"missing {GOLDEN}")
        return 1
    for path in sorted(GOLDEN.glob("*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            errors.append(f"{path.name}: {exc}")
            continue
        if not isinstance(data, dict):
            errors.append(f"{path.name}: root must be object")
    if errors:
        for e in errors:
            print(e)
        return 1
    print(f"ok: {len(list(GOLDEN.glob('*.json')))} mqtt golden files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
