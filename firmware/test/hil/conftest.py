from __future__ import annotations

import os
import subprocess
import sys

import pytest
import requests

from hil_auth_session import configure_hil_session
from hil_env import hil_env
from hil_helpers import get_with_retry

OPENAPI_PATHS = [
    "/api/v1/measurements",
    "/api/v1/system",
    "/api/v1/device",
    "/api/v1/state",
    "/api/v1/health",
    "/api/v1/sources",
    "/api/v1/config",
    "/api/v1/actions",
    "/api/v1/history/power",
    "/api/v1/gpio",
    "/api/v1/openapi.json",
]


@pytest.fixture(scope="session")
def hil_profile() -> str:
    profile = hil_env("BALANSUN_HIL_PROFILE", default="standard").strip().lower()
    if profile not in ("ap", "standard"):
        pytest.skip(f"BALANSUN_HIL_PROFILE must be ap or standard (got {profile!r})")
    return profile


@pytest.fixture(scope="session")
def hil_base_url(hil_profile: str) -> str:
    if hil_profile == "ap":
        return hil_env("BALANSUN_HIL_AP_URL", default="http://192.168.4.1").rstrip("/")
    url = hil_env("BALANSUN_HIL_URL", default="").rstrip("/")
    if not url:
        pytest.skip("BALANSUN_HIL_URL not set")
    return url


@pytest.fixture(scope="session")
def hil_session(hil_base_url: str):
    s = requests.Session()
    configure_hil_session(s, hil_base_url)
    r = get_with_retry(s, f"{hil_base_url}/api/v1/health", timeout=15, attempts=5)
    r.raise_for_status()
    return s


@pytest.fixture(scope="session")
def home_fixture_applied(hil_base_url: str):
    flag = hil_env("BALANSUN_HIL_APPLY_FIXTURE", default="").strip().lower()
    if flag not in ("1", "true", "yes"):
        return None
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    script = os.path.join(root, "scripts", "apply_home_config_fixture.py")
    env = os.environ.copy()
    env["BALANSUN_HIL_URL"] = hil_base_url
    proc = subprocess.run(
        [sys.executable, script, hil_base_url],
        cwd=root,
        env=env,
        capture_output=True,
        text=True,
        timeout=120,
        check=False,
    )
    if proc.returncode != 0:
        pytest.fail(
            "apply_home_config_fixture failed:\n"
            + (proc.stdout or "")
            + (proc.stderr or "")
        )
    return True


@pytest.fixture(scope="session")
def hil_expected_profile() -> str | None:
    from profile_matrix import expected_profile_name

    return expected_profile_name()


@pytest.fixture(scope="session")
def profile_row(hil_expected_profile: str | None):
    if not hil_expected_profile:
        pytest.skip("BALANSUN_HIL_EXPECT_PROFILE not set")
    from profile_matrix import row_for_profile

    row = row_for_profile(hil_expected_profile)
    if row is None:
        pytest.skip(f"unknown profile {hil_expected_profile!r}")
    return row


@pytest.fixture(scope="session")
def openapi_paths(hil_session, hil_base_url: str) -> list[str]:
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/openapi.json", timeout=15, attempts=5)
    r.raise_for_status()
    doc = r.json()
    return sorted(doc.get("paths", {}).keys())


@pytest.fixture(scope="session", autouse=True)
def hil_restore_passing_self_test(hil_session, hil_base_url: str):
    """Bench hygiene: clear safety lockout left by prior runs when HIL simulate API exists."""
    from hil_helpers import restore_passing_self_test

    restore_passing_self_test(hil_session, hil_base_url)
    yield
    restore_passing_self_test(hil_session, hil_base_url)
