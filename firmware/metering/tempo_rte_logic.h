#pragma once

/*
 * tempo_rte_logic.h — Host-testable EDF Tempo calendar rules (api-couleur-tempo.fr + 6:00 rollover).
 * HTTPS fetch lives in tempo_rte.cpp.
 */

#include <cstdint>
#include <string>

/** Undefined tariff/color label (MQTT + previous UI wire value). */
constexpr const char kTempoRteLabelUndefined[] = "UNDEFINED";

constexpr const char kTempoColorBlue[] = "TEMPO_BLEU";
constexpr const char kTempoColorWhite[] = "TEMPO_BLANC";
constexpr const char kTempoColorRed[] = "TEMPO_ROUGE";

/** Linky STGE tomorrow nibble (hex character). */
constexpr const char kStgeTomorrowUnset[] = "0";
constexpr const char kStgeTomorrowBlue[] = "4";
constexpr const char kStgeTomorrowWhite[] = "8";
constexpr const char kStgeTomorrowRed[] = "C";

constexpr int kHalfHourSlotRollover6h = 300;
constexpr int kHalfHourSlotResetTomorrowStge1030 = 529;

struct TempoRteState {
  bool enabled = false;
  std::string ltarf;
  std::string tomorrow_stge_hex;
  std::string today_color_label;
  std::string tomorrow_color_label;
  int last_poll_time_decihours = -1;
  uint32_t last_fetch_epoch = 0;
};

/** Half-hour index for poll debounce (time_decihours / 2). */
int tempo_rte_logic_half_hour_slot(int time_decihours);

/** Clear tomorrow STGE at 10:30 half-hour slot. */
void tempo_rte_logic_reset_tomorrow_stge_at_1030(int half_hour_slot, std::string &tomorrow_stge_hex);

bool tempo_rte_logic_has_today_tempo_color(const std::string &ltarf);
bool tempo_rte_logic_has_tomorrow_stge_color(const std::string &tomorrow_stge_hex);

/** At 6:00 slot, promote tomorrow STGE nibble to today LTARF. */
bool tempo_rte_logic_apply_6h_rollover(int half_hour_slot, TempoRteState &state);

bool tempo_rte_logic_should_poll(const TempoRteState &state, int time_decihours, bool time_valid,
                                 bool network_allowed);

/** Calendar dates for Tempo day (6:00 Europe/Paris boundary via localtime). */
void tempo_rte_logic_format_tempo_dates(std::string &date_today_out, std::string &date_tomorrow_out);

/** Reject HTML error pages mistaken for JSON. */
bool tempo_rte_logic_body_looks_like_json(const std::string &body);

std::string tempo_rte_logic_map_rte_color_token(const std::string &token);

/**
 * Parse tempoLight JSON for date keys (YYYY-MM-DD).
 * @return true if at least one date key matched.
 */
bool tempo_rte_logic_parse_tempo_light(const std::string &rte_json, const std::string &date_today,
                                       const std::string &date_tomorrow, TempoRteState &state);

/** Parse tempo_like_calendars sandbox/production JSON (start_date + value fields). */
bool tempo_rte_logic_parse_tempo_like_calendars(const std::string &rte_json, const std::string &date_today,
                                                const std::string &date_tomorrow, TempoRteState &state);

/**
 * Parse api-couleur-tempo.fr jourTempo JSON (dateJour, libCouleur, codeJour).
 * @return true when dateJour and a color token were extracted.
 */
bool tempo_rte_logic_parse_jour_tempo(const std::string &json, std::string &date_out,
                                      std::string &color_token_out);

/** Apply today + tomorrow jourTempo response bodies into TempoRteState. */
bool tempo_rte_logic_apply_jour_tempo_responses(const std::string &today_json,
                                                const std::string &tomorrow_json, TempoRteState &state);

/** LTARF bitmask for tariff-conditioned actions. */
int tempo_rte_logic_ltarf_bin(const std::string &ltarf);

/** MQTT Code_Tarifaire from LTARF text (1–16 Linky, 17–19 Tempo RTE). */
int tempo_rte_logic_tariff_code(const std::string &ltarf);

bool tempo_rte_logic_expose_tariff_mqtt(bool enabled, bool source_is_linky);
