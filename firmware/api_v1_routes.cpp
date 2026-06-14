/*
 * api_v1_routes.cpp — register REST /api/v1 routes (handlers in firmware/api/*).
 */
#include <WebServer.h>
#include "api.h"
#include "api_v1_common.h"
#include "balansun_product_profile.h"

extern WebServer server;

void Init_ApiRoutes() {
  server.on("/api/v1/measurements", HTTP_GET, handle_get_measurements);
  server.on("/api/v1/telemetry/snapshot", HTTP_GET, handle_get_telemetry_snapshot);
  server.on("/api/v1/triac/override", HTTP_POST, handle_post_triac_override);
  server.on("/api/v1/tariff/tempo", HTTP_GET, handle_get_tariff_tempo);
  server.on("/api/v1/system", HTTP_GET, handle_get_system);
  server.on("/api/v1/system/audit", HTTP_GET, handle_get_system_audit);
  server.on("/api/v1/device", HTTP_GET, handle_get_device);
  server.on("/api/v1/state", HTTP_GET, handle_get_state);
  server.on("/api/v1/health", HTTP_GET, handle_get_health);
  server.on("/api/v1/health/self-test/run", HTTP_POST, handle_post_health_self_test_run);
  server.on("/api/v1/health/self-test/skip", HTTP_POST, handle_post_health_self_test_skip);
#if defined(BALANSUN_ENABLE_SOURCE_TEST_API)
  server.on("/api/v1/health/self-test/simulate", HTTP_POST, handle_post_health_self_test_simulate);
#endif
  server.on("/api/v1/health/hardware/recheck", HTTP_POST, handle_post_health_hardware_recheck);
  server.on("/api/v1/hardware/status-led/test", HTTP_POST, handle_post_hardware_status_led_test);
  server.on("/api/v1/public", HTTP_GET, handle_get_public);
  server.on("/api/v1/auth/login", HTTP_POST, handle_post_auth_login);
  server.on("/api/v1/auth/logout", HTTP_POST, handle_post_auth_logout);
  server.on("/api/v1/auth/tokens", HTTP_GET, handle_get_auth_tokens);
  server.on("/api/v1/auth/tokens", HTTP_POST, handle_post_auth_tokens);

  server.on("/api/v1/sources", HTTP_GET, handle_get_sources);
  server.on("/api/v1/sources/brute_panel", HTTP_GET, handle_get_sources_brute_panel);
  server.on("/api/v1/config", HTTP_GET, handle_get_config);
  server.on("/api/v1/config", HTTP_PUT, handle_put_config);
  server.on("/api/v1/config", HTTP_PATCH, handle_patch_config);
  server.on("/api/v1/actions", HTTP_GET, handle_get_actions_live);
  server.on("/api/v1/actions/schema", HTTP_GET, handle_get_actions_schema);
  server.on("/api/v1/actions/config", HTTP_GET, handle_get_actions_config);
  server.on("/api/v1/actions/config", HTTP_PUT, handle_put_actions_config);
  server.on("/api/v1/actions/config", HTTP_PATCH, handle_patch_actions_config_batch);

  server.on("/api/v1/history/power", HTTP_GET, handle_get_history_power);
  server.on("/api/v1/history/energy/daily", HTTP_GET, handle_get_history_energy_daily);
  server.on("/api/v1/history/energy/daily/import", HTTP_POST, handle_post_history_energy_daily_import);
  server.on("/api/v1/gpio", HTTP_GET, handle_get_gpio);
  server.on("/api/v1/gpio", HTTP_PUT, handle_put_gpio);
  server.on("/api/v1/pwm", HTTP_GET, handle_get_pwm);
  server.on("/api/v1/pwm", HTTP_PUT, handle_put_pwm);

  server.on("/api/v1/fleet/export", HTTP_GET, handle_get_fleet_export);
  server.on("/api/v1/fleet/import", HTTP_POST, handle_post_fleet_import);
  server.on("/api/v1/fleet/trust-key", HTTP_PUT, handle_put_fleet_trust_key);

  server.on("/api/v1/system/reboot", HTTP_POST, handle_post_reboot);
  server.on("/api/v1/system/pins", HTTP_GET, handle_get_system_pins);
  server.on("/api/v1/system/pins", HTTP_PUT, handle_put_system_pins);
  server.on("/api/v1/system/pins/reset", HTTP_POST, handle_post_pins_reset);
  server.on("/api/v1/wifi", HTTP_GET, handle_get_wifi);
  server.on("/api/v1/wifi", HTTP_PUT, handle_put_wifi);
  server.on("/api/v1/wifi/scan", HTTP_GET, handle_get_wifi_scan);
  server.on("/api/v1/system/factory-reset", HTTP_POST, handle_post_factory_reset);
  server.on("/api/v1/system/save-now", HTTP_POST, handle_post_save_now);
  server.on("/api/v1/system/eeprom", HTTP_GET, handle_get_eeprom);
  server.on("/api/v1/system/arduino-ota", HTTP_GET, handle_get_system_arduino_ota);
  server.on("/api/v1/system/arduino-ota", HTTP_PUT, handle_put_system_arduino_ota);
  server.on("/api/v1/system/http-auth", HTTP_PUT, handle_put_system_http_auth);
  server.on("/api/v1/system/backup", HTTP_GET, handle_get_system_backup);
  server.on("/api/v1/system/backup", HTTP_PUT, handle_put_system_backup);
  server.on("/api/v1/time", HTTP_GET, handle_get_time);
  server.on("/api/v1/time", HTTP_PUT, handle_put_time);
  server.on("/api/v1/firmware/ota", HTTP_POST, handle_post_firmware_ota_done, handle_firmware_ota_upload);
  server.on("/api/v1/sources/diagnostics", HTTP_GET, handle_get_sources_diagnostics);
  server.on("/api/v1/history/reset", HTTP_POST, handle_post_history_reset);

  server.on("/api/v1/mqtt/discover", HTTP_POST, handle_post_mqtt_discover);
  server.on("/api/v1/mqtt/reconnect", HTTP_POST, handle_post_mqtt_reconnect);
  server.on("/api/v1/mqtt/publish-now", HTTP_POST, handle_post_mqtt_publish_now);
  server.on("/api/v1/mqtt/test", HTTP_POST, handle_post_mqtt_test);
  server.on("/api/v1/sources/pmqtt/preview", HTTP_POST, handle_post_pmqtt_preview);
#if defined(BALANSUN_ENABLE_SOURCE_TEST_API)
  server.on("/api/v1/sources/test/inject", HTTP_POST, handle_post_sources_test_inject);
#endif
  server.on("/api/v1/openapi.json", HTTP_GET, handle_get_openapi);
}
