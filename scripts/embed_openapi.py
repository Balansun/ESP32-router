#!/usr/bin/env python3
"""Generate firmware OpenAPI PROGMEM string from openapi/balansun-v1.yaml."""

from __future__ import annotations

import json
import sys
from pathlib import Path

from openapi_paths import REPO_ROOT as REPO, require_yaml

YAML_PATH = require_yaml()
INJECT_PATH = "/api/v1/sources/test/inject"
ROUTES_CPP = REPO / "firmware" / "api" / "api_v1_openapi.cpp"
OPENAPI_FN_MARKER = "void handle_get_openapi()"
EMBED_START = "#if defined(BALANSUN_ENABLE_SOURCE_TEST_API)"


def load_paths() -> dict:
    try:
        import yaml  # type: ignore
    except ImportError:
        print("ERROR: PyYAML required (pip install pyyaml)", file=sys.stderr)
        raise SystemExit(2)
    doc = yaml.safe_load(YAML_PATH.read_text(encoding="utf-8"))
    paths = doc.get("paths") or {}
    return {
        "openapi": doc.get("openapi", "3.0.0"),
        "info": doc.get("info", {"title": "Balansun API", "version": "1.0"}),
        "paths": paths,
    }


def compact_json(doc: dict) -> str:
    return json.dumps(doc, separators=(",", ":"), ensure_ascii=True)


def production_doc(full: dict) -> dict:
    paths = dict(full["paths"])
    paths.pop(INJECT_PATH, None)
    return {"openapi": full["openapi"], "info": full["info"], "paths": paths}


def escape_c_string(oa_json: str) -> str:
    return oa_json.replace("\\", "\\\\").replace('"', '\\"')


def embed_block(oa_lab: str, oa_prod: str) -> str:
    return (
        f"  {EMBED_START}\n"
        f"  static const char PROGMEM oa[] =\n"
        f'        "{escape_c_string(oa_lab)}";\n'
        f"  #else\n"
        f"  static const char PROGMEM oa[] =\n"
        f'        "{escape_c_string(oa_prod)}";\n'
        f"  #endif"
    )


def patch_api_routes(oa_lab: str, oa_prod: str) -> None:
    text = ROUTES_CPP.read_text(encoding="utf-8")
    fn_start = text.find(OPENAPI_FN_MARKER)
    if fn_start < 0:
        print("ERROR: handle_get_openapi not found", file=sys.stderr)
        raise SystemExit(2)
    embed_start = text.find(EMBED_START, fn_start)
    if embed_start < 0:
        print("ERROR: OpenAPI embed #if block not found", file=sys.stderr)
        raise SystemExit(2)
    embed_end = text.find("#endif", embed_start)
    if embed_end < 0:
        raise SystemExit(2)
    embed_end = text.find("\n", embed_end) + 1
    new_text = text[:embed_start] + embed_block(oa_lab, oa_prod) + "\n" + text[embed_end:]
    ROUTES_CPP.write_text(new_text, encoding="utf-8")


def main() -> None:
    if not YAML_PATH.is_file():
        print(f"ERROR: missing {YAML_PATH}", file=sys.stderr)
        raise SystemExit(2)
    full = load_paths()
    lab_json = compact_json(full)
    prod_json = compact_json(production_doc(full))
    patch_api_routes(lab_json, prod_json)
    print(
        f"OK: embedded OpenAPI ({len(full['paths'])} paths lab, "
        f"{len(full['paths']) - (1 if INJECT_PATH in full['paths'] else 0)} prod) into {ROUTES_CPP.name}"
    )


if __name__ == "__main__":
    main()
