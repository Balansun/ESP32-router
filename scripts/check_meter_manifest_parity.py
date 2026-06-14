#!/usr/bin/env python3
"""Golden parity: meter_pack_manifest.json ↔ SourceId enum ↔ web registry."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "firmware" / "metering" / "meter_pack_manifest.json"
SOURCE_TYPES = ROOT / "firmware" / "core" / "balansun_source_types.h"
WEB_REGISTRY = ROOT / "web" / "src" / "routes" / "settings" / "sourceRegistry.generated.ts"

ALLOWED_DRIVER_BASES = {
    "MeterDriverBase",
    "LanHttpJsonMeter",
    "LanHttpCustomMeter",
    "SerialMeterDriver",
    "EventDrivenMeter",
    "TlsHttpMeter",
    "AdcMeterDriver",
}

BASE_KIND_MAP = {
    "LanHttpJsonMeter": "lanHttpJson",
    "LanHttpCustomMeter": "lanHttpCustom",
    "SerialMeterDriver": "serial",
    "AdcMeterDriver": "adc",
    "TlsHttpMeter": "tls",
    "EventDrivenMeter": "mqtt",
    "MeterDriverBase": "bench",
}


def parse_source_ids(header: str) -> set[str]:
    block = re.search(r"enum class SourceId\s*:\s*uint8_t\s*\{([^}]+)\}", header, re.S)
    if not block:
        raise RuntimeError("SourceId enum not found")
    ids = set(re.findall(r"\b([A-Z][A-Za-z0-9]+)\s*,", block.group(1)))
    ids.discard("Unknown")
    return ids


def parse_web_wire_ids(ts: str) -> set[str]:
    block = re.search(
        r"export type SourceWireId\s*=\s*(.+?)\n\nexport type ConfigField",
        ts,
        re.S,
    )
    if not block:
        raise RuntimeError("SourceWireId union not found")
    return set(re.findall(r'"([^"]+)"', block.group(1)))


def parse_web_registry_ids(ts: str) -> set[str]:
    return set(re.findall(r'\bid:\s*"([^"]+)"', ts))


def all_manifest_entries(manifest: dict) -> list[dict]:
    entries = list(manifest["packs"])
    entries.append(dict(manifest["notdef"]))
    return entries


def main() -> int:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    source_ids = parse_source_ids(SOURCE_TYPES.read_text(encoding="utf-8"))
    web_ts = WEB_REGISTRY.read_text(encoding="utf-8")
    web_wire_ids = parse_web_wire_ids(web_ts)
    web_registry_ids = parse_web_registry_ids(web_ts)

    errors: list[str] = []
    manifest_source_ids: set[str] = set()
    spa_wires: set[str] = set()

    for entry in all_manifest_entries(manifest):
        wire = entry.get("wire", "")
        source_id = entry.get("source_id", "")
        driver_class = entry.get("driver_class", "")
        driver_base = entry.get("driver_base", "")

        if not wire or not source_id:
            errors.append(f"{entry.get('id')}: missing wire or source_id")
            continue
        if wire != source_id:
            errors.append(f"{entry.get('id')}: wire {wire!r} != source_id {source_id!r}")
        manifest_source_ids.add(source_id)

        if not driver_class:
            errors.append(f"{entry.get('id')}: missing driver_class")
        if driver_base not in ALLOWED_DRIVER_BASES:
            errors.append(f"{entry.get('id')}: unknown driver_base {driver_base!r}")

        if entry.get("spa_visible", True) and entry.get("id") != "notdef":
            spa_wires.add(wire)

    if manifest_source_ids != source_ids:
        missing_enum = sorted(manifest_source_ids - source_ids)
        extra_enum = sorted(source_ids - manifest_source_ids)
        if missing_enum:
            errors.append(f"manifest entries missing from SourceId enum: {missing_enum}")
        if extra_enum:
            errors.append(f"SourceId enum entries missing from manifest: {extra_enum}")

    if spa_wires != web_wire_ids:
        errors.append(
            f"spa_visible wires {sorted(spa_wires)} != SourceWireId {sorted(web_wire_ids)}"
        )
    if spa_wires != web_registry_ids:
        errors.append(
            f"spa_visible wires {sorted(spa_wires)} != GENERATED_SOURCE_REGISTRY ids "
            f"{sorted(web_registry_ids)}"
        )

    if errors:
        print("check_meter_manifest_parity: FAIL", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        return 1

    print(
        f"check_meter_manifest_parity: OK ({len(manifest['packs'])} packs + notdef, "
        f"{len(spa_wires)} spa sources)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
