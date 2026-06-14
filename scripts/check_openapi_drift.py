#!/usr/bin/env python3
"""Ensure OpenAPI paths match across YAML, firmware embed, and web fixture."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

from openapi_paths import REPO_ROOT, require_yaml

YAML_PATH = require_yaml()
FIRMWARE_OA = REPO_ROOT / "firmware" / "api" / "api_v1_openapi.cpp"
WEB_FIXTURE = REPO_ROOT / "web" / "test" / "fixtures" / "openapi-snapshot.json"
PATH_RE = re.compile(r'\\"(/api/v1/[^\\]+)\\"')


def extract_yaml_paths() -> set[str]:
    try:
        import yaml  # type: ignore
    except ImportError:
        print("ERROR: PyYAML required (pip install pyyaml)", file=sys.stderr)
        raise SystemExit(2)
    doc = yaml.safe_load(YAML_PATH.read_text(encoding="utf-8"))
    return set((doc.get("paths") or {}).keys())


def extract_firmware_paths() -> set[str]:
    text = FIRMWARE_OA.read_text(encoding="utf-8", errors="replace")
    start = text.find('static const char PROGMEM oa[]')
    if start < 0:
        print("ERROR: OpenAPI PROGMEM string not found", file=sys.stderr)
        raise SystemExit(2)
    # Search from first embed through end of handle_get_openapi (both #if branches).
    end = text.find("void ", start + 1)
    chunk = text[start:end] if end > start else text[start:]
    return set(PATH_RE.findall(chunk))


def extract_web_paths() -> set[str]:
    doc = json.loads(WEB_FIXTURE.read_text(encoding="utf-8"))
    return set(doc.get("paths", {}).keys())


def report_diff(label: str, only_a: set[str], only_b: set[str]) -> bool:
    ok = True
    if only_a:
        print(f"Only in {label} (first set):", sorted(only_a), file=sys.stderr)
        ok = False
    if only_b:
        print(f"Only in {label} (second set):", sorted(only_b), file=sys.stderr)
        ok = False
    return ok


def main() -> None:
    yaml_p = extract_yaml_paths()
    fw_p = extract_firmware_paths()
    web_p = extract_web_paths()
    ok = True
    ok = report_diff("yaml vs firmware", yaml_p - fw_p, fw_p - yaml_p) and ok
    ok = report_diff("yaml vs web", yaml_p - web_p, web_p - yaml_p) and ok
    if not ok:
        raise SystemExit(1)
    print(f"OK: {len(yaml_p)} OpenAPI paths match (yaml, firmware, web).")


if __name__ == "__main__":
    main()
