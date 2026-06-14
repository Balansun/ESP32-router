#!/usr/bin/env bash
# Full verification: host checks, web tests, compile all matrix envs, optional HIL profile matrix.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
PIO="${ROOT}/.venv/bin/pio"
PY="${ROOT}/.venv/bin/python3"
if [[ ! -x "$PIO" ]]; then PIO=pio; fi
if [[ ! -x "$PY" ]] || ! "$PY" -m pytest --version >/dev/null 2>&1; then PY=python3; fi

echo "== Profile matrix generator =="
"$PY" scripts/generate_profile_test_matrix.py --check

echo "== Host checks (skip coverage) =="
"${ROOT}/scripts/ci_host_checks.sh" --skip-coverage

echo "== Web tests =="
unset BASE BALANSUN_FIELD_URL
(cd "${ROOT}/web" && npm ci && npm test)

echo "== Compile all product variant envs =="
ENVS=$("$PY" -c "import json; m=json.load(open('firmware/test/golden/profile_test_matrix.json')); print(' '.join('-e '+e for e in m['all_variant_envs']))")
# shellcheck disable=SC2086
"$PIO" run $ENVS
echo "== Flash size gates =="
"${ROOT}/scripts/check_variant_flash_sizes.sh"

if [[ -n "${BALANSUN_HIL_URL:-}" && -n "${BALANSUN_UPLOAD_PORT:-}" ]]; then
  echo "== HIL profile matrix (USB) =="
  "${ROOT}/scripts/run_hil_profile_matrix.sh"
else
  echo "Skip HIL profile matrix (set BALANSUN_HIL_URL and BALANSUN_UPLOAD_PORT to run)"
fi

echo "run_intensive_verification: OK"
