#include <gtest/gtest.h>

#include "balansun_heater_load_feedback_logic.h"

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

TEST(HeaterLoadFeedback, ExitsWhenTriacLow) {
  HeaterLoadFeedbackState st;
  st.backoff_active = true;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  const auto r = balansun_heater_load_feedback_logic_tick(st, cfg, 5, 10, 2000);
  EXPECT_TRUE(r.exited_backoff);
  EXPECT_FALSE(st.backoff_active);
}

TEST(HeaterLoadFeedback, NoEnterWhenTriacLow) {
  HeaterLoadFeedbackState st;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  balansun_heater_load_feedback_logic_tick(st, cfg, 10, 0, 0);
  balansun_heater_load_feedback_logic_tick(st, cfg, 10, 0, 5000);
  EXPECT_FALSE(st.backoff_active);
}

TEST(HeaterLoadFeedback, InvalidMeterClearsSuspect) {
  HeaterLoadFeedbackState st;
  st.suspect_active = true;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  cfg.meter_valid = false;
  balansun_heater_load_feedback_logic_tick(st, cfg, 50, 0, 0);
  EXPECT_FALSE(st.suspect_active);
  cfg.meter_valid = true;
  cfg.source_has_second_channel = false;
  balansun_heater_load_feedback_logic_tick(st, cfg, 50, 0, 0);
  EXPECT_FALSE(st.suspect_active);
}

TEST(HeaterLoadFeedback, SuspectHoldInProgress) {
  HeaterLoadFeedbackState st;
  st.suspect_active = true;
  st.suspect_since_ms = 0;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  const auto r = balansun_heater_load_feedback_logic_tick(st, cfg, 50, 10, 500);
  EXPECT_FALSE(r.entered_backoff);
  EXPECT_FALSE(st.backoff_active);
}

TEST(HeaterLoadFeedback, LoadPresentClearsSuspect) {
  HeaterLoadFeedbackState st;
  st.suspect_active = true;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  balansun_heater_load_feedback_logic_tick(st, cfg, 50, 200, 0);
  EXPECT_FALSE(st.suspect_active);
}

TEST(HeaterLoadFeedback, BackoffPersistsUntilRelease) {
  HeaterLoadFeedbackState st;
  st.backoff_active = true;
  HeaterLoadFeedbackConfig cfg = base_cfg();
  const auto r = balansun_heater_load_feedback_logic_tick(st, cfg, 50, 10, 2000);
  EXPECT_TRUE(r.backoff_active);
  EXPECT_TRUE(st.backoff_active);
  EXPECT_FALSE(r.exited_backoff);
}
