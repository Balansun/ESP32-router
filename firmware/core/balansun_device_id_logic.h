#pragma once

/*
 * balansun_device_id_logic.h — Stable device_uid formatting (host-testable, no Arduino I/O).
 */

#include <cstddef>
#include <cstdint>

/** Format 48-bit factory MAC as 12 lowercase hex digits. Returns false if out_len < 13. */
bool balansun_device_uid_format(uint64_t efuse_mac48, char *out, size_t out_len);

/** True when name is empty or the factory default `balansun`. */
bool balansun_mqtt_device_name_is_factory_default(const char *name);
