#!/usr/bin/env python3
"""Regenerate web/test/fixtures/openapi-snapshot.json from openapi/balansun-v1.yaml."""

from __future__ import annotations

import json
import sys
from pathlib import Path

from openapi_paths import REPO_ROOT as REPO, require_yaml

YAML_PATH = require_yaml()

OUT = REPO / "web" / "test" / "fixtures" / "openapi-snapshot.json"


def main() -> None:
    try:
        import yaml  # type: ignore
    except ImportError:
        print("ERROR: PyYAML required", file=sys.stderr)
        raise SystemExit(2)
    doc = yaml.safe_load(YAML_PATH.read_text(encoding="utf-8"))
    OUT.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT}")


if __name__ == "__main__":
    main()
