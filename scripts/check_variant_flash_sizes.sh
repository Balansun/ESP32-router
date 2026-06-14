#!/usr/bin/env bash
# Flash-size gate for all wroom32-class variant envs (skips esp32s3 / different partitions).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
PY="${ROOT}/.venv/bin/python3"
if [[ ! -x "$PY" ]]; then PY=python3; fi

SKIP_ENVS=("esp32s3" "esp32s3_opi" "esp32s3_ota" "hil" "wrover")

ALL_ENVS=$("$PY" -c "
import json
m = json.load(open('firmware/test/golden/profile_test_matrix.json'))
print(' '.join(m.get('all_variant_envs', [])))
")

for env in $ALL_ENVS; do
  for skip in "${SKIP_ENVS[@]}"; do
    if [[ "$env" == "$skip" ]]; then
      continue 2
    fi
  done
  # full_router wroom32 links every meter pack — largest image, still within OTA slot.
  threshold_pct="${BALANSUN_FLASH_SIZE_THRESHOLD_PCT:-95}"
  if [[ "$env" == "wroom32" ]]; then
    threshold_pct="${BALANSUN_WROOM32_FLASH_SIZE_THRESHOLD_PCT:-100}"
  fi
  echo "== Flash size gate: ${env} =="
  BALANSUN_FLASH_SIZE_THRESHOLD_PCT="$threshold_pct" "${ROOT}/scripts/check_firmware_flash_size.sh" "$env"
done

echo "check_variant_flash_sizes: OK"
