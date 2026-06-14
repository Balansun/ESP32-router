#!/usr/bin/env bash
# P0-6e API-level field smoke (run against home router or mock on LAN).
# Full USB flash / Wi‑Fi / GPIO checklist: /en/getting-started/
set -euo pipefail
BASE="${1:-${BALANSUN_FIELD_URL:-${BALANSUN_MOCK_URL:-${BALANSUN_HIL_URL:-http://127.0.0.1:8787}}}}"
BASE="${BASE%/}"

BALANSUN_SMOKE_AUTH=()
_resolve_smoke_auth() {
  local token=""
  if [[ -n "${BALANSUN_API_BEARER_TOKEN:-}" ]]; then
    token="${BALANSUN_API_BEARER_TOKEN}"
  elif [[ -n "${BALANSUN_HIL_PASSWORD:-}" ]]; then
    token="$(
      curl -fsS -m 15 -X POST -H "Content-Type: application/json" \
        -d "{\"password\":\"${BALANSUN_HIL_PASSWORD}\"}" \
        "${BASE}/api/v1/auth/login" \
        | python3 -c "import json,sys; print(json.load(sys.stdin)['token'])"
    )"
  fi
  if [[ -n "${token}" ]]; then
    BALANSUN_SMOKE_AUTH=(-H "Authorization: Bearer ${token}")
    return 0
  fi
  local code
  code="$(curl -sS -m 5 -o /dev/null -w "%{http_code}" "${BASE}/api/v1/health" 2>/dev/null || echo "000")"
  if [[ "${code}" == "401" ]]; then
    echo "FAIL: ${BASE}/api/v1/health returned 401 — set BALANSUN_API_BEARER_TOKEN or BALANSUN_HIL_PASSWORD" >&2
    exit 1
  fi
}

smoke_curl() {
  curl -fsS -m 10 "${BALANSUN_SMOKE_AUTH[@]}" "$@"
}

echo "== Field smoke API (${BASE}) =="
_resolve_smoke_auth

smoke_curl "${BASE}/api/v1/health" | python3 -c "
import json, sys
d = json.load(sys.stdin)
assert d.get('ok') is True or d.get('status') == 'ok' or 'ok' in str(d).lower(), d
lock = d.get('safety_lockout') or {}
assert isinstance(lock.get('active'), bool), lock
assert isinstance(lock.get('reasons'), list), lock
st = d.get('self_test') or {}
if st:
    assert 'safety_lockout_active' in st, st
    assert 'severity' in st, st
"

dev="$(smoke_curl "${BASE}/api/v1/device")"
python3 -c "
import json, sys
dev = json.loads(sys.argv[1])
caps = (dev.get('capabilities') or {}).get('firmware_capabilities') or {}
if caps.get('surplus_regulation'):
    health = json.loads(sys.argv[2])
    assert 'safety_lockout' in health, health
    assert 'self_test' in health, health
" "$dev" "$(smoke_curl "${BASE}/api/v1/health")"

m="$(smoke_curl "${BASE}/api/v1/measurements")"
python3 -c "
import json, sys
m = json.loads(sys.argv[1])
house = m['house']
for k in ('active_import_w', 'active_export_w'):
    assert k in house, k
net = house['active_import_w'] - house['active_export_w']
if 'grid_net_w' in house:
    assert house['grid_net_w'] == net, house
    print('measurements OK:', house['grid_net_w'], 'W net')
else:
    print('measurements OK:', net, 'W net (derived from import/export)')
" "$m"

inject_code="$(curl -fsS -m 5 -o /dev/null -w "%{http_code}" "${BALANSUN_SMOKE_AUTH[@]}" -X POST "${BASE}/api/v1/sources/test/inject" \
  -H 'Content-Type: application/json' \
  -d '{"house":{"active_import_w":80,"active_export_w":3500}}' 2>/dev/null || echo "000")"
if [[ "${inject_code}" =~ ^2 ]]; then
  if [[ "${BALANSUN_STRICT_PRODUCTION:-0}" == "1" ]]; then
    echo "FAIL: POST /api/v1/sources/test/inject returned ${inject_code} (lab API on production target)" >&2
    exit 1
  fi
  sleep 1
  st="$(smoke_curl "${BASE}/api/v1/state")"
  python3 -c "
import json, sys
st = json.loads(sys.argv[1])
pct = float(st['triac_open_percent'])
assert pct >= 0, st
print('triac_open_percent after export inject:', pct)
" "$st"
else
  echo "inject route absent (expected on wroom32 production) — skipping triac inject check"
fi

echo "Field smoke API checks passed for ${BASE}"
