#include <gtest/gtest.h>

#include <balansun/radiator_schedule.h>

TEST(RadiatorSchedule, DefaultRegulationResolvesEcoWithSurplus) {
  RadiatorScheduleConfig cfg{};
  radiator_schedule_init_defaults(cfg);
  WarmActuatorConfig warm{.sku = WarmHardwareSku::R2};
  EXPECT_EQ(radiator_schedule_resolve_order(cfg, 1200, true, warm), PilotWireOrder::Eco);
  EXPECT_EQ(radiator_schedule_resolve_order(cfg, 1200, false, warm), PilotWireOrder::Confort);
}

TEST(RadiatorSchedule, FixedPeriodHonorsHorsGelAndArret) {
  RadiatorScheduleConfig cfg{};
  radiator_schedule_init_defaults(cfg);
  cfg.period_count = 2;
  cfg.periods[0].kind = static_cast<uint8_t>(RadiatorPeriodKind::Fixed);
  cfg.periods[0].fixed_order = PilotWireOrder::HorsGel;
  cfg.periods[0].period_end = 1200;
  cfg.periods[1].kind = static_cast<uint8_t>(RadiatorPeriodKind::Fixed);
  cfg.periods[1].fixed_order = PilotWireOrder::Arret;
  cfg.periods[1].period_start = 1200;
  cfg.periods[1].period_end = 2400;
  WarmActuatorConfig warm{.sku = WarmHardwareSku::R2};
  EXPECT_TRUE(radiator_schedule_valid(cfg, warm));
  EXPECT_EQ(radiator_schedule_resolve_order(cfg, 600, false, warm), PilotWireOrder::HorsGel);
  EXPECT_EQ(radiator_schedule_resolve_order(cfg, 1800, false, warm), PilotWireOrder::Arret);
}

TEST(RadiatorSchedule, TempSupBlocksOrder) {
  RadiatorSchedulePeriod period{};
  period.temp_sup_c = 19.0f;
  EXPECT_EQ(radiator_schedule_apply_temp_gate(PilotWireOrder::Eco, &period, 20.0f, true),
            PilotWireOrder::Confort);
  EXPECT_EQ(radiator_schedule_apply_temp_gate(PilotWireOrder::Eco, &period, 18.0f, true), PilotWireOrder::Eco);
}
