#include "balansun_meter_json_logic.h"

#include <cstring>

namespace {

bool contains_block(const std::string &json, const char *key) {
  const std::string needle = std::string("\"") + key + "\"";
  return json.find(needle) != std::string::npos;
}

bool block_has_negative_power(const std::string &json, const char *block_key) {
  const std::string block = std::string("\"") + block_key + "\"";
  const auto start = json.find(block);
  const auto brace = json.find('{', start);
  if (brace == std::string::npos) return false;
  int depth = 0;
  for (size_t i = brace; i < json.size(); ++i) {
    if (json[i] == '{') depth++;
    if (json[i] == '}') {
      depth--;
      if (depth == 0) {
        const std::string slice = json.substr(brace, i - brace + 1);
        const char *keys[] = {"active_import_w", "active_export_w", "apparent_import_va", "apparent_export_va"};
        bool any = false;
        for (const char *k : keys) {
          const std::string field = std::string("\"") + k + "\":";
          const auto fp = slice.find(field);
          if (fp == std::string::npos) continue;
          const auto num_start = slice.find_first_of("-0123456789", fp + field.size());
          if (num_start == std::string::npos) continue;
          const double v = std::strtod(slice.c_str() + num_start, nullptr);
          if (v < 0) return true;
          any = true;
        }
        return !any;
      }
    }
  }
  return false;
}

}  // namespace

bool balansun_meter_json_logic_validate_inject(const std::string &json, std::string *err_out) {
  if (json.empty()) {
    if (err_out) *err_out = "empty_body";
    return false;
  }
  bool any = false;
  if (contains_block(json, "house")) {
    if (block_has_negative_power(json, "house")) {
      if (err_out) *err_out = "negative_power";
      return false;
    }
    any = true;
  }
  if (contains_block(json, "second")) {
    if (block_has_negative_power(json, "second")) {
      if (err_out) *err_out = "negative_power";
      return false;
    }
    any = true;
  }
  if (contains_block(json, "raw_meter")) {
    any = true;
  }
  if (!any) {
    if (err_out) *err_out = "no_meter_fields";
    return false;
  }
  return true;
}
