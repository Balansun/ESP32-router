#include <gtest/gtest.h>

#include "actions_logic.h"
#include "balansun_meter_logic.h"
#include "balansun_regulation_logic.h"
#include "balansun_regulation_modes.h"
#include "balansun_regulation_state.h"
#include "balansun_runtime.h"
#include "balansun_triac_logic.h"

namespace {

class RegulationLogic : public ::testing::Test {
 protected:
  void SetUp() override { balansun_regulation_state_init(); }
};

/** Matches `balansun_apply_surplus_regulation`: house net W = import − export (export → negative net). */
int house_net_w(int active_import_w, int active_export_w) {
  return active_import_w - active_export_w;
}

int triac_open_percent_from_triac_delay_percent_f(float triac_delay_percent_f) { return 100 - static_cast<int>(triac_delay_percent_f); }

TriacRegulationInput regulating_defaults() {
  TriacRegulationInput in;
  in.actif = kModeDecoupeOnoff;
  in.schedule_type_triac = 4;
  in.loop_gain = 4;
  in.ki = 4;
  in.triac_threshold_min_w = 0;
  in.triac_max_percent = 100;
  in.current_triac_delay_percent_f = 50.0f;
  in.override_state = kActionOverrideAuto;
  in.itmode_ok = true;
  return in;
}

}  // namespace

TEST(ActionsLogic, ActiveTypeAfterPeriodEnd) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 1000, 0, 80, 101, 101};
  EXPECT_EQ(actions_logic_active_type(cfg, 2500), 0u);
}

TEST(ActionsLogic, ThresholdMinMaxNonType2InPeriod) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 2400, 500, 3000, 101, 101};
  EXPECT_EQ(actions_logic_threshold_min(cfg, 1200), 500);
  EXPECT_EQ(actions_logic_threshold_max(cfg, 1200), 3000);
}

TEST(ActionsLogic, ActiveTypeAndThresholds) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  EXPECT_EQ(actions_logic_active_type(cfg, 1200), static_cast<uint8_t>(4));
  cfg.periods[0].type = 2;
  EXPECT_EQ(actions_logic_threshold_min(cfg, 500), 32000);
  EXPECT_EQ(actions_logic_threshold_max(cfg, 500), 100);
}

TEST(ActionsLogic, OverrideExpired) {
  EXPECT_FALSE(actions_logic_override_expired(kActionOverrideAuto, 1000, 2000));
  EXPECT_TRUE(actions_logic_override_expired(kActionOverrideOn, 1000, 2000));
  EXPECT_FALSE(actions_logic_override_expired(kActionOverrideOn, 0, 2000));
}

TEST(ActionsLogic, TriacScheduleRespectsTemperature) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, 20.0f), static_cast<uint8_t>(4));
  cfg.periods[0].temp_max = 30;
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, 25.0f), static_cast<uint8_t>(0));
}

TEST_F(RegulationLogic, OverrideOffClosesTriac) {
  TriacRegulationInput in;
  in.override_state = kActionOverrideOff;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.triac_on);
  EXPECT_EQ(triac_open_percent_from_triac_delay_percent_f(out.triac_delay_percent_f), 0);
}

TEST_F(RegulationLogic, OverrideOnOpensToScheduleMax) {
  TriacRegulationInput in = regulating_defaults();
  in.override_state = kActionOverrideOn;
  in.triac_max_percent = 60;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 40.0f);
  EXPECT_TRUE(out.triac_on);
  EXPECT_EQ(triac_open_percent_from_triac_delay_percent_f(out.triac_delay_percent_f), 60);
}

TEST_F(RegulationLogic, OverrideTriacFixedHonorsPercent) {
  TriacRegulationInput in = regulating_defaults();
  in.override_state = kActionOverrideTriacFixed;
  in.override_triac_percent = 35;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 65.0f);
  EXPECT_TRUE(out.triac_on);
  EXPECT_EQ(triac_open_percent_from_triac_delay_percent_f(out.triac_delay_percent_f), 35);
}

TEST_F(RegulationLogic, SurplusExportOpensCumulusTriac) {
  TriacRegulationInput in = regulating_defaults();
  in.net_power_w = house_net_w(80, 3500);
  in.current_triac_delay_percent_f = 85.0f;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_LT(out.triac_delay_percent_f, in.current_triac_delay_percent_f);
  EXPECT_TRUE(out.triac_on);
  EXPECT_GT(triac_open_percent_from_triac_delay_percent_f(out.triac_delay_percent_f), triac_open_percent_from_triac_delay_percent_f(in.current_triac_delay_percent_f));
}

TEST_F(RegulationLogic, GridImportClosesCumulusTriac) {
  TriacRegulationInput in = regulating_defaults();
  in.net_power_w = house_net_w(4000, 50);
  in.current_triac_delay_percent_f = 25.0f;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_GT(out.triac_delay_percent_f, in.current_triac_delay_percent_f);
  EXPECT_TRUE(out.triac_on);
  EXPECT_LT(triac_open_percent_from_triac_delay_percent_f(out.triac_delay_percent_f), triac_open_percent_from_triac_delay_percent_f(in.current_triac_delay_percent_f));
}

TEST_F(RegulationLogic, RespectsTriacMaxPercentCap) {
  TriacRegulationInput in = regulating_defaults();
  in.triac_max_percent = 30;
  in.current_triac_delay_percent_f = 10.0f;
  in.net_power_w = house_net_w(0, 8000);
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_GE(out.triac_delay_percent_f, 70.0f - 0.01f);
  EXPECT_LE(triac_open_percent_from_triac_delay_percent_f(out.triac_delay_percent_f), 30);
}

TEST_F(RegulationLogic, InactiveScheduleKeepsTriacClosed) {
  TriacRegulationInput in = regulating_defaults();
  in.schedule_type_triac = 0;
  in.net_power_w = house_net_w(0, 5000);
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.triac_on);
}

TEST_F(RegulationLogic, ScheduleWiringMatchesbalansun_apply_surplus_regulation) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 2400, 0, 80, 101, 101};
  const int wall_decihours_val = 1200;
  const float temp = 20.0f;

  TriacRegulationInput in = regulating_defaults();
  in.wall_decihours = wall_decihours_val;
  in.temperature = temp;
  in.schedule_type_triac = actions_logic_active_type_triac(cfg, wall_decihours_val, temp);
  in.triac_threshold_min_w = actions_logic_threshold_min(cfg, wall_decihours_val);
  in.triac_max_percent = actions_logic_threshold_max(cfg, wall_decihours_val);
  ASSERT_GE(in.schedule_type_triac, 2u);

  in.net_power_w = house_net_w(100, 2800);
  in.current_triac_delay_percent_f = 88.0f;
  const auto export_out = balansun_regulation_compute_triac(in);
  EXPECT_LT(export_out.triac_delay_percent_f, in.current_triac_delay_percent_f);

  in.net_power_w = house_net_w(3500, 100);
  in.current_triac_delay_percent_f = export_out.triac_delay_percent_f;
  const auto import_out = balansun_regulation_compute_triac(in);
  EXPECT_GT(import_out.triac_delay_percent_f, in.current_triac_delay_percent_f);
}

TEST_F(RegulationLogic, HotTankBlocksScheduleType) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 2400, 0, 80, 55, 101};
  TriacRegulationInput in = regulating_defaults();
  in.wall_decihours = 1200;
  in.temperature = 60.0f;
  in.schedule_type_triac = actions_logic_active_type_triac(cfg, in.wall_decihours, in.temperature);
  EXPECT_EQ(in.schedule_type_triac, 0u);
  in.net_power_w = house_net_w(0, 5000);
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.triac_on);
}

TEST(MeterLogic, ApplyFieldsToRuntime) {
  RmsRuntime rt;
  MeterSnapshotFields f;
  f.has_house = true;
  f.house.active_import_w = 100;
  f.house.active_export_w = 40;
  EXPECT_TRUE(balansun_meter_logic_apply_fields(rt, f, nullptr));
  EXPECT_EQ(rt.house.active_import_w, 100);
}

TEST(MeterLogic, SecondRawAndNoFields) {
  RmsRuntime rt;
  MeterSnapshotFields f;
  f.has_second = true;
  f.second.active_import_w = 50;
  EXPECT_TRUE(balansun_meter_logic_apply_fields(rt, f, nullptr));
  f = MeterSnapshotFields{};
  f.has_raw = true;
  f.raw.voltage_house_v = 230.0f;
  EXPECT_TRUE(balansun_meter_logic_apply_fields(rt, f, nullptr));
  EXPECT_FLOAT_EQ(rt.raw.voltage_house_v, 230.0f);
  f = MeterSnapshotFields{};
  std::string err;
  EXPECT_FALSE(balansun_meter_logic_apply_fields(rt, f, &err));
  EXPECT_EQ(err, "no_meter_fields");
  EXPECT_FALSE(balansun_meter_logic_apply_fields(rt, f, nullptr));
}

TEST(BalansunMeterLogic, ApplySecondOrRawChannelOnly) {
  RmsRuntime rt;
  MeterSnapshotFields f;
  f.has_second = true;
  f.second.active_import_w = 42;
  EXPECT_TRUE(balansun_meter_logic_apply_fields(rt, f, nullptr));
  EXPECT_EQ(rt.second.active_import_w, 42);
  f = MeterSnapshotFields{};
  f.has_raw = true;
  f.raw.voltage_house_v = 230.0f;
  EXPECT_TRUE(balansun_meter_logic_apply_fields(rt, f, nullptr));
  EXPECT_FLOAT_EQ(rt.raw.voltage_house_v, 230.0f);
}

TEST(ActionsLogic, ScheduleThresholdsAndOverride) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {2, 800, 1200, 100, 200, 101, 101};
  EXPECT_EQ(actions_logic_active_type(cfg, 900), static_cast<uint8_t>(2));
  EXPECT_EQ(actions_logic_threshold_min(cfg, 900), 32000);
  EXPECT_EQ(actions_logic_threshold_max(cfg, 900), 100);
  EXPECT_FALSE(actions_logic_override_expired(kActionOverrideAuto, 1000, 2000));
  EXPECT_TRUE(actions_logic_override_expired(kActionOverrideOn, 1000, 2000));
  EXPECT_FALSE(actions_logic_override_expired(kActionOverrideOn, 0, 2000));
}

TEST(ActionsLogic, ActiveTypeOutsidePeriod) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {3, 800, 1200, 0, 0, 101, 101};
  EXPECT_EQ(actions_logic_active_type(cfg, 500), static_cast<uint8_t>(0));
  EXPECT_EQ(actions_logic_active_type(cfg, 900), static_cast<uint8_t>(3));
}

TEST(ActionsLogic, MultiplePeriodsLastMatchWins) {
  ActionScheduleConfig cfg;
  cfg.period_count = 2;
  cfg.periods[0] = {2, 0, 2400, 10, 20, 101, 101};
  cfg.periods[1] = {5, 0, 2400, 30, 40, 101, 101};
  EXPECT_EQ(actions_logic_active_type(cfg, 1200), static_cast<uint8_t>(5));
  EXPECT_EQ(actions_logic_threshold_min(cfg, 1200), 30);
  EXPECT_EQ(actions_logic_threshold_max(cfg, 1200), 40);
}

TEST(ActionsLogic, NonType2UsesConfiguredPowerThresholds) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 2400, 150, 75, 101, 101};
  EXPECT_EQ(actions_logic_threshold_min(cfg, 1200), 150);
  EXPECT_EQ(actions_logic_threshold_max(cfg, 1200), 75);
  EXPECT_EQ(actions_logic_threshold_min(cfg, 3000), 0);
}

TEST(ActionsLogic, TriacTempMinMaxAndSensorDisabled) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 2400, 0, 80, 20, 50};
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, -150.0f), static_cast<uint8_t>(4));
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, 15.0f), static_cast<uint8_t>(0));
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, 55.0f), static_cast<uint8_t>(0));
  cfg.periods[0].temp_min = 101;
  cfg.periods[0].temp_max = 101;
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, 15.0f), static_cast<uint8_t>(4));
}

TEST(ActionsLogic, NoPeriodsReturnsZero) {
  ActionScheduleConfig cfg;
  cfg.period_count = 0;
  EXPECT_EQ(actions_logic_active_type(cfg, 1200), static_cast<uint8_t>(0));
  EXPECT_EQ(actions_logic_threshold_min(cfg, 1200), 0);
}

TEST(ActionsLogic, TriacPicksMatchingPeriodAmongSeveral) {
  ActionScheduleConfig cfg;
  cfg.period_count = 2;
  cfg.periods[0] = {2, 0, 800, 0, 0, 101, 101};
  cfg.periods[1] = {4, 900, 2400, 0, 80, 101, 101};
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, 20.0f), static_cast<uint8_t>(4));
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 500, 20.0f), static_cast<uint8_t>(2));
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 850, 20.0f), static_cast<uint8_t>(0));
}

TEST(ActionsLogic, ThresholdMaxOutsidePeriodAndType2) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {3, 800, 1200, 0, 55, 101, 101};
  EXPECT_EQ(actions_logic_threshold_max(cfg, 2000), 0);
  EXPECT_EQ(actions_logic_threshold_max(cfg, 1000), 55);
  cfg.periods[0].type = 2;
  EXPECT_EQ(actions_logic_threshold_max(cfg, 1000), 100);
}

TEST(ActionsLogic, OverrideNotYetExpired) {
  EXPECT_FALSE(actions_logic_override_expired(kActionOverrideOn, 5000, 1000));
}

TEST(ActionsLogic, OverrideExpiredEdges) {
  EXPECT_FALSE(actions_logic_override_expired(kActionOverrideAuto, 1000, 2000));
  EXPECT_FALSE(actions_logic_override_expired(kActionOverrideOn, 0, 9000));
  EXPECT_TRUE(actions_logic_override_expired(kActionOverrideOff, 1000, 1000));
}

TEST(ActionsLogic, TriacTempMaxOnlyBlocks) {
  ActionScheduleConfig cfg;
  cfg.period_count = 1;
  cfg.periods[0] = {4, 0, 2400, 0, 80, 101, 30};
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, 35.0f), static_cast<uint8_t>(4));
  EXPECT_EQ(actions_logic_active_type_triac(cfg, 1200, 10.0f), static_cast<uint8_t>(0));
}

TEST_F(RegulationLogic, ActionIndexOutOfRange) {
  ActionRegulationInput in;
  in.action_index = 99;
  const auto out = balansun_regulation_compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  EXPECT_FALSE(out.active);
}

TEST_F(RegulationLogic, ScheduleTypeOnForcesDelay) {
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 2;
  in.max_open_percent = 40;
  const auto out = balansun_regulation_compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 60.0f);
}

TEST_F(RegulationLogic, DecoupeOnoffSecondaryAction) {
  ActionRegulationInput in;
  in.action_index = 1;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.net_power_w = 5000;
  auto out = balansun_regulation_compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 100.0f);
  in.net_power_w = -100;
  out = balansun_regulation_compute_action(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 0.0f);
}

TEST_F(RegulationLogic, TriacMaxPercentZeroDefaults) {
  TriacRegulationInput in = regulating_defaults();
  in.override_state = kActionOverrideOn;
  in.triac_max_percent = 0;
  const auto out = balansun_regulation_compute_triac(in);
  EXPECT_FLOAT_EQ(out.triac_delay_percent_f, 0.0f);
}

TEST_F(RegulationLogic, PidAndGainBranches) {
  ActionRegulationInput in;
  in.action_index = 0;
  in.regulation_mode = kModeDecoupeOnoff;
  in.schedule_type = 4;
  in.net_power_w = 50;
  in.threshold_min_w = 100;
  in.max_open_percent = 80;
  in.regulation_gain = 50;
  in.ki = 10;
  in.pid_enabled = true;
  in.expert_regulation_mode = 1;
  in.kp = 100;
  in.kd = 50;
  in.ki = 0;
  (void)balansun_regulation_compute_action(in);

  in.itmode_ok = false;
  in.pid_enabled = false;
  in.ki = 10;
  (void)balansun_regulation_compute_action(in);
}

TEST(TriacLogic, ThresholdTicksAtHalfDuty) {
  EXPECT_EQ(triac_delay_threshold_ticks(50, 98), 49);
  EXPECT_EQ(triac_delay_threshold_ticks(100, 98), 99);
}

TEST(TriacLogic, MaxRetardFromHalfPeriod) {
  EXPECT_EQ(triac_max_delay_ticks_from_half_period_us(10000), 98);
  EXPECT_EQ(triac_delay_threshold_ticks(100, 98), 99);
  EXPECT_EQ(triac_delay_threshold_ticks(0, 98), 0);
  EXPECT_EQ(triac_max_delay_ticks_from_half_period_us(2), 1);
}
