#include <balansun/heater_load_feedback.h>

#include <cstdlib>

namespace {

int abs_int(int v) { return v < 0 ? -v : v; }

void reset_state(HeaterLoadFeedbackState &st) {
  st.backoff_active = false;
  st.suspect_active = false;
  st.suspect_since_ms = 0;
}

}  // namespace

HeaterLoadFeedbackResult balansun_heater_load_feedback_logic_tick(HeaterLoadFeedbackState &st,
                                                                    const HeaterLoadFeedbackConfig &cfg,
                                                                    int triac_open_percent, int second_net_w,
                                                                    unsigned long now_ms) {
  HeaterLoadFeedbackResult out;
  if (!cfg.enabled || !cfg.source_has_second_channel || !cfg.meter_valid) {
    if (st.backoff_active) out.exited_backoff = true;
    reset_state(st);
    return out;
  }

  if (st.backoff_active) {
    out.backoff_active = true;
    if (second_net_w >= cfg.min_load_second_net_w || triac_open_percent < cfg.release_triac_open_percent) {
      reset_state(st);
      out.backoff_active = false;
      out.exited_backoff = true;
    }
    return out;
  }

  const bool triac_commanding = triac_open_percent >= cfg.min_triac_open_percent;
  const bool load_idle = abs_int(second_net_w) < cfg.max_idle_second_net_w;
  if (triac_commanding && load_idle) {
    if (!st.suspect_active) {
      st.suspect_active = true;
      st.suspect_since_ms = now_ms;
    } else if (now_ms - st.suspect_since_ms >= cfg.idle_hold_ms) {
      st.backoff_active = true;
      st.suspect_active = false;
      st.suspect_since_ms = 0;
      out.backoff_active = true;
      out.entered_backoff = true;
    }
  } else {
    st.suspect_active = false;
    st.suspect_since_ms = 0;
  }
  return out;
}
