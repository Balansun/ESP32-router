"""HIL: backup export/import via /api/v1/system/backup."""

from __future__ import annotations

import pytest

from hil_helpers import get_with_retry

BACKUP_REQUIRED_TOP = ("backupSchemaVersion", "exportedAt", "config", "actions", "wifi")
BACKUP_OPTIONAL_ON_SPARSE = frozenset({"time", "api"})


@pytest.fixture(scope="module")
def backup_doc(hil_session, hil_base_url):
    r = get_with_retry(hil_session, f"{hil_base_url}/api/v1/system/backup", timeout=30)
    assert r.status_code == 200, r.text[:500]
    doc = r.json()
    export_mode = (doc.get("device") or {}).get("export_mode") or (doc.get("config") or {}).get(
        "export_mode"
    )
    sparse = export_mode == "sparse"
    for key in BACKUP_REQUIRED_TOP:
        assert key in doc, f"missing {key}"
    for key in BACKUP_OPTIONAL_ON_SPARSE:
        if sparse and key not in doc:
            continue
        assert key in doc, f"missing {key}"
    assert doc["backupSchemaVersion"] == 1
    assert isinstance(doc["config"], dict)
    assert isinstance(doc["actions"], dict)
    assert "actions" in doc["actions"]
    return doc


def test_system_backup_export_schema(backup_doc):
    assert backup_doc["wifi"]["ssid"]
    assert "password" in backup_doc["wifi"]


def test_system_backup_api_tokens_when_auth(hil_session, hil_base_url, backup_doc):
    """When HTTP auth is on, export includes PAT secrets if any exist."""
    pub = hil_session.get(f"{hil_base_url}/api/v1/public", timeout=10).json()
    if not pub.get("http_auth", {}).get("enabled"):
        pytest.skip("HTTP API auth disabled on device")
    r = hil_session.post(
        f"{hil_base_url}/api/v1/auth/tokens",
        json={"label": "hil-backup-test"},
        timeout=15,
    )
    assert r.status_code == 200, r.text[:300]
    created = r.json()
    token = created["token"]
    assert len(token) == 64

    r2 = get_with_retry(hil_session, f"{hil_base_url}/api/v1/system/backup", timeout=30)
    assert r2.status_code == 200
    doc2 = r2.json()
    assert "api" in doc2, "backup should include api when auth enabled"
    api = doc2["api"]
    assert api.get("http_api_password") or api.get("access_tokens")
    tokens = api.get("access_tokens") or []
    found = [t for t in tokens if t.get("token") == token]
    assert found, "created PAT secret must appear in backup export"
    assert found[0].get("label")

    # Cleanup: revoke test token
    tid = created["id"]
    hil_session.delete(f"{hil_base_url}/api/v1/auth/tokens/{tid}", timeout=15)


def test_system_backup_roundtrip(hil_session, hil_base_url, backup_doc):
    """PUT the exported document back (idempotent restore)."""
    r = hil_session.put(
        f"{hil_base_url}/api/v1/system/backup",
        json=backup_doc,
        timeout=60,
    )
    if r.status_code == 400 and "unsupported_meter_field" in r.text:
        pytest.skip("backup config includes fields not allowed on this meter profile")
    assert r.status_code == 200, r.text[:500]
    body = r.json()
    assert body.get("ok") is True

    r2 = get_with_retry(hil_session, f"{hil_base_url}/api/v1/system/backup", timeout=30)
    assert r2.status_code == 200
    doc2 = r2.json()
    assert doc2["backupSchemaVersion"] == 1
    assert doc2["wifi"]["ssid"] == backup_doc["wifi"]["ssid"]
