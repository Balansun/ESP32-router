#include <balansun/radiator_schedule.h>

namespace {

PilotWireOrder clamp_order(PilotWireOrder order, const WarmActuatorConfig &warm_cfg) {
  return balansun_pilot_wire_clamp_order(order, balansun_warm_pilot_sku(warm_cfg));
}

}  // namespace

void radiator_schedule_init_defaults(RadiatorScheduleConfig &cfg) {
  cfg.period_count = 1;
  cfg.periods[0] = RadiatorSchedulePeriod{};
  cfg.periods[0].kind = static_cast<uint8_t>(RadiatorPeriodKind::Regulation);
  cfg.periods[0].period_start = 0;
  cfg.periods[0].period_end = kRadiatorScheduleTimeMax;
}

int radiator_schedule_clamp_period_count(int count) {
  if (count < 1) return 1;
  if (count > kRadiatorScheduleMaxPeriods) return kRadiatorScheduleMaxPeriods;
  return count;
}

bool radiator_schedule_periods_valid(uint8_t period_count, const int period_end[8], const uint8_t kind[8]) {
  const int n = radiator_schedule_clamp_period_count(period_count);
  if (static_cast<int>(period_count) != n) return false;
  int prev_end = 0;
  for (int i = 0; i < n; i++) {
    if (kind[i] < static_cast<uint8_t>(RadiatorPeriodKind::Regulation) ||
        kind[i] > static_cast<uint8_t>(RadiatorPeriodKind::Fixed)) {
      return false;
    }
    const int end = period_end[i];
    if (end < 0 || end > kRadiatorScheduleTimeMax) return false;
    if (end <= prev_end) return false;
    prev_end = end;
  }
  if (period_end[n - 1] != kRadiatorScheduleTimeMax) return false;
  return true;
}

bool radiator_schedule_valid(const RadiatorScheduleConfig &cfg, const WarmActuatorConfig &warm_cfg) {
  const int n = radiator_schedule_clamp_period_count(cfg.period_count);
  if (static_cast<int>(cfg.period_count) != n) return false;
  int period_end[kRadiatorScheduleMaxPeriods]{};
  uint8_t kind[kRadiatorScheduleMaxPeriods]{};
  for (int i = 0; i < n; i++) {
    period_end[i] = cfg.periods[i].period_end;
    kind[i] = cfg.periods[i].kind;
    if (cfg.periods[i].kind == static_cast<uint8_t>(RadiatorPeriodKind::Fixed)) {
      const PilotWireOrder clamped = clamp_order(cfg.periods[i].fixed_order, warm_cfg);
      if (!balansun_pilot_wire_order_supported(clamped, balansun_warm_pilot_sku(warm_cfg))) {
        return false;
      }
    }
  }
  return radiator_schedule_periods_valid(cfg.period_count, period_end, kind);
}

const RadiatorSchedulePeriod *radiator_schedule_active_period(const RadiatorScheduleConfig &cfg, int wall_decihours) {
  const RadiatorSchedulePeriod *active = nullptr;
  const int n = radiator_schedule_clamp_period_count(cfg.period_count);
  for (int i = 0; i < n; i++) {
    const RadiatorSchedulePeriod &p = cfg.periods[i];
    if (wall_decihours >= p.period_start && wall_decihours <= p.period_end) {
      active = &p;
    }
  }
  return active;
}

uint8_t radiator_schedule_active_kind(const RadiatorScheduleConfig &cfg, int wall_decihours) {
  const RadiatorSchedulePeriod *p = radiator_schedule_active_period(cfg, wall_decihours);
  return p ? p->kind : static_cast<uint8_t>(RadiatorPeriodKind::Regulation);
}

PilotWireOrder radiator_schedule_active_fixed_order(const RadiatorScheduleConfig &cfg, int wall_decihours) {
  const RadiatorSchedulePeriod *p = radiator_schedule_active_period(cfg, wall_decihours);
  if (!p || p->kind != static_cast<uint8_t>(RadiatorPeriodKind::Fixed)) {
    return PilotWireOrder::Confort;
  }
  return p->fixed_order;
}

bool radiator_schedule_allows_surplus(const RadiatorScheduleConfig &cfg, int wall_decihours) {
  return radiator_schedule_active_kind(cfg, wall_decihours) ==
         static_cast<uint8_t>(RadiatorPeriodKind::Regulation);
}

PilotWireOrder radiator_schedule_resolve_order(const RadiatorScheduleConfig &cfg, int wall_decihours,
                                               bool surplus_active, const WarmActuatorConfig &warm_cfg) {
  const RadiatorSchedulePeriod *p = radiator_schedule_active_period(cfg, wall_decihours);
  if (!p) {
    return clamp_order(PilotWireOrder::Confort, warm_cfg);
  }
  if (p->kind == static_cast<uint8_t>(RadiatorPeriodKind::Fixed)) {
    return clamp_order(p->fixed_order, warm_cfg);
  }
  const PilotWireOrder base = surplus_active ? PilotWireOrder::Eco : PilotWireOrder::Confort;
  return clamp_order(base, warm_cfg);
}

PilotWireOrder radiator_schedule_apply_temp_gate(PilotWireOrder order, const RadiatorSchedulePeriod *period,
                                                 float temperature_c, bool temperature_ok) {
  if (!period || !temperature_ok) return order;
  if (period->temp_inf_c < kRadiatorScheduleTempDisabled &&
      temperature_c < period->temp_inf_c) {
    return PilotWireOrder::Confort;
  }
  if (period->temp_sup_c < kRadiatorScheduleTempDisabled &&
      temperature_c > period->temp_sup_c) {
    return PilotWireOrder::Confort;
  }
  return order;
}
