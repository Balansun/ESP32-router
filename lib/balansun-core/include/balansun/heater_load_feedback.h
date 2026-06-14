#pragma once

#include <cstdint>

struct HeaterLoadFeedbackConfig {
  bool enabled = false;
  bool source_has_second_channel = false;
  bool meter_valid = false;
  int min_triac_open_percent = 25;
  int max_idle_second_net_w = 50;
  int min_load_second_net_w = 100;
  int release_triac_open_percent = 10;
  uint32_t idle_hold_ms = 45000;
};

struct HeaterLoadFeedbackState {
  bool backoff_active = false;
  bool suspect_active = false;
  unsigned long suspect_since_ms = 0;
};

struct HeaterLoadFeedbackResult {
  bool backoff_active = false;
  bool entered_backoff = false;
  bool exited_backoff = false;
};

HeaterLoadFeedbackResult balansun_heater_load_feedback_logic_tick(HeaterLoadFeedbackState &st,
                                                                    const HeaterLoadFeedbackConfig &cfg,
                                                                    int triac_open_percent, int second_net_w,
                                                                    unsigned long now_ms);
