"""Load limit catalog and helpers for HIL over-limit tests."""
from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import requests

from hil_helpers import get_with_retry

CATALOG_PATH = Path(__file__).resolve().parent / "limit_catalog.json"


def load_limit_catalog() -> list[dict[str, Any]]:
    doc = json.loads(CATALOG_PATH.read_text(encoding="utf-8"))
    return list(doc.get("entries", []))


def oversized_body(max_bytes: int, *, as_csv: bool = False) -> bytes:
    """Return a body of length exactly max_bytes + 1."""
    target = max_bytes + 1
    if as_csv:
        line = "date_iso,ch1_import_wh,ch1_export_wh,ch2_import_wh,ch2_export_wh\n"
        if len(line) >= target:
            return line[:target].encode()
        return (line + ("x" * (target - len(line)))).encode()
    prefix = b'{"pad":"'
    suffix = b'"}'
    pad_len = target - len(prefix) - len(suffix)
    if pad_len < 0:
        return b"x" * target
    return prefix + (b"x" * pad_len) + suffix


def http_auth_enabled(session: requests.Session, base_url: str) -> bool:
    r = session.get(f"{base_url.rstrip('/')}/api/v1/public", timeout=10)
    r.raise_for_status()
    return bool(r.json().get("http_auth", {}).get("enabled"))


def inject_route_available(session: requests.Session, base_url: str) -> bool:
    r = session.post(
        f"{base_url.rstrip('/')}/api/v1/sources/test/inject",
        json={"house": {"active_import_w": 0, "active_export_w": 0}},
        timeout=10,
    )
    return r.status_code != 404


def run_over_limit(
    session: requests.Session,
    base_url: str,
    entry: dict[str, Any],
) -> requests.Response:
    url = f"{base_url.rstrip('/')}{entry['path']}"
    method = entry["method"].upper()
    max_bytes = int(entry["max"])
    as_csv = entry.get("content_type") == "text/csv"
    data = oversized_body(max_bytes, as_csv=as_csv)
    headers: dict[str, str] = {}
    if as_csv:
        headers["Content-Type"] = "text/csv"
    else:
        headers["Content-Type"] = "application/json"
    return session.request(method, url, data=data, headers=headers, timeout=20)


def run_query_clamp(
    session: requests.Session,
    base_url: str,
    entry: dict[str, Any],
) -> requests.Response:
    param = entry["query_param"]
    over = int(entry["max"]) + 1
    url = f"{base_url.rstrip('/')}{entry['path']}?{param}={over}"
    return get_with_retry(session, url, timeout=20)
