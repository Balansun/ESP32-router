#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>

#include "actions_logic.h"
#include "balansun_regulation_logic.h"
#include "balansun_regulation_modes.h"
#include "balansun_regulation_state.h"

TEST(BalansunRegulationLogic, InactiveForcesTriacOff) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeInactif;
  in.schedule_type = 4;
  const ActionRegulationOutput out = SurplusRegulator::compute_action(in);
  EXPECT_GE(out.triac_delay_percent_f, 99.0f);
  EXPECT_FALSE(out.active);
}

TEST(BalansunRegulationLogic, IntegralOpensOnExport) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 200;
  in.max_open_percent = 80;
  in.ki = 40;
  in.net_power_w = -800.0f;
  in.itmode_ok = true;
  ActionRegulationOutput out = SurplusRegulator::compute_action(in);
  for (int i = 0; i < 50; i++) {
    out = SurplusRegulator::compute_action(in);
  }
  EXPECT_LT(out.triac_delay_percent_f, 50.0f);
  EXPECT_TRUE(out.active);
}

TEST(BalansunRegulationLogic, ExportStepFixtureOpensTriac) {
  std::ifstream fixture("firmware/test/fixtures/regulation/export_step.json");
  ASSERT_TRUE(fixture.good());
  std::stringstream ss;
  ss << fixture.rdbuf();
  const std::string blob = ss.str();
  const size_t impPos = blob.find("\"active_import_w\"");
  const size_t expPos = blob.find("\"active_export_w\"");
  ASSERT_NE(impPos, std::string::npos);
  ASSERT_NE(expPos, std::string::npos);
  const int importW = std::atoi(blob.c_str() + blob.find(':', impPos) + 1);
  const int exportW = std::atoi(blob.c_str() + blob.find(':', expPos) + 1);
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 200;
  in.max_open_percent = 80;
  in.ki = 40;
  in.net_power_w = static_cast<float>(importW - exportW);
  in.itmode_ok = true;
  ActionRegulationOutput out;
  for (int i = 0; i < 60; i++) {
    out = SurplusRegulator::compute_action(in);
  }
  EXPECT_LT(out.triac_delay_percent_f, 50.0f);
  EXPECT_TRUE(out.active);
}

TEST(BalansunRegulationLogic, ScheduleTypeOneOrLessForcesOff) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 1;
  const auto out = SurplusRegulator::compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.active);
}

TEST(BalansunRegulationLogic, GainScalesKiBelowThreshold) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 500;
  in.max_open_percent = 80;
  in.regulation_gain = 50;
  in.ki = 20;
  in.net_power_w = 100.0f;
  (void)SurplusRegulator::compute_action(in);
}

TEST(BalansunRegulationLogic, TriacOverrideMaxAbove100Clamps) {
  balansun_regulation_state_init();
  TriacRegulationInput in;
  in.override_state = kActionOverrideOn;
  in.triac_max_percent = 150;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 0.0f);
  EXPECT_TRUE(out.triac_on);
}

TEST(BalansunRegulationLogic, TriacFixedZeroPercentOff) {
  balansun_regulation_state_init();
  TriacRegulationInput in;
  in.override_state = kActionOverrideTriacFixed;
  in.override_triac_percent = 0;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.triac_on);
}

TEST(BalansunRegulationLogic, InactiveActifClosesTriac) {
  balansun_regulation_state_init();
  TriacRegulationInput in;
  in.actif = kModeInactif;
  in.schedule_type_triac = 4;
  in.net_power_w = -5000;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.triac_on);
}

TEST(BalansunRegulationLogic, ItmodeNotOkDoesNotForceClosedOnPrimary) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 0;
  in.max_open_percent = 80;
  in.ki = 40;
  in.net_power_w = -2000.0f;
  in.itmode_ok = false;
  for (int i = 0; i < 30; i++) {
    (void)SurplusRegulator::compute_action(in);
  }
  const auto out = SurplusRegulator::compute_action(in);
  EXPECT_LT(out.triac_delay_percent_f, 100.0f);
}

TEST(BalansunRegulationLogic, IntegralCapsAtMaxOpenPercent) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 0;
  in.max_open_percent = 10;
  in.ki = 80;
  in.net_power_w = -8000.0f;
  ActionRegulationOutput out;
  for (int i = 0; i < 80; i++) {
    out = SurplusRegulator::compute_action(in);
  }
  EXPECT_GE(out.triac_delay_percent_f, 90.0f);
}

TEST(BalansunRegulationLogic, ScheduleTypeOnFixedDuty) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 2;
  in.max_open_percent = 35;
  const auto out = SurplusRegulator::compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 65.0f);
}

TEST(BalansunRegulationLogic, PidResetsIntegratorWhenKiZero) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 0;
  in.max_open_percent = 80;
  in.net_power_w = -500.0f;
  in.pid_enabled = true;
  in.expert_regulation_mode = 1;
  in.kp = 50;
  in.ki = 0;
  in.kd = 10;
  (void)SurplusRegulator::compute_action(in);
}

TEST(BalansunRegulationLogic, TriacUsesLoopGainWhenKiUnset) {
  balansun_regulation_state_init();
  TriacRegulationInput in;
  in.actif = kModeDecoupeOnoff;
  in.schedule_type_triac = 4;
  in.net_power_w = -3000;
  in.ki = 0;
  in.loop_gain = 0;
  in.triac_threshold_min_w = 0;
  in.triac_max_percent = 80;
  (void)balansun_regulation_compute_triac(in);
  in.loop_gain = 8;
  in.triac_threshold_min_w = 0;
  in.triac_max_percent = 80;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_LT(out.triac_delay_percent_f, 100.0f);
}

TEST(BalansunRegulationLogic, IntegralOnlyWithoutPid) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 0;
  in.max_open_percent = 80;
  in.pid_enabled = false;
  in.expert_regulation_mode = 1;
  in.net_power_w = -600.0f;
  in.ki = 30;
  const auto out = SurplusRegulator::compute_action(in);
  EXPECT_LT(out.triac_delay_percent_f, 100.0f);
}

TEST(BalansunRegulationLogic, GainScalesKiWhenExportBelowThreshold) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 500;
  in.regulation_gain = 50;
  in.ki = 20;
  in.net_power_w = 100.0f;
  (void)SurplusRegulator::compute_action(in);
  in.threshold_min_w = 0;
  in.net_power_w = -200.0f;
  (void)SurplusRegulator::compute_action(in);
}

TEST(BalansunRegulationLogic, GainOutOfRangeSkipsScale) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 500;
  in.regulation_gain = 1;
  in.ki = 20;
  in.net_power_w = 100.0f;
  (void)SurplusRegulator::compute_action(in);
  in.regulation_gain = 100;
  (void)SurplusRegulator::compute_action(in);
}

TEST(BalansunRegulationLogic, PidDerivativeWithAlternatingExport) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 0;
  in.max_open_percent = 80;
  in.pid_enabled = true;
  in.expert_regulation_mode = 1;
  in.kp = 80;
  in.ki = 5;
  in.kd = 80;
  for (int i = 0; i < 6; i++) {
    in.net_power_w = (i % 2) ? -800.0f : -200.0f;
    (void)SurplusRegulator::compute_action(in);
  }
}

TEST(BalansunRegulationLogic, DecoupeSecondaryBothBranches) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 1;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.max_open_percent = 50;
  in.threshold_min_w = 100;
  in.net_power_w = 200.0f;
  auto out = SurplusRegulator::compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  in.net_power_w = 50.0f;
  out = SurplusRegulator::compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 0.0f);
}

TEST(BalansunRegulationLogic, ItmodeNotOkStillRegulates) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.itmode_ok = false;
  in.net_power_w = -3000.0f;
  in.ki = 40;
  in.max_open_percent = 80;
  const auto out = SurplusRegulator::compute_action(in);
  EXPECT_LT(out.triac_delay_percent_f, 100.0f);
}

TEST(BalansunRegulationLogic, TriacOverrideOffForcesOpen) {
  balansun_regulation_state_init();
  TriacRegulationInput in;
  in.override_state = kActionOverrideOff;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.triac_on);
}

TEST(BalansunRegulationLogic, ExpertPidRequiresModeOne) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.threshold_min_w = 0;
  in.max_open_percent = 80;
  in.pid_enabled = true;
  in.expert_regulation_mode = 0;
  in.net_power_w = -400.0f;
  in.ki = 10;
  in.kp = 20;
  (void)SurplusRegulator::compute_action(in);
}

TEST(BalansunRegulationLogic, TriacFixedOverridePercent) {
  balansun_regulation_state_init();
  TriacRegulationInput in;
  in.override_state = kActionOverrideTriacFixed;
  in.override_triac_percent = 35;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 65.0f);
  EXPECT_TRUE(out.triac_on);
}

TEST(BalansunRegulationLogic, TriacOverrideOnClampsInvalidMaxPercent) {
  balansun_regulation_state_init();
  TriacRegulationInput in;
  in.override_state = kActionOverrideOn;
  in.triac_max_percent = 0;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 0.0f);
  EXPECT_TRUE(out.triac_on);
  in.triac_max_percent = 150;
  const auto out2 = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out2.triac_delay_percent_f, 0.0f);
}

TEST(BalansunRegulationLogic, RejectsOutOfRangeActionIndex) {
  balansun_regulation_state_init();
  ActionRegulationInput in;
  in.action_index = kRegulationActions;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  const ActionRegulationOutput out = SurplusRegulator::compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.active);

  in.action_index = -1;
  const ActionRegulationOutput neg = SurplusRegulator::compute_action(in);
  EXPECT_FLOAT_EQ(neg.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(neg.active);
}
