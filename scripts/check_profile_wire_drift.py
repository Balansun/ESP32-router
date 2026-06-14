#!/usr/bin/env python3
"""Ensure product_profile wires in profile_test_matrix.json exist in balansun_meter_pack.cpp."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "firmware" / "test" / "golden" / "profile_test_matrix.json"
METER_PACK_CPP = ROOT / "firmware" / "core" / "balansun_meter_pack.cpp"


def main() -> int:
    matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
    cpp = METER_PACK_CPP.read_text(encoding="utf-8")
    # Strings returned from balansun_product_profile_wire_compile_time and fallbacks.
    wires_in_cpp = set(re.findall(r'return "([a-z0-9_]+)";', cpp))
    wires_in_cpp.update({"full_router", "meter_router", "meter_gateway"})

    missing: list[str] = []
    for v in matrix["variants"]:
        profile = v["product_profile"]
        if profile not in wires_in_cpp:
            missing.append(profile)

    if missing:
        print("check_profile_wire_drift: profiles missing from balansun_meter_pack.cpp:", file=sys.stderr)
        for m in sorted(set(missing)):
            print(f"  - {m}", file=sys.stderr)
        return 1

    print(f"check_profile_wire_drift: OK ({len(matrix['variants'])} variants)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
