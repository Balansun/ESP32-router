#!/usr/bin/env python3
"""Import daily CH1/CH2 history CSV into Balansun API.

Expected compact CSV (same as export script / UI download):
date_iso,ch1_import_wh,ch1_export_wh,ch2_import_wh,ch2_export_wh

By default clears all on-device history (power buffers + daily ring) before import.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
import time
import urllib.parse

import requests

IMPORT_COMPAT_FIELDNAMES = [
    "date_iso",
    "ch1_import_wh",
    "ch1_export_wh",
    "ch2_import_wh",
    "ch2_export_wh",
]


def reset_history(base_url: str, headers: dict[str, str], timeout: float) -> None:
    url = f"{base_url.rstrip('/')}/api/v1/history/reset"
    res = requests.post(url, headers=headers, timeout=timeout)
    print(f"reset: {res.status_code} {res.text.strip()}")
    res.raise_for_status()


def import_csv(
    base_url: str,
    headers: dict[str, str],
    csv_text: str,
    timeout: float,
) -> requests.Response:
    url = f"{base_url.rstrip('/')}/api/v1/history/energy/daily/import"
    # ESP32 WebServer expects urlencoded form field "plain" (not raw text/csv body).
    body = "plain=" + urllib.parse.quote(csv_text, safe="")
    hdrs = {
        **headers,
        "Content-Type": "application/x-www-form-urlencoded",
    }
    return requests.post(url, headers=hdrs, data=body.encode("utf-8"), timeout=timeout)


def verify_history_paginated(
    base_url: str,
    headers: dict[str, str],
    timeout: float,
    *,
    expected_days: int | None = None,
    max_attempts: int = 20,
    retry_delay_s: float = 2.0,
) -> bool:
    """Paginate GET daily history; retry on 503/empty body or pending_commit."""
    url = f"{base_url.rstrip('/')}/api/v1/history/energy/daily"
    for attempt in range(1, max_attempts + 1):
        try:
            offset = 0
            total = 0
            pending = False
            last: dict | None = None
            while True:
                page = requests.get(
                    url,
                    params={"limit": 10, "offset": offset},
                    headers=headers,
                    timeout=timeout,
                )
                if page.status_code == 503:
                    raise requests.HTTPError(
                        f"503 {page.text.strip()}", response=page
                    )
                page.raise_for_status()
                if not page.text.strip():
                    raise ValueError("empty response body")
                data = page.json()
                if data.get("pending_commit"):
                    pending = True
                n = len(data.get("day_dates_iso") or [])
                total += n
                offset += n
                last = data
                if not data.get("has_more") or n == 0:
                    break
            if pending:
                print(
                    f"verify attempt {attempt}/{max_attempts}: "
                    "pending_commit (NVS flush in progress)"
                )
                time.sleep(retry_delay_s)
                continue
            ref = (last or {}).get("reference_date_iso")
            tc = (last or {}).get("total_count")
            print(
                f"verify: reference={ref} days_read={total} total_count={tc}"
            )
            if expected_days is not None and total < expected_days:
                print(
                    f"verify attempt {attempt}/{max_attempts}: "
                    f"expected>={expected_days} got {total}"
                )
                time.sleep(retry_delay_s)
                continue
            return True
        except (requests.RequestException, json.JSONDecodeError, ValueError) as exc:
            print(f"verify attempt {attempt}/{max_attempts}: {exc}")
            if attempt >= max_attempts:
                return False
            time.sleep(retry_delay_s)
    return False


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_file", type=Path, help="CSV with date_iso + CH1/CH2 import/export Wh")
    parser.add_argument("--base-url", default="http://192.168.2.159")
    parser.add_argument("--token", default="", help="Bearer token")
    parser.add_argument(
        "--no-reset",
        action="store_true",
        help="Skip POST /api/v1/history/reset before import (default: reset first)",
    )
    parser.add_argument(
        "--post-reset-wait",
        type=float,
        default=2.0,
        help="Seconds to wait after reset before import",
    )
    parser.add_argument("--timeout", type=float, default=180.0, help="HTTP timeout seconds")
    parser.add_argument(
        "--verify-attempts",
        type=int,
        default=20,
        help="Max paginated GET verify attempts after import",
    )
    parser.add_argument(
        "--verify-delay",
        type=float,
        default=2.0,
        help="Seconds between verify retries",
    )
    args = parser.parse_args()

    csv_text = args.csv_file.read_text(encoding="utf-8")
    if not csv_text.strip():
        print("error: empty CSV file", file=sys.stderr)
        return 1

    data_lines = [
        ln
        for ln in csv_text.splitlines()
        if ln.strip() and not ln.strip().startswith("#") and not ln.startswith("date_iso,")
    ]
    expected_days = len(data_lines)

    headers: dict[str, str] = {"Accept": "application/json"}
    if args.token:
        headers["Authorization"] = f"Bearer {args.token}"

    base = args.base_url.rstrip("/")

    if not args.no_reset:
        reset_history(base, headers, args.timeout)
        if args.post_reset_wait > 0:
            time.sleep(args.post_reset_wait)

    try:
        res = import_csv(base, headers, csv_text, args.timeout)
    except requests.RequestException as exc:
        print(f"import failed: {exc}", file=sys.stderr)
        return 1

    print(f"import: {res.status_code}")
    print(res.text)
    if not res.ok:
        return 1

    try:
        body = res.json()
        if body.get("pending_commit"):
            print("import: pending_commit=true (NVS flush deferred)")
    except json.JSONDecodeError:
        pass

    time.sleep(1)
    if not verify_history_paginated(
        base,
        headers,
        args.timeout,
        expected_days=expected_days,
        max_attempts=args.verify_attempts,
        retry_delay_s=args.verify_delay,
    ):
        print("verify failed", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
