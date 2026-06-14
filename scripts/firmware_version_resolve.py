#!/usr/bin/env python3
"""
Resolve firmware Version for builds (firmware binary + embedded SPA).

Nightly style (CI main builds): ``{base}-nightly.{short_sha}``
e.g. ``0.1.0-nightly.abc1234`` — matches Git tag ``v0.1.0-nightly.abc1234``.

Local dev pre-release builds may use the current git branch slug instead of ``nightly``.

Resolution order (same for PlatformIO and build_web.py):
  1. BALANSUN_FIRMWARE_VERSION
  2. GITHUB_REF_NAME when it is a v-prefixed release tag
  3. Pre-release from git when BALANSUN_FIRMWARE_PRERELEASE=1 or PIOENV *\_prerelease
  4. #define Version in firmware/core/balansun_board.h
  5. "dev"
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
BOARD_H = REPO / "firmware" / "core" / "balansun_board.h"
_VERSION_RE = re.compile(r'#define\s+Version\s+"([^"]+)"')


def _env_truthy(name: str) -> bool:
    return os.environ.get(name, "").strip().lower() in ("1", "true", "yes", "on")


def read_base_version(board_h: Path = BOARD_H) -> str | None:
    if not board_h.is_file():
        return None
    m = _VERSION_RE.search(board_h.read_text(encoding="utf-8"))
    return m.group(1) if m else None


def sanitize_branch(branch: str) -> str:
    """GitHub-safe slug: lowercase, slashes and odd chars → hyphen."""
    s = branch.strip().lower()
    s = re.sub(r"[^a-z0-9]+", "-", s)
    return s.strip("-") or "unknown"


def _git(args: list[str], *, cwd: Path = REPO) -> str | None:
    try:
        r = subprocess.run(
            ["git", *args],
            cwd=cwd,
            capture_output=True,
            text=True,
            check=False,
            timeout=10,
        )
    except (OSError, subprocess.TimeoutExpired):
        return None
    if r.returncode != 0:
        return None
    out = (r.stdout or "").strip()
    return out or None


def git_short_sha(full_sha: str | None = None) -> str | None:
    if full_sha:
        return full_sha.strip()[:7] or None
    return _git(["rev-parse", "--short=7", "HEAD"])


def git_branch() -> str | None:
    raw = _git(["rev-parse", "--abbrev-ref", "HEAD"])
    if not raw:
        return None
    if raw == "HEAD":
        return "detached"
    return sanitize_branch(raw)


def prerelease_version_string(
    *,
    branch: str | None = None,
    sha: str | None = None,
    base: str | None = None,
) -> str | None:
    """``0.1.0-nightly.abc1234`` (or branch slug locally) — None if base or sha missing."""
    base_ver = (base or read_base_version() or "").strip()
    short = git_short_sha(sha)
    if not base_ver or not short:
        return None
    branch_slug = sanitize_branch(branch) if branch else (git_branch() or "unknown")
    return f"{base_ver}-{branch_slug}.{short}"


def prerelease_tag_string(
    *,
    branch: str | None = None,
    sha: str | None = None,
) -> str | None:
    ver = prerelease_version_string(branch=branch, sha=sha)
    return f"v{ver}" if ver else None


def resolve_firmware_version(*, prerelease_local: bool = False) -> str:
    explicit = os.environ.get("BALANSUN_FIRMWARE_VERSION", "").strip()
    if explicit:
        return explicit

    ref = os.environ.get("GITHUB_REF_NAME", "").strip()
    if ref.startswith("v") and re.match(r"^v\d", ref):
        return ref[1:]

    use_prerelease = prerelease_local or _env_truthy("BALANSUN_FIRMWARE_PRERELEASE")
    if use_prerelease:
        forced_branch = os.environ.get("BALANSUN_FIRMWARE_BRANCH", "").strip() or None
        forced_sha = os.environ.get("BALANSUN_FIRMWARE_SHA", "").strip() or None
        pre = prerelease_version_string(branch=forced_branch, sha=forced_sha)
        if pre:
            return pre

    return read_base_version() or "dev"


def env_uses_prerelease(pioenv: str) -> bool:
    return pioenv.strip().endswith("_prerelease")


def pioenv_uses_prerelease(pioenv: str | None = None) -> bool:
    name = (pioenv if pioenv is not None else os.environ.get("PIOENV", "")).strip()
    return env_uses_prerelease(name)


def resolve_for_build(pioenv: str | None = None) -> str:
    name = (pioenv if pioenv is not None else os.environ.get("PIOENV", "")).strip()
    use_prerelease = env_uses_prerelease(name) or _env_truthy("BALANSUN_FIRMWARE_PRERELEASE")
    return resolve_firmware_version(prerelease_local=use_prerelease)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--prerelease-tag",
        action="store_true",
        help="Print v-prefixed tag (CI); use with --branch and optional SHA arg",
    )
    parser.add_argument(
        "--prerelease",
        action="store_true",
        help="Print embedded version (no v prefix)",
    )
    parser.add_argument(
        "--export",
        action="store_true",
        help="Print shell: export BALANSUN_FIRMWARE_VERSION=…",
    )
    parser.add_argument(
        "--branch",
        metavar="NAME",
        help="Branch slug for --prerelease-tag (default: current git branch)",
    )
    parser.add_argument(
        "sha",
        nargs="?",
        help="Full commit SHA (default: HEAD)",
    )
    args = parser.parse_args()

    if args.prerelease_tag:
        tag = prerelease_tag_string(branch=args.branch or "nightly", sha=args.sha)
        if not tag:
            print("firmware_version_resolve: could not compute pre-release tag", file=sys.stderr)
            return 1
        print(tag)
        return 0

    if args.prerelease:
        ver = prerelease_version_string(branch=args.branch, sha=args.sha)
        if not ver:
            print("firmware_version_resolve: could not compute pre-release version", file=sys.stderr)
            return 1
        print(ver)
        return 0

    if args.export:
        ver = resolve_for_build()
        print(f'export BALANSUN_FIRMWARE_VERSION="{ver.replace(chr(34), chr(92)+chr(34))}"')
        return 0

    print(resolve_for_build())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
