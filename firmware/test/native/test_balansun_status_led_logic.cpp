#include <gtest/gtest.h>

#include "balansun_status_led_logic.h"

static StatusLedBoardPins default_pins() {
  StatusLedBoardPins pins;
  pins.triac_dim = 22;
  pins.zero_cross = 23;
  pins.uart_rx = 26;
  pins.uart_tx = 27;
  pins.temp = 13;
  pins.analog0 = 35;
  pins.analog1 = 32;
  pins.analog2 = 33;
  pins.pwm_gpio = -1;
  pins.target_s3 = false;
  return pins;
}

TEST(BalansunStatusLedLogic, DefaultConfigByTarget) {
  const StatusLedConfig wroom = balansun_status_led_logic_default_config(false);
  EXPECT_EQ(wroom.mode, StatusLedMode::DualGpio);
  EXPECT_EQ(wroom.gpio_activity, 18);
  EXPECT_EQ(wroom.color_reboot.r, 255);
  EXPECT_EQ(wroom.color_reboot.g, 0);
  EXPECT_EQ(wroom.color_ap.r, 102);
  EXPECT_EQ(wroom.color_ap.g, 0);
  EXPECT_EQ(wroom.color_ap.b, 255);
  const StatusLedConfig s3 = balansun_status_led_logic_default_config(true);
  EXPECT_EQ(s3.mode, StatusLedMode::RgbWs2812);
  EXPECT_EQ(s3.rgb_gpio, -1);
}

TEST(BalansunStatusLedLogic, ParseModeAndRole) {
  StatusLedMode mode;
  StatusLedTestRole role;
  std::string err;
  EXPECT_TRUE(balansun_status_led_logic_parse_mode("dual_gpio", mode, err));
  EXPECT_EQ(mode, StatusLedMode::DualGpio);
  EXPECT_TRUE(balansun_status_led_logic_parse_mode("rgb_ws2812", mode, err));
  EXPECT_FALSE(balansun_status_led_logic_parse_mode("invalid", mode, err));
  EXPECT_TRUE(balansun_status_led_logic_parse_test_role("both", role, err));
  EXPECT_EQ(role, StatusLedTestRole::Both);
  EXPECT_TRUE(balansun_status_led_logic_parse_test_role("activity", role, err));
  EXPECT_EQ(role, StatusLedTestRole::Activity);
  EXPECT_TRUE(balansun_status_led_logic_parse_test_role("regulation", role, err));
  EXPECT_EQ(role, StatusLedTestRole::Regulation);
  EXPECT_FALSE(balansun_status_led_logic_parse_test_role("", role, err));
}

TEST(BalansunStatusLedLogic, ValidatesReservedGpio) {
  auto pins = default_pins();
  std::string err;
  EXPECT_FALSE(balansun_status_led_logic_validate_gpio(22, pins, err));
  EXPECT_TRUE(balansun_status_led_logic_validate_gpio(18, pins, err));
  pins.target_s3 = true;
  pins.uart_rx = 4;
  pins.uart_tx = 5;
  EXPECT_FALSE(balansun_status_led_logic_validate_gpio(30, pins, err));
}

TEST(BalansunStatusLedLogic, LiveGpioPhase) {
  StatusLedLiveInput in(3, 7);
  const StatusLedGpioOutput out = balansun_status_led_logic_live_gpio(in);
  EXPECT_TRUE(out.activity_on);
  EXPECT_FALSE(out.regulation_on);
}

TEST(BalansunStatusLedLogic, RgbBlendBothOn) {
  StatusLedConfig cfg;
  cfg.color_activity = StatusLedRgb(255, 180, 0);
  cfg.color_regulation = StatusLedRgb(0, 255, 0);
  StatusLedLiveInput in(2, 2);
  const StatusLedRgb rgb = balansun_status_led_logic_live_rgb(cfg, in);
  EXPECT_EQ(rgb.r, 255);
  EXPECT_EQ(rgb.g, 255);
  EXPECT_EQ(rgb.b, 0);
}

TEST(BalansunStatusLedLogic, TestRgbBothAlternatesColors) {
  StatusLedConfig cfg;
  cfg.color_activity = StatusLedRgb(255, 0, 0);
  cfg.color_regulation = StatusLedRgb(0, 0, 255);
  const StatusLedRgb early = balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Both, 1);
  const StatusLedRgb late = balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Both, 4);
  EXPECT_EQ(early.r, 255);
  EXPECT_EQ(early.b, 0);
  EXPECT_EQ(late.b, 255);
  EXPECT_EQ(late.r, 0);
}

TEST(BalansunStatusLedLogic, TestRgbUsesConfigColors) {
  StatusLedConfig cfg;
  cfg.color_activity = StatusLedRgb(10, 20, 30);
  cfg.color_regulation = StatusLedRgb(40, 50, 60);
  const StatusLedRgb act = balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Activity, 2);
  const StatusLedRgb reg = balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Regulation, 2);
  EXPECT_EQ(act.r, 10);
  EXPECT_EQ(reg.g, 50);
}

TEST(BalansunStatusLedLogic, TestPatternBlinks) {
  StatusLedConfig cfg;
  const StatusLedGpioOutput actOn = balansun_status_led_logic_test_gpio(cfg, StatusLedTestRole::Activity, 2);
  const StatusLedGpioOutput actOff = balansun_status_led_logic_test_gpio(cfg, StatusLedTestRole::Activity, 8);
  EXPECT_TRUE(actOn.activity_on);
  EXPECT_FALSE(actOff.activity_on);
  const StatusLedGpioOutput regOn = balansun_status_led_logic_test_gpio(cfg, StatusLedTestRole::Regulation, 2);
  EXPECT_TRUE(regOn.regulation_on);
  const StatusLedGpioOutput bothOn = balansun_status_led_logic_test_gpio(cfg, StatusLedTestRole::Both, 2);
  EXPECT_TRUE(bothOn.activity_on);
  EXPECT_TRUE(bothOn.regulation_on);
}

TEST(BalansunStatusLedLogic, ParseColorJson) {
  StatusLedRgb color;
  std::string err;
  const int good[] = {10, 20, 30};
  EXPECT_TRUE(balansun_status_led_logic_parse_color_json(good, 3, color, err));
  EXPECT_EQ(color.r, 10);
  const int bad[] = {300, 0, 0};
  EXPECT_FALSE(balansun_status_led_logic_parse_color_json(bad, 3, color, err));
}

TEST(BalansunStatusLedLogic, OverlayRgbRebootAndAp) {
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.color_reboot = StatusLedRgb(255, 0, 0);
  cfg.color_ap = StatusLedRgb(255, 105, 180);
  const StatusLedRgb rebootOn = balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::Reboot, 2);
  const StatusLedRgb rebootOff = balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::Reboot, 8);
  EXPECT_EQ(rebootOn.r, 255);
  EXPECT_EQ(rebootOff.r, 0);
  const StatusLedRgb apOn = balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::ApSetup, 3);
  EXPECT_EQ(apOn.g, 105);
  cfg.mode = StatusLedMode::Off;
  EXPECT_EQ(balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::Reboot, 2).r, 0);
}

TEST(BalansunStatusLedLogic, OverlayGpioBothBlink) {
  StatusLedConfig cfg;
  const StatusLedGpioOutput on = balansun_status_led_logic_overlay_gpio(cfg, StatusLedOverlay::Reboot, 2);
  const StatusLedGpioOutput off = balansun_status_led_logic_overlay_gpio(cfg, StatusLedOverlay::ApSetup, 8);
  EXPECT_TRUE(on.activity_on);
  EXPECT_TRUE(on.regulation_on);
  EXPECT_FALSE(off.activity_on);
}

TEST(BalansunStatusLedLogic, ParseRebootApTestRole) {
  StatusLedTestRole role;
  std::string err;
  EXPECT_TRUE(balansun_status_led_logic_parse_test_role("reboot", role, err));
  EXPECT_EQ(role, StatusLedTestRole::Reboot);
  EXPECT_TRUE(balansun_status_led_logic_parse_test_role("ap", role, err));
  EXPECT_EQ(role, StatusLedTestRole::Ap);
}

TEST(BalansunStatusLedLogic, TestRgbRebootAndApRoles) {
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.color_reboot = StatusLedRgb(200, 0, 0);
  cfg.color_ap = StatusLedRgb(100, 50, 150);
  const StatusLedRgb reboot = balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Reboot, 3);
  const StatusLedRgb ap = balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Ap, 3);
  EXPECT_EQ(reboot.r, 200);
  EXPECT_EQ(ap.g, 50);
}

TEST(BalansunStatusLedLogic, TestGpioRebootAndApRoles) {
  StatusLedConfig cfg;
  const StatusLedGpioOutput rebootOn = balansun_status_led_logic_test_gpio(cfg, StatusLedTestRole::Reboot, 2);
  const StatusLedGpioOutput apOff = balansun_status_led_logic_test_gpio(cfg, StatusLedTestRole::Ap, 9);
  EXPECT_TRUE(rebootOn.activity_on);
  EXPECT_TRUE(rebootOn.regulation_on);
  EXPECT_FALSE(apOff.activity_on);
}

TEST(BalansunStatusLedLogic, OverlayNoneReturnsOff) {
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.color_reboot = StatusLedRgb(255, 0, 0);
  EXPECT_EQ(balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::None, 2).r, 0);
}

TEST(BalansunStatusLedLogic, OverlayApSetupRgb) {
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.color_ap = StatusLedRgb(10, 20, 30);
  const StatusLedRgb on = balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::ApSetup, 4);
  EXPECT_EQ(on.b, 30);
}

TEST(BalansunStatusLedLogic, ValidateConfigRgbAndDual) {
  auto pins = default_pins();
  std::string err;
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::Off;
  EXPECT_TRUE(balansun_status_led_logic_validate_config(cfg, pins, err));
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.rgb_gpio = 18;
  EXPECT_TRUE(balansun_status_led_logic_validate_config(cfg, pins, err));
  cfg.mode = StatusLedMode::DualGpio;
  cfg.gpio_activity = -1;
  cfg.gpio_regulation = -1;
  EXPECT_FALSE(balansun_status_led_logic_validate_config(cfg, pins, err));
  cfg.gpio_activity = 18;
  cfg.gpio_regulation = 18;
  EXPECT_FALSE(balansun_status_led_logic_validate_config(cfg, pins, err));
}

TEST(BalansunStatusLedLogic, ParseModeOffAndRgb) {
  StatusLedMode mode;
  std::string err;
  EXPECT_TRUE(balansun_status_led_logic_parse_mode("off", mode, err));
  EXPECT_EQ(mode, StatusLedMode::Off);
  EXPECT_TRUE(balansun_status_led_logic_parse_mode("rgb_ws2812", mode, err));
  EXPECT_EQ(mode, StatusLedMode::RgbWs2812);
  EXPECT_FALSE(balansun_status_led_logic_parse_mode(nullptr, mode, err));
}

TEST(BalansunStatusLedLogic, BlendRgbOnlyBOn) {
  const StatusLedRgb a(10, 10, 10);
  const StatusLedRgb b(20, 30, 40);
  const StatusLedRgb out = balansun_status_led_logic_blend_rgb(a, b, false, true);
  EXPECT_EQ(out.r, 20);
  EXPECT_EQ(out.g, 30);
}

TEST(BalansunStatusLedLogic, ValidateGpioRangeAndReserved) {
  auto pins = default_pins();
  std::string err;
  EXPECT_FALSE(balansun_status_led_logic_validate_gpio(34, pins, err));
  pins.target_s3 = true;
  pins.uart_rx = 4;
  pins.uart_tx = 5;
  EXPECT_FALSE(balansun_status_led_logic_validate_gpio(30, pins, err));
  EXPECT_FALSE(balansun_status_led_logic_validate_gpio(49, pins, err));
  pins.pwm_gpio = 21;
  EXPECT_FALSE(balansun_status_led_logic_validate_gpio(21, pins, err));
}

TEST(BalansunStatusLedLogic, LivePathsAndParseColor) {
  StatusLedLiveInput in(8, 2);
  const StatusLedGpioOutput gpio = balansun_status_led_logic_live_gpio(in);
  EXPECT_FALSE(gpio.activity_on);
  EXPECT_TRUE(gpio.regulation_on);
  StatusLedConfig cfg;
  cfg.color_activity = StatusLedRgb(1, 2, 3);
  cfg.color_regulation = StatusLedRgb(4, 5, 6);
  const StatusLedRgb rgb = balansun_status_led_logic_live_rgb(cfg, in);
  EXPECT_EQ(rgb.r, 4);
  StatusLedRgb color;
  std::string err;
  EXPECT_FALSE(balansun_status_led_logic_parse_color_json(nullptr, 3, color, err));
  const int shortRgb[] = {1, 2};
  EXPECT_FALSE(balansun_status_led_logic_parse_color_json(shortRgb, 2, color, err));
}

TEST(BalansunStatusLedLogic, TestRgbOffPhaseAndRegulation) {
  StatusLedConfig cfg;
  cfg.color_regulation = StatusLedRgb(7, 8, 9);
  EXPECT_EQ(balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Regulation, 9).r, 0);
  EXPECT_EQ(balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Regulation, 2).r, 7);
  cfg.color_activity = StatusLedRgb(1, 2, 3);
  EXPECT_EQ(balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Activity, 2).g, 2);
}

TEST(BalansunStatusLedLogic, IsReservedWroomFlashPins) {
  auto pins = default_pins();
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(6, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(2, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(15, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(0, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(22, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(26, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(35, pins));
  EXPECT_FALSE(balansun_status_led_logic_is_reserved_gpio(-1, pins));
  pins.pwm_gpio = 17;
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(17, pins));
}

TEST(BalansunStatusLedLogic, BlendRgbNeitherOn) {
  const StatusLedRgb a(10, 20, 30);
  const StatusLedRgb b(40, 50, 60);
  const StatusLedRgb out = balansun_status_led_logic_blend_rgb(a, b, false, false);
  EXPECT_EQ(out.r, 0);
  EXPECT_EQ(out.g, 0);
  EXPECT_EQ(out.b, 0);
}

TEST(BalansunStatusLedLogic, BlinkPhaseBoundaries) {
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.color_reboot = StatusLedRgb(255, 0, 0);
  EXPECT_EQ(balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::Reboot, 6).r, 0);
  EXPECT_EQ(balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::Reboot, 5).r, 255);
}

TEST(BalansunStatusLedLogic, BlendRgbClampsSum) {
  const StatusLedRgb a(200, 200, 200);
  const StatusLedRgb b(200, 200, 200);
  const StatusLedRgb out = balansun_status_led_logic_blend_rgb(a, b, true, true);
  EXPECT_EQ(out.r, 255);
  EXPECT_EQ(out.g, 255);
  EXPECT_EQ(out.b, 255);
}

TEST(BalansunStatusLedLogic, ParseTestRoleInvalid) {
  StatusLedTestRole role;
  std::string err;
  EXPECT_FALSE(balansun_status_led_logic_parse_test_role("invalid", role, err));
}

TEST(BalansunStatusLedLogic, ValidateRgbGpioConflict) {
  auto pins = default_pins();
  std::string err;
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.rgb_gpio = 22;
  EXPECT_FALSE(balansun_status_led_logic_validate_config(cfg, pins, err));
}

TEST(BalansunStatusLedLogic, TestRgbBothMidPhase) {
  StatusLedConfig cfg;
  cfg.color_activity = StatusLedRgb(1, 0, 0);
  cfg.color_regulation = StatusLedRgb(0, 0, 2);
  const StatusLedRgb mid = balansun_status_led_logic_test_rgb(cfg, StatusLedTestRole::Both, 5);
  EXPECT_EQ(mid.b, 2);
}

TEST(BalansunStatusLedLogic, S3ReservedFlashBusAndRgbGpioOk) {
  StatusLedBoardPins pins = default_pins();
  pins.target_s3 = true;
  pins.uart_rx = 4;
  pins.uart_tx = 5;
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(26, pins));
  EXPECT_FALSE(balansun_status_led_logic_is_reserved_gpio(12, pins));
  std::string err;
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.rgb_gpio = -1;
  EXPECT_TRUE(balansun_status_led_logic_validate_config(cfg, pins, err));
}

TEST(BalansunStatusLedLogic, ParseModeEmptyAndColorBounds) {
  StatusLedMode mode;
  std::string err;
  EXPECT_FALSE(balansun_status_led_logic_parse_mode("", mode, err));
  StatusLedRgb color;
  const int neg[] = {-1, 0, 0};
  EXPECT_FALSE(balansun_status_led_logic_parse_color_json(neg, 3, color, err));
}

TEST(BalansunStatusLedLogic, UnknownTestRoleDefaults) {
  StatusLedConfig cfg;
  const auto unknown = static_cast<StatusLedTestRole>(99);
  EXPECT_EQ(balansun_status_led_logic_test_rgb(cfg, unknown, 2).r, 0);
  EXPECT_FALSE(balansun_status_led_logic_test_gpio(cfg, unknown, 2).activity_on);
}

TEST(BalansunStatusLedLogic, ValidateDualSingleGpioOk) {
  auto pins = default_pins();
  std::string err;
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::DualGpio;
  cfg.gpio_activity = 18;
  cfg.gpio_regulation = -1;
  EXPECT_TRUE(balansun_status_led_logic_validate_config(cfg, pins, err));
  cfg.gpio_activity = -1;
  cfg.gpio_regulation = 19;
  EXPECT_TRUE(balansun_status_led_logic_validate_config(cfg, pins, err));
}

TEST(BalansunStatusLedLogic, LiveRgbBothOff) {
  StatusLedConfig cfg;
  cfg.color_activity = StatusLedRgb(9, 8, 7);
  cfg.color_regulation = StatusLedRgb(1, 2, 3);
  const StatusLedRgb rgb = balansun_status_led_logic_live_rgb(cfg, StatusLedLiveInput(9, 9));
  EXPECT_EQ(rgb.r, 0);
}

TEST(BalansunStatusLedLogic, LiveActivityOnlyRgb) {
  StatusLedConfig cfg;
  cfg.color_activity = StatusLedRgb(9, 8, 7);
  cfg.color_regulation = StatusLedRgb(1, 2, 3);
  const StatusLedRgb rgb = balansun_status_led_logic_live_rgb(cfg, StatusLedLiveInput(2, 9));
  EXPECT_EQ(rgb.r, 9);
}

TEST(BalansunStatusLedLogic, ParseColorTooHigh) {
  StatusLedRgb color;
  std::string err;
  const int hi[] = {256, 0, 0};
  EXPECT_FALSE(balansun_status_led_logic_parse_color_json(hi, 3, color, err));
}

TEST(BalansunStatusLedLogic, OverlayGpioWhenModeOffStillRuns) {
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::Off;
  const StatusLedGpioOutput out = balansun_status_led_logic_overlay_gpio(cfg, StatusLedOverlay::Reboot, 3);
  EXPECT_TRUE(out.activity_on);
}

TEST(BalansunStatusLedLogic, ReservedAnalogTempAndUart) {
  auto pins = default_pins();
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(13, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(32, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(33, pins));
  EXPECT_TRUE(balansun_status_led_logic_is_reserved_gpio(27, pins));
}

TEST(BalansunStatusLedLogic, OverlayBlinkOffTicks) {
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::RgbWs2812;
  cfg.color_reboot = StatusLedRgb(5, 5, 5);
  EXPECT_EQ(balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::Reboot, 7).r, 0);
  EXPECT_EQ(balansun_status_led_logic_overlay_rgb(cfg, StatusLedOverlay::Reboot, 0).r, 5);
}

TEST(BalansunStatusLedLogic, ValidateGpioNegativeOk) {
  std::string err;
  EXPECT_TRUE(balansun_status_led_logic_validate_gpio(-1, default_pins(), err));
}

TEST(BalansunStatusLedLogic, DualGpioValidateEachPinFails) {
  auto pins = default_pins();
  std::string err;
  StatusLedConfig cfg;
  cfg.mode = StatusLedMode::DualGpio;
  cfg.gpio_activity = 22;
  cfg.gpio_regulation = 19;
  EXPECT_FALSE(balansun_status_led_logic_validate_config(cfg, pins, err));
  cfg.gpio_activity = 18;
  cfg.gpio_regulation = 23;
  EXPECT_FALSE(balansun_status_led_logic_validate_config(cfg, pins, err));
}
