#!/usr/bin/env bash
# Balansun naming guard: no new rms_* firmware filenames or product RMS tokens in balansun_* sources.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

FIRMWARE_CORE="${ROOT}/firmware/core"
failures=0

fail() {
  echo "check_naming: FAIL: $*" >&2
  failures=$((failures + 1))
}

if ! command -v rg >/dev/null 2>&1; then
  echo "check_naming: FAIL — install ripgrep (rg)" >&2
  exit 1
fi

# No rms_*.h|cpp under firmware (post-rebrand)
while IFS= read -r -d '' f; do
  fail "forbidden filename: ${f#"$ROOT"/}"
done < <(find "${ROOT}/firmware" -type f \( -name 'rms_*.h' -o -name 'rms_*.cpp' \) -print0 2>/dev/null || true)

# Product RMS tokens in balansun_* sources (allow electrical RMS in metering/)
if rg -n --glob 'balansun_*.{h,cpp}' -e '\bRMS\b|\bRMSext\b|rms_ext_|rms_setup|rms_loop' "${FIRMWARE_CORE}" 2>/dev/null; then
  fail "product RMS tokens remain in firmware/core/balansun_* (see /en/developer/ § Naming)"
fi

# Balansun-owned wire JSON must not reintroduce camelCase keys (vendor parsers excluded).
# Settings backup file uses camelCase keys shared with the web SPA — not REST snake_case.
if rg -n \
  --glob 'firmware/api/api_v1_*.cpp' \
  --glob '!firmware/api/api_v1_system_backup.cpp' \
  --glob 'firmware/core/actions_api.cpp' \
  --glob 'firmware/mqtt_ha.cpp' \
  --glob 'firmware/mqtt_ha_discovery.cpp' \
  --glob 'firmware/mqtt_ha_client.cpp' \
  -e '"[a-z]+[A-Z][a-zA-Z]*"' \
  "${ROOT}" 2>/dev/null; then
  fail "camelCase JSON string keys in Balansun REST/MQTT emitters (use snake_case)"
fi

# Product RMS tokens in metering sources
if rg -n --glob '*.cpp' -e 'clientESP_RMS|rmsExtRecordPoll|\bESP_RMS\b' "${ROOT}/firmware/metering" 2>/dev/null; then
  fail "product RMS identifiers in firmware/metering/"
fi

if [[ "$failures" -gt 0 ]]; then
  exit 1
fi

echo "check_naming: OK"
