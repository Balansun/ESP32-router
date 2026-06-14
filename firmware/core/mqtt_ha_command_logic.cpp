#include "mqtt_ha_command_logic.h"

#include <cctype>

namespace {

std::string trim_copy(const std::string &msg) {
  size_t start = 0;
  while (start < msg.size() && std::isspace(static_cast<unsigned char>(msg[start]))) start++;
  size_t end = msg.size();
  while (end > start && std::isspace(static_cast<unsigned char>(msg[end - 1]))) end--;
  return msg.substr(start, end - start);
}

bool equals_ignore_case(const std::string &a, const char *literal) {
  const size_t n = std::char_traits<char>::length(literal);
  if (a.size() != n) return false;
  for (size_t i = 0; i < n; i++) {
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(literal[i]))) {
      return false;
    }
  }
  return true;
}

bool is_unsigned_decimal(const std::string &s) {
  if (s.empty()) return false;
  for (char c : s) {
    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
  }
  return true;
}

}  // namespace

bool mqtt_ha_command_is_json_like_payload(const std::string &msg) {
  const std::string t = trim_copy(msg);
  return !t.empty() && (t.front() == '{' || t.front() == '[');
}

bool mqtt_ha_command_parse_triac(const std::string &msg, MqttTriacCmd *out) {
  if (!out) return false;
  *out = MqttTriacCmd{};
  const std::string t = trim_copy(msg);
  if (t.empty() || mqtt_ha_command_is_json_like_payload(t)) return false;
  if (equals_ignore_case(t, "AUTO")) {
    out->kind = MqttTriacCmdKind::Auto;
    return true;
  }
  if (!is_unsigned_decimal(t)) return false;
  int pct = 0;
  for (char c : t) {
    pct = pct * 10 + (c - '0');
    if (pct > 100) {
      pct = 100;
      break;
    }
  }
  out->kind = MqttTriacCmdKind::FixedPercent;
  out->percent = pct;
  return true;
}

bool mqtt_ha_command_parse_action(const std::string &msg, MqttActionCmdKind *out) {
  if (!out) return false;
  *out = MqttActionCmdKind::Invalid;
  const std::string t = trim_copy(msg);
  if (t.empty() || mqtt_ha_command_is_json_like_payload(t)) return false;
  if (equals_ignore_case(t, "ON")) {
    *out = MqttActionCmdKind::On;
    return true;
  }
  if (equals_ignore_case(t, "OFF")) {
    *out = MqttActionCmdKind::Off;
    return true;
  }
  if (equals_ignore_case(t, "AUTO")) {
    *out = MqttActionCmdKind::Auto;
    return true;
  }
  return false;
}
