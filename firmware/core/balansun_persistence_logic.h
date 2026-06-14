#pragma once

#include <cstdint>

/*
 * balansun_persistence_logic.h — Pure (Arduino-free) write-coalescing decision logic.
 *
 * The firmware persists configuration as one NVS blob (Arduino EEPROM is NVS-backed:
 * commit() rewrites the whole blob). Rewriting on every change wears the small 20 KB
 * nvs partition. This logic implements a tiered/debounced policy so high-frequency
 * (deferred) changes coalesce into roughly one write per debounce window — and at most
 * one per max-defer window — while critical changes flush immediately (handled by the
 * Arduino glue, not here).
 *
 * Time is supplied by the caller (millis()) so the policy is fully unit-testable.
 * All deltas use signed 32-bit subtraction so millis() wraparound (~49 days) is safe.
 */

enum class PersistTier : uint8_t {
  /** Rare / user-initiated / security-relevant: flush now (Wi-Fi, tokens, source, reset). */
  Critical = 0,
  /** High-frequency / derived: coalesce and flush later (MQTT tweaks, caps, tuning). */
  Deferred = 1,
};

struct PersistCoalescerState {
  bool dirty = false;
  uint32_t dirty_since_ms = 0;  // when the pending batch first became dirty
  uint32_t last_change_ms = 0;  // most recent deferred change
  uint32_t last_flush_ms = 0;   // most recent flush (debug/telemetry)
  uint32_t flush_count = 0;     // total flushes observed (telemetry)
};

struct PersistCoalescerConfig {
  /** Flush this long after the last deferred change settles. */
  uint32_t debounce_ms = 5UL * 60UL * 1000UL;  // 5 minutes
  /** Hard ceiling so a continuously-changing value still flushes about once a day. */
  uint32_t max_defer_ms = 24UL * 60UL * 60UL * 1000UL;  // ~24 hours
};

/** Record a deferred change. First change of a batch starts the max-defer clock. */
inline void persist_mark_dirty(PersistCoalescerState &s, uint32_t now_ms) {
  if (!s.dirty) {
    s.dirty = true;
    s.dirty_since_ms = now_ms;
  }
  s.last_change_ms = now_ms;
}

/** Clear the pending batch after a flush actually happened (critical or deferred). */
inline void persist_after_flush(PersistCoalescerState &s, uint32_t now_ms) {
  s.dirty = false;
  s.last_flush_ms = now_ms;
  s.flush_count++;
}

/**
 * Decide whether a deferred batch should be flushed now.
 * True when dirty AND (debounce window since last change elapsed
 * OR the max-defer ceiling since the batch started elapsed).
 */
inline bool persist_should_flush(const PersistCoalescerState &s, const PersistCoalescerConfig &cfg,
                                 uint32_t now_ms) {
  if (!s.dirty) return false;
  const int32_t since_change = static_cast<int32_t>(now_ms - s.last_change_ms);
  const int32_t since_dirty = static_cast<int32_t>(now_ms - s.dirty_since_ms);
  if (since_change >= static_cast<int32_t>(cfg.debounce_ms)) return true;
  if (since_dirty >= static_cast<int32_t>(cfg.max_defer_ms)) return true;
  return false;
}
