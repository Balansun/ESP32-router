#pragma once

#include <stddef.h>
#include <string>

/** Extract Bearer token from `Authorization` header; false if missing or malformed. */
bool api_logic_parse_bearer_token(const std::string &authHeader, std::string &tokenOut);

/** Session tokens are 64 lowercase hex chars (32 random bytes). */
bool api_logic_is_valid_session_token_format(const std::string &token);

bool api_logic_session_token_constant_time_eq(const std::string &a, const std::string &b);

bool api_logic_password_constant_time_eq(const std::string &a, const std::string &b);
