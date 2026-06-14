#pragma once

#include <cstdint>
#include <string>

/** GPIO read mask (SPI flash bus 6–11 only). */
bool api_logic_is_restricted_gpio_read(int gpio);

/** GPIO write mask for REST PUT /api/v1/gpio. */
bool api_logic_is_restricted_gpio_write(int gpio, bool serial_adc_gpio_restrict);

std::string api_logic_ip32_to_dotted(uint32_t ip);
bool api_logic_dotted_to_ip32(const char *s, uint32_t &out);

bool api_logic_validate_password_ascii(const std::string &s, std::string &err);

/** True when URI is under /api/v1/ (lab CORS and preflight scope). */
bool api_logic_uri_is_api_v1(const char *uri);

/** Lab CORS preflight: OPTIONS on /api/v1/* when flag is enabled. */
bool api_logic_should_handle_cors_preflight(bool http_cors_enabled, bool is_options_method, const char *uri);

/** OPTIONS on /api/v1/* bypasses session auth when lab CORS is on (browser preflight). */
bool api_logic_cors_preflight_bypasses_auth(bool http_cors_enabled, bool is_options_method, const char *uri);
