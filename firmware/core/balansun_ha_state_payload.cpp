#include "balansun_ha_state_payload.h"

#include "balansun_device_id.h"
#include "balansun_globals.h"
#include "balansun_source_logic.h"
#include "balansun_diag.h"
#include "triac_api_shim.h"
#include "balansun_source.h"
#include "balansun_source_health_logic.h"
#include "balansun_triac_isr.h"
#include "balansun_vacation_logic.h"
#include "mqtt_ha_command_logic.h"
#include "tempo_rte_logic.h"
#include "api.h"
#include "balansun_temperature.h"
#include "balansun_daily_energy_logic.h"

int balansun_compute_source_health_score(void) { return balansun_source_health_score_now(); }

bool balansun_source_health_is_stale(int health_score) {
  return balansun_source_health_logic_is_stale(health_score);
}

void balansun_append_measurements_diagnostics(JsonObject diag) {
  if (balansun_diag_uxi_adc_clipping_active()) {
    diag["adc_clipping"] = true;
  }
  if (g_regulation_hunting_active) {
    diag["regulation_hunting"] = true;
  }
  const int health = balansun_compute_source_health_score();
  diag["source_health"] = health;
  diag["source_stale"] = balansun_source_health_is_stale(health);
  const int triac_open = TriacGetOpenPercent();
  diag["regulation_active"] = triac_open > 5;
  diag["site_cap_active"] = siteCapActive;
  diag["mqtt_connected"] = clientMQTT.connected();
}

void balansun_append_ha_state_payload(JsonObject doc) {
  doc["source"] = Source;
  doc["device_uid"] = balansun_device_uid();
  long houseYesterdayImportWh = 0;
  long houseYesterdayExportWh = 0;
  long secondYesterdayImportWh = 0;
  long secondYesterdayExportWh = 0;
  balansun_yesterday_daily_metrics(&houseYesterdayImportWh, &houseYesterdayExportWh,
                                   &secondYesterdayImportWh, &secondYesterdayExportWh);
  if (balansun_source_logic_second_channel_snapshot_visible(
          second_voltage_v, second_active_import_w, second_active_export_w, second_current_a)) {
    doc["second_active_import_w"] = second_active_import_w;
    doc["second_active_export_w"] = second_active_export_w;
    doc["second_voltage_v"] = second_voltage_v;
    doc["second_current_a"] = second_current_a;
    doc["second_power_factor"] = second_power_factor;
    doc["second_energy_import_wh"] = second_energy_import_wh;
    doc["second_energy_export_wh"] = second_energy_export_wh;
    doc["second_day_energy_import_wh"] = second_day_energy_import_wh;
    doc["second_day_energy_export_wh"] = second_day_energy_export_wh;
    doc["second_routed_day_energy_wh"] =
        balansun_routed_day_energy_wh(second_day_energy_import_wh, second_day_energy_export_wh);
    doc["second_yesterday_energy_import_wh"] = secondYesterdayImportWh;
    doc["second_yesterday_energy_export_wh"] = secondYesterdayExportWh;
    doc["second_apparent_import_va"] = second_apparent_import_va;
    doc["second_apparent_export_va"] = second_apparent_export_va;
    doc["mains_frequency_hz"] = mains_frequency_hz;
  }
  balansun_temperature_set_primary_c_json(doc);
  for (int s = 0; s < kBalansunTempMaxSensors; s++) {
    const BalansunTempSlotState &st = g_temperature_slot_states[s];
    if (!st.config.enabled || !st.reading.valid) continue;
    if (st.primary) continue;
    char key[24];
    snprintf(key, sizeof(key), "temperature_slot_%d_c", s);
    doc[key] = st.reading.c;
  }
  if (balansun_cap_mqtt_linky_tariff()) {
    doc["linky_ltarf"] = LTARF;
    doc["tariff_code"] = tempo_rte_logic_tariff_code(std::string(LTARF.c_str()));
  }
  if (tempoRteEnabled) {
    doc["rte_today"] = rte_today;
    doc["rte_tomorrow"] = rte_tomorrow;
  }
  doc["house_net_power_w"] = house_active_import_w - house_active_export_w;
  doc["house_active_import_w"] = house_active_import_w;
  doc["house_active_export_w"] = house_active_export_w;
  doc["house_voltage_v"] = house_voltage_v;
  doc["house_current_a"] = house_current_a;
  doc["house_power_factor"] = house_power_factor;
  doc["house_energy_import_wh"] = house_energy_import_wh;
  doc["house_energy_export_wh"] = house_energy_export_wh;
  doc["house_day_energy_import_wh"] = house_day_energy_import_wh;
  doc["house_day_energy_export_wh"] = house_day_energy_export_wh;
  doc["house_yesterday_energy_import_wh"] = houseYesterdayImportWh;
  doc["house_yesterday_energy_export_wh"] = houseYesterdayExportWh;
  doc["house_apparent_import_va"] = house_apparent_import_va;
  doc["house_apparent_export_va"] = house_apparent_export_va;

  doc["triac_open_percent"] = TriacGetOpenPercent();
  doc["adc_clipping"] = balansun_diag_uxi_adc_clipping_active() ? "ON" : "OFF";
  doc["regulation_hunting"] = g_regulation_hunting_active ? "ON" : "OFF";
  const int health = balansun_compute_source_health_score();
  doc["source_health"] = health;
  doc["source_stale"] = balansun_source_health_is_stale(health) ? "ON" : "OFF";
  const int triac_open = TriacGetOpenPercent();
  doc["regulation_active"] = triac_open > 5 ? "ON" : "OFF";
  doc["mqtt_connected"] = clientMQTT.connected() ? "ON" : "OFF";
  doc["site_cap_active"] = siteCapActive ? "ON" : "OFF";
  doc["heater_load_backoff_active"] = heaterLoadBackoffActive ? "ON" : "OFF";
  doc["max_routed_w"] = maxRoutedW;
  const uint32_t now_epoch = static_cast<uint32_t>(time(NULL));
  doc["vacation"] =
      balansun_vacation_logic_active(vacationEnabled, vacationEndEpoch, now_epoch) ? "ON" : "OFF";
  char action_key[20];
  for (int i = 0; i < NbActions; i++) {
    snprintf(action_key, sizeof(action_key), "action_%d", i);
    doc[action_key] = load_channels[i].On ? "ON" : "OFF";
  }
}

void balansun_append_mqtt_ha_state_payload(JsonObject doc) {
  balansun_append_ha_state_payload(doc);
  // ponytail: REST omits invalid aggregate temperature_c; MQTT keeps the pre-d186256 key when > -100.
  if (!doc["temperature_c"].is<float>() && temperature > -100.0f) {
    doc["temperature_c"] = temperature;
  }
}

bool balansun_apply_triac_command(const char *msg, String &err) {
  if (!msg) {
    err = "empty command";
    return false;
  }
  MqttTriacCmd triacCmd;
  if (!mqtt_ha_command_parse_triac(msg, &triacCmd)) {
    err = "invalid triac command (use AUTO or 0-100)";
    return false;
  }
  if (triacCmd.kind == MqttTriacCmdKind::Auto) {
    return ApiSetActionOverride(0, "auto", 0, 0, err);
  }
  return ApiSetActionOverride(0, "triac_fixed", triacCmd.percent, 0, err);
}
