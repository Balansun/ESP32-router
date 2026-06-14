#include "balansun_source_health_logic.h"

#include <algorithm>

namespace {
int clamp_score(int v) { return std::max(0, std::min(100, v)); }
}  // namespace

SourceHealthScoreResult balansun_source_health_logic_compute(const SourceHealthScoreInput &in) {
  SourceHealthScoreResult out;
  const uint32_t period = in.poll_period_ms > 0 ? in.poll_period_ms : 500U;
  const uint32_t stale_ms = period * 3U;

  if (in.last_poll_ms_ago < 0) {
    out.freshness_pts = 0;
  } else if ((uint32_t)in.last_poll_ms_ago <= period) {
    out.freshness_pts = 40;
  } else if ((uint32_t)in.last_poll_ms_ago <= stale_ms) {
    const int span = (int)stale_ms - (int)period;
    const int age = in.last_poll_ms_ago - (int)period;
    out.freshness_pts = std::max(0, (40 * (span - age)) / span);
  } else {
    out.freshness_pts = 0;
  }

  out.poll_ok_pts = in.last_poll_ok ? 40 : 0;

  if (in.error_streak == 0) {
    out.streak_pts = 20;
  } else if (in.error_streak == 1) {
    out.streak_pts = 10;
  } else {
    out.streak_pts = 0;
  }

  out.health_score =
      clamp_score(out.freshness_pts + out.poll_ok_pts + out.streak_pts);
  return out;
}

bool balansun_source_health_logic_is_stale(int health_score) { return health_score < 50; }

void balansun_source_health_input_from_poll_state(const BalansunSourceHealthPollState &state,
                                               SourceHealthScoreInput &out) {
  out.last_poll_ms_ago = state.last_poll_ms_ago;
  out.poll_period_ms = state.poll_period_ms > 0 ? state.poll_period_ms : 500U;
  if (state.active_source == SourceId::BalansunPeer) {
    out.last_poll_ok = state.ext_peer_last_poll_ok;
  } else {
    out.last_poll_ok = state.time_sync_valid;
  }
  out.error_streak = state.error_streak;
}

int balansun_source_health_score_from_poll_state(const BalansunSourceHealthPollState &state) {
  SourceHealthScoreInput in;
  balansun_source_health_input_from_poll_state(state, in);
  return balansun_source_health_logic_compute(in).health_score;
}

bool balansun_source_is_stale_from_poll_state(const BalansunSourceHealthPollState &state) {
  return balansun_source_health_logic_is_stale(balansun_source_health_score_from_poll_state(state));
}
