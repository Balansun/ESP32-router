#!/usr/bin/env python3
"""Sign a fleet export bundle JSON for POST /api/v1/fleet/import."""
from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path, help="Unsigned JSON bundle (no signature field)")
    parser.add_argument("--key", required=True, help="Fleet trust key (same as device fleet_trust_key)")
    parser.add_argument("-o", "--output", type=Path, help="Write signed bundle (default: stdout)")
    args = parser.parse_args()
    data = json.loads(args.bundle.read_text(encoding="utf-8"))
    if "signature" in data:
        print("error: input already has signature", file=sys.stderr)
        return 1
    unsigned = json.dumps(data, separators=(",", ":"), sort_keys=True)
    sig = hmac.new(args.key.encode("utf-8"), unsigned.encode("utf-8"), hashlib.sha256).hexdigest()
    data["signature"] = sig
    out = json.dumps(data, indent=2) + "\n"
    if args.output:
        args.output.write_text(out, encoding="utf-8")
    else:
        sys.stdout.write(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
