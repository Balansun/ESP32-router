#include "api_util_logic.h"

#include <cstdio>
#include <cstring>

bool api_logic_is_restricted_gpio_read(int gpio) {
  if (gpio < 0 || gpio > 33) return true;
  if (gpio >= 6 && gpio <= 11) return true;
  return false;
}

bool api_logic_is_restricted_gpio_write(int gpio, bool serial_adc_gpio_restrict) {
  if (gpio < 0 || gpio > 33) return true;
  if (gpio >= 6 && gpio <= 11) return true;
  if (gpio == 13 || gpio == 18 || gpio == 19 || gpio == 22 || gpio == 23 || gpio == 26 || gpio == 27)
    return true;
  if (gpio == 0 || gpio == 2 || gpio == 12 || gpio == 15) return true;
  if (serial_adc_gpio_restrict && (gpio == 32 || gpio == 33)) return true;
  return false;
}

std::string api_logic_ip32_to_dotted(uint32_t ip) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%u.%u.%u.%u", (unsigned)((ip >> 24) & 0xFF), (unsigned)((ip >> 16) & 0xFF),
           (unsigned)((ip >> 8) & 0xFF), (unsigned)(ip & 0xFF));
  return std::string(buf);
}

bool api_logic_dotted_to_ip32(const char *s, uint32_t &out) {
  if (!s) return false;
  unsigned a = 0, b = 0, c = 0, d = 0;
  if (sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return false;
  if (a > 255 || b > 255 || c > 255 || d > 255) return false;
  out = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d;
  return true;
}

bool api_logic_uri_is_api_v1(const char *uri) {
  if (!uri) return false;
  if (strcmp(uri, "/api/v1") == 0) return true;
  return strncmp(uri, "/api/v1/", 8) == 0;
}

bool api_logic_should_handle_cors_preflight(bool http_cors_enabled, bool is_options_method,
                                            const char *uri) {
  if (!http_cors_enabled || !is_options_method) return false;
  return api_logic_uri_is_api_v1(uri);
}

bool api_logic_cors_preflight_bypasses_auth(bool http_cors_enabled, bool is_options_method,
                                            const char *uri) {
  return api_logic_should_handle_cors_preflight(http_cors_enabled, is_options_method, uri);
}

bool api_logic_validate_password_ascii(const std::string &s, std::string &err) {
  if (s.length() > 64) {
    err = "password max length is 64";
    return false;
  }
  for (unsigned char c : s) {
    if (c < 32 || c > 126) {
      err = "password must be printable ASCII";
      return false;
    }
  }
  return true;
}
