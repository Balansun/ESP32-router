"""Accelerated replay of synthetic summer weekday (subset of slots)."""

from __future__ import annotations

import time

from hil_helpers import (
    inject_house,
    load_json_fixture,
    require_inject_api,
    skip_or_fail_not_regulating,
    triac_open_percent,
)


def test_summer_weekday_subset(hil_session, hil_base_url):
    require_inject_api(hil_session, hil_base_url)
    day = load_json_fixture("days", "summer_weekday.json")
    stride = int(day.get("meta", {}).get("ci_stride", 4))
    slots = day["slots"][::stride][:12]
    night = slots[2]
    midday = slots[len(slots) // 2]
    inject_house(
        hil_session,
        hil_base_url,
        night["active_import_w"],
        night["active_export_w"],
        wall_decihours=night["wall_decihours"],
        temperature_c=night.get("temperature_c"),
    )
    time.sleep(min(2.0, night.get("loops", 15) * 0.2))
    pct_night = triac_open_percent(hil_session, hil_base_url)
    inject_house(
        hil_session,
        hil_base_url,
        midday["active_import_w"],
        midday["active_export_w"],
        wall_decihours=midday["wall_decihours"],
        temperature_c=midday.get("temperature_c"),
    )
    time.sleep(min(3.0, midday.get("loops", 15) * 0.2))
    pct_midday = triac_open_percent(hil_session, hil_base_url)
    skip_or_fail_not_regulating(pct_night, pct_midday)
    assert pct_midday >= pct_night
