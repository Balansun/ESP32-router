#include <gtest/gtest.h>

#include "api_util_logic.h"

TEST(ApiUtilLogic, GpioReadRestrictsFlashBus) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_read(6));
  EXPECT_TRUE(api_logic_is_restricted_gpio_read(11));
  EXPECT_FALSE(api_logic_is_restricted_gpio_read(22));
  EXPECT_TRUE(api_logic_is_restricted_gpio_read(34));
}

TEST(ApiUtilLogic, GpioWriteBlocksTriacAndUart) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(22, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(26, false));
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(4, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(32, true));
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(32, false));
}

TEST(ApiUtilLogic, IpConversionRoundTrip) {
  uint32_t ip = 0;
  ASSERT_TRUE(api_logic_dotted_to_ip32("192.168.1.42", ip));
  EXPECT_EQ(ip, 0xC0A8012Au);
  EXPECT_EQ(api_logic_ip32_to_dotted(ip), "192.168.1.42");
  EXPECT_FALSE(api_logic_dotted_to_ip32("999.1.1.1", ip));
}

TEST(ApiUtilLogic, PasswordAsciiValidation) {
  std::string err;
  EXPECT_TRUE(api_logic_validate_password_ascii("secret", err));
  EXPECT_FALSE(api_logic_validate_password_ascii(std::string(65, 'a'), err));
  EXPECT_FALSE(api_logic_validate_password_ascii("bad\nchar", err));
}

TEST(ApiUtilLogic, GpioReadWriteEdgeCases) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_read(-1));
  EXPECT_TRUE(api_logic_is_restricted_gpio_read(40));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(-1, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(13, false));
  uint32_t ip = 0;
  EXPECT_FALSE(api_logic_dotted_to_ip32(nullptr, ip));
}

TEST(ApiUtilLogic, PasswordEmptyOk) {
  std::string err;
  EXPECT_TRUE(api_logic_validate_password_ascii("", err));
}

TEST(ApiUtilLogic, GpioWriteRestrictedPins) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(12, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(15, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(19, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(35, false));
}

TEST(ApiUtilLogic, SerialAdcGpio35Restricted) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(35, true));
}

TEST(ApiUtilLogic, DottedIpPartialParseFails) {
  uint32_t ip = 0xFFFFFFFFu;
  EXPECT_FALSE(api_logic_dotted_to_ip32("192.168.1", ip));
  EXPECT_FALSE(api_logic_dotted_to_ip32("192.168", ip));
}

TEST(ApiUtilLogic, PasswordAsciiBoundaryChars) {
  std::string err;
  EXPECT_TRUE(api_logic_validate_password_ascii(std::string(1, static_cast<char>(32)), err));
  EXPECT_TRUE(api_logic_validate_password_ascii(std::string(1, static_cast<char>(126)), err));
  EXPECT_FALSE(api_logic_validate_password_ascii(std::string(1, static_cast<char>(31)), err));
  EXPECT_FALSE(api_logic_validate_password_ascii(std::string(1, static_cast<char>(127)), err));
}

TEST(ApiUtilLogic, GpioReadAllowedPins) {
  EXPECT_FALSE(api_logic_is_restricted_gpio_read(5));
  EXPECT_FALSE(api_logic_is_restricted_gpio_read(12));
  EXPECT_FALSE(api_logic_is_restricted_gpio_read(15));
  EXPECT_FALSE(api_logic_is_restricted_gpio_read(21));
}

TEST(ApiUtilLogic, GpioReadFlashBusRange) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_read(6));
  EXPECT_TRUE(api_logic_is_restricted_gpio_read(11));
}

TEST(ApiUtilLogic, GpioWriteMoreRestrictedPins) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(0, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(2, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(7, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(18, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(23, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(27, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(34, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(38, false));
}

TEST(ApiUtilLogic, Gpio32AllowedWithoutSerialAdcRestrict) {
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(32, false));
}

TEST(ApiUtilLogic, SerialAdcGpio32And33Restricted) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(32, true));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(33, true));
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(21, false));
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(21, true));
}

TEST(ApiUtilLogic, DottedIpOctetOverflowFails) {
  uint32_t ip = 0;
  EXPECT_FALSE(api_logic_dotted_to_ip32("256.0.0.0", ip));
  EXPECT_FALSE(api_logic_dotted_to_ip32("192.168.1.256", ip));
  EXPECT_FALSE(api_logic_dotted_to_ip32("192.256.0.0", ip));
  EXPECT_FALSE(api_logic_dotted_to_ip32("192.168.256.0", ip));
}

TEST(ApiUtilLogic, GpioWriteAllowedPins) {
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(5, false));
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(14, false));
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(16, false));
  EXPECT_FALSE(api_logic_is_restricted_gpio_write(25, false));
}

TEST(ApiUtilLogic, GpioWriteUartPinsRestricted) {
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(22, false));
  EXPECT_TRUE(api_logic_is_restricted_gpio_write(26, false));
}

TEST(ApiUtilLogic, PasswordMaxLengthOk) {
  std::string err;
  EXPECT_TRUE(api_logic_validate_password_ascii(std::string(64, 'a'), err));
}

TEST(ApiUtilLogic, UriIsApiV1) {
  EXPECT_TRUE(api_logic_uri_is_api_v1("/api/v1/public"));
  EXPECT_TRUE(api_logic_uri_is_api_v1("/api/v1/config"));
  EXPECT_TRUE(api_logic_uri_is_api_v1("/api/v1"));
  EXPECT_FALSE(api_logic_uri_is_api_v1("/api/v2/public"));
  EXPECT_FALSE(api_logic_uri_is_api_v1(nullptr));
}

TEST(ApiUtilLogic, CorsPreflightWhenEnabled) {
  EXPECT_TRUE(api_logic_should_handle_cors_preflight(true, true, "/api/v1/measurements"));
  EXPECT_TRUE(api_logic_cors_preflight_bypasses_auth(true, true, "/api/v1/public"));
  EXPECT_FALSE(api_logic_should_handle_cors_preflight(false, true, "/api/v1/public"));
  EXPECT_FALSE(api_logic_should_handle_cors_preflight(true, false, "/api/v1/public"));
  EXPECT_FALSE(api_logic_should_handle_cors_preflight(true, true, "/wifi"));
}
