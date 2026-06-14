#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_LINKY

#include "tempo_rte_logic.h"

#include <cctype>
#include <cstring>
#include <ctime>

#if defined(BALANSUN_NATIVE_TEST)
bool g_balansun_test_force_localtime_fail = false;
#endif

namespace {

bool contains_icase(const std::string &haystack, const char *needle) {
  const std::string n(needle);
  if (haystack.size() < n.size()) return false;
  for (size_t i = 0; i + n.size() <= haystack.size(); ++i) {
    bool match = true;
    for (size_t j = 0; j < n.size(); ++j) {
      if (std::tolower(static_cast<unsigned char>(haystack[i + j])) !=
          std::tolower(static_cast<unsigned char>(n[j]))) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

std::string extract_json_string_value(const std::string &json, const std::string &quoted_key) {
  const size_t pos = json.find(quoted_key);
  if (pos == std::string::npos) return "";
  size_t i = pos + quoted_key.size();
  while (i < json.size() && (json[i] == ' ' || json[i] == ':' || json[i] == '\t')) i++;
  if (i >= json.size() || json[i] != '"') return "";
  i++;
  const size_t start = i;
  while (i < json.size() && json[i] != '"') i++;
  if (i <= start) return "";
  return json.substr(start, i - start);
}

bool is_tempo_color_label(const std::string &ltarf) {
  return ltarf == kTempoColorBlue || ltarf == kTempoColorWhite || ltarf == kTempoColorRed;
}

void set_tomorrow_from_token(const std::string &token, TempoRteState &state) {
  state.tomorrow_stge_hex = kStgeTomorrowUnset;
  state.tomorrow_color_label = kTempoRteLabelUndefined;
  if (contains_icase(token, "BLUE")) {
    state.tomorrow_stge_hex = kStgeTomorrowBlue;
    state.tomorrow_color_label = kTempoColorBlue;
  } else if (contains_icase(token, "WHITE")) {
    state.tomorrow_stge_hex = kStgeTomorrowWhite;
    state.tomorrow_color_label = kTempoColorWhite;
  } else if (contains_icase(token, "RED")) {
    state.tomorrow_stge_hex = kStgeTomorrowRed;
    state.tomorrow_color_label = kTempoColorRed;
  }
}

void set_tomorrow_from_ltarf(const std::string &ltarf, TempoRteState &state) {
  state.tomorrow_stge_hex = kStgeTomorrowUnset;
  state.tomorrow_color_label = kTempoRteLabelUndefined;
  if (ltarf == kTempoColorBlue) {
    state.tomorrow_stge_hex = kStgeTomorrowBlue;
    state.tomorrow_color_label = kTempoColorBlue;
  } else if (ltarf == kTempoColorWhite) {
    state.tomorrow_stge_hex = kStgeTomorrowWhite;
    state.tomorrow_color_label = kTempoColorWhite;
  } else if (ltarf == kTempoColorRed) {
    state.tomorrow_stge_hex = kStgeTomorrowRed;
    state.tomorrow_color_label = kTempoColorRed;
  }
}

int extract_json_int_value(const std::string &json, const std::string &quoted_key) {
  const size_t pos = json.find(quoted_key);
  if (pos == std::string::npos) return -1;
  size_t i = pos + quoted_key.size();
  while (i < json.size() && (json[i] == ' ' || json[i] == ':' || json[i] == '\t')) i++;
  if (i >= json.size() || !std::isdigit(static_cast<unsigned char>(json[i]))) return -1;
  int value = 0;
  while (i < json.size() && std::isdigit(static_cast<unsigned char>(json[i]))) {
    value = value * 10 + (json[i] - '0');
    i++;
  }
  return value;
}

std::string color_token_from_code_jour(int code_jour) {
  if (code_jour == 1) return "Bleu";
  if (code_jour == 2) return "Blanc";
  if (code_jour == 3) return "Rouge";
  return "";
}

}  // namespace

int tempo_rte_logic_half_hour_slot(int time_decihours) { return time_decihours / 2; }

void tempo_rte_logic_reset_tomorrow_stge_at_1030(int half_hour_slot, std::string &tomorrow_stge_hex) {
  if (half_hour_slot == kHalfHourSlotResetTomorrowStge1030) {
    tomorrow_stge_hex = kStgeTomorrowUnset;
  }
}

bool tempo_rte_logic_has_today_tempo_color(const std::string &ltarf) { return is_tempo_color_label(ltarf); }

bool tempo_rte_logic_has_tomorrow_stge_color(const std::string &tomorrow_stge_hex) {
  return tomorrow_stge_hex == kStgeTomorrowBlue || tomorrow_stge_hex == kStgeTomorrowWhite ||
         tomorrow_stge_hex == kStgeTomorrowRed;
}

bool tempo_rte_logic_apply_6h_rollover(int half_hour_slot, TempoRteState &state) {
  if (half_hour_slot != kHalfHourSlotRollover6h) return false;
  if (state.ltarf.empty() || state.tomorrow_stge_hex.empty()) return false;
  if (state.tomorrow_stge_hex == kStgeTomorrowBlue) {
    state.ltarf = kTempoColorBlue;
  } else if (state.tomorrow_stge_hex == kStgeTomorrowWhite) {
    state.ltarf = kTempoColorWhite;
  } else if (state.tomorrow_stge_hex == kStgeTomorrowRed) {
    state.ltarf = kTempoColorRed;
  } else {
    return false;
  }
  state.tomorrow_stge_hex.clear();
  if (state.enabled) {
    state.today_color_label = kTempoRteLabelUndefined;
    state.tomorrow_color_label = kTempoRteLabelUndefined;
  }
  return true;
}

bool tempo_rte_logic_should_poll(const TempoRteState &state, int time_decihours, bool time_valid,
                                 bool network_allowed) {
  if (!time_valid || !network_allowed || !state.enabled) return false;
  if (tempo_rte_logic_has_today_tempo_color(state.ltarf) &&
      tempo_rte_logic_has_tomorrow_stge_color(state.tomorrow_stge_hex)) {
    return false;
  }
  const int slot = tempo_rte_logic_half_hour_slot(time_decihours);
  const int last_slot = tempo_rte_logic_half_hour_slot(state.last_poll_time_decihours);
  if (state.last_poll_time_decihours >= 0 && last_slot == slot) return false;
  return !tempo_rte_logic_has_today_tempo_color(state.ltarf) ||
         !tempo_rte_logic_has_tomorrow_stge_color(state.tomorrow_stge_hex);
}

void tempo_rte_logic_format_tempo_dates(std::string &date_today_out, std::string &date_tomorrow_out) {
  const time_t ts = time(nullptr) - 6 * 3600;
  struct tm tm_buf;
  struct tm *pt = localtime(&ts);
#if defined(BALANSUN_NATIVE_TEST)
  if (g_balansun_test_force_localtime_fail) {
    date_today_out.clear();
    date_tomorrow_out.clear();
    return;
  }
#endif
  if (!pt) {
    date_today_out.clear();
    date_tomorrow_out.clear();
    return;
  }
  tm_buf = *pt;
  char buffer[16];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm_buf);
  date_today_out = buffer;
  tm_buf.tm_mday += 1;
  mktime(&tm_buf);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm_buf);
  date_tomorrow_out = buffer;
}

bool tempo_rte_logic_body_looks_like_json(const std::string &body) {
  const size_t i = body.find_first_not_of(" \t\r\n");
  if (i == std::string::npos) return false;
  if (body[i] != '{' && body[i] != '[') return false;
  if (body.find("<html") != std::string::npos || body.find("<!DOCTYPE") != std::string::npos ||
      body.find("<HTML") != std::string::npos) {
    return false;
  }
  return true;
}

std::string tempo_rte_logic_map_rte_color_token(const std::string &token) {
  if (token == "BLUE") return kTempoColorBlue;
  if (token == "WHITE") return kTempoColorWhite;
  if (token == "RED") return kTempoColorRed;
  if (contains_icase(token, "BLUE")) return kTempoColorBlue;
  if (contains_icase(token, "WHITE")) return kTempoColorWhite;
  if (contains_icase(token, "RED")) return kTempoColorRed;
  if (contains_icase(token, "BLEU")) return kTempoColorBlue;
  if (contains_icase(token, "BLANC")) return kTempoColorWhite;
  if (contains_icase(token, "ROUGE")) return kTempoColorRed;
  return "";
}

bool tempo_rte_logic_parse_jour_tempo(const std::string &json, std::string &date_out,
                                      std::string &color_token_out) {
  date_out.clear();
  color_token_out.clear();
  if (!tempo_rte_logic_body_looks_like_json(json)) return false;
  date_out = extract_json_string_value(json, "\"dateJour\"");
  color_token_out = extract_json_string_value(json, "\"libCouleur\"");
  if (color_token_out.empty()) {
    const int code_jour = extract_json_int_value(json, "\"codeJour\"");
    color_token_out = color_token_from_code_jour(code_jour);
  }
  return !date_out.empty() && !color_token_out.empty();
}

bool tempo_rte_logic_apply_jour_tempo_responses(const std::string &today_json,
                                                const std::string &tomorrow_json, TempoRteState &state) {
  bool applied = false;

  if (!today_json.empty()) {
    std::string date;
    std::string color;
    if (tempo_rte_logic_parse_jour_tempo(today_json, date, color)) {
      const std::string mapped = tempo_rte_logic_map_rte_color_token(color);
      if (!mapped.empty()) {
        state.ltarf = mapped;
        state.today_color_label = mapped;
        applied = true;
      }
    }
  }

  if (!tomorrow_json.empty()) {
    std::string date;
    std::string color;
    if (tempo_rte_logic_parse_jour_tempo(tomorrow_json, date, color)) {
      const std::string mapped = tempo_rte_logic_map_rte_color_token(color);
      if (!mapped.empty()) {
        set_tomorrow_from_ltarf(mapped, state);
        applied = true;
      }
    }
  }

  if (!applied) return false;
  if (state.today_color_label.empty()) {
    state.today_color_label = state.ltarf.empty() ? kTempoRteLabelUndefined : state.ltarf;
  }
  return true;
}

static bool find_calendar_color_for_date(const std::string &json, const std::string &date_yyyy_mm_dd,
                                         std::string &token_out) {
  size_t pos = 0;
  while ((pos = json.find(date_yyyy_mm_dd, pos)) != std::string::npos) {
    const size_t slice_end = std::min(pos + 400, json.size());
    const std::string slice = json.substr(pos, slice_end - pos);
    const std::string val = extract_json_string_value(slice, "\"value\"");
    if (!val.empty()) {
      token_out = val;
      return true;
    }
    pos += date_yyyy_mm_dd.size();
  }
  return false;
}

bool tempo_rte_logic_parse_tempo_like_calendars(const std::string &rte_json, const std::string &date_today,
                                                const std::string &date_tomorrow, TempoRteState &state) {
  std::string today_token;
  std::string tomorrow_token;
  const bool has_today = find_calendar_color_for_date(rte_json, date_today, today_token);
  const bool has_tomorrow = find_calendar_color_for_date(rte_json, date_tomorrow, tomorrow_token);
  if (!has_today && !has_tomorrow) return false;
  if (!today_token.empty()) {
    const std::string mapped = tempo_rte_logic_map_rte_color_token(today_token);
    if (!mapped.empty()) state.ltarf = mapped;
  }
  const std::string tomorrow_token_use = tomorrow_token;
  state.today_color_label = state.ltarf.empty() ? kTempoRteLabelUndefined : state.ltarf;
  state.tomorrow_color_label = kTempoRteLabelUndefined;
  state.tomorrow_stge_hex = kStgeTomorrowUnset;
  set_tomorrow_from_token(tomorrow_token_use, state);
  return true;
}

bool tempo_rte_logic_parse_tempo_light(const std::string &rte_json, const std::string &date_today,
                                       const std::string &date_tomorrow, TempoRteState &state) {
  const std::string key_today = "\"" + date_today + "\"";
  const std::string key_tomorrow = "\"" + date_tomorrow + "\"";
  if (rte_json.find(key_today) == std::string::npos && rte_json.find(key_tomorrow) == std::string::npos) {
    return false;
  }
  const std::string today_token = extract_json_string_value(rte_json, key_today);
  if (!today_token.empty()) {
    const std::string mapped = tempo_rte_logic_map_rte_color_token(today_token);
    if (!mapped.empty()) state.ltarf = mapped;
  }
  const std::string tomorrow_token = extract_json_string_value(rte_json, key_tomorrow);
  state.today_color_label = state.ltarf.empty() ? kTempoRteLabelUndefined : state.ltarf;
  state.tomorrow_color_label = kTempoRteLabelUndefined;
  state.tomorrow_stge_hex = kStgeTomorrowUnset;
  set_tomorrow_from_token(tomorrow_token, state);
  return true;
}

int tempo_rte_logic_ltarf_bin(const std::string &ltarf) {
  int bin = 0;
  if (contains_icase(ltarf, "PLEINE")) bin += 1;
  if (contains_icase(ltarf, "CREUSE")) bin += 2;
  if (contains_icase(ltarf, "BLEU")) bin += 4;
  if (contains_icase(ltarf, "BLANC")) bin += 8;
  if (contains_icase(ltarf, "ROUGE")) bin += 16;
  return bin;
}

int tempo_rte_logic_tariff_code(const std::string &ltarf) {
  int code = 0;
  if (ltarf.find("HEURE  CREUSE") != std::string::npos) code = 1;
  if (ltarf.find("HEURE  PLEINE") != std::string::npos) code = 2;
  if (ltarf.find("HC BLEU") != std::string::npos) code = 11;
  if (ltarf.find("HP BLEU") != std::string::npos) code = 12;
  if (ltarf.find("HC BLANC") != std::string::npos) code = 13;
  if (ltarf.find("HP BLANC") != std::string::npos) code = 14;
  if (ltarf.find("HC ROUGE") != std::string::npos) code = 15;
  if (ltarf.find("HP ROUGE") != std::string::npos) code = 16;
  if (ltarf.find(kTempoColorBlue) != std::string::npos) code = 17;
  if (ltarf.find(kTempoColorWhite) != std::string::npos) code = 18;
  if (ltarf.find(kTempoColorRed) != std::string::npos) code = 19;
  return code;
}

bool tempo_rte_logic_expose_tariff_mqtt(bool enabled, bool source_is_linky) {
  return source_is_linky || enabled;
}

#else

#include "tempo_rte_logic.h"

int tempo_rte_logic_ltarf_bin(const std::string &) { return 0; }

int tempo_rte_logic_tariff_code(const std::string &) { return 0; }

#endif
