#include <gtest/gtest.h>

#include "balansun_source_health_logic.h"

TEST(BalansunSourceHealth, FullScoreWhenFreshAndOk) {
  SourceHealthScoreInput in;
  in.last_poll_ms_ago = 100;
  in.poll_period_ms = 400;
  in.last_poll_ok = true;
  in.error_streak = 0;
  const auto r = balansun_source_health_logic_compute(in);
  EXPECT_EQ(r.health_score, 100);
}

TEST(BalansunSourceHealth, DegradesWhenStale) {
  SourceHealthScoreInput in;
  in.last_poll_ms_ago = 5000;
  in.poll_period_ms = 400;
  in.last_poll_ok = false;
  in.error_streak = 3;
  const auto r = balansun_source_health_logic_compute(in);
  EXPECT_LT(r.health_score, 50);
}

TEST(BalansunSourceHealth, MidFreshnessAndSingleError) {
  SourceHealthScoreInput in;
  in.last_poll_ms_ago = -1;
  in.poll_period_ms = 400;
  in.last_poll_ok = false;
  in.error_streak = 1;
  const auto missing = balansun_source_health_logic_compute(in);
  EXPECT_EQ(missing.freshness_pts, 0);
  EXPECT_EQ(missing.streak_pts, 10);

  in.last_poll_ms_ago = 800;
  in.last_poll_ok = true;
  in.error_streak = 0;
  const auto mid = balansun_source_health_logic_compute(in);
  EXPECT_GT(mid.freshness_pts, 0);
  EXPECT_LT(mid.freshness_pts, 40);
  EXPECT_FALSE(balansun_source_health_logic_is_stale(mid.health_score));
}

TEST(BalansunSourceHealth, DefaultPollPeriodWhenZero) {
  SourceHealthScoreInput in;
  in.last_poll_ms_ago = 100;
  in.poll_period_ms = 0;
  in.last_poll_ok = true;
  in.error_streak = 0;
  const auto r = balansun_source_health_logic_compute(in);
  EXPECT_EQ(r.freshness_pts, 40);
  EXPECT_EQ(r.health_score, 100);
}

TEST(BalansunSourceHealth, StaleThresholdAndHighErrorStreak) {
  SourceHealthScoreInput in;
  in.last_poll_ms_ago = 2000;
  in.poll_period_ms = 400;
  in.last_poll_ok = false;
  in.error_streak = 5;
  const auto r = balansun_source_health_logic_compute(in);
  EXPECT_TRUE(balansun_source_health_logic_is_stale(r.health_score));
  EXPECT_EQ(r.streak_pts, 0);

  in.error_streak = 2;
  const auto r2 = balansun_source_health_logic_compute(in);
  EXPECT_EQ(r2.streak_pts, 0);

  in.error_streak = 3;
  const auto r3 = balansun_source_health_logic_compute(in);
  EXPECT_EQ(r3.streak_pts, 0);
  in.last_poll_ms_ago = 500;
  in.last_poll_ok = true;
  const auto aged = balansun_source_health_logic_compute(in);
  EXPECT_GE(aged.freshness_pts, 0);
  EXPECT_LT(aged.freshness_pts, 40);
}

TEST(BalansunSourceHealthPollState, BalansunPeerTwoFailedPollsIsStale) {
  BalansunSourceHealthPollState st{};
  st.active_source = SourceId::BalansunPeer;
  st.ext_peer_last_poll_ok = false;
  st.time_sync_valid = true;
  st.last_poll_ms_ago = 200;
  st.poll_period_ms = 800;
  st.error_streak = 2;
  EXPECT_TRUE(balansun_source_is_stale_from_poll_state(st));
  EXPECT_LT(balansun_source_health_score_from_poll_state(st), 50);
}

TEST(BalansunSourceHealthPollState, BalansunPeerFreshOkPollIsNotStale) {
  BalansunSourceHealthPollState st{};
  st.active_source = SourceId::BalansunPeer;
  st.ext_peer_last_poll_ok = true;
  st.time_sync_valid = true;
  st.last_poll_ms_ago = 100;
  st.poll_period_ms = 800;
  st.error_streak = 0;
  EXPECT_FALSE(balansun_source_is_stale_from_poll_state(st));
  EXPECT_GE(balansun_source_health_score_from_poll_state(st), 50);
}

TEST(BalansunSourceHealthPollState, BalansunPeerUsesPeerPollNotNtp) {
  BalansunSourceHealthPollState st{};
  st.active_source = SourceId::BalansunPeer;
  st.ext_peer_last_poll_ok = false;
  st.time_sync_valid = true;
  st.last_poll_ms_ago = 100;
  st.poll_period_ms = 800;
  st.error_streak = 0;
  EXPECT_LT(balansun_source_health_score_from_poll_state(st), 100);

  BalansunSourceHealthPollState local{};
  local.active_source = SourceId::JsyMk194;
  local.time_sync_valid = true;
  local.last_poll_ms_ago = 100;
  local.poll_period_ms = 400;
  local.error_streak = 0;
  EXPECT_EQ(balansun_source_health_score_from_poll_state(local), 100);
}

TEST(BalansunSourceHealthPollState, SingleBalansunPeerFailureNotYetStale) {
  BalansunSourceHealthPollState st{};
  st.active_source = SourceId::BalansunPeer;
  st.ext_peer_last_poll_ok = false;
  st.time_sync_valid = true;
  st.last_poll_ms_ago = 200;
  st.poll_period_ms = 800;
  st.error_streak = 1;
  EXPECT_FALSE(balansun_source_is_stale_from_poll_state(st));
}

TEST(BalansunSourceHealthPollState, LocalSourceWithoutNtpUsesTimeSyncFlag) {
  BalansunSourceHealthPollState st{};
  st.active_source = SourceId::Linky;
  st.time_sync_valid = false;
  st.last_poll_ms_ago = 100;
  st.poll_period_ms = 0;
  st.error_streak = 0;
  EXPECT_LT(balansun_source_health_score_from_poll_state(st), 100);
  SourceHealthScoreInput in;
  balansun_source_health_input_from_poll_state(st, in);
  EXPECT_EQ(in.poll_period_ms, 500U);
  EXPECT_FALSE(in.last_poll_ok);
}
