#!/usr/bin/env bash
# Fail when wroom32 firmware.bin exceeds 95% of the OTA app partition (0x1C0000 bytes).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ENV="${1:-wroom32}"
PARTITION_BYTES=$((0x1C0000))
THRESHOLD_PCT="${BALANSUN_FLASH_SIZE_THRESHOLD_PCT:-95}"
THRESHOLD_BYTES=$((PARTITION_BYTES * THRESHOLD_PCT / 100))
BIN="${ROOT}/.pio/build/${ENV}/firmware.bin"

if [[ ! -f "$BIN" ]]; then
  echo "check_firmware_flash_size: missing $BIN — run: pio run -e ${ENV}" >&2
  exit 1
fi

if [[ "$(uname -s)" == "Darwin" ]]; then
  SIZE=$(stat -f%z "$BIN")
else
  SIZE=$(stat -c%s "$BIN")
fi

PCT=$((SIZE * 100 / PARTITION_BYTES))
echo "Firmware size: ${SIZE} / ${PARTITION_BYTES} bytes (${PCT}%, limit ${THRESHOLD_PCT}%)"

if [[ "$SIZE" -gt "$THRESHOLD_BYTES" ]]; then
  echo "check_firmware_flash_size: FAIL — exceeds ${THRESHOLD_PCT}% of OTA slot" >&2
  exit 1
fi

echo "check_firmware_flash_size: OK"
