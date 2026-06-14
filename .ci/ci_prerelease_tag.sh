#!/usr/bin/env sh
# Print the Git tag for a main-branch nightly pre-release (v{Version}-nightly.{short_sha}).
# Usage: ./.ci/ci_prerelease_tag.sh [FULL_SHA]
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SHA="${1:-${GITHUB_SHA:-}}"
if [ -z "$SHA" ]; then
  echo "ci_prerelease_tag: need commit SHA as argument or GITHUB_SHA" >&2
  exit 1
fi

exec python3 "$ROOT/scripts/firmware_version_resolve.py" --prerelease-tag --branch nightly "$SHA"
