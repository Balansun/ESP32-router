#!/usr/bin/env python3
"""Fail if OpenAPI YAML paths diverge from firmware route registration."""

from __future__ import annotations

import re
import sys
from pathlib import Path

from openapi_paths import REPO_ROOT, require_yaml

YAML_PATH = require_yaml()

ROUTES_CPP = REPO_ROOT / "firmware" / "api_v1_routes.cpp"

# Sub-resources handled via handleNotFound (not server.on()): actions in
# api_v1_actions_uri.cpp; auth token DELETE in api_v1_auth_tokens.cpp.
STATIC_SUBRESOURCE_PATHS = {
    "/api/v1/actions/config/{index}",
    "/api/v1/actions/{index}/override",
    "/api/v1/actions/{index}/override/clear",
    "/api/v1/auth/tokens/{id}",
    "/api/v1/system/backup/manifest",
    "/api/v1/system/backup/part/{part}",
}

ROUTE_ON_RE = re.compile(r'server\.on\("(/api/v1/[^"]+)"')


def extract_firmware_paths() -> set[str]:
    text = ROUTES_CPP.read_text(encoding="utf-8")
    return set(ROUTE_ON_RE.findall(text)) | STATIC_SUBRESOURCE_PATHS


def extract_yaml_paths() -> set[str]:
    try:
        import yaml  # type: ignore
    except ImportError:
        print("ERROR: PyYAML required (pip install pyyaml)", file=sys.stderr)
        raise SystemExit(2)
    doc = yaml.safe_load(YAML_PATH.read_text(encoding="utf-8"))
    return set((doc.get("paths") or {}).keys())


def main() -> None:
    fw = extract_firmware_paths()
    yaml_p = extract_yaml_paths()
    ok = True
    only_fw = sorted(fw - yaml_p)
    only_yaml = sorted(yaml_p - fw)
    if only_fw:
        print("Registered in firmware but missing from OpenAPI YAML:", only_fw, file=sys.stderr)
        ok = False
    if only_yaml:
        print("In OpenAPI YAML but not registered in firmware:", only_yaml, file=sys.stderr)
        ok = False
    if not ok:
        raise SystemExit(1)
    print(f"OK: {len(fw)} firmware routes match OpenAPI paths.")


if __name__ == "__main__":
    main()
