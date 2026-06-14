#!/usr/bin/env python3
"""Generate profile_test_matrix.json for web, native, HIL, and CI from meter_pack_manifest.json."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "firmware" / "metering" / "meter_pack_manifest.json"
OUT_FIRMWARE = ROOT / "firmware" / "test" / "golden" / "profile_test_matrix.json"
OUT_WEB = ROOT / "web" / "test" / "fixtures" / "profile-test-matrix.json"
OUT_NATIVE_INC = ROOT / "firmware" / "test" / "native" / "profile_matrix_cases.inc"
OUT_CATALOG = ROOT / "web" / "public" / "firmware-catalog.json"
OUT_BUNDLED_CATALOG = ROOT / "web" / "src" / "data" / "bundledFirmwareCatalog.ts"

BUNDLED_CATALOG_TS = """\
import raw from "../../public/firmware-catalog.json";
import type { FirmwareCatalog } from "../utils/productProfile";

/** Embedded at build time — device firmware does not serve /firmware-catalog.json. */
export const bundledFirmwareCatalog: FirmwareCatalog = raw as FirmwareCatalog;
"""

# Mirrors balansun_product_caps_logic.cpp role masks (API-facing flags only).
ROLE_CAPS = {
    "meter_gateway": {
        "surplus_regulation": False,
        "triac": False,
        "multi_action": False,
        "source_test_inject": False,
    },
    "meter_router": {
        "surplus_regulation": True,
        "triac": True,
        "multi_action": False,
        "source_test_inject": False,
    },
    "full_router": {
        "surplus_regulation": True,
        "triac": True,
        "multi_action": True,
        "source_test_inject": True,
    },
}

HIL_SUITE = {
    "meter_gateway": "caps",
    "meter_router": "router_api",
    "full_router": "inject_regulation",
}

FULL_ENVS = ("wroom32", "esp32s3", "hil")

PACK_LABELS: dict[str, tuple[str, str]] = {
    "jsy_mk194": ("JSY-MK-194", "JSY-MK-194"),
    "jsy_mk333": ("JSY-MK-333", "JSY-MK-333"),
    "analog": ("Sonde U/I analogique", "Analog U/I probe"),
    "linky": ("Linky TIC", "Linky TIC"),
    "enphase": ("Enphase Envoy", "Enphase Envoy"),
    "shelly_em": ("Shelly EM", "Shelly EM"),
    "shelly_pro": ("Shelly Pro EM", "Shelly Pro EM"),
    "smartg": ("Smart Gateway", "Smart Gateway"),
    "homew": ("HomeWizard", "HomeWizard"),
    "pmqtt": ("Compteur MQTT", "MQTT meter"),
    "balansun_peer": ("Peer Balansun", "Balansun peer"),
}


def load_manifest() -> dict:
    with MANIFEST.open(encoding="utf-8") as f:
        return json.load(f)


def variant_row(
    *,
    pio_env: str,
    pack_id: str,
    role: str,
    product_profile: str,
    wire: str,
    board_profile: str,
) -> dict:
    caps = dict(ROLE_CAPS[role])
    row = {
        "pio_env": pio_env,
        "pack_id": pack_id,
        "role": role,
        "product_profile": product_profile,
        "meters": [wire] if wire else [],
        "board_profile": board_profile,
        "caps": caps,
        "hil_suite": HIL_SUITE[role],
        "ui": {
            "single_meter": True,
            "show_measurement_source": False,
            "show_advanced_meter_sources": _show_advanced_meter_sources([wire] if wire else []),
            "show_follow_triac_pwm_option": caps["surplus_regulation"],
        },
    }
    return row


def _show_advanced_meter_sources(meters: list[str]) -> bool:
    if len(meters) == 1:
        return False
    if "Pmqtt" in meters or "JsyMk333" in meters:
        return True
    return "Linky" not in meters


def _pack_labels(pack_id: str) -> tuple[str, str]:
    return PACK_LABELS.get(pack_id, (pack_id, pack_id))


def build_firmware_catalog(manifest: dict, variants: list[dict]) -> dict:
    families = [
        {"id": "meter", "label_fr": "Compteur", "label_en": "Meter gateway"},
        {"id": "router", "label_fr": "Routeur", "label_en": "Router"},
        {"id": "full", "label_fr": "Routeur complet", "label_en": "Full router"},
    ]
    entries: list[dict] = []

    for p in manifest["packs"]:
        pid = p["id"]
        fr, en = _pack_labels(pid)
        entries.append(
            {
                "family": "meter",
                "declination_id": pid,
                "label_fr": fr,
                "label_en": en,
                "pio_env": f"{pid}_meter",
                "product_profile": f"{pid}_meter",
            }
        )
        entries.append(
            {
                "family": "router",
                "declination_id": pid,
                "label_fr": fr,
                "label_en": en,
                "pio_env": f"{pid}_router",
                "product_profile": f"{pid}_router",
            }
        )

    for env in FULL_ENVS:
        label = {
            "wroom32": ("ESP32-WROOM32 (tous compteurs)", "ESP32-WROOM32 (all meters)"),
            "esp32s3": ("ESP32-S3 (tous compteurs)", "ESP32-S3 (all meters)"),
            "hil": ("Labo HIL (tous compteurs)", "HIL lab (all meters)"),
        }[env]
        entries.append(
            {
                "family": "full",
                "declination_id": env,
                "label_fr": label[0],
                "label_en": label[1],
                "pio_env": env,
                "product_profile": "full_router",
            }
        )

    return {"version": 1, "families": families, "entries": entries}


def build_matrix(manifest: dict) -> dict:
    variants: list[dict] = []
    all_wires = [p["wire"] for p in manifest["packs"]]

    for p in manifest["packs"]:
        pid = p["id"]
        wire = p["wire"]
        variants.append(
            variant_row(
                pio_env=f"{pid}_router",
                pack_id=pid,
                role="meter_router",
                product_profile=f"{pid}_router",
                wire=wire,
                board_profile="generic",
            )
        )
        variants.append(
            variant_row(
                pio_env=f"{pid}_meter",
                pack_id=pid,
                role="meter_gateway",
                product_profile=f"{pid}_meter",
                wire=wire,
                board_profile="generic",
            )
        )

    jsy = next(p for p in manifest["packs"] if p["id"] == "jsy_mk194")
    for pio_env, role, profile_suffix in (
        ("esp32_32u_jsy_router", "meter_router", "router"),
        ("esp32_32u_jsy_meter", "meter_gateway", "meter"),
    ):
        variants.append(
            variant_row(
                pio_env=pio_env,
                pack_id="jsy_mk194",
                role=role,
                product_profile=f"jsy_mk194_{profile_suffix}",
                wire=jsy["wire"],
                board_profile="esp32_32u",
            )
        )

    for env in FULL_ENVS:
        variants.append(
            {
                "pio_env": env,
                "pack_id": "full",
                "role": "full_router",
                "product_profile": "full_router",
                "meters": all_wires,
                "board_profile": "generic",
                "caps": dict(ROLE_CAPS["full_router"]),
                "hil_suite": HIL_SUITE["full_router"],
                "ui": {
                    "single_meter": False,
                    "show_measurement_source": True,
                    "show_advanced_meter_sources": True,
                    "show_follow_triac_pwm_option": True,
                },
            }
        )

    meter_pack_envs = sorted(
        {
            v["pio_env"]
            for v in variants
            if v["role"] in ("meter_gateway", "meter_router")
            and v["pio_env"] not in FULL_ENVS
        }
    )
    full_envs = list(FULL_ENVS)
    all_variant_envs = sorted(set(meter_pack_envs + full_envs))

    router_envs = sorted(v["pio_env"] for v in variants if v["role"] == "meter_router")
    router_envs.append("hil")

    return {
        "version": 1,
        "variants": variants,
        "router_envs": router_envs,
        "meter_pack_envs": meter_pack_envs,
        "full_envs": full_envs,
        "all_variant_envs": all_variant_envs,
        "firmware_catalog": build_firmware_catalog(manifest, variants),
    }


def render_native_inc(matrix: dict) -> str:
    lines = [
        "/* AUTO-GENERATED by scripts/generate_profile_test_matrix.py — do not edit. */",
        "struct ProfileMatrixCase {",
        "  const char *role;",
        "  const char *product_profile;",
        "  bool surplus_regulation;",
        "  bool triac;",
        "  bool multi_action;",
        "  bool source_test_inject;",
        "};",
        "static const ProfileMatrixCase kProfileMatrixCases[] = {",
    ]
    role_map = {
        "meter_gateway": "meter_gateway",
        "meter_router": "meter_router",
        "full_router": "full_router",
    }
    for v in matrix["variants"]:
        caps = v["caps"]
        lines.append(
            f'  {{"{role_map[v["role"]]}", "{v["product_profile"]}", '
            f'{"true" if caps["surplus_regulation"] else "false"}, '
            f'{"true" if caps["triac"] else "false"}, '
            f'{"true" if caps["multi_action"] else "false"}, '
            f'{"true" if caps["source_test_inject"] else "false"}}},'
        )
    lines.append("};")
    lines.append(f"static const size_t kProfileMatrixCaseCount = {len(matrix['variants'])};")
    lines.append("")
    return "\n".join(lines)


def write_outputs(matrix: dict) -> None:
    text = json.dumps(matrix, indent=2, ensure_ascii=False) + "\n"
    OUT_FIRMWARE.write_text(text, encoding="utf-8")
    OUT_WEB.write_text(text, encoding="utf-8")
    OUT_NATIVE_INC.write_text(render_native_inc(matrix), encoding="utf-8")
    OUT_CATALOG.parent.mkdir(parents=True, exist_ok=True)
    catalog_text = json.dumps(matrix["firmware_catalog"], indent=2, ensure_ascii=False) + "\n"
    OUT_CATALOG.write_text(catalog_text, encoding="utf-8")
    OUT_BUNDLED_CATALOG.parent.mkdir(parents=True, exist_ok=True)
    OUT_BUNDLED_CATALOG.write_text(BUNDLED_CATALOG_TS, encoding="utf-8")


def check_outputs(matrix: dict) -> int:
    rc = 0
    expected = json.dumps(matrix, indent=2, ensure_ascii=False) + "\n"
    for path in (OUT_FIRMWARE, OUT_WEB):
        if not path.is_file():
            print(f"generate_profile_test_matrix: missing {path}", file=sys.stderr)
            rc = 1
            continue
        if path.read_text(encoding="utf-8") != expected:
            print(f"generate_profile_test_matrix: drift in {path} (run without --check)", file=sys.stderr)
            rc = 1
    native_expected = render_native_inc(matrix)
    if not OUT_NATIVE_INC.is_file() or OUT_NATIVE_INC.read_text(encoding="utf-8") != native_expected:
        print(f"generate_profile_test_matrix: drift in {OUT_NATIVE_INC}", file=sys.stderr)
        rc = 1
    catalog_expected = json.dumps(matrix["firmware_catalog"], indent=2, ensure_ascii=False) + "\n"
    if not OUT_CATALOG.is_file() or OUT_CATALOG.read_text(encoding="utf-8") != catalog_expected:
        print(f"generate_profile_test_matrix: drift in {OUT_CATALOG}", file=sys.stderr)
        rc = 1
    if not OUT_BUNDLED_CATALOG.is_file() or OUT_BUNDLED_CATALOG.read_text(encoding="utf-8") != BUNDLED_CATALOG_TS:
        print(f"generate_profile_test_matrix: drift in {OUT_BUNDLED_CATALOG}", file=sys.stderr)
        rc = 1
    return rc


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="Exit 1 if generated files drift")
    args = parser.parse_args()
    matrix = build_matrix(load_manifest())
    if args.check:
        return check_outputs(matrix)
    write_outputs(matrix)
    print(f"Wrote {OUT_FIRMWARE}")
    print(f"Wrote {OUT_WEB}")
    print(f"Wrote {OUT_NATIVE_INC}")
    print(f"Wrote {OUT_CATALOG}")
    print(f"Wrote {OUT_BUNDLED_CATALOG}")
    print(f"variants: {len(matrix['variants'])}")
    print(f"all_variant_envs: {len(matrix['all_variant_envs'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
