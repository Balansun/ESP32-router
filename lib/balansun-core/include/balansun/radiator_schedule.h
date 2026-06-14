#pragma once

#include <balansun/pilot_wire.h>
#include <balansun/warm_actuator.h>

#include <cstdint>

constexpr int kRadiatorScheduleMaxPeriods = 8;
constexpr int kRadiatorScheduleTimeMax = 2400;
constexpr float kRadiatorScheduleTempDisabled = 150.0f;

enum class RadiatorPeriodKind : uint8_t {
  Regulation = 1,
  Fixed = 2,
};

struct RadiatorSchedulePeriod {
  uint8_t kind = static_cast<uint8_t>(RadiatorPeriodKind::Regulation);
  PilotWireOrder fixed_order = PilotWireOrder::Confort;
  int period_start = 0;
  int period_end = kRadiatorScheduleTimeMax;
  float temp_inf_c = kRadiatorScheduleTempDisabled;
  float temp_sup_c = kRadiatorScheduleTempDisabled;
};

struct RadiatorScheduleConfig {
  uint8_t period_count = 1;
  RadiatorSchedulePeriod periods[kRadiatorScheduleMaxPeriods];
};

void radiator_schedule_init_defaults(RadiatorScheduleConfig &cfg);
int radiator_schedule_clamp_period_count(int count);
bool radiator_schedule_periods_valid(uint8_t period_count, const int period_end[8], const uint8_t kind[8]);
bool radiator_schedule_valid(const RadiatorScheduleConfig &cfg, const WarmActuatorConfig &warm_cfg);
uint8_t radiator_schedule_active_kind(const RadiatorScheduleConfig &cfg, int wall_decihours);
PilotWireOrder radiator_schedule_active_fixed_order(const RadiatorScheduleConfig &cfg, int wall_decihours);
bool radiator_schedule_allows_surplus(const RadiatorScheduleConfig &cfg, int wall_decihours);
const RadiatorSchedulePeriod *radiator_schedule_active_period(const RadiatorScheduleConfig &cfg, int wall_decihours);

PilotWireOrder radiator_schedule_resolve_order(const RadiatorScheduleConfig &cfg, int wall_decihours,
                                               bool surplus_active, const WarmActuatorConfig &warm_cfg);

/** When temperature is outside period bounds, fall back to Confort (action node spec). */
PilotWireOrder radiator_schedule_apply_temp_gate(PilotWireOrder order, const RadiatorSchedulePeriod *period,
                                                 float temperature_c, bool temperature_ok);
