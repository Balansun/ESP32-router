#pragma once
#include <Arduino.h>

/** Coherent meter readings published from core-0 metering task for core-1 consumers. */
struct BalansunPublic {
  bool valid;
  int house_active_import_w;
  int house_active_export_w;
  int second_active_import_w;
  int second_active_export_w;
  int house_apparent_import_va;
  int house_apparent_export_va;
  int second_apparent_import_va;
  int second_apparent_export_va;
  double house_energy_import_wh;
  double house_energy_export_wh;
  double second_energy_import_wh;
  double second_energy_export_wh;
  double house_day_energy_import_wh;
  double house_day_energy_export_wh;
  double second_day_energy_import_wh;
  double second_day_energy_export_wh;
  float house_voltage_v;
  float house_current_a;
  float house_power_factor;
  float second_voltage_v;
  float second_current_a;
  float second_power_factor;
  float mains_frequency_hz;
};

void BalansunPublishFromGlobals();
BalansunPublic BalansunReadSnapshot();
