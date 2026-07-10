#include "balansun_hw_presence.h"

#include "balansun_app.h"
#include "balansun_board.h"
#include "balansun_forward.h"
#include "balansun_meter_sources_enable.h"
#include "balansun_globals.h"
#include "balansun_source.h"
#include "balansun_source_types.h"
#include "balansun_triac_isr.h"
#include "balansun_temperature.h"
#include "triac_api_shim.h"

#include <esp_task_wdt.h>
#include <string.h>

namespace {

constexpr uint8_t kMissStreakWarn = 8;
constexpr uint16_t kZcEdgesMinPerSec = 20;

unsigned long g_last_zc_sample_ms = 0;
uint16_t g_zc_edges_per_sec = 0;
bool g_zc_isr_attached = false;
int g_zc_poll_prev_level = LOW;
uint16_t g_zc_poll_edges_sec = 0;
uint8_t g_jsy_miss_streak = 0;
bool g_jsy_uart_ready = false;
bool g_jsy_uart_attempted = false;
uint8_t g_temp_miss_streak = 0;

const char *state_ok() { return "ok"; }
const char *state_missing() { return "missing"; }
const char *state_paused() { return "paused"; }
const char *state_na() { return "not_applicable"; }

void write_item(JsonObject o, const char *id, const char *role, const char *state, const char *detail,
                int gpio = -1, int gpio_rx = -1, int gpio_tx = -1) {
  o["id"] = id;
  o["role"] = role;
  o["state"] = state;
  if (detail && detail[0]) {
    o["detail"] = detail;
  }
  if (gpio >= 0) {
    o["gpio"] = gpio;
  }
  if (gpio_rx >= 0) {
    o["gpio_rx"] = gpio_rx;
  }
  if (gpio_tx >= 0) {
    o["gpio_tx"] = gpio_tx;
  }
}

void append_item(JsonArray items, const char *id, const char *role, const char *state, const char *detail,
                 int gpio = -1, int gpio_rx = -1, int gpio_tx = -1) {
  JsonObject o = items.add<JsonObject>();
  write_item(o, id, role, state, detail, gpio, gpio_rx, gpio_tx);
}

bool zc_expected() {
#ifndef METER_ONLY_BUILD
  return true;
#else
  return false;
#endif
}

bool jsy_source_active() {
#if BALANSUN_ENABLE_SOURCE_JSY_MK194
  const SourceId eff = balansun_effective_meter_id();
  if (eff == SourceId::JsyMk194) {
    return true;
  }
#if BALANSUN_ENABLE_SOURCE_VictronGx
  if (balansun_active_source_get() == SourceId::VictronGx) {
    return true;
  }
#endif
#endif
  return false;
}

void sample_zero_cross_window() {
  g_last_zc_sample_ms = 0;
  g_zc_poll_edges_sec = 0;
  g_zc_poll_prev_level = digitalRead(kZeroCrossGpio);
  const unsigned long start = millis();
  while ((millis() - start) < 1100UL) {
    balansun_hw_presence_tick(millis());
    delay(10);
    esp_task_wdt_reset();
  }
}

void append_zero_cross_item(JsonObject o) {
#ifndef METER_ONLY_BUILD
  const bool zc_ok = g_zc_edges_per_sec >= kZcEdgesMinPerSec;
  write_item(o, "zero_cross", "zero_cross", zc_ok ? state_ok() : state_missing(),
             zc_ok ? "Mains zero-cross detected" : "No zero-cross edges on GPIO (check Z-C wiring)",
             kZeroCrossGpio);
#endif
}

void append_triac_item(JsonObject o) {
#ifndef METER_ONLY_BUILD
  const bool zc_ok = g_zc_edges_per_sec >= kZcEdgesMinPerSec;
  const bool triac_ok = zc_ok && zc_sync_state > 0;
  write_item(o, "triac_dim", "triac_dim", triac_ok ? state_ok() : (zc_ok ? state_paused() : state_missing()),
             triac_ok ? "Triac timing synchronized to mains"
                      : (zc_ok ? "Triac dimmer idle until zero-cross sync"
                               : "Triac regulation paused without zero-cross"),
             kTriacDimGpio);
#endif
}

void append_jsy_item(JsonObject o) {
  if (jsy_source_active()) {
    if (!g_jsy_uart_attempted) {
      write_item(o, "jsy_mk194", "jsy_mk194", state_paused(), "JSY UART pending first meter poll", -1, RXD2, TXD2);
    } else if (!g_jsy_uart_ready) {
#if CONFIG_IDF_TARGET_ESP32S3
      write_item(o, "jsy_mk194", "jsy_mk194", state_paused(),
                 "JSY UART paused on ESP32-S3 DevKit (Serial2.begin trips WDT); use NotDef on bench", -1, RXD2,
                 TXD2);
#else
      write_item(o, "jsy_mk194", "jsy_mk194", state_missing(), "Serial2 init failed (RX/TX pins)", -1, RXD2,
                 TXD2);
#endif
    } else if (g_jsy_miss_streak >= kMissStreakWarn) {
      write_item(o, "jsy_mk194", "jsy_mk194", state_missing(), "No JSY-MK-194 Modbus response on UART", -1, RXD2,
                 TXD2);
    } else {
      write_item(o, "jsy_mk194", "jsy_mk194", state_ok(), "JSY meter responding or awaiting first frame", -1, RXD2,
                 TXD2);
    }
  } else {
    write_item(o, "jsy_mk194", "jsy_mk194", state_na(), "Source is not JsyMk194", -1, RXD2, TXD2);
  }
}

void append_temp_item(JsonObject o) {
  const int gpio = balansun_temperature_effective_gpio();
  if (!balansun_temperature_bus_should_run()) {
    write_item(o, "temp_ds18b20", "temp_ds18b20", state_na(), "Temperature bus disabled (all slots off)", gpio);
    return;
  }
  char detail[96];
  const int active = balansun_temperature_active_valid_count();
  const float primary_c = balansun_temp_logic_primary_c(g_temperature_slot_states);
  if (balansun_temp_logic_is_valid_c(primary_c)) {
    snprintf(detail, sizeof(detail), "%.1f C · %d/%d active on GPIO %d (%d discovered)", primary_c, active,
             kBalansunTempMaxSensors, gpio, g_temperature_discovered_count);
  } else {
    snprintf(detail, sizeof(detail), "%d/%d active on GPIO %d (%d discovered)", active, kBalansunTempMaxSensors, gpio,
             g_temperature_discovered_count);
  }
  if (g_temp_miss_streak >= kMissStreakWarn) {
    write_item(o, "temp_ds18b20", "temp_ds18b20", state_missing(),
               active > 0 ? detail : "No DS18B20 reading on 1-Wire (sensor optional)", gpio);
  } else {
    write_item(o, "temp_ds18b20", "temp_ds18b20", state_ok(), detail, gpio);
  }
}

}  // namespace

void balansun_hw_presence_tick(unsigned long now_ms) {
  if (g_last_zc_sample_ms == 0 || (now_ms - g_last_zc_sample_ms) >= 1000UL) {
    if (g_zc_isr_attached) {
      int in_d = 0;
      int raw = 0;
      TriacReadAndResetCounters(in_d, raw);
      g_zc_edges_per_sec = (uint16_t)(in_d < 0 ? 0 : in_d);
    } else {
      g_zc_edges_per_sec = g_zc_poll_edges_sec;
      g_zc_poll_edges_sec = 0;
      if (g_zc_edges_per_sec >= kZcEdgesMinPerSec) {
        balansun_triac_enable_zc_interrupt();
        g_zc_isr_attached = true;
      }
    }
    g_last_zc_sample_ms = now_ms;
  }
#ifndef METER_ONLY_BUILD
  if (!g_zc_isr_attached) {
    const int lvl = digitalRead(kZeroCrossGpio);
    if (lvl == HIGH && g_zc_poll_prev_level == LOW && g_zc_poll_edges_sec < 60000) {
      g_zc_poll_edges_sec++;
    }
    g_zc_poll_prev_level = lvl;
  }
#endif
}

void balansun_hw_presence_on_jsy_poll(bool uart_ready, bool frame_ok) {
  g_jsy_uart_attempted = true;
  g_jsy_uart_ready = uart_ready;
  if (frame_ok) {
    g_jsy_miss_streak = 0;
  } else if (g_jsy_miss_streak < 255) {
    g_jsy_miss_streak++;
  }
}

void balansun_hw_presence_on_temp_poll(bool ok) {
  if (ok) {
    g_temp_miss_streak = 0;
  } else if (g_temp_miss_streak < 255) {
    g_temp_miss_streak++;
  }
}

void balansun_hw_presence_append_json(JsonObject root) {
  JsonObject hw = root["hardware"].to<JsonObject>();
  JsonArray items = hw["items"].to<JsonArray>();

#ifndef METER_ONLY_BUILD
  if (zc_expected()) {
    JsonObject zc = items.add<JsonObject>();
    append_zero_cross_item(zc);
    JsonObject triac = items.add<JsonObject>();
    append_triac_item(triac);
  }
#endif

  JsonObject jsy = items.add<JsonObject>();
  append_jsy_item(jsy);

  JsonObject temp = items.add<JsonObject>();
  append_temp_item(temp);
}

bool balansun_hw_presence_recheck(const char *id, JsonObject out_item) {
  if (!id || !id[0]) {
    return false;
  }

  if (strcmp(id, "zero_cross") == 0) {
#ifndef METER_ONLY_BUILD
    if (!zc_expected()) {
      return false;
    }
    sample_zero_cross_window();
    append_zero_cross_item(out_item);
    return true;
#else
    return false;
#endif
  }

  if (strcmp(id, "triac_dim") == 0) {
#ifndef METER_ONLY_BUILD
    if (!zc_expected()) {
      return false;
    }
    sample_zero_cross_window();
    append_triac_item(out_item);
    return true;
#else
    return false;
#endif
  }

  if (strcmp(id, "jsy_mk194") == 0) {
#if BALANSUN_ENABLE_SOURCE_JSY_MK194
    if (!jsy_source_active()) {
      return false;
    }
    g_jsy_miss_streak = 0;
    jsy_mk194t_poll();
    append_jsy_item(out_item);
    return true;
#else
    return false;
#endif
  }

  if (strcmp(id, "temp_ds18b20") == 0) {
    balansun_poll_temperature();
    append_temp_item(out_item);
    return true;
  }

  return false;
}
