#!/usr/bin/env bash
# Smoke-test ESP8266 action node REST API (LAN or mock, no auth v1).
set -euo pipefail

BASE="${BALANSUN_ACTION_URL:-${BALANSUN_MOCK_URL:-http://192.168.4.1}}"
BASE="${BASE%/}"

curl -fsS "${BASE}/api/v1/health" | grep -q '"role"[[:space:]]*:[[:space:]]*"action_node"'

STATE=$(curl -fsS "${BASE}/api/v1/action/state")
echo "$STATE" | grep -q '"supported_orders"'

curl -fsS -X PUT "${BASE}/api/v1/action/command" \
  -H 'Content-Type: application/json' \
  -d '{"pilot_wire_order":"eco"}' | grep -q '"pilot_wire_order"[[:space:]]*:[[:space:]]*"eco"'

curl -fsS -X PUT "${BASE}/api/v1/action/command" \
  -H 'Content-Type: application/json' \
  -d '{"pilot_wire_order":"auto"}' | grep -q '"control_mode"[[:space:]]*:[[:space:]]*"auto"'

curl -fsS -X PUT "${BASE}/api/v1/action/command" \
  -H 'Content-Type: application/json' \
  -d '{"pilot_wire_order":"eco","duration_sec":2}' | grep -q '"control_mode"[[:space:]]*:[[:space:]]*"manual"'

sleep 3
curl -fsS "${BASE}/api/v1/action/state" | grep -q '"control_mode"[[:space:]]*:[[:space:]]*"auto"'

SCHEDULE=$(curl -fsS -X PUT "${BASE}/api/v1/action/schedule" \
  -H 'Content-Type: application/json' \
  -d '{"periods":[{"kind":"fixed","hour_end":600,"order":"hors_gel"},{"kind":"fixed","hour_end":2400,"order":"arret"}]}')
echo "$SCHEDULE" | grep -q '"periods"'

GET_SCHEDULE=$(curl -fsS "${BASE}/api/v1/action/schedule")
echo "$GET_SCHEDULE" | grep -q '"hour_end"[[:space:]]*:[[:space:]]*600'
echo "$GET_SCHEDULE" | grep -q '"hour_end"[[:space:]]*:[[:space:]]*2400'

curl -fsS "${BASE}/api/v1/temperature" | grep -q '"temperature_ok"'

echo "action_node_smoke: OK (${BASE})"
