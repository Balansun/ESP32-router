#include <gtest/gtest.h>

#include "balansun_persistence_logic.h"
#include "balansun_psram_logic.h"

namespace {
constexpr uint32_t kMin = 60UL * 1000UL;  // 1 minute

PersistCoalescerConfig make_cfg(uint32_t debounce_ms, uint32_t max_defer_ms) {
  PersistCoalescerConfig c;
  c.debounce_ms = debounce_ms;
  c.max_defer_ms = max_defer_ms;
  return c;
}
}  // namespace

TEST(PersistCoalescer, CleanStateNeverFlushes) {
  PersistCoalescerState s;
  PersistCoalescerConfig cfg = make_cfg(5 * kMin, 24 * 60 * kMin);
  EXPECT_FALSE(persist_should_flush(s, cfg, 0));
  EXPECT_FALSE(persist_should_flush(s, cfg, 10 * 24 * 60 * kMin));
}

TEST(PersistCoalescer, DebounceHoldsThenFlushes) {
  PersistCoalescerState s;
  PersistCoalescerConfig cfg = make_cfg(5 * kMin, 24 * 60 * kMin);
  persist_mark_dirty(s, 1000);
  // Within debounce window: no flush.
  EXPECT_FALSE(persist_should_flush(s, cfg, 1000 + 4 * kMin));
  // After debounce window since last change: flush.
  EXPECT_TRUE(persist_should_flush(s, cfg, 1000 + 5 * kMin));
}

TEST(PersistCoalescer, ContinuousChangesResetDebounceButHitMaxDefer) {
  PersistCoalescerState s;
  PersistCoalescerConfig cfg = make_cfg(5 * kMin, 60 * kMin);
  uint32_t now = 0;
  persist_mark_dirty(s, now);
  // A change every 4 minutes keeps resetting the debounce window...
  for (int i = 0; i < 14; i++) {
    now += 4 * kMin;
    persist_mark_dirty(s, now);
    // debounce never elapses on its own here (4 < 5)
  }
  // ...but the max-defer ceiling (60 min since first dirty) forces a flush.
  now = 61 * kMin;
  EXPECT_TRUE(persist_should_flush(s, cfg, now));
}

TEST(PersistCoalescer, AfterFlushClearsAndCounts) {
  PersistCoalescerState s;
  PersistCoalescerConfig cfg = make_cfg(5 * kMin, 24 * 60 * kMin);
  persist_mark_dirty(s, 1000);
  EXPECT_TRUE(s.dirty);
  persist_after_flush(s, 1000 + 6 * kMin);
  EXPECT_FALSE(s.dirty);
  EXPECT_EQ(s.flush_count, 1u);
  EXPECT_FALSE(persist_should_flush(s, cfg, 1000 + 100 * kMin));
}

TEST(PersistCoalescer, MillisWraparoundSafe) {
  PersistCoalescerState s;
  PersistCoalescerConfig cfg = make_cfg(5 * kMin, 24 * 60 * kMin);
  const uint32_t near_max = 0xFFFFFFFFUL - kMin;  // ~1 min before wrap
  persist_mark_dirty(s, near_max);
  // 6 minutes later, millis() has wrapped past 0.
  const uint32_t after_wrap = near_max + 6 * kMin;  // wraps around
  EXPECT_TRUE(persist_should_flush(s, cfg, after_wrap));
}

TEST(PsramPolicy, OnlyLargeWhenAvailable) {
  EXPECT_FALSE(psram_should_use(4096, /*available=*/false));
  EXPECT_FALSE(psram_should_use(16, /*available=*/true));   // too small
  EXPECT_TRUE(psram_should_use(4096, /*available=*/true));
  EXPECT_TRUE(psram_should_use(kPsramMinUsefulBytes, /*available=*/true));
  EXPECT_FALSE(psram_should_use(kPsramMinUsefulBytes - 1, /*available=*/true));
}
