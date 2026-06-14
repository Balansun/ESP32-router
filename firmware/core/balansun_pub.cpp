#include "balansun_pub.h"
#include "freertos/portmacro.h"

extern bool meter_reading_valid;
extern int house_active_import_w, house_active_export_w, second_active_import_w, second_active_export_w;
extern int house_apparent_import_va, house_apparent_export_va, second_apparent_import_va, second_apparent_export_va;
extern double house_energy_import_wh, house_energy_export_wh, second_energy_import_wh, second_energy_export_wh;
extern double house_day_energy_import_wh, house_day_energy_export_wh, second_day_energy_import_wh, second_day_energy_export_wh;
extern float house_voltage_v, house_current_a, house_power_factor, second_voltage_v, second_current_a, second_power_factor,
    mains_frequency_hz;

static portMUX_TYPE rmsPubMux = portMUX_INITIALIZER_UNLOCKED;
static BalansunPublic g_balansun_pub;

void BalansunPublishFromGlobals() {
  portENTER_CRITICAL(&rmsPubMux);
  g_balansun_pub.valid = meter_reading_valid;
  g_balansun_pub.house_active_import_w = house_active_import_w;
  g_balansun_pub.house_active_export_w = house_active_export_w;
  g_balansun_pub.second_active_import_w = second_active_import_w;
  g_balansun_pub.second_active_export_w = second_active_export_w;
  g_balansun_pub.house_apparent_import_va = house_apparent_import_va;
  g_balansun_pub.house_apparent_export_va = house_apparent_export_va;
  g_balansun_pub.second_apparent_import_va = second_apparent_import_va;
  g_balansun_pub.second_apparent_export_va = second_apparent_export_va;
  g_balansun_pub.house_energy_import_wh = house_energy_import_wh;
  g_balansun_pub.house_energy_export_wh = house_energy_export_wh;
  g_balansun_pub.second_energy_import_wh = second_energy_import_wh;
  g_balansun_pub.second_energy_export_wh = second_energy_export_wh;
  g_balansun_pub.house_day_energy_import_wh = house_day_energy_import_wh;
  g_balansun_pub.house_day_energy_export_wh = house_day_energy_export_wh;
  g_balansun_pub.second_day_energy_import_wh = second_day_energy_import_wh;
  g_balansun_pub.second_day_energy_export_wh = second_day_energy_export_wh;
  g_balansun_pub.house_voltage_v = house_voltage_v;
  g_balansun_pub.house_current_a = house_current_a;
  g_balansun_pub.house_power_factor = house_power_factor;
  g_balansun_pub.second_voltage_v = second_voltage_v;
  g_balansun_pub.second_current_a = second_current_a;
  g_balansun_pub.second_power_factor = second_power_factor;
  g_balansun_pub.mains_frequency_hz = mains_frequency_hz;
  portEXIT_CRITICAL(&rmsPubMux);
}

BalansunPublic BalansunReadSnapshot() {
  BalansunPublic out;
  portENTER_CRITICAL(&rmsPubMux);
  out = g_balansun_pub;
  portEXIT_CRITICAL(&rmsPubMux);
  return out;
}
