#!/usr/bin/env bash
# Fail if generated or orphan files under web/public/ are git-tracked.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
NAME="check_tracked_assets"

tracked_public="$(git ls-files 'web/public/' 2>/dev/null || true)"
if [[ -n "${tracked_public}" ]]; then
  echo "${NAME}: FAIL — do not commit web/public/ (generated on npm run build):" >&2
  echo "${tracked_public}" >&2
  echo "Remove from git (git rm --cached) and ensure web/public/ is in .gitignore." >&2
  exit 1
fi

if command -v rg >/dev/null 2>&1; then
  for f in $(git ls-files 'assets/**' 2>/dev/null); do
    base="$(basename "$f")"
    if [[ "$base" == README.md ]]; then
      continue
    fi
    if ! rg -q -F "$base" README.md web scripts assets 2>/dev/null; then
      echo "${NAME}: WARN — no reference found for tracked asset ${f}" >&2
    fi
  done
fi

echo "${NAME}: OK"
