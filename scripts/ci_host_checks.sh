#!/usr/bin/env bash
# Host-side CI parity: naming, contracts, native tests (+ optional coverage).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PIO="${ROOT}/.venv/bin/pio"
PY="${ROOT}/.venv/bin/python3"
if [[ ! -x "$PIO" ]]; then PIO=pio; fi
if [[ ! -x "$PY" ]]; then PY=python3; fi

SKIP_COVERAGE=0
SKIP_ROUTER_NATIVE=0
for arg in "$@"; do
  case "$arg" in
    --skip-coverage) SKIP_COVERAGE=1 ;;
    --skip-router-native) SKIP_ROUTER_NATIVE=1 ;;
  esac
done

echo "== Tracked assets guard =="
"${ROOT}/scripts/check_tracked_assets.sh"

echo "== Naming guard =="
"${ROOT}/scripts/check_naming.sh"

if [[ "$SKIP_ROUTER_NATIVE" -eq 0 ]]; then
  echo "== Native unit tests (Router logic) =="
  "$PIO" test -e native
else
  echo "== Native unit tests (Router logic): skipped (coverage step follows) =="
fi

echo "== Native unit tests (Balansun-Core domain) =="
"$PIO" test -d "${ROOT}/lib/balansun-core" -e native

if [[ "$SKIP_COVERAGE" -eq 0 ]]; then
  echo "== Native logic coverage (>=95%) =="
  "${ROOT}/.ci/coverage_native.sh"
fi

echo "== HIL limit catalog drift =="
"$PY" scripts/check_limit_catalog_drift.py

echo "== OpenAPI sync + drift =="
"$PY" -m pip install -q pyyaml 2>/dev/null || pip install -q pyyaml
"$PY" scripts/sync_openapi_snapshot.py
"$PY" scripts/embed_openapi.py
"$PY" scripts/check_openapi_drift.py
"$PY" scripts/check_openapi_routes.py

echo "== Meter pack generator drift =="
"$PY" scripts/generate_meter_pack.py --check

echo "== Profile test matrix drift =="
"$PY" scripts/generate_profile_test_matrix.py --check
"$PY" scripts/check_profile_wire_drift.py

echo "== Golden contract suite =="
"$PY" scripts/check_firmware_golden.py --eeprom-magic
"$PY" scripts/check_firmware_golden.py --suite firmware/test/golden/captures
"$PY" scripts/check_mqtt_goldens.py

echo "ci_host_checks: OK"
