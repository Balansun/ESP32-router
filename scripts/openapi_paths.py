"""Resolve canonical OpenAPI YAML (in-repo copy; optional website override)."""

from __future__ import annotations

import os
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
REPO_YAML = REPO_ROOT / "openapi" / "balansun-v1.yaml"

WEBSITE_ROOT = Path(
    os.environ.get("BALANSUN_WEBSITE_ROOT", REPO_ROOT.parent / "Balansun-Website")
)
WEBSITE_CANONICAL = WEBSITE_ROOT / "openapi" / "balansun-v1.yaml"
WEBSITE_OPENAPI_FALLBACK = WEBSITE_ROOT / "assets" / "openapi" / "balansun-v1.yaml"

# Preferred path for imports; use require_yaml() for resolution.
YAML_PATH = REPO_YAML


def require_yaml() -> Path:
    if REPO_YAML.is_file():
        return REPO_YAML
    for path in (WEBSITE_CANONICAL, WEBSITE_OPENAPI_FALLBACK):
        if path.is_file():
            return path
    print(
        f"ERROR: OpenAPI YAML not found at {REPO_YAML}\n"
        f"Optional override: set BALANSUN_WEBSITE_ROOT "
        f"(default: {WEBSITE_ROOT}).",
        file=__import__("sys").stderr,
    )
    raise SystemExit(2)
