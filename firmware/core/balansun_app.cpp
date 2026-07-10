/*
 * balansun_app.cpp — balansun_setup/balansun_loop, balansun_metering_task (core 0), balansun_apply_surplus_regulation (core 1).
 * Initializes WiFi, EEPROM, triac ISRs, HTTP routes, MQTT, OTA, and the metering FreeRTOS task.
 * See: /en/project-overview/; triac ISRs in balansun_triac_isr.cpp (minimal ISR work only).
 */
#include "balansun_app.h"
#include "balansun_api_ready.h"
#include "balansun_diag.h"
#include "balansun_pwm.h"
#include "balansun_self_test.h"
#include "balansun_triac_calibration_logic.h"
#include "triac_api_shim.h"
#include "balansun_board.h"
#include "balansun_bench_serial.h"
#include "balansun_device_id.h"
#include "balansun_globals.h"
#include "balansun_pin_map.h"
#include "balansun_temperature.h"
#include "actions_logic.h"
#include "balansun_pulse_modes.h"
#include "balansun_regulation_logic.h"
#include "balansun_vacation_logic.h"
#include "balansun_source_health_logic.h"
#include "balansun_action_cap_logic.h"
#include "balansun_site_cap_logic.h"
#include "balansun_regulation_modes.h"
#include "balansun_regulation_state.h"
#include "tempo_rte_logic.h"
#include "balansun_forward.h"
#include "balansun_daily_energy_logic.h"
#include "metering/intraday_day_nvs.h"
#include "balansun_triac_isr.h"
#include "balansun_pub.h"
#include "balansun_reboot.h"
#include "balansun_persistence.h"
#include "balansun_psram.h"
#include "balansun_hw_profile.h"
#include "balansun_fixed_str.h"
#include "balansun_source.h"
#include "balansun_source_logic.h"
#include "metering/pmqtt_bindings.h"
#include "metering/victron_gx_mqtt.h"
#include "balansun_measurement.h"
#include "app_wifi_setup.h"
#include "captive_dns.h"
#include "storage_eeprom.h"
#include "tempo_rte.h"
#include "balansun_http_service.h"
#include "balansun_hw_presence.h"
#include "balansun_status_led.h"
#include "balansun_state_orchestrator.h"
#include "balansun_product_profile.h"

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp_arduino_version.h>
#include <esp_sntp.h>
#include <esp_task_wdt.h>

static unsigned long previousTempoPollMillis = 0;

static void balansun_watchdog_init(void) {
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  esp_task_wdt_config_t wdt_cfg = {
      .timeout_ms = (uint32_t)WDT_TIMEOUT_SEC * 1000u,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&wdt_cfg);
#else
  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
#endif
}

void balansun_metering_task(void *pvParameters) {
  (void)pvParameters;
  esp_task_wdt_add(nullptr);
  esp_task_wdt_reset();
  for (;;) {
    unsigned long tps = millis();
    float deltaT = float(tps - last_metering_task_at_ms);
    last_metering_task_at_ms = tps;
    metering_task_ms_min = min(metering_task_ms_min, deltaT);
    metering_task_ms_min = metering_task_ms_min + 0.001f;
    metering_task_ms_max = max(metering_task_ms_max, deltaT);
    metering_task_ms_max = metering_task_ms_max * 0.9999f;
    metering_task_ms_avg = deltaT * 0.001f + metering_task_ms_avg * 0.999f;

    if (tps - last_metering_task_ms > poll_period_ms) {
      last_metering_task_ms = tps;
      unsigned long pollBackoffMs = static_cast<unsigned long>(house_active_import_w / 10);
      balansun_source_run_poll_cycle(pollBackoffMs);
      BalansunPublishFromGlobals();
      balansun_measurement_refresh_last();
    }
    delay(2);
    esp_task_wdt_reset();
  }
}

void balansun_setup(void) {
  Serial.begin(115200);
#if CONFIG_IDF_TARGET_ESP32S3
  delay(300);
  balansun_bench_serial_begin();
  balansun_bench_log(F("Booting"));
#endif
  balansun_boot_mark_started();
  startMillis = millis();
  previousLEDsMillis = startMillis;

  pinMode(kZeroCrossGpio, INPUT_PULLDOWN);
  pinMode(kTriacDimGpio, OUTPUT);
  digitalWrite(kTriacDimGpio, LOW);

  balansun_watchdog_init();

#if !CONFIG_IDF_TARGET_ESP32S3
  Serial.println(F("Booting"));
#endif
  balansun_psram_log_boot();
  balansun_hw_profile_init();
  balansun_fixed_str_boot_init();

  for (int i = 0; i < kMaxRoutingActions; i++) {
    load_channels[i] = Action(i);
  }

  esp_task_wdt_reset();

  balansun_wifi_prepare_hostname();
  balansun_wifi_prepare_stack();

  eepromInit();
  eeprom_layout_key = kEepromLayoutInit;
  unsigned long Rcle = eepromReadLayoutKey();
  Serial.println(String(F("ROM key: ")) + String(Rcle));
  const bool eepromConfigLoaded = (Rcle == eeprom_layout_key);
  balansun_eeprom_config_loaded = eepromConfigLoaded;
  if (eepromConfigLoaded) {
    loadConfigFromEeprom();
    eepromLoadMorningDayEnergy();
    balansun_init_action_gpios();
  } else {
    eepromClearConsumptionHistory();
    balansun_pins_apply_boot();
    balansun_temperature_init_defaults();
  }
  String mqttDevName = String(MQTTdeviceName);
  if (balansun_apply_default_mqtt_device_name(mqttDevName) && eepromConfigLoaded) {
    MQTTdeviceName = mqttDevName;
    persistConfigToEeprom();
  }
  balansun_wifi_load_sta_from_nvs();
#if defined(BALANSUN_HIL_FORCE_AP) && BALANSUN_HIL_FORCE_AP
  ssid = "";
  password = "";
  balansun_wifi_clear_sta_profile();
  Serial.println(F("HIL test profile: forced setup AP (STA credentials cleared)"));
#endif
  balansun_active_source_refresh_from_global_string();
  balansun_status_led_init();

  sntp_set_time_sync_notification_cb(time_sync_notification);
  sntp_servermode_dhcp(1);
  configTzTime(TimeTz.c_str(), TimeNtp1.c_str(), TimeNtp2.c_str());

  /* Register HTTP routes before WiFi join so the setup AP can serve /wifi during STA attempts. */
  Init_Server();

  balansun_wifi_connect_sta_or_ap();
  balansun_http_ensure_listening();

#if BALANSUN_REMOTE_DEBUG
  Debug.begin("ESP32");
  Debug.println(F("Ready"));
  Debug.print(F("IP address: "));
  Debug.println(WiFi.localIP());
#endif

#ifndef METER_ONLY_BUILD
  balansun_regulation_state_init();
  balansun_pulse_modes_init_tables();
  balansun_triac_hw_init();
  balansun_self_test_on_boot();
#endif

  ArduinoOTA.setHostname(balansun_ap_ssid_storage.c_str());
  if (arduinoOtaPassword.length() > 0) {
    ArduinoOTA.setPassword(arduinoOtaPassword.c_str());
  }
  /* Flush any deferred config before the firmware image is replaced. */
  ArduinoOTA.setRebootOnSuccess(false);
  ArduinoOTA.onStart([]() { persistence_flush_all(); });
  ArduinoOTA.onEnd([]() { RequestReboot(500); });
  ArduinoOTA.begin();

  Serial.println(String(F("Source: ")) + Source.c_str());
  balansun_source_apply_hardware_setup();

  xTaskCreatePinnedToCore(balansun_metering_task, "balansun_metering_task", 10000, nullptr, 10, &Task1, 0);

  previousWifiMillis = millis() + 300000;
  previousHistoryMillis = millis() - 290000;
  previousTimer2sMillis = millis();
  previousTempoPollMillis = millis();
  previousLoop = millis();
  last_metering_task_at_ms = millis();
  previousMqttMillis = millis() - 5000;
  previousETX = millis();
  previousOverProdMillis = millis();
  if (balansun_active_source_get() == SourceId::JsyMk333) {
    last_metering_task_ms = millis();
  } else {
    last_metering_task_ms = millis() + 500;
  }
  balansun_boot_mark_complete();
}

void balansun_loop(void) {
  balansun_reboot_poll();
  balansun_triac_poll_hw();
  unsigned long tps = millis();
  balansun_hw_presence_tick(tps);
  balansun_temperature_service(tps);
  float deltaT = float(tps - previousLoop);
  previousLoop = tps;
  previousLoopMin = min(previousLoopMin, deltaT);
  previousLoopMin = previousLoopMin + 0.001f;
  previousLoopMax = max(previousLoopMax, deltaT);
  previousLoopMax = previousLoopMax * 0.9999f;
  previousLoopMoy = deltaT * 0.001f + previousLoopMoy * 0.999f;

  ArduinoOTA.handle();
#if BALANSUN_REMOTE_DEBUG
  Debug.handle();
#endif
  balansun_http_ensure_listening();
  if (balansun_wifi_soft_ap_setup_active()) {
    balansun_captive_dns_process();
  } else if (balansun_captive_dns_active()) {
    balansun_captive_dns_stop();
  }
  const int httpDrainMax = balansun_wifi_soft_ap_setup_active() ? 48 : 32;
  balansun_http_pump_server(server, httpDrainMax);
  eepromHistoryServicePendingCommit();
  persistence_service();

  if (tps - previousTempoPollMillis >= 2000) {
    previousTempoPollMillis = tps;
    tempo_rte_poll();
    LTARFbin = tempo_rte_logic_ltarf_bin(std::string(LTARF.c_str()));
  }

  if (balansun_active_source_get() == SourceId::Pmqtt) {
    pmqtt_mqtt_service_tick();
  }
  if (balansun_active_source_get() == SourceId::VictronGx) {
    victron_gx_mqtt_service_tick();
  }

  if (tps - previousHistoryMillis >= 300000) {
    previousHistoryMillis = tps;
    if (meter_reading_valid) {
      tabPwHouse_5mn[IdxStockPW] = house_active_import_w - house_active_export_w;
      tabPw_Triac_5mn[IdxStockPW] = second_active_import_w - second_active_export_w;
    } else {
      tabPwHouse_5mn[IdxStockPW] = 0;
      tabPw_Triac_5mn[IdxStockPW] = 0;
    }
    tabTemperature_5mn[IdxStockPW] = balansun_history_temperature_sample(temperature);
    if (meter_reading_valid || temperature > -100) {
      IdxStockPW = (IdxStockPW + 1) % kBalansunPwrHist5mnSlots;
    }
  }

  if (tps - previousTimer2sMillis >= 2000) {
    previousTimer2sMillis = tps;
    if (meter_reading_valid) {
      tabPwHouse_2s[IdxStock2s] = house_active_import_w - house_active_export_w;
      tabPw_Triac_2s[IdxStock2s] = second_active_import_w - second_active_export_w;
      tabPvaHouse_2s[IdxStock2s] = house_apparent_import_va - house_apparent_export_va;
      tabPva_Triac_2s[IdxStock2s] = second_apparent_import_va - second_apparent_export_va;
      balansun_on_clock_tick();
      balansun_daily_energy_tick();
      // ponytail: keep HTTP snapshot coherent with MQTT payload after day-energy tick.
      BalansunPublishFromGlobals();
    }
    tabTemperature_2s[IdxStock2s] = balansun_history_temperature_sample(temperature);
    if (meter_reading_valid || temperature > -100) {
      IdxStock2s = (IdxStock2s + 1) % kBalansunPwrHist2sSlots;
    }
    publishMqttLoop();
  }

#ifndef METER_ONLY_BUILD
  if (tps - previousOverProdMillis >= 200) {
    previousOverProdMillis = tps;
    balansun_self_test_tick(millis());
    if (meter_reading_valid || balansun_self_test_blocks_outputs()) {
      balansun_apply_surplus_regulation();
      if (meter_reading_valid) {
        balansun_diag_regulation_hunting_tick(millis(), TriacGetOpenPercent());
      }
      balansun_pwm_tick(TriacGetOpenPercent());
    }
  }
#else
  if (meter_reading_valid) {
    if (tps - previousOverProdMillis >= 200) {
      previousOverProdMillis = tps;
      balansun_state_orchestrator_tick_runtime();
    }
  }
#endif
  if (tps - previousLEDsMillis >= 50) {
    previousLEDsMillis = tps;
    balansun_update_status_leds();
  }
  /* Signed delta: `previousWifiMillis = millis() + delay` must not use unsigned subtraction. */
  if ((int32_t)(tps - previousWifiMillis) > 30000) {
    previousWifiMillis = tps;
    /* Non-blocking reconnect check: avoid blocking the web stack for seconds. */
    const bool sta_linked = ssid.length() > 0 && WiFi.status() == WL_CONNECTED &&
                            WiFi.localIP() != IPAddress(0, 0, 0, 0);
    if (sta_linked) {
      WIFIbug = 0;
      if (balansun_wifi_soft_ap_setup_active()) {
        balansun_captive_dns_stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        delay(50);
        balansun_http_invalidate_binding();
        balansun_wifi_start_mdns();
      }
      balansun_http_ensure_listening();
    } else if (ssid.length() == 0 && !balansun_wifi_soft_ap_setup_active()) {
      Serial.println(F("Setup AP lost — restarting soft AP."));
      balansun_wifi_connect_sta_or_ap();
      balansun_http_ensure_listening();
      WIFIbug = 0;
    } else if (ssid.length() > 0 && !balansun_wifi_soft_ap_setup_active()) {
      const uint32_t wifi_poll_deadline = millis() + 500;
      while (WiFi.status() != WL_CONNECTED && (int32_t)(millis() - wifi_poll_deadline) < 0) {
        balansun_http_pump_server(server, 4);
        delay(10);
        yield();
      }
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(String(F("Connection Failed! #")) + String(WIFIbug));
        WIFIbug++;
        if (WIFIbug > 2) {
          persistence_flush_all();
          ESP.restart();
        }
      } else {
        WIFIbug = 0;
      }
    }

    if (WiFi.getMode() != WIFI_STA) {
      Serial.print(F("Access Point Mode. IP address: "));
      Serial.println(WiFi.softAPIP());
#if CONFIG_IDF_TARGET_ESP32S3
      balansun_bench_log(String(F("AP IP: ")) + WiFi.softAPIP().toString());
#endif
    } else {
      Serial.print(F("WiFi RSSI (dBm): "));
      Serial.println(WiFi.RSSI());
      Serial.print(F("IP address: "));
      Serial.println(WiFi.localIP());
#if CONFIG_IDF_TARGET_ESP32S3
      balansun_bench_log(String(F("STA IP: ")) + WiFi.localIP().toString());
#endif
      Serial.print(F("WIFIbug:"));
      Serial.println(WIFIbug);
      Serial.print(F("meterPeerFailures:"));
      Serial.println(meterPeerFailures);
#if BALANSUN_REMOTE_DEBUG
      Debug.print(F("WiFi RSSI (dBm): "));
      Debug.println(WiFi.RSSI());
      Debug.print(F("WIFIbug:"));
      Debug.println(WIFIbug);
      Debug.print(F("meterPeerFailures:"));
      Debug.println(meterPeerFailures);
#endif
      Serial.println(String(F("Metering task load (core 0) ms — min: ")) + String(int(metering_task_ms_min)) +
                     F(" avg: ") + String(int(metering_task_ms_avg)) + F(" max: ") + String(int(metering_task_ms_max)));
#if BALANSUN_REMOTE_DEBUG
      Debug.println(String(F("Metering task load (core 0) ms — min: ")) + String(int(metering_task_ms_min)) +
                    F(" avg: ") + String(int(metering_task_ms_avg)) + F(" max: ") + String(int(metering_task_ms_max)));
#endif
      Serial.println(String(F("Main loop load (core 1) ms — min: ")) + String(int(previousLoopMin)) + F(" avg: ") +
                     String(int(previousLoopMoy)) + F(" max: ") + String(int(previousLoopMax)));
#if BALANSUN_REMOTE_DEBUG
      Debug.println(String(F("Main loop load (core 1) ms — min: ")) + String(int(previousLoopMin)) + F(" avg: ") +
                    String(int(previousLoopMoy)) + F(" max: ") + String(int(previousLoopMax)));
#endif
    }
    int T = int(millis() / 1000);
    float DureeOn = float(T) / 3600.0f;
    Serial.println(String(F("ESP32 uptime (hours): ")) + String(DureeOn));
#if BALANSUN_REMOTE_DEBUG
    Debug.println(String(F("ESP32 uptime (hours): ")) + String(DureeOn));
#endif
    balansun_poll_temperature();
    balansun_on_clock_tick();
  }
}

void balansun_init_action_gpios(void) {
  for (int i = 1; i < NbActions; i++) {
    if (action_regulation_enabled(load_channels[i].Actif)) {
      load_channels[i].InitGpio();
    }
  }
}

void balansun_daily_energy_reset_day_floors(void) {
  g_day_floor_house_import_wh = 0;
  g_day_floor_house_export_wh = 0;
  g_day_floor_second_import_wh = 0;
  g_day_floor_second_export_wh = 0;
}

void balansun_daily_energy_tick(void) {
  if (!time_sync_valid) return;
  const bool pmqttDayFromBroker =
      balansun_active_source_get() == SourceId::Pmqtt && pmqtt_bindings_provides_day_energy();
  const bool defer_ch2_tick =
      balansun_active_source_get() == SourceId::VictronGx && !g_second_channel_meter_valid_this_boot;
  if (!pmqttDayFromBroker) {
    house_day_energy_import_wh = balansun_daily_energy_wh(static_cast<long>(house_energy_import_wh),
                                                          EAS_M_J0,
                                                          g_day_floor_house_import_wh);
    house_day_energy_export_wh = balansun_daily_energy_wh(static_cast<long>(house_energy_export_wh),
                                                          EAI_M_J0,
                                                          g_day_floor_house_export_wh);
    if (!defer_ch2_tick) {
      second_day_energy_import_wh = balansun_daily_energy_wh(static_cast<long>(second_energy_import_wh),
                                                             EAS_T_J0,
                                                             g_day_floor_second_import_wh);
      second_day_energy_export_wh = balansun_daily_energy_wh(static_cast<long>(second_energy_export_wh),
                                                             EAI_T_J0,
                                                             g_day_floor_second_export_wh);
    }
    if (!currentDateStr.empty()) {
      intraday_day_nvs_maybe_save(currentDateStr.c_str(),
                                  static_cast<long>(house_day_energy_import_wh),
                                  static_cast<long>(house_day_energy_export_wh),
                                  static_cast<long>(second_day_energy_import_wh),
                                  static_cast<long>(second_day_energy_export_wh));
    }
    return;
  }
  /* Pmqtt: day energies come from MQTT bindings; still track J0 anchors from cumulative totals. */
  (void)balansun_daily_energy_wh(static_cast<long>(house_energy_import_wh), EAS_M_J0, g_day_floor_house_import_wh);
  (void)balansun_daily_energy_wh(static_cast<long>(house_energy_export_wh), EAI_M_J0, g_day_floor_house_export_wh);
  if (!defer_ch2_tick) {
    (void)balansun_daily_energy_wh(static_cast<long>(second_energy_import_wh), EAS_T_J0,
                                   g_day_floor_second_import_wh);
    (void)balansun_daily_energy_wh(static_cast<long>(second_energy_export_wh), EAI_T_J0,
                                   g_day_floor_second_export_wh);
  }
}

void time_sync_notification(struct timeval *tv) {
  (void)tv;
  Serial.println(F("Time synchronization event"));
  time_sync_valid = true;
  balansun_on_clock_tick();
}

void balansun_poll_temperature(void) { balansun_temperature_poll(); }

void balansun_update_status_leds(void) {
  cptLEDyellow++;

  if (balansun_reboot_pending()) {
    balansun_status_led_apply_overlay(StatusLedOverlay::Reboot, static_cast<unsigned>(cptLEDyellow));
    return;
  }
  if (balansun_wifi_soft_ap_setup_active()) {
    balansun_status_led_apply_overlay(StatusLedOverlay::ApSetup, static_cast<unsigned>(cptLEDyellow));
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (WiFi.getMode() == WIFI_STA) {
      cptLEDyellow = (cptLEDyellow + 6) % 10;
      cptLEDgreen = cptLEDyellow;
    } else {
      cptLEDyellow = cptLEDyellow % 10;
      cptLEDgreen = (cptLEDyellow + 5) % 10;
    }
  } else {
    if (triac_delay_percent < 100) {
      cptLEDgreen = int((cptLEDgreen + 1 + 8 / (1 + triac_delay_percent / 10))) % 10;
    } else {
      cptLEDgreen = 10;
    }
  }
  balansun_status_led_apply_live(cptLEDyellow, cptLEDgreen);
}
