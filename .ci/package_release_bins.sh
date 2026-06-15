#!/usr/bin/env bash
# Package firmware bins, catalog, and SHA256SUMS into release/ (CI + release workflow).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${1:-}"
if [[ -z "$VERSION" ]]; then
  if [[ -n "${BALANSUN_FIRMWARE_VERSION:-}" ]]; then
    VERSION="${BALANSUN_FIRMWARE_VERSION}"
  else
    VERSION="$(grep '#define Version' firmware/core/balansun_board.h | sed -n 's/.*"\([^"]*\)".*/\1/p')"
  fi
fi

mkdir -p release
package_board() {
  local env="$1"
  local build=".pio/build/${env}"
  local prefix="balansun-${VERSION}-${env}"
  if [[ -f "$build/firmware.bin" ]]; then
    cp "$build/firmware.bin" "release/${prefix}-firmware.bin"
  else
    echo "Missing $build/firmware.bin" >&2
    exit 1
  fi
  if [[ -f "$build/bootloader.bin" ]]; then
    cp "$build/bootloader.bin" "release/${prefix}-bootloader.bin"
  fi
  if [[ -f "$build/partitions.bin" ]]; then
    cp "$build/partitions.bin" "release/${prefix}-partitions.bin"
  fi
}

ENVS=$(python3 -c "import json; m=json.load(open('firmware/test/golden/profile_test_matrix.json')); print(' '.join(m['all_variant_envs']))")
for env in $ENVS; do
  package_board "$env"
done
cp web/public/firmware-catalog.json release/firmware-catalog.json
(cd release && sha256sum ./*.bin) > release/SHA256SUMS.txt
cat release/SHA256SUMS.txt
ls -la release/
