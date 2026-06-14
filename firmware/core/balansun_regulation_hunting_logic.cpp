#include "balansun_regulation_hunting_logic.h"

#include <cstdlib>

namespace {
constexpr unsigned long kSampleIntervalMs = 30000UL;
}  // namespace

int balansun_regulation_hunting_logic_count_reversals(const int *samples, int count) {
  if (count < 2) return 0;
  int reversals = 0;
  int prev_delta = 0;
  for (int i = 1; i < count; i++) {
    const int delta = samples[i] - samples[i - 1];
    if (delta == 0) continue;
    if (prev_delta != 0 && ((delta > 0) != (prev_delta > 0))) {
      reversals++;
    }
    prev_delta = delta;
  }
  return reversals;
}

void balansun_regulation_hunting_logic_sample(RegulationHuntingState &st, const RegulationHuntingConfig &cfg,
                                         int triac_open_percent, unsigned long now_ms) {
  if (st.last_sample_ms != 0 && (now_ms - st.last_sample_ms) < kSampleIntervalMs) {
    return;
  }
  st.last_sample_ms = now_ms;
  const int v = triac_open_percent < 0 ? 0 : (triac_open_percent > 100 ? 100 : triac_open_percent);
  st.samples[st.head] = v;
  st.head = (uint8_t)((st.head + 1) % RegulationHuntingState::kMaxSamples);
  if (st.count < RegulationHuntingState::kMaxSamples) {
    st.count++;
  }

  const int start = st.count < RegulationHuntingState::kMaxSamples
                        ? 0
                        : (int)st.head;
  int ordered_buf[RegulationHuntingState::kMaxSamples];
  for (int i = 0; i < st.count; i++) {
    ordered_buf[i] = st.samples[(start + i) % RegulationHuntingState::kMaxSamples];
  }

  const int reversals =
      balansun_regulation_hunting_logic_count_reversals(ordered_buf, st.count);
  const unsigned long window_ms = (unsigned long)cfg.window_min * 60000UL;
  const unsigned long clear_ms = (unsigned long)cfg.stable_clear_min * 60000UL;

  if (reversals >= cfg.reversal_threshold) {
    if (!st.hunting) {
      st.hunting = true;
      st.hunting_since_ms = now_ms;
    }
    st.stable_since_ms = 0;
  } else if (st.hunting) {
    if (st.stable_since_ms == 0) {
      st.stable_since_ms = now_ms;
    } else if ((now_ms - st.stable_since_ms) >= clear_ms) {
      st.hunting = false;
      st.hunting_since_ms = 0;
      st.stable_since_ms = 0;
    }
  }
  (void)window_ms;
}

bool balansun_regulation_hunting_logic_is_hunting(const RegulationHuntingState &st) { return st.hunting; }
