#!/usr/bin/env bash
# Flash hil_test_ap + hil_test_standard, apply home fixture, run route/limit HIL suites.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
PIO="${ROOT}/.venv/bin/pio"
PY="${ROOT}/.venv/bin/python3"
if [[ ! -x "$PIO" ]]; then PIO=pio; fi
if [[ ! -x "$PY" ]] || ! "$PY" -m pytest --version >/dev/null 2>&1; then PY=python3; fi

DEVICE="${BALANSUN_HIL_DEVICE:-wroom32}"
UPLOAD_PORT="${BALANSUN_UPLOAD_PORT:-}"
PROFILES=(ap standard)

run_profile() {
  local profile="$1"
  local env_name="hil_test_${profile}"
  export BALANSUN_HIL_PROFILE="$profile"
  export BALANSUN_HIL_APPLY_FIXTURE=1

  if [[ "$profile" == "ap" ]]; then
    export BALANSUN_HIL_URL="${BALANSUN_HIL_AP_URL:-http://192.168.4.1}"
  else
    : "${BALANSUN_HIL_URL:?Set BALANSUN_HIL_URL for standard profile}"
    : "${BALANSUN_LAB_WIFI_SSID:?Set BALANSUN_LAB_WIFI_SSID for hil_test_standard build}"
    : "${BALANSUN_LAB_WIFI_PASSWORD:?Set BALANSUN_LAB_WIFI_PASSWORD for hil_test_standard build}"
    export BALANSUN_LAB_WIFI_SSID BALANSUN_LAB_WIFI_PASSWORD
  fi

  echo "== Flash ${env_name} (${DEVICE}) -> ${BALANSUN_HIL_URL} =="
  if [[ -n "$UPLOAD_PORT" ]]; then
    BALANSUN_UPLOAD_PORT="$UPLOAD_PORT" "$PIO" run -e "$env_name" -t upload
  else
    "$PIO" run -e "$env_name" -t upload
  fi

  echo "== Wait health (${BALANSUN_HIL_URL}) =="
  "$PY" firmware/test/hil/wait_for_device.py

  echo "== Apply home fixture =="
  "$PY" scripts/apply_home_config_fixture.py "${BALANSUN_HIL_URL}"

  echo "== HIL route matrix (${profile}) =="
  "$PY" -m pytest firmware/test/hil/test_api_route_matrix.py -q

  echo "== HIL software limits (${profile}) =="
  "$PY" -m pytest firmware/test/hil/test_api_limits.py -q

  echo "== HIL smoke subset (${profile}) =="
  "$PY" -m pytest firmware/test/hil/test_api_smoke.py firmware/test/hil/test_api_parity_routes.py -q

  echo "== Profile ${profile} OK =="
}

echo "HIL device matrix: device=${DEVICE}"
for p in "${PROFILES[@]}"; do
  run_profile "$p"
done
echo "HIL device matrix passed for ${DEVICE} (ap + standard)."
