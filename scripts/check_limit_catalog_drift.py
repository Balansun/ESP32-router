#!/usr/bin/env python3
"""Fail if limit_catalog.json body_bytes max values diverge from api_v1_common.h."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMMON_H = ROOT / "firmware/api/api_v1_common.h"
AUTH_TOKENS_CPP = ROOT / "firmware/api/api_v1_auth_tokens.cpp"
CATALOG = ROOT / "firmware/test/hil/limit_catalog.json"

# catalog entry id -> C constant name in api_v1_common.h
BODY_MAP = {
    "put_config_body": "kPutBodyMax",
    "patch_config_body": "kPatchBodyMax",
    "put_actions_config_body": "kPutBodyMax",
    "patch_actions_config_body": "kPatchBodyMax",
    "put_system_backup_body": "kPutBodyMax",
    "put_wifi_body": "kWifiBodyMax",
    "put_time_body": "kTimeBodyMax",
    "put_http_auth_body": "kHttpAuthBodyMax",
    "put_arduino_ota_body": "kArduinoOtaBodyMax",
    "post_auth_login_body": "kHttpAuthBodyMax",
    "post_auth_tokens_body": "kAuthTokenBodyMax",
    "post_history_daily_import_body": "kHistoryDailyImportBodyMax",
    "post_fleet_import_body": "kPutBodyMax",
}

QUERY_MAP = {
    "get_history_power_max_points": "kHistAbsMax",
    "get_history_daily_limit": "kHistoryDailyPageMax",
}


def parse_constants(text: str) -> dict[str, int]:
    out: dict[str, int] = {}
    for m in re.finditer(
        r"static const (?:size_t|int) (k[A-Za-z0-9_]+)\s*=\s*(\d+)\s*;",
        text,
    ):
        out[m.group(1)] = int(m.group(2))
    return out


def main() -> int:
    if not COMMON_H.is_file() or not CATALOG.is_file():
        print("missing api_v1_common.h or limit_catalog.json", file=sys.stderr)
        return 1
    consts = parse_constants(COMMON_H.read_text())
    if AUTH_TOKENS_CPP.is_file():
        consts.update(parse_constants(AUTH_TOKENS_CPP.read_text()))
    catalog = json.loads(CATALOG.read_text())
    failed = False
    for entry in catalog.get("entries", []):
        eid = entry.get("id", "")
        lt = entry.get("limit_type")
        if lt == "body_bytes" and eid in BODY_MAP:
            cname = BODY_MAP[eid]
            expected = consts.get(cname)
            actual = entry.get("max")
            if expected is None:
                print(f"WARN: constant {cname} not found for {eid}", file=sys.stderr)
                failed = True
            elif expected != actual:
                print(f"DRIFT {eid}: catalog max={actual} != {cname}={expected}", file=sys.stderr)
                failed = True
        elif lt == "query_clamp" and eid in QUERY_MAP:
            cname = QUERY_MAP[eid]
            expected = consts.get(cname)
            actual = entry.get("max")
            if expected != actual:
                print(f"DRIFT {eid}: catalog max={actual} != {cname}={expected}", file=sys.stderr)
                failed = True
    if failed:
        return 1
    print(f"check_limit_catalog_drift: OK ({len(catalog.get('entries', []))} entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
