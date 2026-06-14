"""HIL: surplus solar → cumulus (triac) should open more on export than on import."""

from __future__ import annotations

import time

from hil_helpers import (
    inject_house,
    require_inject_api,
    skip_or_fail_not_regulating,
    triac_open_percent,
)


def test_export_profile_raises_triac_vs_import(hil_session, hil_base_url):
    require_inject_api(hil_session, hil_base_url)

    inject_house(hil_session, hil_base_url, 4000, 0)
    time.sleep(1.0)
    pct_import = triac_open_percent(hil_session, hil_base_url)
    m_import = hil_session.get(f"{hil_base_url.rstrip('/')}/api/v1/measurements", timeout=15).json()
    assert m_import["house"]["grid_net_w"] > 0

    inject_house(hil_session, hil_base_url, 80, 3500)
    time.sleep(1.0)
    pct_export = triac_open_percent(hil_session, hil_base_url)
    m_export = hil_session.get(f"{hil_base_url.rstrip('/')}/api/v1/measurements", timeout=15).json()
    assert m_export["house"]["grid_net_w"] < 0

    skip_or_fail_not_regulating(pct_import, pct_export)
    assert pct_export > pct_import, (
        f"expected higher triac on export ({pct_export}%) than import ({pct_import}%) "
        f"(grid_net import={m_import['house']['grid_net_w']} export={m_export['house']['grid_net_w']})"
    )
