"""Inject firmware Version from tag/env at compile time (matches GitHub release names)."""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

Import("env")

PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
sys.path.insert(0, str(PROJECT_DIR / "scripts"))
from firmware_version_resolve import (  # noqa: E402
    env_uses_prerelease,
    resolve_firmware_version,
)

PIOENV = env.subst("$PIOENV").strip()


def _env_truthy(name: str) -> bool:
    return os.environ.get(name, "").strip().lower() in ("1", "true", "yes", "on")


def _should_inject() -> bool:
    if os.environ.get("BALANSUN_FIRMWARE_VERSION", "").strip():
        return True
    ref = os.environ.get("GITHUB_REF_NAME", "").strip()
    if ref.startswith("v") and re.match(r"^v\d", ref):
        return True
    return env_uses_prerelease(PIOENV) or _env_truthy("BALANSUN_FIRMWARE_PRERELEASE")


ver = resolve_firmware_version(
    prerelease_local=env_uses_prerelease(PIOENV) or _env_truthy("BALANSUN_FIRMWARE_PRERELEASE")
)
if _should_inject() and ver:
    escaped = ver.replace("\\", "\\\\").replace('"', '\\"')
    env.Append(CPPDEFINES=[("Version", f'\\"{escaped}\\"')])
    print(f"set_firmware_version: Version={ver}")
