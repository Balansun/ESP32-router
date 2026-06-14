#pragma once

#include <cstddef>

/*
 * balansun_psram_logic.h — Pure (Arduino-free) PSRAM allocation policy.
 *
 * PSRAM (when a module has it: WROVER, S3-with-PSRAM) is slower and has per-allocation
 * overhead, so only larger buffers are worth routing there; tiny allocations stay in
 * fast internal RAM. This keeps the decision testable on host.
 */

/** Default lower bound (bytes) below which PSRAM is not worth using. */
constexpr size_t kPsramMinUsefulBytes = 256;

/** True when an allocation of @p bytes should prefer PSRAM (only if available and large enough). */
inline bool psram_should_use(size_t bytes, bool psram_available,
                             size_t min_psram_bytes = kPsramMinUsefulBytes) {
  return psram_available && bytes >= min_psram_bytes;
}
