#pragma once

#include <cstdint>

#include "balansun_source_types.h"

struct SourceHealthScoreInput {
  int last_poll_ms_ago = -1;
  uint32_t poll_period_ms = 500;
  bool last_poll_ok = false;
  uint8_t error_streak = 0;
};

struct SourceHealthScoreResult {
  int health_score = 0;
  int freshness_pts = 0;
  int poll_ok_pts = 0;
  int streak_pts = 0;
};

SourceHealthScoreResult balansun_source_health_logic_compute(const SourceHealthScoreInput &in);

/** Matches MQTT `source_stale` when health_score is below 50. */
bool balansun_source_health_logic_is_stale(int health_score);

/** Runtime poll snapshot used to build health inputs (diagnostics + regulation). */
struct BalansunSourceHealthPollState {
  SourceId active_source = SourceId::NotDef;
  bool ext_peer_last_poll_ok = false;
  bool time_sync_valid = false;
  int last_poll_ms_ago = -1;
  uint32_t poll_period_ms = 500;
  uint8_t error_streak = 0;
};

void balansun_source_health_input_from_poll_state(const BalansunSourceHealthPollState &state,
                                               SourceHealthScoreInput &out);

int balansun_source_health_score_from_poll_state(const BalansunSourceHealthPollState &state);

bool balansun_source_is_stale_from_poll_state(const BalansunSourceHealthPollState &state);
