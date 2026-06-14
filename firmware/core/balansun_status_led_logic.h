#pragma once

/*
 * balansun_status_led_logic.h — Status LED config validation and output mapping (host-testable).
 */

#include <cstdint>
#include <string>

enum class StatusLedMode : uint8_t { Off = 0, DualGpio = 1, RgbWs2812 = 2 };

enum class StatusLedTestRole : uint8_t { Activity = 0, Regulation = 1, Both = 2, Reboot = 3, Ap = 4 };

enum class StatusLedOverlay : uint8_t { None = 0, Reboot = 1, ApSetup = 2 };

struct StatusLedRgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  StatusLedRgb() : r(0), g(0), b(0) {}
  StatusLedRgb(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
};

struct StatusLedConfig {
  StatusLedMode mode = StatusLedMode::DualGpio;
  int gpio_activity = 18;
  int gpio_regulation = 19;
  int rgb_gpio = -1;
  bool active_low = false;
  StatusLedRgb color_activity;
  StatusLedRgb color_regulation;
  StatusLedRgb color_reboot;
  StatusLedRgb color_ap;
};

struct StatusLedLiveInput {
  int cpt_activity = 0;
  int cpt_regulation = 0;
  StatusLedLiveInput() = default;
  StatusLedLiveInput(int activity, int regulation) : cpt_activity(activity), cpt_regulation(regulation) {}
};

struct StatusLedGpioOutput {
  bool activity_on = false;
  bool regulation_on = false;
};

struct StatusLedBoardPins {
  int triac_dim = 22;
  int zero_cross = 23;
  int uart_rx = 26;
  int uart_tx = 27;
  int temp = 13;
  int analog0 = 35;
  int analog1 = 32;
  int analog2 = 33;
  int pwm_gpio = -1;
  bool target_s3 = false;
};

StatusLedConfig balansun_status_led_logic_default_config(bool target_s3);

bool balansun_status_led_logic_parse_mode(const char *s, StatusLedMode &out, std::string &err);
bool balansun_status_led_logic_parse_test_role(const char *s, StatusLedTestRole &out, std::string &err);

bool balansun_status_led_logic_is_reserved_gpio(int gpio, const StatusLedBoardPins &pins);
bool balansun_status_led_logic_validate_gpio(int gpio, const StatusLedBoardPins &pins, std::string &err);
bool balansun_status_led_logic_validate_config(const StatusLedConfig &cfg, const StatusLedBoardPins &pins,
                                            std::string &err);

bool balansun_status_led_logic_parse_color_json(const int *rgb, int len, StatusLedRgb &out, std::string &err);

StatusLedGpioOutput balansun_status_led_logic_live_gpio(const StatusLedLiveInput &in);

StatusLedRgb balansun_status_led_logic_live_rgb(const StatusLedConfig &cfg, const StatusLedLiveInput &in);

StatusLedGpioOutput balansun_status_led_logic_test_gpio(const StatusLedConfig &cfg, StatusLedTestRole role,
                                                     unsigned tick);

StatusLedRgb balansun_status_led_logic_test_rgb(const StatusLedConfig &cfg, StatusLedTestRole role, unsigned tick);

StatusLedRgb balansun_status_led_logic_blend_rgb(const StatusLedRgb &a, const StatusLedRgb &b, bool a_on, bool b_on);

StatusLedRgb balansun_status_led_logic_overlay_rgb(const StatusLedConfig &cfg, StatusLedOverlay overlay, unsigned tick);

StatusLedGpioOutput balansun_status_led_logic_overlay_gpio(const StatusLedConfig &cfg, StatusLedOverlay overlay,
                                                         unsigned tick);
