"""HIL environment variables (Balansun)."""
from __future__ import annotations

import os


def hil_env(*keys: str, default: str = "") -> str:
    for key in keys:
        v = os.environ.get(key)
        if v:
            return v
    return default
