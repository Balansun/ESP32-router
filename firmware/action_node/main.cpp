#include "action_node_app.h"

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <time.h>

ESP8266WebServer g_server(80);
ActionNodeAppState g_state;

void action_node_http_register_routes(void);
void action_node_temperature_begin(void);
void action_node_poll_temperature(ActionNodeAppState &state);

namespace {

constexpr char kApSsid[] = "Balansun-Action";

void setup_time(void) {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void connect_wifi(ActionNodeAppState &state) {
  if (state.wifi_ssid[0]) {
    WiFi.mode(WIFI_STA);
    WiFi.hostname("balansun-action");
    WiFi.begin(state.wifi_ssid, state.wifi_password);
    for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
      delay(250);
    }
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(kApSsid);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);
  action_node_app_init(g_state);
  g_state.config.wiring = action_node_build_wiring();
  action_node_app_save_config(g_state);
  connect_wifi(g_state);
  setup_time();
  action_node_temperature_begin();
  action_node_http_register_routes();
  g_server.begin();
  action_node_tick(g_state, millis());
  Serial.println("Balansun action node ready");
}

void loop() {
  g_server.handleClient();
  static unsigned long last_temp = 0;
  static unsigned long last_tick = 0;
  const unsigned long now = millis();
  if (now - last_temp >= 5000UL) {
    last_temp = now;
    action_node_poll_temperature(g_state);
  }
  if (now - last_tick >= 1000UL) {
    last_tick = now;
    action_node_tick(g_state, now);
  }
}
