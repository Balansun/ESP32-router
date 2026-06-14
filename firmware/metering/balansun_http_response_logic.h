#pragma once

/*
 * balansun_http_response_logic.h — Host-testable HTTP/1.x wire parsing for balansun_lan_http_get.
 */

#include <cstddef>
#include <string>

/** True when the status line contains an HTTP 2xx code. */
bool balansun_http_response_status_ok(const std::string &wire);

/**
 * Extract response body from a full HTTP wire buffer (headers + body).
 * Decodes simple Transfer-Encoding: chunked when present.
 * @return empty string on missing headers, non-2xx, or empty body.
 */
std::string balansun_http_response_extract_body(const std::string &wire);
