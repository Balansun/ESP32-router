"""Configure requests sessions for HIL (Bearer token or password login)."""

from __future__ import annotations

import requests

from hil_env import hil_env


def hil_bearer_token() -> str:
    return hil_env(
        "BALANSUN_API_BEARER_TOKEN",
        "BALANSUN_HIL_BEARER_TOKEN",
        default="",
    ).strip()


def hil_login_session_token(base_url: str) -> str | None:
    password = hil_env("BALANSUN_HIL_PASSWORD", default="")
    if not password:
        return None
    r = requests.post(
        f"{base_url.rstrip('/')}/api/v1/auth/login",
        json={"password": password},
        timeout=15,
    )
    if r.status_code != 200:
        r.raise_for_status()
    token = r.json().get("token")
    if isinstance(token, str) and token:
        return token
    return None


def configure_hil_session(session: requests.Session, base_url: str) -> None:
    """Attach Bearer auth when token or password login succeeds; else open LAN."""
    session.auth = None
    session.headers.pop("Authorization", None)
    token = hil_bearer_token()
    if token:
        session.headers["Authorization"] = f"Bearer {token}"
        try:
            probe = session.get(f"{base_url.rstrip('/')}/api/v1/health", timeout=5)
            if probe.status_code == 401:
                token = ""
        except requests.RequestException:
            token = ""
    if not token:
        token = hil_login_session_token(base_url)
    if token:
        session.headers["Authorization"] = f"Bearer {token}"
