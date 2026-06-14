#include "balansun_status_led_logic.h"

#include <cstring>

namespace {

bool gpio_in_range(int gpio, bool target_s3) {
  if (gpio < 0) return true;
  if (target_s3) return gpio <= 48;
  return gpio <= 33;
}

bool s3_psram_flash_gpio(int gpio) {
  return gpio >= 26 && gpio <= 37;
}

bool test_blink_on(unsigned tick) { return (tick % 10) <= 5; }

}  // namespace

StatusLedConfig balansun_status_led_logic_default_config(bool target_s3) {
  StatusLedConfig cfg;
  cfg.color_activity = StatusLedRgb(255, 180, 0);
  cfg.color_regulation = StatusLedRgb(0, 255, 0);
  cfg.color_reboot = StatusLedRgb(255, 0, 0);
  cfg.color_ap = StatusLedRgb(102, 0, 255);
  if (target_s3) {
    cfg.mode = StatusLedMode::RgbWs2812;
    cfg.rgb_gpio = -1;
  } else {
    cfg.mode = StatusLedMode::DualGpio;
    cfg.gpio_activity = 18;
    cfg.gpio_regulation = 19;
  }
  return cfg;
}

bool balansun_status_led_logic_parse_mode(const char *s, StatusLedMode &out, std::string &err) {
  if (!s || !s[0]) {
    err = "status_led_mode required";
    return false;
  }
  if (strcmp(s, "off") == 0) {
    out = StatusLedMode::Off;
    return true;
  }
  if (strcmp(s, "dual_gpio") == 0) {
    out = StatusLedMode::DualGpio;
    return true;
  }
  if (strcmp(s, "rgb_ws2812") == 0) {
    out = StatusLedMode::RgbWs2812;
    return true;
  }
  err = "status_led_mode must be off, dual_gpio, or rgb_ws2812";
  return false;
}

bool balansun_status_led_logic_parse_test_role(const char *s, StatusLedTestRole &out, std::string &err) {
  if (!s || !s[0]) {
    err = "role required";
    return false;
  }
  if (strcmp(s, "activity") == 0) {
    out = StatusLedTestRole::Activity;
    return true;
  }
  if (strcmp(s, "regulation") == 0) {
    out = StatusLedTestRole::Regulation;
    return true;
  }
  if (strcmp(s, "both") == 0) {
    out = StatusLedTestRole::Both;
    return true;
  }
  if (strcmp(s, "reboot") == 0) {
    out = StatusLedTestRole::Reboot;
    return true;
  }
  if (strcmp(s, "ap") == 0) {
    out = StatusLedTestRole::Ap;
    return true;
  }
  err = "role must be activity, regulation, both, reboot, or ap";
  return false;
}

bool balansun_status_led_logic_is_reserved_gpio(int gpio, const StatusLedBoardPins &pins) {
  if (gpio < 0) return false;
  if (!gpio_in_range(gpio, pins.target_s3)) return true;
  if (pins.target_s3 && s3_psram_flash_gpio(gpio)) return true;
  if (gpio == pins.triac_dim || gpio == pins.zero_cross) return true;
  if (gpio == pins.uart_rx || gpio == pins.uart_tx) return true;
  if (gpio == pins.temp) return true;
  if (gpio == pins.analog0 || gpio == pins.analog1 || gpio == pins.analog2) return true;
  if (pins.pwm_gpio >= 0 && gpio == pins.pwm_gpio) return true;
  if (!pins.target_s3) {
    if (gpio >= 6 && gpio <= 11) return true;
    if (gpio == 0 || gpio == 2 || gpio == 12 || gpio == 15) return true;
  }
  return false;
}

bool balansun_status_led_logic_validate_gpio(int gpio, const StatusLedBoardPins &pins, std::string &err) {
  if (gpio < 0) return true;
  if (!gpio_in_range(gpio, pins.target_s3)) {
    err = "status LED GPIO out of range";
    return false;
  }
  if (balansun_status_led_logic_is_reserved_gpio(gpio, pins)) {
    err = "status LED GPIO conflicts with board functions";
    return false;
  }
  return true;
}

bool balansun_status_led_logic_validate_config(const StatusLedConfig &cfg, const StatusLedBoardPins &pins,
                                            std::string &err) {
  if (cfg.mode == StatusLedMode::Off) return true;
  if (cfg.mode == StatusLedMode::DualGpio) {
    if (cfg.gpio_activity < 0 && cfg.gpio_regulation < 0) {
      err = "status LED dual_gpio requires at least one GPIO";
      return false;
    }
    if (!balansun_status_led_logic_validate_gpio(cfg.gpio_activity, pins, err)) return false;
    if (!balansun_status_led_logic_validate_gpio(cfg.gpio_regulation, pins, err)) return false;
    if (cfg.gpio_activity >= 0 && cfg.gpio_activity == cfg.gpio_regulation) {
      err = "status LED GPIOs must differ";
      return false;
    }
    return true;
  }
  if (cfg.rgb_gpio >= 0 && !balansun_status_led_logic_validate_gpio(cfg.rgb_gpio, pins, err)) return false;
  return true;
}

bool balansun_status_led_logic_parse_color_json(const int *rgb, int len, StatusLedRgb &out, std::string &err) {
  if (!rgb || len != 3) {
    err = "status LED color must be [r,g,b]";
    return false;
  }
  for (int i = 0; i < 3; i++) {
    if (rgb[i] < 0 || rgb[i] > 255) {
      err = "status LED color values must be 0..255";
      return false;
    }
  }
  out.r = static_cast<uint8_t>(rgb[0]);
  out.g = static_cast<uint8_t>(rgb[1]);
  out.b = static_cast<uint8_t>(rgb[2]);
  return true;
}

StatusLedGpioOutput balansun_status_led_logic_live_gpio(const StatusLedLiveInput &in) {
  StatusLedGpioOutput out;
  out.activity_on = in.cpt_activity <= 5;
  out.regulation_on = in.cpt_regulation <= 5;
  return out;
}

StatusLedRgb balansun_status_led_logic_blend_rgb(const StatusLedRgb &a, const StatusLedRgb &b, bool a_on, bool b_on) {
  StatusLedRgb out{0, 0, 0};
  if (a_on) {
    out.r = a.r;
    out.g = a.g;
    out.b = a.b;
  }
  if (b_on) {
    const int r = static_cast<int>(out.r) + static_cast<int>(b.r);
    const int g = static_cast<int>(out.g) + static_cast<int>(b.g);
    const int bl = static_cast<int>(out.b) + static_cast<int>(b.b);
    out.r = static_cast<uint8_t>(r > 255 ? 255 : r);
    out.g = static_cast<uint8_t>(g > 255 ? 255 : g);
    out.b = static_cast<uint8_t>(bl > 255 ? 255 : bl);
  }
  return out;
}

StatusLedRgb balansun_status_led_logic_live_rgb(const StatusLedConfig &cfg, const StatusLedLiveInput &in) {
  const StatusLedGpioOutput gpio = balansun_status_led_logic_live_gpio(in);
  return balansun_status_led_logic_blend_rgb(cfg.color_activity, cfg.color_regulation, gpio.activity_on,
                                          gpio.regulation_on);
}

StatusLedRgb balansun_status_led_logic_overlay_rgb(const StatusLedConfig &cfg, StatusLedOverlay overlay,
                                                unsigned tick) {
  if (cfg.mode == StatusLedMode::Off || overlay == StatusLedOverlay::None) {
    return StatusLedRgb(0, 0, 0);
  }
  if (!test_blink_on(tick)) return StatusLedRgb(0, 0, 0);
  if (overlay == StatusLedOverlay::Reboot) return cfg.color_reboot;
  return cfg.color_ap;
}

StatusLedGpioOutput balansun_status_led_logic_overlay_gpio(const StatusLedConfig &cfg, StatusLedOverlay overlay,
                                                        unsigned tick) {
  (void)cfg;
  (void)overlay;
  StatusLedGpioOutput out;
  const bool on = test_blink_on(tick);
  out.activity_on = on;
  out.regulation_on = on;
  return out;
}

StatusLedGpioOutput balansun_status_led_logic_test_gpio(const StatusLedConfig &cfg, StatusLedTestRole role,
                                                     unsigned tick) {
  if (role == StatusLedTestRole::Reboot) {
    return balansun_status_led_logic_overlay_gpio(cfg, StatusLedOverlay::Reboot, tick);
  }
  if (role == StatusLedTestRole::Ap) {
    return balansun_status_led_logic_overlay_gpio(cfg, StatusLedOverlay::ApSetup, tick);
  }
  StatusLedGpioOutput out;
  const bool on = test_blink_on(tick);
  if (role == StatusLedTestRole::Activity) {
    out.activity_on = on;
  } else if (role == StatusLedTestRole::Regulation) {
    out.regulation_on = on;
  } else if (role == StatusLedTestRole::Both) {
    out.activity_on = on;
    out.regulation_on = on;
  }
  return out;
}

StatusLedRgb balansun_status_led_logic_test_rgb(const StatusLedConfig &cfg, StatusLedTestRole role, unsigned tick) {
  if (role == StatusLedTestRole::Reboot) {
    return balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::Reboot, tick);
  }
  if (role == StatusLedTestRole::Ap) {
    return balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::ApSetup, tick);
  }
  const bool on = test_blink_on(tick);
  if (!on) return StatusLedRgb(0, 0, 0);
  if (role == StatusLedTestRole::Activity) return cfg.color_activity;
  if (role == StatusLedTestRole::Regulation) return cfg.color_regulation;
  if (role == StatusLedTestRole::Both) {
    /* Single RGB LED: alternate colors; additive blend would read as yellow. */
    const unsigned phase = tick % 10;
    return (phase <= 2) ? cfg.color_activity : cfg.color_regulation;
  }
  return StatusLedRgb(0, 0, 0);
}
