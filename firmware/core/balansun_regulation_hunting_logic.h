#pragma once

#include <cstdint>

struct RegulationHuntingConfig {
  uint8_t reversal_threshold = 8;
  uint16_t window_min = 5;
  uint16_t stable_clear_min = 10;
};

struct RegulationHuntingState {
  static constexpr int kMaxSamples = 16;
  int samples[kMaxSamples] = {};
  uint8_t count = 0;
  uint8_t head = 0;
  unsigned long last_sample_ms = 0;
  bool hunting = false;
  unsigned long hunting_since_ms = 0;
  unsigned long stable_since_ms = 0;
};

void balansun_regulation_hunting_logic_sample(RegulationHuntingState &st, const RegulationHuntingConfig &cfg,
                                         int triac_open_percent, unsigned long now_ms);

bool balansun_regulation_hunting_logic_is_hunting(const RegulationHuntingState &st);

int balansun_regulation_hunting_logic_count_reversals(const int *samples, int count);
