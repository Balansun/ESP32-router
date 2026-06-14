"""Pre-build: RemoteDebug WebSockets SHA-1 helper for Arduino-ESP32 3.x / mbed TLS 3 (no hwcrypto/sha.h)."""
from pathlib import Path

Import("env")


def patch_one(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="replace")
    if '#include "mbedtls/build_info.h"' in text:
        text = text.replace(
            '#include "mbedtls/build_info.h"',
            "#include <esp_arduino_version.h>",
        ).replace(
            "MBEDTLS_VERSION_MAJOR >= 3",
            "ESP_ARDUINO_VERSION_MAJOR >= 3",
        )
        path.write_text(text, encoding="utf-8")
        print(f"Repaired RemoteDebug WebSockets (mbedtls/build_info.h): {path}")
        return
    if "#include <hwcrypto/sha.h>" not in text:
        return

    block_old = """#ifdef ESP8266
#include <Hash.h>
#elif defined(ESP32)
#include <hwcrypto/sha.h>
#else"""

    block_new = """#ifdef ESP8266
#include <Hash.h>
#elif defined(ESP32)
#include "mbedtls/sha1.h"
#include <esp_arduino_version.h>
#else"""

    if block_old not in text:
        raise SystemExit(f"patch_remote_debug_ws: unexpected layout in {path}")

    body_old = """#elif defined(ESP32)
    String data = clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    esp_sha(SHA1, (unsigned char*)data.c_str(), data.length(), &sha1HashBin[0]);
#else"""

    body_new = """#elif defined(ESP32)
    {
        String data = clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        mbedtls_sha1((const unsigned char *)data.c_str(), data.length(), sha1HashBin);
#else
        mbedtls_sha1_ret((const unsigned char *)data.c_str(), data.length(), sha1HashBin);
#endif
    }
#else"""

    if body_old not in text:
        raise SystemExit(f"patch_remote_debug_ws: missing ESP32 acceptKey blob in {path}")

    path.write_text(
        text.replace(block_old, block_new, 1).replace(body_old, body_new, 1),
        encoding="utf-8",
    )
    print(f"Patched RemoteDebug WebSockets ESP32 SHA-1 helper: {path}")


pioenv = env.get("PIOENV", "")
root = Path(env["PROJECT_DIR"])
ws_files = sorted(root.glob(f".pio/libdeps/{pioenv}/RemoteDebug/src/utility/WebSockets.cpp"))
if not ws_files:
    print(f"patch_remote_debug_ws: skipped (no RemoteDebug for env {pioenv})")
else:
    for f in ws_files:
        patch_one(f)
