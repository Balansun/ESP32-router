#!/usr/bin/env python3
"""
Validate captured Balansun JSON against golden key lists (host-side smoke test).

Usage:
  python3 scripts/check_firmware_golden.py path/to/get_device.json
  python3 scripts/check_firmware_golden.py --suite firmware/test/golden/captures
  python3 scripts/check_firmware_golden.py --eeprom-magic

Exit code 0 on success, non-zero on failure.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
GOLDEN_DIR = REPO_ROOT / "firmware" / "test" / "golden"
DEVICE_SPEC = GOLDEN_DIR / "required_device_keys.json"
STORAGE_CPP = REPO_ROOT / "firmware" / "metering" / "storage_eeprom.cpp"

ROUTE_SPECS: dict[str, str] = {
    "device": "required_device_keys.json",
    "measurements": "required_measurements_keys.json",
    "config": "required_config_keys.json",
    "state": "required_state_keys.json",
    "actions": "required_actions_keys.json",
    "actions_config": "required_actions_config_keys.json",
    "actions_schema": "required_actions_schema_keys.json",
    "sources": "required_sources_keys.json",
    "sources_diagnostics": "required_sources_diagnostics_keys.json",
    "health": "required_health_keys.json",
    "history_power": "required_history_power_keys.json",
    "history_energy_daily": "required_history_energy_daily_keys.json",
    "system": "required_system_keys.json",
}


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"ERROR: cannot read JSON {path}: {exc}", file=sys.stderr)
        raise SystemExit(2) from exc


def check_nested_keys(payload: dict, spec: dict, label: str) -> None:
    parent = spec.get("parent")
    required = spec.get("required_keys")
    if not isinstance(parent, str) or not parent:
        print(f"ERROR: {label} nested spec missing parent", file=sys.stderr)
        raise SystemExit(2)
    if not isinstance(required, list) or not all(isinstance(k, str) for k in required):
        print(f"ERROR: {label} nested spec missing valid required_keys array", file=sys.stderr)
        raise SystemExit(2)
    node = payload.get(parent)
    if not isinstance(node, dict):
        print(f"ERROR: {label} missing or invalid parent {parent!r}", file=sys.stderr)
        raise SystemExit(1)
    missing = [k for k in required if k not in node]
    if missing:
        print(f"ERROR: {label}.{parent} missing keys: {missing}", file=sys.stderr)
        raise SystemExit(1)
    print(f"OK: {label}.{parent} nested keys present.")


def check_response(payload: dict, spec: dict, label: str) -> None:
    required = spec.get("required_keys")
    if not isinstance(required, list) or not all(isinstance(k, str) for k in required):
        print(f"ERROR: {label} spec missing valid required_keys array", file=sys.stderr)
        raise SystemExit(2)
    missing = [k for k in required if k not in payload]
    if missing:
        print(f"ERROR: {label} missing keys: {missing}", file=sys.stderr)
        raise SystemExit(1)
    exp_ver = spec.get("expected_firmware_version")
    if isinstance(exp_ver, str) and exp_ver:
        got = payload.get("firmware_version")
        if got != exp_ver:
            print(f"ERROR: {label} firmware_version expected {exp_ver!r} got {got!r}", file=sys.stderr)
            raise SystemExit(1)
    print(f"OK: {label} JSON matches golden contract.")


def check_device_response(payload: dict, spec: dict) -> None:
    check_response(payload, spec, "device")
    caps_spec_path = GOLDEN_DIR / "required_device_capabilities_keys.json"
    if caps_spec_path.is_file():
        check_nested_keys(payload, load_json(caps_spec_path), "device")


def check_eeprom_magic_constant() -> None:
    if not STORAGE_CPP.is_file():
        print(f"ERROR: missing {STORAGE_CPP}", file=sys.stderr)
        raise SystemExit(2)
    text = STORAGE_CPP.read_text(encoding="utf-8", errors="replace")
    if "adr_ParaActions" not in text:
        print("ERROR: storage_eeprom.cpp missing adr_ParaActions", file=sys.stderr)
        raise SystemExit(1)
    board = (REPO_ROOT / "firmware" / "core" / "balansun_board.h").read_text(encoding="utf-8", errors="replace")
    m = re.search(r"#define\s+kEepromLayoutInit\s+(\d+)", board)
    if not m:
        print("ERROR: kEepromLayoutInit not found in balansun_board.h", file=sys.stderr)
        raise SystemExit(1)
    print(f"OK: EEPROM header sanity (kEepromLayoutInit={m.group(1)}, adr_ParaActions present).")


def check_suite(suite_dir: Path) -> None:
    if not suite_dir.is_dir():
        print(f"ERROR: suite directory not found: {suite_dir}", file=sys.stderr)
        raise SystemExit(2)
    ran = 0
    for route, spec_name in ROUTE_SPECS.items():
        capture = suite_dir / f"{route}.json"
        if not capture.is_file():
            print(f"SKIP: no capture for {route} ({capture.name})")
            continue
        spec = load_json(GOLDEN_DIR / spec_name)
        payload = load_json(capture)
        if not isinstance(payload, dict):
            print(f"ERROR: {capture} root must be an object", file=sys.stderr)
            raise SystemExit(1)
        check_response(payload, spec, route)
        if route == "state":
            status_spec = GOLDEN_DIR / "required_state_status_keys.json"
            if status_spec.is_file():
                check_nested_keys(payload, load_json(status_spec), route)
        if route == "health":
            st_spec = GOLDEN_DIR / "required_health_self_test_keys.json"
            if st_spec.is_file():
                check_nested_keys(payload, load_json(st_spec), route)
        if route == "device":
            caps_spec = GOLDEN_DIR / "required_device_capabilities_keys.json"
            if caps_spec.is_file():
                check_nested_keys(payload, load_json(caps_spec), route)
        ran += 1
    if ran == 0:
        print("ERROR: suite directory has no matching captures", file=sys.stderr)
        raise SystemExit(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("json_file", nargs="?", type=Path, help="Captured JSON for device route")
    parser.add_argument("--suite", type=Path, help="Directory of route captures (device.json, ...)")
    parser.add_argument("--eeprom-magic", action="store_true", help="Static checks on EEPROM sources")
    args = parser.parse_args()

    if args.eeprom_magic:
        check_eeprom_magic_constant()
        return

    if args.suite:
        check_suite(args.suite.resolve())
        return

    if not args.json_file:
        parser.error("json_file required unless --suite or --eeprom-magic")

    spec = load_json(DEVICE_SPEC)
    payload = load_json(args.json_file)
    if not isinstance(payload, dict):
        print("ERROR: root JSON must be an object", file=sys.stderr)
        raise SystemExit(1)
    check_device_response(payload, spec)


if __name__ == "__main__":
    main()
