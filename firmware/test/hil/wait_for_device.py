#!/usr/bin/env python3
"""Poll GET /api/v1/health until the HIL device responds."""

from __future__ import annotations

import sys
import time

import requests

from hil_auth_session import configure_hil_session
from hil_env import hil_env

TIMEOUT_S = int(hil_env("BALANSUN_HIL_WAIT_S", default="120"))
INTERVAL_S = 2


def wait_telemetry_ready() -> bool:
    flag = hil_env("BALANSUN_HIL_WAIT_TELEMETRY", default="").strip().lower()
    return flag in ("1", "true", "yes")


def main() -> int:
    base = hil_env("BALANSUN_HIL_URL", default="").rstrip("/")
    if not base:
        print("ERROR: BALANSUN_HIL_URL is not set", file=sys.stderr)
        return 2
    session = requests.Session()
    try:
        configure_hil_session(session, base)
    except requests.RequestException:
        pass
    need_telemetry = wait_telemetry_ready()
    deadline = time.time() + TIMEOUT_S
    url = f"{base}/api/v1/health"
    while time.time() < deadline:
        try:
            r = session.get(url, timeout=5)
            if r.status_code == 200:
                body = r.json()
                if not need_telemetry or body.get("telemetry_ready") is True:
                    if need_telemetry:
                        print(f"OK: device telemetry ready at {base}")
                    else:
                        print(f"OK: device health at {base} (profile={body.get('product_profile', '?')})")
                    return 0
        except requests.RequestException:
            pass
        time.sleep(INTERVAL_S)
    if need_telemetry:
        print(f"ERROR: device telemetry not ready at {url} within {TIMEOUT_S}s", file=sys.stderr)
    else:
        print(f"ERROR: device not reachable at {url} within {TIMEOUT_S}s", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
