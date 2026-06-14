#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO (pio) required" >&2
  exit 1
fi
pio test -e native "$@"
