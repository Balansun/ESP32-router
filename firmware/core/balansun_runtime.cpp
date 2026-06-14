#include "balansun_runtime.h"

#include "balansun_globals.h"

static RmsRuntime g_runtime;

RmsRuntime &balansun_runtime() { return g_runtime; }

void RmsRuntime::reset_meter() {
  house = MeterChannelState{};
  second = MeterChannelState{};
  raw = RawMeterState{};
  energie_active_valide = false;
}

void RmsRuntime::sync_from_globals() {
  house.active_import_w = house_active_import_w;
  house.active_export_w = house_active_export_w;
  house.apparent_import_va = house_apparent_import_va;
  house.apparent_export_va = house_apparent_export_va;
  house.energy_day_import_wh = house_day_energy_import_wh;
  house.energy_day_export_wh = house_day_energy_export_wh;
  house.energy_total_import_wh = house_energy_import_wh;
  house.energy_total_export_wh = house_energy_export_wh;
  second.active_import_w = second_active_import_w;
  second.active_export_w = second_active_export_w;
  second.apparent_import_va = second_apparent_import_va;
  second.apparent_export_va = second_apparent_export_va;
  second.energy_day_import_wh = second_day_energy_import_wh;
  second.energy_day_export_wh = second_day_energy_export_wh;
  second.energy_total_import_wh = second_energy_import_wh;
  second.energy_total_export_wh = second_energy_export_wh;
  raw.voltage_house_v = house_voltage_v;
  raw.current_house_a = house_current_a;
  raw.pf_house = house_power_factor;
  raw.voltage_second_v = second_voltage_v;
  raw.current_second_a = second_current_a;
  raw.pf_second = second_power_factor;
  raw.freq_hz = mains_frequency_hz;
  energie_active_valide = meter_reading_valid;
}

void RmsRuntime::sync_to_globals() const {
  house_active_import_w = house.active_import_w;
  house_active_export_w = house.active_export_w;
  house_apparent_import_va = house.apparent_import_va;
  house_apparent_export_va = house.apparent_export_va;
  house_day_energy_import_wh = house.energy_day_import_wh;
  house_day_energy_export_wh = house.energy_day_export_wh;
  house_energy_import_wh = house.energy_total_import_wh;
  house_energy_export_wh = house.energy_total_export_wh;
  second_active_import_w = second.active_import_w;
  second_active_export_w = second.active_export_w;
  second_apparent_import_va = second.apparent_import_va;
  second_apparent_export_va = second.apparent_export_va;
  second_day_energy_import_wh = second.energy_day_import_wh;
  second_day_energy_export_wh = second.energy_day_export_wh;
  second_energy_import_wh = second.energy_total_import_wh;
  second_energy_export_wh = second.energy_total_export_wh;
  house_voltage_v = raw.voltage_house_v;
  house_current_a = raw.current_house_a;
  house_power_factor = raw.pf_house;
  second_voltage_v = raw.voltage_second_v;
  second_current_a = raw.current_second_a;
  second_power_factor = raw.pf_second;
  mains_frequency_hz = raw.freq_hz;
  meter_reading_valid = energie_active_valide;
}
