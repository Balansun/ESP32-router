#!/usr/bin/env python3
"""Compare firmware.bin sizes between PlatformIO environments (meter packs)."""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def firmware_size_bytes(env: str, root: Path, pio: str) -> int:
    subprocess.run(
        [pio, "run", "-e", env],
        cwd=root,
        check=True,
        capture_output=True,
    )
    bin_path = root / ".pio" / "build" / env / "firmware.bin"
    if not bin_path.is_file():
        raise FileNotFoundError(bin_path)
    return bin_path.stat().st_size


def load_router_envs(root: Path) -> list[str]:
    matrix_path = root / "firmware" / "test" / "golden" / "profile_test_matrix.json"
    if matrix_path.is_file():
        import json

        data = json.loads(matrix_path.read_text(encoding="utf-8"))
        return list(data.get("router_envs", []))
    return ["jsy_mk194_router", "hil"]


def default_pio(root: Path) -> str:
    venv_pio = root / ".venv" / "bin" / "pio"
    if venv_pio.is_file():
        return str(venv_pio)
    return "pio"


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=root)
    parser.add_argument("--baseline", default="wroom32")
    parser.add_argument("--pio", default=default_pio(root))
    parser.add_argument(
        "--profile",
        action="append",
        default=[],
        help="Pack env that must be smaller than baseline (repeatable)",
    )
    parser.add_argument(
        "--routers-only",
        action="store_true",
        help="Size-check all router envs from profile_test_matrix.json",
    )
    parser.add_argument("--min-delta-bytes", type=int, default=20480)
    args = parser.parse_args()

    if args.routers_only:
        profiles = load_router_envs(args.root)
    else:
        profiles = args.profile or ["jsy_mk194_router", "jsy_mk194_meter"]
    base = firmware_size_bytes(args.baseline, args.root, args.pio)
    print(f"{args.baseline}: {base} bytes")

    rc = 0
    for prof in profiles:
        size = firmware_size_bytes(prof, args.root, args.pio)
        delta = base - size
        print(f"{prof}: {size} bytes (delta vs baseline: {delta} bytes)")
        if prof == "hil":
            # HIL adds RemoteDebug; same full-router meter set as wroom32 — build check only.
            continue
        if delta < args.min_delta_bytes:
            print(
                f"WARN: expected at least {args.min_delta_bytes} bytes smaller for {prof}",
                file=sys.stderr,
            )
            rc = 1

    router = firmware_size_bytes("jsy_mk194_router", args.root, args.pio)
    meter = firmware_size_bytes("jsy_mk194_meter", args.root, args.pio)
    if meter >= router:
        print(
            f"WARN: jsy_mk194_meter ({meter}) should be smaller than jsy_mk194_router ({router})",
            file=sys.stderr,
        )
        rc = 1
    else:
        print(f"jsy_mk194_meter smaller than router by {router - meter} bytes")

    return rc


if __name__ == "__main__":
    raise SystemExit(main())
