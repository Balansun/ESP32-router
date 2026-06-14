#!/usr/bin/env python3
"""Export paginated daily CH1/CH2 history CSV from Balansun API.

Output schema is intentionally the same compact import format accepted by
/api/v1/history/energy/daily/import:
date_iso,ch1_import_wh,ch1_export_wh,ch2_import_wh,ch2_export_wh
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import requests

IMPORT_COMPAT_FIELDNAMES = [
    "date_iso",
    "ch1_import_wh",
    "ch1_export_wh",
    "ch2_import_wh",
    "ch2_export_wh",
]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output_csv", type=Path)
    parser.add_argument("--base-url", default="http://192.168.2.159")
    parser.add_argument("--token", default="", help="Bearer token")
    parser.add_argument("--from-date", required=True, help="YYYY-MM-DD")
    parser.add_argument("--to-date", required=True, help="YYYY-MM-DD")
    args = parser.parse_args()

    headers = {"Accept": "application/json"}
    if args.token:
        headers["Authorization"] = f"Bearer {args.token}"

    rows: list[dict[str, int | str]] = []
    offset = 0
    while True:
        params = {
            "from_date": args.from_date,
            "to_date": args.to_date,
            "limit": 10,
            "offset": offset,
        }
        url = f"{args.base_url.rstrip('/')}/api/v1/history/energy/daily"
        res = requests.get(url, params=params, headers=headers, timeout=30)
        res.raise_for_status()
        data = res.json()
        day_dates = data.get("day_dates_iso") or []
        ch1_import = data.get("ch1_import_wh_per_day") or []
        ch1_export = data.get("ch1_export_wh_per_day") or []
        ch2_import = data.get("ch2_import_wh_per_day") or []
        ch2_export = data.get("ch2_export_wh_per_day") or []
        page_len = len(day_dates)
        for i in range(page_len):
            rows.append(
                {
                    "date_iso": str(day_dates[i]),
                    "ch1_import_wh": int(ch1_import[i]) if i < len(ch1_import) else 0,
                    "ch1_export_wh": int(ch1_export[i]) if i < len(ch1_export) else 0,
                    "ch2_import_wh": int(ch2_import[i]) if i < len(ch2_import) else 0,
                    "ch2_export_wh": int(ch2_export[i]) if i < len(ch2_export) else 0,
                }
            )
        offset += page_len
        if not data.get("has_more") or page_len == 0:
            break

    args.output_csv.parent.mkdir(parents=True, exist_ok=True)
    # Keep chronological order and write the exact compact import schema.
    rows.sort(key=lambda r: str(r["date_iso"]))
    with args.output_csv.open("w", encoding="utf-8", newline="") as fp:
        writer = csv.DictWriter(fp, fieldnames=IMPORT_COMPAT_FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)
    print(
        f"wrote {len(rows)} rows to {args.output_csv} "
        f"(import-compatible schema: {','.join(IMPORT_COMPAT_FIELDNAMES)})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
