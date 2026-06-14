#include "balansun_meter_transport_guards.h"

#if BALANSUN_METER_NEEDS_HTTP_RESPONSE

#include "balansun_http_response_logic.h"

#include <cctype>

namespace {

bool header_value_icase_contains(const std::string &headers, const char *name, const char *needle) {
  const std::string key(name);
  size_t pos = 0;
  while (pos < headers.size()) {
    const size_t line_end = headers.find("\r\n", pos);
    const size_t end = (line_end == std::string::npos) ? headers.size() : line_end;
    const std::string line = headers.substr(pos, end - pos);
    pos = (line_end == std::string::npos) ? headers.size() : line_end + 2;
    if (line.size() < key.size()) continue;
    bool match = true;
    for (size_t i = 0; i < key.size(); ++i) {
      if (std::tolower(static_cast<unsigned char>(line[i])) !=
          std::tolower(static_cast<unsigned char>(key[i]))) {
        match = false;
        break;
      }
    }
    if (!match || line.size() <= key.size() || line[key.size()] != ':') continue;
    size_t v = key.size() + 1;
    while (v < line.size() && (line[v] == ' ' || line[v] == '\t')) v++;
    const std::string value = line.substr(v);
    if (value.find(needle) != std::string::npos) return true;
  }
  return false;
}

std::string decode_chunked_body(const std::string &chunked) {
  std::string out;
  size_t pos = 0;
  while (pos < chunked.size()) {
    const size_t line_end = chunked.find("\r\n", pos);
    if (line_end == std::string::npos) return "";
    const std::string size_line = chunked.substr(pos, line_end - pos);
    pos = line_end + 2;
    char *end = nullptr;
    const unsigned long chunk_size = std::strtoul(size_line.c_str(), &end, 16);
    if (end == size_line.c_str()) return "";
    if (chunk_size == 0) {
      return out;
    }
    if (pos + chunk_size > chunked.size()) return "";
    out.append(chunked, pos, chunk_size);
    pos += chunk_size;
    if (pos + 2 > chunked.size() || chunked[pos] != '\r' || chunked[pos + 1] != '\n') return "";
    pos += 2;
  }
  return "";
}

}  // namespace

bool balansun_http_response_status_ok(const std::string &wire) {
  const size_t sp = wire.find(' ');
  if (sp == std::string::npos) return false;
  const size_t sp2 = wire.find(' ', sp + 1);
  if (sp2 == std::string::npos) return false;
  int code = 0;
  for (size_t i = sp + 1; i < sp2; ++i) {
    if (!std::isdigit(static_cast<unsigned char>(wire[i]))) return false;
    code = code * 10 + (wire[i] - '0');
  }
  return code >= 200 && code < 300;
}

std::string balansun_http_response_extract_body(const std::string &wire) {
  const size_t hdr_end = wire.find("\r\n\r\n");
  if (hdr_end == std::string::npos) return "";
  if (!balansun_http_response_status_ok(wire)) return "";
  const std::string headers = wire.substr(0, hdr_end);
  std::string body = wire.substr(hdr_end + 4);
  if (header_value_icase_contains(headers, "Transfer-Encoding", "chunked")) {
    body = decode_chunked_body(body);
  } else {
    while (!body.empty() && (body.back() == '\r' || body.back() == '\n' || body.back() == ' ')) {
      body.pop_back();
    }
  }
  return body;
}

#endif
