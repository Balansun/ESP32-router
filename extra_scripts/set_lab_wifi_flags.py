"""Inject compile-time lab Wi-Fi credentials from BALANSUN_LAB_WIFI_SSID / BALANSUN_LAB_WIFI_PASSWORD."""

from __future__ import annotations

import os

Import("env")


def _escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


ssid = os.environ.get("BALANSUN_LAB_WIFI_SSID", "").strip()
password = os.environ.get("BALANSUN_LAB_WIFI_PASSWORD", "").strip()
if not ssid:
    print("set_lab_wifi_flags: BALANSUN_LAB_WIFI_SSID not set - skipping")
else:
    env.Append(CPPDEFINES=[
        ("BALANSUN_DEFAULT_WIFI_SSID", f'\\"{_escape(ssid)}\\"'),
        ("BALANSUN_DEFAULT_WIFI_PASSWORD", f'\\"{_escape(password)}\\"'),
    ])
    print(f"set_lab_wifi_flags: embedded SSID={ssid!r}")
