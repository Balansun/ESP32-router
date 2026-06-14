#pragma once

#include <cstddef>
#include <cstdlib>

#include "balansun_psram_logic.h"

/*
 * balansun_psram.h — Opportunistic PSRAM cache allocator.
 *
 * Durable data always lives in NVS; PSRAM only holds volatile caches (history ring,
 * large JSON scratch). When PSRAM is absent (e.g. plain WROOM32) everything transparently
 * falls back to the internal heap, so behavior is unchanged on those boards.
 */

/** True when usable PSRAM is present on this board at runtime. */
bool balansun_psram_available();

/** Total / free PSRAM in bytes (0 when absent). */
size_t balansun_psram_size();
size_t balansun_psram_free();

/** One-line boot log describing PSRAM availability. */
void balansun_psram_log_boot();

/** Allocate from PSRAM when available and large enough, else from the internal heap. */
void *balansun_cache_malloc(size_t bytes);

/** Free a pointer returned by balansun_cache_malloc (works for both PSRAM and heap). */
void balansun_cache_free(void *p);

/**
 * STL allocator routing large container storage to PSRAM when present.
 * Used for the daily history ring cache so it does not consume scarce internal RAM.
 */
template <typename T>
struct PsramAllocator {
  using value_type = T;

  PsramAllocator() noexcept = default;
  template <typename U>
  PsramAllocator(const PsramAllocator<U> &) noexcept {}

  T *allocate(std::size_t n) {
    void *p = balansun_cache_malloc(n * sizeof(T));
    if (!p) abort();  // mirror std::vector OOM behavior deterministically on embedded
    return static_cast<T *>(p);
  }
  void deallocate(T *p, std::size_t) noexcept { balansun_cache_free(p); }
};

template <typename A, typename B>
bool operator==(const PsramAllocator<A> &, const PsramAllocator<B> &) noexcept {
  return true;
}
template <typename A, typename B>
bool operator!=(const PsramAllocator<A> &, const PsramAllocator<B> &) noexcept {
  return false;
}
