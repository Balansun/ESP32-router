#include "balansun_status_led.h"

#include "balansun_board.h"
#include "balansun_globals.h"
#include "balansun_temperature.h"

#include <ArduinoJson.h>
#include <Arduino.h>
#if __has_include("esp32-hal-rmt.h")
#include "esp32-hal-rmt.h"
#define BALANSUN_HAS_RGB_RMT 1
#else
#define BALANSUN_HAS_RGB_RMT 0
#endif
#if CONFIG_IDF_TARGET_ESP32S3 && __has_include("esp32-hal-rgb-led.h")
#include "esp32-hal-rgb-led.h"
#define BALANSUN_HAS_RGB_BUILTIN 1
#else
#define BALANSUN_HAS_RGB_BUILTIN 0
#endif

namespace {

StatusLedConfig g_cfg;
unsigned long g_test_until_ms = 0;
StatusLedTestRole g_test_role = StatusLedTestRole::Activity;
StatusLedConfig g_test_cfg;
unsigned g_test_tick = 0;
bool g_cfg_pending_reinit = false;

#if BALANSUN_HAS_RGB_RMT || BALANSUN_HAS_RGB_BUILTIN
StatusLedRgb g_rgb_last_sent;
bool g_rgb_last_valid = false;

bool uses_s3_builtin_rgb(int rgb_gpio) {
#if CONFIG_IDF_TARGET_ESP32S3
  return rgb_gpio < 0 || rgb_gpio == kS3OnboardRgbGpio;
#else
  (void)rgb_gpio;
  return false;
#endif
}

uint8_t scale_rgb_brightness(uint8_t v) {
#ifndef RGB_BRIGHTNESS
#define RGB_BRIGHTNESS 64
#endif
  return static_cast<uint8_t>((static_cast<unsigned>(v) * static_cast<unsigned>(RGB_BRIGHTNESS)) / 255U);
}
#endif

#if BALANSUN_HAS_RGB_RMT
rmt_obj_t *g_rgb_rmt = nullptr;
int g_rgb_rmt_pin = -1;

int resolve_rgb_pin(int rgb_gpio) { return rgb_gpio; }

void rgb_rmt_shutdown(void) {
  if (g_rgb_rmt != nullptr) {
    rmtDeinit(g_rgb_rmt);
    g_rgb_rmt = nullptr;
  }
  g_rgb_rmt_pin = -1;
  g_rgb_last_valid = false;
}

bool rgb_rmt_ensure(int pin) {
  if (pin < 0) return false;
  if (g_rgb_rmt != nullptr && g_rgb_rmt_pin == pin) return true;
  rgb_rmt_shutdown();
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  g_rgb_rmt = rmtInit(pin, RMT_TX_MODE, RMT_MEM_64);
  if (g_rgb_rmt == nullptr) return false;
  rmtSetTick(g_rgb_rmt, 100);
  g_rgb_rmt_pin = pin;
  g_rgb_last_valid = false;
  return true;
}

void encode_ws2812_grb(uint8_t green_val, uint8_t red_val, uint8_t blue_val, rmt_data_t *led_data) {
  const int color[] = {green_val, red_val, blue_val};
  int i = 0;
  for (int col = 0; col < 3; col++) {
    for (int bit = 0; bit < 8; bit++) {
      if ((color[col] & (1 << (7 - bit))) != 0) {
        led_data[i].level0 = 1;
        led_data[i].duration0 = 8;
        led_data[i].level1 = 0;
        led_data[i].duration1 = 4;
      } else {
        led_data[i].level0 = 1;
        led_data[i].duration0 = 4;
        led_data[i].level1 = 0;
        led_data[i].duration1 = 8;
      }
      i++;
    }
  }
}
#endif

void write_gpio(int gpio, bool active_on, bool active_low) {
  if (gpio < 0) return;
  const int level = active_on ? (active_low ? LOW : HIGH) : (active_low ? HIGH : LOW);
  digitalWrite(gpio, level);
}

void write_rgb(int rgb_gpio, const StatusLedRgb &rgb) {
#if BALANSUN_HAS_RGB_RMT || BALANSUN_HAS_RGB_BUILTIN
  if (g_rgb_last_valid && g_rgb_last_sent.r == rgb.r && g_rgb_last_sent.g == rgb.g && g_rgb_last_sent.b == rgb.b) {
    return;
  }
#if BALANSUN_HAS_RGB_BUILTIN
  if (uses_s3_builtin_rgb(rgb_gpio)) {
    const uint8_t r = scale_rgb_brightness(rgb.r);
    const uint8_t g = scale_rgb_brightness(rgb.g);
    const uint8_t b = scale_rgb_brightness(rgb.b);
#if defined(RGB_BUILTIN)
    neopixelWrite(RGB_BUILTIN, r, g, b);
#else
    neopixelWrite(static_cast<uint8_t>(kS3OnboardRgbGpio), r, g, b);
#endif
    g_rgb_last_sent = rgb;
    g_rgb_last_valid = true;
    return;
  }
#endif
#if BALANSUN_HAS_RGB_RMT
  const int pin = resolve_rgb_pin(rgb_gpio);
  if (pin < 0) return;
  if (!rgb_rmt_ensure(pin)) return;
  rmt_data_t led_data[24];
  encode_ws2812_grb(rgb.g, rgb.r, rgb.b, led_data);
  if (!rmtWriteBlocking(g_rgb_rmt, led_data, 24)) return;
  g_rgb_last_sent = rgb;
  g_rgb_last_valid = true;
#endif
#else
  (void)rgb_gpio;
  (void)rgb;
#endif
}

void apply_gpio_output(const StatusLedConfig &cfg, const StatusLedGpioOutput &out) {
  write_gpio(cfg.gpio_activity, out.activity_on, cfg.active_low);
  write_gpio(cfg.gpio_regulation, out.regulation_on, cfg.active_low);
}

void apply_rgb_output(const StatusLedConfig &cfg, const StatusLedRgb &rgb) {
  write_rgb(cfg.rgb_gpio, rgb);
}

void all_off(const StatusLedConfig &cfg) {
  if (cfg.mode == StatusLedMode::DualGpio) {
    apply_gpio_output(cfg, StatusLedGpioOutput{});
  } else if (cfg.mode == StatusLedMode::RgbWs2812) {
    apply_rgb_output(cfg, StatusLedRgb(0, 0, 0));
  }
}

}  // namespace

StatusLedBoardPins balansun_status_led_board_pins(void) {
  StatusLedBoardPins pins;
  pins.triac_dim = kTriacDimGpio;
  pins.zero_cross = kZeroCrossGpio;
  pins.uart_rx = RXD2;
  pins.uart_tx = TXD2;
  pins.temp = balansun_temperature_effective_gpio();
  pins.analog0 = AnalogIn0;
  pins.analog1 = AnalogIn1;
  pins.analog2 = AnalogIn2;
  pins.pwm_gpio = pwmGpio;
#if CONFIG_IDF_TARGET_ESP32S3
  pins.target_s3 = true;
#else
  pins.target_s3 = false;
#endif
  return pins;
}

const char *balansun_status_led_mode_string(StatusLedMode mode) {
  switch (mode) {
    case StatusLedMode::Off:
      return "off";
    case StatusLedMode::DualGpio:
      return "dual_gpio";
    case StatusLedMode::RgbWs2812:
      return "rgb_ws2812";
  }
  return "off";
}

bool balansun_status_led_mode_from_string(const char *s, StatusLedMode &out) {
  std::string err;
  return balansun_status_led_logic_parse_mode(s, out, err);
}

StatusLedConfig balansun_status_led_config_now(void) { return g_cfg; }

void balansun_status_led_init(void) {
  if (!statusLedPersistPresent) {
    const StatusLedConfig defaults = balansun_status_led_logic_default_config(balansun_status_led_board_pins().target_s3);
    statusLedMode = static_cast<uint8_t>(defaults.mode);
    statusLedGpioActivity = static_cast<int8_t>(defaults.gpio_activity);
    statusLedGpioRegulation = static_cast<int8_t>(defaults.gpio_regulation);
    statusLedRgbGpio = static_cast<int8_t>(defaults.rgb_gpio);
    statusLedActiveLow = defaults.active_low;
    statusLedColorActivity = defaults.color_activity;
    statusLedColorRegulation = defaults.color_regulation;
    statusLedColorReboot = defaults.color_reboot;
    statusLedColorAp = defaults.color_ap;
  }
  balansun_status_led_load_globals_into_config();
#if CONFIG_IDF_TARGET_ESP32S3
  /* Web UI defaults to WROOM dual GPIO; DevKitC onboard LED is WS2812 only. */
  if (g_cfg.mode == StatusLedMode::DualGpio && g_cfg.gpio_activity == 18 && g_cfg.gpio_regulation == 19) {
    g_cfg.mode = StatusLedMode::RgbWs2812;
    g_cfg.rgb_gpio = -1;
    balansun_status_led_sync_globals_from_config();
  }
#endif
  balansun_status_led_reinit();
}

void balansun_status_led_reinit(void) {
  g_test_until_ms = 0;
#if BALANSUN_HAS_RGB_RMT
  if (g_cfg.mode != StatusLedMode::RgbWs2812) {
    rgb_rmt_shutdown();
  }
#endif
#if BALANSUN_HAS_RGB_RMT || BALANSUN_HAS_RGB_BUILTIN
  g_rgb_last_valid = false;
#endif
  if (g_cfg.mode == StatusLedMode::DualGpio) {
    if (g_cfg.gpio_activity >= 0) pinMode(g_cfg.gpio_activity, OUTPUT);
    if (g_cfg.gpio_regulation >= 0) pinMode(g_cfg.gpio_regulation, OUTPUT);
    all_off(g_cfg);
  } else if (g_cfg.mode == StatusLedMode::Off) {
    all_off(g_cfg);
  }
}

void balansun_status_led_apply_live(int cpt_activity, int cpt_regulation) {
  const unsigned long now = millis();
  if (g_test_until_ms != 0) {
    if ((long)(now - g_test_until_ms) >= 0) {
      g_test_until_ms = 0;
      all_off(g_cfg);
      if (g_cfg_pending_reinit) {
        g_cfg_pending_reinit = false;
        balansun_status_led_reinit();
      }
    } else {
      g_test_tick++;
#if BALANSUN_HAS_RGB_RMT || BALANSUN_HAS_RGB_BUILTIN
      g_rgb_last_valid = false;
#endif
      if (g_test_cfg.mode == StatusLedMode::DualGpio) {
        apply_gpio_output(g_test_cfg, balansun_status_led_logic_test_gpio(g_test_cfg, g_test_role, g_test_tick));
      } else if (g_test_cfg.mode == StatusLedMode::RgbWs2812) {
        apply_rgb_output(g_test_cfg, balansun_status_led_logic_test_rgb(g_test_cfg, g_test_role, g_test_tick));
      }
      return;
    }
  }
  if (g_cfg.mode == StatusLedMode::Off) return;
  StatusLedLiveInput in(cpt_activity, cpt_regulation);
  if (g_cfg.mode == StatusLedMode::DualGpio) {
    apply_gpio_output(g_cfg, balansun_status_led_logic_live_gpio(in));
  } else {
    apply_rgb_output(g_cfg, balansun_status_led_logic_live_rgb(g_cfg, in));
  }
}

void balansun_status_led_apply_overlay(StatusLedOverlay overlay, unsigned tick) {
  const unsigned long now = millis();
  if (g_test_until_ms != 0) {
    if ((long)(now - g_test_until_ms) >= 0) {
      g_test_until_ms = 0;
      all_off(g_cfg);
      if (g_cfg_pending_reinit) {
        g_cfg_pending_reinit = false;
        balansun_status_led_reinit();
      }
    } else {
      g_test_tick++;
#if BALANSUN_HAS_RGB_RMT || BALANSUN_HAS_RGB_BUILTIN
      g_rgb_last_valid = false;
#endif
      if (g_test_cfg.mode == StatusLedMode::DualGpio) {
        apply_gpio_output(g_test_cfg, balansun_status_led_logic_test_gpio(g_test_cfg, g_test_role, g_test_tick));
      } else if (g_test_cfg.mode == StatusLedMode::RgbWs2812) {
        apply_rgb_output(g_test_cfg, balansun_status_led_logic_test_rgb(g_test_cfg, g_test_role, g_test_tick));
      }
      return;
    }
  }
  if (g_cfg.mode == StatusLedMode::Off) return;
#if BALANSUN_HAS_RGB_RMT || BALANSUN_HAS_RGB_BUILTIN
  g_rgb_last_valid = false;
#endif
  if (g_cfg.mode == StatusLedMode::DualGpio) {
    apply_gpio_output(g_cfg, balansun_status_led_logic_overlay_gpio(g_cfg, overlay, tick));
  } else if (g_cfg.mode == StatusLedMode::RgbWs2812) {
    apply_rgb_output(g_cfg, balansun_status_led_logic_overlay_rgb(g_cfg, overlay, tick));
  }
}

bool balansun_status_led_start_test(StatusLedTestRole role, unsigned duration_ms, const StatusLedConfig *override_cfg) {
  StatusLedConfig cfg = override_cfg ? *override_cfg : g_cfg;
  if (cfg.mode == StatusLedMode::Off) return false;
  std::string err;
  if (!balansun_status_led_logic_validate_config(cfg, balansun_status_led_board_pins(), err)) return false;
  g_test_cfg = cfg;
  g_test_role = role;
  g_test_tick = 0;
  if (duration_ms < 500) duration_ms = 500;
  if (duration_ms > 10000) duration_ms = 10000;
  g_test_until_ms = millis() + duration_ms;
#if BALANSUN_HAS_RGB_RMT || BALANSUN_HAS_RGB_BUILTIN
  g_rgb_last_valid = false;
#endif
  if (cfg.mode == StatusLedMode::RgbWs2812) {
#if BALANSUN_HAS_RGB_RMT
    if (!uses_s3_builtin_rgb(cfg.rgb_gpio)) {
      rgb_rmt_ensure(resolve_rgb_pin(cfg.rgb_gpio));
    }
#endif
    apply_rgb_output(cfg, balansun_status_led_logic_test_rgb(cfg, role, 1));
  }
  if (cfg.mode == StatusLedMode::DualGpio) {
    if (cfg.gpio_activity >= 0) pinMode(cfg.gpio_activity, OUTPUT);
    if (cfg.gpio_regulation >= 0) pinMode(cfg.gpio_regulation, OUTPUT);
  }
  return true;
}

void balansun_status_led_sync_globals_from_config(void) {
  statusLedMode = static_cast<uint8_t>(g_cfg.mode);
  statusLedGpioActivity = static_cast<int8_t>(g_cfg.gpio_activity);
  statusLedGpioRegulation = static_cast<int8_t>(g_cfg.gpio_regulation);
  statusLedRgbGpio = static_cast<int8_t>(g_cfg.rgb_gpio);
  statusLedActiveLow = g_cfg.active_low;
  statusLedColorActivity = g_cfg.color_activity;
  statusLedColorRegulation = g_cfg.color_regulation;
  statusLedColorReboot = g_cfg.color_reboot;
  statusLedColorAp = g_cfg.color_ap;
}

void balansun_status_led_load_globals_into_config(void) {
  g_cfg.mode = static_cast<StatusLedMode>(statusLedMode);
  g_cfg.gpio_activity = statusLedGpioActivity;
  g_cfg.gpio_regulation = statusLedGpioRegulation;
  g_cfg.rgb_gpio = statusLedRgbGpio;
  g_cfg.active_low = statusLedActiveLow;
  g_cfg.color_activity = statusLedColorActivity;
  g_cfg.color_regulation = statusLedColorRegulation;
  g_cfg.color_reboot = statusLedColorReboot;
  g_cfg.color_ap = statusLedColorAp;
}

static bool parse_color_array(JsonArrayConst arr, StatusLedRgb &out, String &err) {
  if (arr.size() != 3) {
    err = "status LED color must be [r,g,b]";
    return false;
  }
  int rgb[3];
  for (int i = 0; i < 3; i++) rgb[i] = arr[i].as<int>();
  std::string errStd;
  if (!balansun_status_led_logic_parse_color_json(rgb, 3, out, errStd)) {
    err = errStd.c_str();
    return false;
  }
  return true;
}

bool balansun_status_led_merge_json(JsonObjectConst root, StatusLedConfig &cfg, String &err) {
  std::string errStd;
  if (!root["status_led_mode"].isNull()) {
    StatusLedMode mode;
    const char *ms = root["status_led_mode"].as<const char *>();
    if (!balansun_status_led_logic_parse_mode(ms, mode, errStd)) {
      err = errStd.c_str();
      return false;
    }
    cfg.mode = mode;
  }
  if (!root["status_led_gpio_activity"].isNull()) {
    cfg.gpio_activity = (int)root["status_led_gpio_activity"];
  }
  if (!root["status_led_gpio_regulation"].isNull()) {
    cfg.gpio_regulation = (int)root["status_led_gpio_regulation"];
  }
  if (!root["status_led_rgb_gpio"].isNull()) {
    cfg.rgb_gpio = (int)root["status_led_rgb_gpio"];
  }
  if (!root["status_led_active_low"].isNull()) {
    cfg.active_low = root["status_led_active_low"].as<bool>();
  }
  {
    JsonVariantConst actVar = root["status_led_color_activity"];
    if (!actVar.isNull()) {
      JsonArrayConst actArr = actVar.as<JsonArrayConst>();
      if (actArr.isNull() || actArr.size() != 3) {
        err = "status_led_color_activity must be [r,g,b]";
        return false;
      }
      if (!parse_color_array(actArr, cfg.color_activity, err)) return false;
    }
  }
  {
    JsonVariantConst regVar = root["status_led_color_regulation"];
    if (!regVar.isNull()) {
      JsonArrayConst regArr = regVar.as<JsonArrayConst>();
      if (regArr.isNull() || regArr.size() != 3) {
        err = "status_led_color_regulation must be [r,g,b]";
        return false;
      }
      if (!parse_color_array(regArr, cfg.color_regulation, err)) return false;
    }
  }
  {
    JsonVariantConst rebootVar = root["status_led_color_reboot"];
    if (!rebootVar.isNull()) {
      JsonArrayConst rebootArr = rebootVar.as<JsonArrayConst>();
      if (rebootArr.isNull() || rebootArr.size() != 3) {
        err = "status_led_color_reboot must be [r,g,b]";
        return false;
      }
      if (!parse_color_array(rebootArr, cfg.color_reboot, err)) return false;
    }
  }
  {
    JsonVariantConst apVar = root["status_led_color_ap"];
    if (!apVar.isNull()) {
      JsonArrayConst apArr = apVar.as<JsonArrayConst>();
      if (apArr.isNull() || apArr.size() != 3) {
        err = "status_led_color_ap must be [r,g,b]";
        return false;
      }
      if (!parse_color_array(apArr, cfg.color_ap, err)) return false;
    }
  }
  if (!balansun_status_led_logic_validate_config(cfg, balansun_status_led_board_pins(), errStd)) {
    err = errStd.c_str();
    return false;
  }
  return true;
}

bool balansun_status_led_apply_config_json(JsonObjectConst root, String &err) {
  const bool touched = !root["status_led_mode"].isNull() || !root["status_led_gpio_activity"].isNull() ||
                       !root["status_led_gpio_regulation"].isNull() || !root["status_led_rgb_gpio"].isNull() ||
                       !root["status_led_active_low"].isNull() || !root["status_led_color_activity"].isNull() ||
                       !root["status_led_color_regulation"].isNull() || !root["status_led_color_reboot"].isNull() ||
                       !root["status_led_color_ap"].isNull();
  if (!touched) return true;
  balansun_status_led_load_globals_into_config();
  StatusLedConfig cfg = g_cfg;
  if (!balansun_status_led_merge_json(root, cfg, err)) return false;
  g_cfg = cfg;
  balansun_status_led_sync_globals_from_config();
  statusLedPersistPresent = true;
  if (!root["status_led_color_reboot"].isNull() || !root["status_led_color_ap"].isNull()) {
    statusLedAdvPersistPresent = true;
  }
  if (g_test_until_ms != 0 && (long)(millis() - g_test_until_ms) < 0) {
    g_cfg_pending_reinit = true;
    return true;
  }
  balansun_status_led_reinit();
  return true;
}

bool balansun_status_led_build_test_config(JsonObjectConst root, StatusLedConfig &cfg, String &err) {
  balansun_status_led_load_globals_into_config();
  cfg = g_cfg;
  return balansun_status_led_merge_json(root, cfg, err);
}
