#!/usr/bin/env bash
# Flash each profile_test_matrix variant on USB bench and run role-appropriate HIL tests.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
PIO="${ROOT}/.venv/bin/pio"
PY="${ROOT}/.venv/bin/python3"
if [[ ! -x "$PIO" ]]; then PIO=pio; fi
if [[ ! -x "$PY" ]] || ! "$PY" -m pytest --version >/dev/null 2>&1; then PY=python3; fi

MATRIX="${ROOT}/firmware/test/golden/profile_test_matrix.json"
UPLOAD_PORT="${BALANSUN_UPLOAD_PORT:-}"
: "${BALANSUN_HIL_URL:?Set BALANSUN_HIL_URL}"
RESTORE_ENV="${BALANSUN_HIL_RESTORE_ENV:-esp32_32u_jsy_router}"

PACK_FILTER="${BALANSUN_HIL_MATRIX_PACK:-}"
ROLE_FILTER="${BALANSUN_HIL_MATRIX_ROLE:-}"
SKIP_RESTORE="${BALANSUN_HIL_SKIP_RESTORE:-0}"
MATRIX_AFTER="${BALANSUN_HIL_MATRIX_AFTER:-}"
MATRIX_REACHED="${MATRIX_AFTER:+0}"

usage() {
  echo "Usage: BALANSUN_HIL_URL=... BALANSUN_UPLOAD_PORT=/dev/cu.usbserial-0001 $0"
  echo "Optional: BALANSUN_HIL_MATRIX_PACK=jsy_mk194 BALANSUN_HIL_MATRIX_ROLE=meter_gateway"
  echo "          BALANSUN_HIL_MATRIX_AFTER=shelly_pro_meter  # resume from profile (inclusive)"
  exit 1
}

should_run_variant() {
  local profile="$1"
  if [[ -n "$MATRIX_AFTER" && "$MATRIX_REACHED" == "0" ]]; then
    if [[ "$profile" == "$MATRIX_AFTER" ]]; then
      MATRIX_REACHED=1
    else
      echo "Skip (resume after ${MATRIX_AFTER}): ${profile}"
      return 1
    fi
  fi
  return 0
}

[[ -f "$MATRIX" ]] || { echo "Missing $MATRIX"; exit 1; }
[[ -n "$UPLOAD_PORT" ]] || { echo "BALANSUN_UPLOAD_PORT required for USB flash"; usage; }

upload_with_retry() {
  local pio_env="$1"
  local tries="${BALANSUN_UPLOAD_RETRIES:-3}"
  local n=1
  while [[ "$n" -le "$tries" ]]; do
    if BALANSUN_UPLOAD_PORT="$UPLOAD_PORT" "$PIO" run -e "$pio_env" -t upload; then
      return 0
    fi
    echo "Upload attempt ${n}/${tries} failed for ${pio_env}; retrying in 5s..."
    sleep 5
    n=$((n + 1))
  done
  return 1
}

run_variant() {
  local profile="$1"
  local pio_env="$2"
  local role="$3"
  local hil_suite="$4"

  echo ""
  echo "======== Profile matrix: ${profile} (${pio_env}) ========"
  export BALANSUN_HIL_EXPECT_PROFILE="$profile"

  BALANSUN_UPLOAD_PORT="$UPLOAD_PORT" upload_with_retry "$pio_env"
  if [[ "$hil_suite" == "inject_regulation" ]]; then
    export BALANSUN_HIL_WAIT_TELEMETRY=1
  else
    unset BALANSUN_HIL_WAIT_TELEMETRY
  fi
  "$PY" firmware/test/hil/wait_for_device.py
  sleep 5

  if [[ "$hil_suite" == "inject_regulation" ]]; then
    export BALANSUN_HIL_APPLY_FIXTURE=1
    fixture_ok=0
    for attempt in 1 2 3; do
      if "$PY" scripts/apply_home_config_fixture.py "${BALANSUN_HIL_URL}"; then
        fixture_ok=1
        break
      fi
      echo "apply_home_config_fixture attempt ${attempt}/3 failed; retrying in 5s..."
      sleep 5
    done
    if [[ "$fixture_ok" != "1" ]]; then
      echo "apply_home_config_fixture failed after 3 attempts" >&2
      return 1
    fi
  else
    unset BALANSUN_HIL_APPLY_FIXTURE
  fi

  "$PY" -m pytest firmware/test/hil/test_device_profile_matrix.py \
    firmware/test/hil/test_profile_caps.py \
    firmware/test/hil/test_pwm_config_caps.py \
    firmware/test/hil/test_config_settings_roundtrip.py \
    firmware/test/hil/test_safety_lockout.py -q

  if [[ "$hil_suite" == "inject_regulation" ]]; then
    export BALANSUN_HIL_REQUIRE_REGULATION="${BALANSUN_HIL_REQUIRE_REGULATION:-1}"
    "$PY" -m pytest firmware/test/hil/test_inject_regulation.py \
      firmware/test/hil/test_inject_validation.py \
      firmware/test/hil/test_regulation_ramp.py \
      firmware/test/hil/test_surplus_routing.py -q
  fi

  echo "======== OK: ${profile} ========"
}

# Order: gateways, routers, hil inject, restore.
while IFS=$'\t' read -r profile pio_env role hil_suite pack_id; do
  [[ -n "$profile" ]] || continue
  if [[ -n "$PACK_FILTER" && "$pack_id" != "$PACK_FILTER" ]]; then
    continue
  fi
  if [[ -n "$ROLE_FILTER" && "$role" != "$ROLE_FILTER" ]]; then
    continue
  fi
  if [[ "$role" == "meter_gateway" ]]; then
    should_run_variant "$profile" || continue
    run_variant "$profile" "$pio_env" "$role" "$hil_suite"
  fi
done < <("$PY" -c "
import json,sys
m=json.load(open('$MATRIX'))
for v in m['variants']:
    if v['role']=='meter_gateway':
        print(v['product_profile'], v['pio_env'], v['role'], v['hil_suite'], v['pack_id'], sep='\t')
")

while IFS=$'\t' read -r profile pio_env role hil_suite pack_id; do
  [[ -n "$profile" ]] || continue
  if [[ -n "$PACK_FILTER" && "$pack_id" != "$PACK_FILTER" ]]; then
    continue
  fi
  if [[ -n "$ROLE_FILTER" && "$role" != "$ROLE_FILTER" ]]; then
    continue
  fi
  if [[ "$role" == "meter_router" ]]; then
    should_run_variant "$profile" || continue
    run_variant "$profile" "$pio_env" "$role" "$hil_suite"
  fi
done < <("$PY" -c "
import json
m=json.load(open('$MATRIX'))
for v in m['variants']:
    if v['role']=='meter_router':
        print(v['product_profile'], v['pio_env'], v['role'], v['hil_suite'], v['pack_id'], sep='\t')
")

while IFS=$'\t' read -r profile pio_env role hil_suite pack_id; do
  [[ -n "$profile" ]] || continue
  if [[ -n "$PACK_FILTER" && "$pack_id" != "$PACK_FILTER" ]]; then
    continue
  fi
  if [[ -n "$ROLE_FILTER" && "$role" != "$ROLE_FILTER" ]]; then
    continue
  fi
  if [[ "$role" == "full_router" ]]; then
    should_run_variant "$profile" || continue
    run_variant "$profile" "$pio_env" "$role" "$hil_suite"
  fi
done < <("$PY" -c "
import json
m=json.load(open('$MATRIX'))
for v in m['variants']:
    if v['role']=='full_router':
        print(v['product_profile'], v['pio_env'], v['role'], v['hil_suite'], v['pack_id'], sep='\t')
")

if [[ "$SKIP_RESTORE" != "1" ]]; then
  echo ""
  echo "======== Restoring bench default: ${RESTORE_ENV} ========"
  export BALANSUN_HIL_EXPECT_PROFILE=""
  upload_with_retry "$RESTORE_ENV"
  "$PY" firmware/test/hil/wait_for_device.py
fi

echo "HIL profile matrix complete."
