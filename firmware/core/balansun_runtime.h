#pragma once

#include <cstdint>

/** Per-channel active power / energy fields (house = false, triac/second = true). */
struct MeterChannelState {
  int active_import_w = 0;
  int active_export_w = 0;
  int apparent_import_va = 0;
  int apparent_export_va = 0;
  double energy_day_import_wh = 0;
  double energy_day_export_wh = 0;
  double energy_total_import_wh = 0;
  double energy_total_export_wh = 0;
};

struct RawMeterState {
  float voltage_house_v = 0;
  float current_house_a = 0;
  float pf_house = 0;
  float voltage_second_v = 0;
  float current_second_a = 0;
  float pf_second = 0;
  float freq_hz = 50;
};

/** Mutable metering snapshot used by logic modules and tests. */
struct RmsRuntime {
  MeterChannelState house;
  MeterChannelState second;
  RawMeterState raw;
  bool energie_active_valide = false;

  void reset_meter();
  void sync_from_globals();
  void sync_to_globals() const;
};

RmsRuntime &balansun_runtime();
