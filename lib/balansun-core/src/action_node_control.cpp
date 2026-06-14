#include <balansun/action_node_control.h>
#include <balansun/action_node_manual.h>

void action_node_resolve(const ActionNodeRuntimeInput &in, ActionNodeRuntimeOutput &out) {
  out.control_mode = ActionNodeControlMode::Auto;
  PilotWireOrder order = PilotWireOrder::Confort;
  const RadiatorSchedulePeriod *period = radiator_schedule_active_period(in.schedule, in.wall_decihours);
  const bool regulation_window =
      period && period->kind == static_cast<uint8_t>(RadiatorPeriodKind::Regulation);

  const bool manual_live =
      action_node_manual_is_live(in.manual.active, in.manual.until_ms, in.now_ms);
  if (manual_live) {
    order = in.manual.order;
    out.control_mode = ActionNodeControlMode::Manual;
  } else if (regulation_window) {
    order = in.router.surplus_on ? PilotWireOrder::Eco : PilotWireOrder::Confort;
  } else {
    order = radiator_schedule_resolve_order(in.schedule, in.wall_decihours, false, in.warm_cfg);
  }

  if (!manual_live) {
    order = radiator_schedule_apply_temp_gate(order, period, in.temperature_c, in.temperature_ok);
  }

  if (!balansun_pilot_wire_full_order_supported(order)) {
    order = PilotWireOrder::Confort;
  }

  out.order = order;
  out.relay_mask = balansun_pilot_wire_full_relays_for_order(order, in.wiring);
}
