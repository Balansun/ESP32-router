#include <gtest/gtest.h>

#include <balansun/heater_load_feedback.h>

static HeaterLoadFeedbackConfig base_cfg() {
  HeaterLoadFeedbackConfig cfg;
  cfg.enabled = true;
  cfg.source_has_second_channel = true;
  cfg.meter_valid = true;
  cfg.idle_hold_ms = 1000;
  return cfg;
}

TEST(HeaterLoadFeedback, DisabledResetsBackoff) {
  HeaterLoadFeedbackState st;
  st.backoff_active = true;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  cfg.enabled = false;
  const auto r = balansun_heater_load_feedback_logic_tick(st, cfg, 80, 0, 5000);
  EXPECT_FALSE(st.backoff_active);
  EXPECT_TRUE(r.exited_backoff);
}

TEST(HeaterLoadFeedback, EntersAfterHold) {
  HeaterLoadFeedbackState st;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  auto r = balansun_heater_load_feedback_logic_tick(st, cfg, 50, 10, 0);
  EXPECT_FALSE(r.backoff_active);
  r = balansun_heater_load_feedback_logic_tick(st, cfg, 50, 10, 500);
  EXPECT_FALSE(r.backoff_active);
  r = balansun_heater_load_feedback_logic_tick(st, cfg, 50, 10, 1001);
  EXPECT_TRUE(r.entered_backoff);
  EXPECT_TRUE(st.backoff_active);
}

TEST(HeaterLoadFeedback, ExitsOnLoad) {
  HeaterLoadFeedbackState st;
  st.backoff_active = true;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  const auto r = balansun_heater_load_feedback_logic_tick(st, cfg, 50, 150, 2000);
  EXPECT_TRUE(r.exited_backoff);
  EXPECT_FALSE(st.backoff_active);
}
