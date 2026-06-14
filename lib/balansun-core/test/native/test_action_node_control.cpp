#include <gtest/gtest.h>

#include <balansun/action_node_control.h>

TEST(ActionNodeControl, ManualOverrideBeatsSchedule) {
  ActionNodeRuntimeInput in{};
  radiator_schedule_init_defaults(in.schedule);
  in.schedule.periods[0].kind = static_cast<uint8_t>(RadiatorPeriodKind::Fixed);
  in.schedule.periods[0].fixed_order = PilotWireOrder::Arret;
  in.manual.active = true;
  in.manual.order = PilotWireOrder::Eco;
  in.manual.until_ms = 100000;
  in.now_ms = 1000;
  ActionNodeRuntimeOutput out{};
  action_node_resolve(in, out);
  EXPECT_EQ(out.order, PilotWireOrder::Eco);
  EXPECT_EQ(out.control_mode, ActionNodeControlMode::Manual);
}

TEST(ActionNodeControl, RouterLatchInRegulationWindow) {
  ActionNodeRuntimeInput in{};
  radiator_schedule_init_defaults(in.schedule);
  in.router.surplus_on = true;
  in.wall_decihours = 600;
  ActionNodeRuntimeOutput out{};
  action_node_resolve(in, out);
  EXPECT_EQ(out.order, PilotWireOrder::Eco);
}

TEST(ActionNodeControl, FixedPeriodIgnoresRouter) {
  ActionNodeRuntimeInput in{};
  radiator_schedule_init_defaults(in.schedule);
  in.schedule.periods[0].kind = static_cast<uint8_t>(RadiatorPeriodKind::Fixed);
  in.schedule.periods[0].fixed_order = PilotWireOrder::HorsGel;
  in.router.surplus_on = true;
  in.wall_decihours = 600;
  ActionNodeRuntimeOutput out{};
  action_node_resolve(in, out);
  EXPECT_EQ(out.order, PilotWireOrder::HorsGel);
}

TEST(ActionNodeControl, TimedManualExpiresToSchedule) {
  ActionNodeRuntimeInput in{};
  radiator_schedule_init_defaults(in.schedule);
  in.schedule.periods[0].kind = static_cast<uint8_t>(RadiatorPeriodKind::Fixed);
  in.schedule.periods[0].fixed_order = PilotWireOrder::Arret;
  in.manual.active = true;
  in.manual.order = PilotWireOrder::Eco;
  in.manual.until_ms = 1000;
  in.now_ms = 2000;
  in.wall_decihours = 600;
  ActionNodeRuntimeOutput out{};
  action_node_resolve(in, out);
  EXPECT_EQ(out.order, PilotWireOrder::Arret);
  EXPECT_EQ(out.control_mode, ActionNodeControlMode::Auto);
}
