#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "tempo_rte_logic.h"

TEST(TempoRteLogic, ParsesTempoLightFixture) {
  std::ifstream in("firmware/test/fixtures/tempo/tempo_light_sample.json");
  std::stringstream ss;
  ss << in.rdbuf();
  TempoRteState st;
  st.enabled = true;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(ss.str(), "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.ltarf, kTempoColorWhite);
  EXPECT_EQ(st.tomorrow_color_label, kTempoColorRed);
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowRed);
}

TEST(TempoRteLogic, SixAmRollover) {
  TempoRteState st;
  st.ltarf = kTempoColorBlue;
  st.tomorrow_stge_hex = kStgeTomorrowWhite;
  ASSERT_TRUE(tempo_rte_logic_apply_6h_rollover(kHalfHourSlotRollover6h, st));
  EXPECT_EQ(st.ltarf, kTempoColorWhite);
  EXPECT_TRUE(st.tomorrow_stge_hex.empty());
}

TEST(TempoRteLogic, TariffCodeTempo) {
  EXPECT_EQ(tempo_rte_logic_tariff_code(kTempoColorRed), 19);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HC BLEU"), 11);
}

TEST(TempoRteLogic, ShouldPollWhenTodayColorMissing) {
  TempoRteState st;
  st.enabled = true;
  st.ltarf.clear();
  st.tomorrow_stge_hex = kStgeTomorrowUnset;
  st.last_poll_time_decihours = 100;
  EXPECT_TRUE(tempo_rte_logic_should_poll(st, 500, true, true));
}

TEST(TempoRteLogic, RejectsHtmlErrorBody) {
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json("<!DOCTYPE html><html>"));
  EXPECT_TRUE(tempo_rte_logic_body_looks_like_json(R"({"values":{"2025-05-17":"BLUE"}})"));
}

TEST(TempoRteLogic, ParsesTempoLightValuesWrapper) {
  const char *json =
      R"({"values":{"2025-05-17":"WHITE","2025-05-18":"RED","2025-05-17-fallback":"false"}})";
  TempoRteState st;
  st.enabled = true;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(json, "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.ltarf, kTempoColorWhite);
}

TEST(TempoRteLogic, ParsesTempoLikeCalendars) {
  const char *json =
      R"({"2025-05-17":{"value":"blue"},"2025-05-18":{"value":"RED"}})";
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_like_calendars(json, "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.ltarf, kTempoColorBlue);
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowRed);
}

TEST(TempoRteLogic, MapRteColorTokenVariants) {
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("BLUE"), kTempoColorBlue);
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("mixed_white"), kTempoColorWhite);
  EXPECT_TRUE(tempo_rte_logic_map_rte_color_token("unknown").empty());
}

TEST(TempoRteLogic, HalfHourSlotAndResetStge) {
  EXPECT_EQ(tempo_rte_logic_half_hour_slot(612), 306);
  std::string stge = kStgeTomorrowBlue;
  tempo_rte_logic_reset_tomorrow_stge_at_1030(kHalfHourSlotResetTomorrowStge1030, stge);
  EXPECT_EQ(stge, kStgeTomorrowUnset);
}

TEST(TempoRteLogic, HasTodayAndTomorrowColors) {
  EXPECT_TRUE(tempo_rte_logic_has_today_tempo_color(kTempoColorBlue));
  EXPECT_TRUE(tempo_rte_logic_has_today_tempo_color(kTempoColorWhite));
  EXPECT_TRUE(tempo_rte_logic_has_today_tempo_color(kTempoColorRed));
  EXPECT_FALSE(tempo_rte_logic_has_today_tempo_color("HC BLEU"));
  EXPECT_TRUE(tempo_rte_logic_has_tomorrow_stge_color(kStgeTomorrowBlue));
  EXPECT_TRUE(tempo_rte_logic_has_tomorrow_stge_color(kStgeTomorrowWhite));
  EXPECT_TRUE(tempo_rte_logic_has_tomorrow_stge_color(kStgeTomorrowRed));
  EXPECT_FALSE(tempo_rte_logic_has_tomorrow_stge_color(kStgeTomorrowUnset));
}

TEST(TempoRteLogic, SixAmRolloverAllStgeAndFailures) {
  TempoRteState st;
  st.ltarf = kTempoColorWhite;
  st.tomorrow_stge_hex = kStgeTomorrowBlue;
  st.enabled = true;
  ASSERT_TRUE(tempo_rte_logic_apply_6h_rollover(kHalfHourSlotRollover6h, st));
  EXPECT_EQ(st.ltarf, kTempoColorBlue);
  EXPECT_EQ(st.today_color_label, kTempoRteLabelUndefined);

  st.tomorrow_stge_hex = kStgeTomorrowRed;
  ASSERT_TRUE(tempo_rte_logic_apply_6h_rollover(kHalfHourSlotRollover6h, st));
  EXPECT_EQ(st.ltarf, kTempoColorRed);

  st.tomorrow_stge_hex = "X";
  EXPECT_FALSE(tempo_rte_logic_apply_6h_rollover(kHalfHourSlotRollover6h, st));
  EXPECT_FALSE(tempo_rte_logic_apply_6h_rollover(0, st));
  st.ltarf.clear();
  st.tomorrow_stge_hex = kStgeTomorrowWhite;
  EXPECT_FALSE(tempo_rte_logic_apply_6h_rollover(kHalfHourSlotRollover6h, st));
}

TEST(TempoRteLogic, ShouldPollBranches) {
  TempoRteState st;
  st.enabled = true;
  st.ltarf = kTempoColorBlue;
  st.tomorrow_stge_hex = kStgeTomorrowRed;
  st.last_poll_time_decihours = 100;
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 500, false, true));
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 500, true, false));
  st.enabled = false;
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 500, true, true));
  st.enabled = true;
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 500, true, true));

  st.ltarf.clear();
  st.tomorrow_stge_hex = kStgeTomorrowUnset;
  st.last_poll_time_decihours = 612;
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 612, true, true));

  st.ltarf = kTempoColorBlue;
  st.tomorrow_stge_hex = kStgeTomorrowUnset;
  st.last_poll_time_decihours = 500;
  EXPECT_TRUE(tempo_rte_logic_should_poll(st, 612, true, true));
}

#if defined(BALANSUN_NATIVE_TEST)
extern bool g_balansun_test_force_localtime_fail;

TEST(TempoRteLogic, FormatTempoDatesWhenLocaltimeFails) {
  g_balansun_test_force_localtime_fail = true;
  std::string today;
  std::string tomorrow;
  tempo_rte_logic_format_tempo_dates(today, tomorrow);
  EXPECT_TRUE(today.empty());
  EXPECT_TRUE(tomorrow.empty());
  g_balansun_test_force_localtime_fail = false;
}
#endif

TEST(TempoRteLogic, FormatTempoDates) {
  std::string today;
  std::string tomorrow;
  tempo_rte_logic_format_tempo_dates(today, tomorrow);
  EXPECT_FALSE(today.empty());
  EXPECT_FALSE(tomorrow.empty());
  EXPECT_NE(today, tomorrow);
}

TEST(TempoRteLogic, BodyLooksLikeJsonEdgeCases) {
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json("   "));
  EXPECT_TRUE(tempo_rte_logic_body_looks_like_json("[1,2]"));
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json("{\"x\":1}<HTML>"));
}

TEST(TempoRteLogic, LtarfBinAndTariffCodes) {
  EXPECT_EQ(tempo_rte_logic_ltarf_bin("HEURE PLEINE CREUSE BLEU BLANC ROUGE"), 31);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HEURE  CREUSE"), 1);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HEURE  PLEINE"), 2);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HC BLEU"), 11);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HP BLANC"), 14);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HC ROUGE"), 15);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HP ROUGE"), 16);
  EXPECT_EQ(tempo_rte_logic_tariff_code(kTempoColorBlue), 17);
  EXPECT_TRUE(tempo_rte_logic_expose_tariff_mqtt(true, false));
  EXPECT_TRUE(tempo_rte_logic_expose_tariff_mqtt(false, true));
  EXPECT_FALSE(tempo_rte_logic_expose_tariff_mqtt(false, false));
}

TEST(TempoRteLogic, ParseTempoLightRejectsMissingKeys) {
  TempoRteState st;
  EXPECT_FALSE(tempo_rte_logic_parse_tempo_light("{}", "2025-05-17", "2025-05-18", st));
  EXPECT_FALSE(tempo_rte_logic_parse_tempo_like_calendars("{}", "2025-05-17", "2025-05-18", st));
}

TEST(TempoRteLogic, TomorrowBlueAndWhiteTokens) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(
      R"({"2025-05-17":"RED","2025-05-18":"BLUE"})", "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowBlue);
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(
      R"({"2025-05-17":"RED","2025-05-18":"WHITE"})", "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowWhite);
}

TEST(TempoRteLogic, CalendarFindsSecondDateOccurrence) {
  const char *json = R"({"lead":"2025-05-17","2025-05-17":{"value":"RED"}})";
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_like_calendars(json, "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.ltarf, kTempoColorRed);
}

TEST(TempoRteLogic, MapRteColorExactTokens) {
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("RED"), kTempoColorRed);
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("mixed_red"), kTempoColorRed);
}

TEST(TempoRteLogic, SixAmRolloverDisabledClearsLabels) {
  TempoRteState st;
  st.ltarf = kTempoColorBlue;
  st.tomorrow_stge_hex = kStgeTomorrowWhite;
  st.enabled = false;
  ASSERT_TRUE(tempo_rte_logic_apply_6h_rollover(kHalfHourSlotRollover6h, st));
  EXPECT_EQ(st.ltarf, kTempoColorWhite);
}

TEST(TempoRteLogic, ParseLightWithUnmappedTodayToken) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(
      R"({"2025-05-17":"UNKNOWN","2025-05-18":"RED"})", "2025-05-17", "2025-05-18", st));
  EXPECT_TRUE(st.ltarf.empty());
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowRed);
}

TEST(TempoRteLogic, ParseCalendarSkipsEmptyValueSlice) {
  const char *json = R"({"2025-05-17":{"value":""},"2025-05-18":{"value":"BLUE"}})";
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_like_calendars(json, "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowBlue);
}

TEST(TempoRteLogic, ContainsIcaseEdgeCases) {
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json("not json"));
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token(""), "");
}

TEST(TempoRteLogic, ParsesJourTempoFixture) {
  std::ifstream in("firmware/test/fixtures/tempo/jour_tempo_bleu.json");
  std::stringstream ss;
  ss << in.rdbuf();
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(ss.str(), date, color));
  EXPECT_EQ(date, "2026-05-20");
  EXPECT_EQ(color, "Bleu");
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token(color), kTempoColorBlue);
}

TEST(TempoRteLogic, ParsesJourTempoFromCodeJour) {
  const char *json = R"({"dateJour":"2026-05-21","codeJour":3,"periode":"2025-2026"})";
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(json, date, color));
  EXPECT_EQ(color, "Rouge");
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token(color), kTempoColorRed);
}

TEST(TempoRteLogic, ParseJourTempoRejectsUnknownCodeJour) {
  const char *json = R"({"dateJour":"2026-05-21","codeJour":9})";
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(json, date, color));
}

TEST(TempoRteLogic, ApplyJourTempoTomorrowWhite) {
  const char *tomorrow = R"({"dateJour":"2026-05-21","codeJour":2})";
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_apply_jour_tempo_responses("", tomorrow, st));
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowWhite);
  EXPECT_EQ(st.tomorrow_color_label, kTempoColorWhite);
}

TEST(TempoRteLogic, ApplyJourTempoTodayAndTomorrow) {
  const char *today = R"({"dateJour":"2026-05-20","codeJour":2,"libCouleur":"Blanc"})";
  const char *tomorrow = R"({"dateJour":"2026-05-21","codeJour":3,"libCouleur":"Rouge"})";
  TempoRteState st;
  st.enabled = true;
  ASSERT_TRUE(tempo_rte_logic_apply_jour_tempo_responses(today, tomorrow, st));
  EXPECT_EQ(st.ltarf, kTempoColorWhite);
  EXPECT_EQ(st.today_color_label, kTempoColorWhite);
  EXPECT_EQ(st.tomorrow_color_label, kTempoColorRed);
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowRed);
}

TEST(TempoRteLogic, ApplyJourTempoTomorrowOnly) {
  const char *tomorrow = R"({"dateJour":"2026-05-21","codeJour":1,"libCouleur":"Bleu"})";
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_apply_jour_tempo_responses("", tomorrow, st));
  EXPECT_TRUE(st.ltarf.empty());
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowBlue);
}

TEST(TempoRteLogic, ParseJourTempoRejectsHtml) {
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo("<!DOCTYPE html><html>", date, color));
}

TEST(TempoRteLogic, MapFrenchCouleurLabels) {
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("Bleu"), kTempoColorBlue);
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("Blanc"), kTempoColorWhite);
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("Rouge"), kTempoColorRed);
}

TEST(TempoRteLogic, MapExactWhiteToken) {
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("WHITE"), kTempoColorWhite);
}

TEST(TempoRteLogic, HalfHourSlotEdges) {
  EXPECT_EQ(tempo_rte_logic_half_hour_slot(0), 0);
  EXPECT_EQ(tempo_rte_logic_half_hour_slot(1), 0);
  EXPECT_EQ(tempo_rte_logic_half_hour_slot(613), 306);
  std::string stge = kStgeTomorrowRed;
  tempo_rte_logic_reset_tomorrow_stge_at_1030(528, stge);
  EXPECT_EQ(stge, kStgeTomorrowRed);
}

TEST(TempoRteLogic, ShouldPollWhenTomorrowStgeMissing) {
  TempoRteState st;
  st.enabled = true;
  st.ltarf = kTempoColorBlue;
  st.tomorrow_stge_hex = kStgeTomorrowUnset;
  st.last_poll_time_decihours = 400;
  EXPECT_TRUE(tempo_rte_logic_should_poll(st, 612, true, true));
}

TEST(TempoRteLogic, ParseJourTempoCodeJourBleu) {
  const char *json = R"({"dateJour":"2026-05-20","codeJour":1})";
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(json, date, color));
  EXPECT_EQ(color, "Bleu");
}

TEST(TempoRteLogic, ParseJourTempoRejectsMissingDateOrColor) {
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"codeJour":1})", date, color));
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"dateJour":"2026-05-20"})", date, color));
}

TEST(TempoRteLogic, ApplyJourTempoRejectsWhenNothingApplied) {
  TempoRteState st;
  EXPECT_FALSE(tempo_rte_logic_apply_jour_tempo_responses("", "", st));
  EXPECT_FALSE(tempo_rte_logic_apply_jour_tempo_responses(
      R"({"dateJour":"2026-05-20","codeJour":9})", "", st));
}

TEST(TempoRteLogic, ApplyJourTempoTodayUnmappedColor) {
  const char *today = R"({"dateJour":"2026-05-20","libCouleur":"UNKNOWN"})";
  TempoRteState st;
  EXPECT_FALSE(tempo_rte_logic_apply_jour_tempo_responses(today, "", st));
}

TEST(TempoRteLogic, ParseTempoLightTomorrowKeyOnly) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(
      R"({"2025-05-18":"WHITE"})", "2025-05-17", "2025-05-18", st));
  EXPECT_TRUE(st.ltarf.empty());
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowWhite);
}

TEST(TempoRteLogic, ParseTempoLightTodayKeyOnly) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(R"({"2025-05-17":"BLUE"})", "2025-05-17",
                                                "2025-05-18", st));
  EXPECT_EQ(st.ltarf, kTempoColorBlue);
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowUnset);
}

TEST(TempoRteLogic, ParseCalendarTomorrowOnlyWhite) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_like_calendars(
      R"({"2025-05-18":{"value":"WHITE"}})", "2025-05-17", "2025-05-18", st));
  EXPECT_TRUE(st.ltarf.empty());
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowWhite);
}

TEST(TempoRteLogic, LtarfBinSingleFlags) {
  EXPECT_EQ(tempo_rte_logic_ltarf_bin("PLEINE"), 1);
  EXPECT_EQ(tempo_rte_logic_ltarf_bin("CREUSE"), 2);
  EXPECT_EQ(tempo_rte_logic_ltarf_bin("BLEU"), 4);
  EXPECT_EQ(tempo_rte_logic_ltarf_bin("BLANC"), 8);
  EXPECT_EQ(tempo_rte_logic_ltarf_bin("ROUGE"), 16);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HP BLEU"), 12);
  EXPECT_EQ(tempo_rte_logic_tariff_code(kTempoColorWhite), 18);
}

TEST(TempoRteLogic, ParseJourTempoPrefersLibCouleur) {
  const char *json =
      R"({"dateJour":"2026-05-20","libCouleur":"Rouge","codeJour":1})";
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(json, date, color));
  EXPECT_EQ(color, "Rouge");
}

TEST(TempoRteLogic, ApplyJourTempoTodayOnly) {
  const char *today = R"({"dateJour":"2026-05-20","codeJour":2,"libCouleur":"Blanc"})";
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_apply_jour_tempo_responses(today, "", st));
  EXPECT_EQ(st.ltarf, kTempoColorWhite);
  EXPECT_EQ(st.today_color_label, kTempoColorWhite);
}

TEST(TempoRteLogic, ShouldPollWhenNeverPolledBefore) {
  TempoRteState st;
  st.enabled = true;
  st.ltarf.clear();
  st.tomorrow_stge_hex = kStgeTomorrowUnset;
  st.last_poll_time_decihours = -1;
  EXPECT_TRUE(tempo_rte_logic_should_poll(st, 612, true, true));
}

TEST(TempoRteLogic, ParseCalendarUnmappedTodayToken) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_like_calendars(
      R"({"2025-05-17":{"value":"GARBAGE"},"2025-05-18":{"value":"BLUE"}})",
      "2025-05-17", "2025-05-18", st));
  EXPECT_TRUE(st.ltarf.empty());
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowBlue);
}

TEST(TempoRteLogic, ParseLightEmptyTodayToken) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(
      R"({"2025-05-17":"","2025-05-18":"RED"})", "2025-05-17", "2025-05-18", st));
  EXPECT_TRUE(st.ltarf.empty());
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowRed);
}

TEST(TempoRteLogic, MapBlancSubstringToken) {
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("demain_BLANC"), kTempoColorWhite);
}

TEST(TempoRteLogic, ApplyJourTempoInvalidTodayValidTomorrow) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_apply_jour_tempo_responses(
      R"({"dateJour":"2026-05-20"})", R"({"dateJour":"2026-05-21","codeJour":3})", st));
  EXPECT_TRUE(st.ltarf.empty());
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowRed);
}

TEST(TempoRteLogic, ShouldPollSkipsWhenDisabledOrOffline) {
  TempoRteState st;
  st.enabled = false;
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 612, true, true));
  st.enabled = true;
  st.ltarf.clear();
  st.tomorrow_stge_hex = kStgeTomorrowUnset;
  st.last_poll_time_decihours = 612;
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 612, true, false));
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 612, false, true));
}

TEST(TempoRteLogic, ExposeTariffMqttWhenEnabledOrLinky) {
  EXPECT_TRUE(tempo_rte_logic_expose_tariff_mqtt(true, false));
  EXPECT_TRUE(tempo_rte_logic_expose_tariff_mqtt(true, true));
  EXPECT_TRUE(tempo_rte_logic_expose_tariff_mqtt(false, true));
  EXPECT_FALSE(tempo_rte_logic_expose_tariff_mqtt(false, false));
}

TEST(TempoRteLogic, BodyLooksLikeJsonArrayAndWhitespace) {
  EXPECT_TRUE(tempo_rte_logic_body_looks_like_json("[1,2]"));
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json("   \n\t"));
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json("<HTML>{}"));
}

TEST(TempoRteLogic, RolloverRejectsUnknownTomorrowStge) {
  TempoRteState st;
  st.ltarf = kTempoColorBlue;
  st.tomorrow_stge_hex = "X";
  EXPECT_FALSE(tempo_rte_logic_apply_6h_rollover(kHalfHourSlotRollover6h, st));
  st.tomorrow_stge_hex.clear();
  EXPECT_FALSE(tempo_rte_logic_apply_6h_rollover(kHalfHourSlotRollover6h, st));
}

TEST(TempoRteLogic, ParseJourTempoRejectsMissingColor) {
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"dateJour":"2026-05-20"})", date, color));
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"dateJour":"2026-05-20","codeJour":9})", date, color));
}

TEST(TempoRteLogic, ResetTomorrowStgeNoOpOffSlot) {
  std::string stge = kStgeTomorrowBlue;
  tempo_rte_logic_reset_tomorrow_stge_at_1030(0, stge);
  EXPECT_EQ(stge, kStgeTomorrowBlue);
}

TEST(TempoRteLogic, HasTomorrowStgeUnset) {
  EXPECT_FALSE(tempo_rte_logic_has_tomorrow_stge_color(kStgeTomorrowUnset));
  EXPECT_FALSE(tempo_rte_logic_has_today_tempo_color(""));
}

TEST(TempoRteLogic, MapExactAndFormerTariffTokens) {
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("WHITE"), kTempoColorWhite);
  EXPECT_EQ(tempo_rte_logic_map_rte_color_token("RED"), kTempoColorRed);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HEURE  PLEINE"), 2);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HEURE  CREUSE"), 1);
  EXPECT_EQ(tempo_rte_logic_ltarf_bin("HC ROUGE CREUSE"), 18);
}

TEST(TempoRteLogic, ParseTempoLightRequiresDateKey) {
  TempoRteState st;
  EXPECT_FALSE(tempo_rte_logic_parse_tempo_light("{}", "2025-05-17", "2025-05-18", st));
}

TEST(TempoRteLogic, ApplyJourIgnoresUnmappedColor) {
  TempoRteState st;
  EXPECT_FALSE(tempo_rte_logic_apply_jour_tempo_responses(
      R"({"dateJour":"2026-05-20","libCouleur":"ZZZ"})", "", st));
}

TEST(TempoRteLogic, ApplyJourSetsTomorrowViaLtarMapping) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_apply_jour_tempo_responses(
      "", R"({"dateJour":"2026-05-21","libCouleur":"TEMPO_BLANC"})", st));
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowWhite);
  EXPECT_EQ(st.tomorrow_color_label, kTempoColorWhite);
}

TEST(TempoRteLogic, ParseJourInvalidCodeJourDigits) {
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"dateJour":"2026-05-20","codeJour":x})", date, color));
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"dateJour":"","codeJour":1})", date, color));
}

TEST(TempoRteLogic, ParseCalendarSkipsDateWithoutValue) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_like_calendars(
      R"({"2025-05-17":"orphan","2025-05-18":{"value":"WHITE"}})", "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowWhite);
}

TEST(TempoRteLogic, ParseLightUnmappedTomorrowToken) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_parse_tempo_light(
      R"({"2025-05-17":"BLUE","2025-05-18":"GARBAGE"})", "2025-05-17", "2025-05-18", st));
  EXPECT_EQ(st.ltarf, kTempoColorBlue);
  EXPECT_EQ(st.tomorrow_stge_hex, kStgeTomorrowUnset);
}

TEST(TempoRteLogic, TariffCodeNoMatch) {
  EXPECT_EQ(tempo_rte_logic_tariff_code("TARIF INCONNU"), 0);
  EXPECT_EQ(tempo_rte_logic_ltarf_bin("NEUTRE"), 0);
}

TEST(TempoRteLogic, ApplyJourTodayLabelFallback) {
  TempoRteState st;
  ASSERT_TRUE(tempo_rte_logic_apply_jour_tempo_responses(
      R"({"dateJour":"2026-05-20","codeJour":1})", "", st));
  EXPECT_EQ(st.ltarf, kTempoColorBlue);
  EXPECT_EQ(st.today_color_label, kTempoColorBlue);
}

TEST(TempoRteLogic, ParseJourTempoWithTabWhitespace) {
  const char *json = "{\"dateJour\"\t:\"2026-05-20\"\t,\t\"codeJour\"\t:\t2}";
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(json, date, color));
  EXPECT_EQ(color, "Blanc");
}

TEST(TempoRteLogic, BodyLooksLikeJsonWithLowerHtmlTag) {
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json(R"({"values":{"x":1}}<html>)"));
}

TEST(TempoRteLogic, TariffCodeHpBleu) {
  EXPECT_EQ(tempo_rte_logic_tariff_code("HP BLEU"), 12);
}

TEST(TempoRteLogic, ParseJourTempoTabWhitespaceAndInvalidCode) {
  const char *json = "{\"dateJour\"\t:\"2026-05-20\"\t,\t\"codeJour\"\t:\tx}";
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(json, date, color));
}

TEST(TempoRteLogic, BodyLooksLikeJsonRejectsUpperHtmlTag) {
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json(R"({"a":1}<HTML>)"));
}

TEST(TempoRteLogic, ParseJourTempoTabAfterColonInCodeJour) {
  const char *json = R"({"dateJour":"2026-05-20","codeJour":	2})";
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(json, date, color));
  EXPECT_EQ(color, "Blanc");
}

TEST(TempoRteLogic, ParseJourTempoSpaceAfterKey) {
  const char *json = R"({"dateJour" : "2026-05-20", "libCouleur" : "Rouge"})";
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(json, date, color));
  EXPECT_EQ(color, "Rouge");
}

TEST(TempoRteLogic, ParseJourTempoRejectsUnquotedDate) {
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"dateJour":2026-05-20,"libCouleur":"Bleu"})", date, color));
}

TEST(TempoRteLogic, TariffCodeHcAndHpVariants) {
  EXPECT_EQ(tempo_rte_logic_tariff_code("HC BLANC"), 13);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HP BLANC"), 14);
  EXPECT_EQ(tempo_rte_logic_tariff_code("HC ROUGE"), 15);
}

TEST(TempoRteLogic, BodyLooksLikeJsonRejectsDoctypeOnly) {
  EXPECT_FALSE(tempo_rte_logic_body_looks_like_json(R"({}<HTML><!DOCTYPE html>)"));
}

TEST(TempoRteLogic, ParseJourTempoRejectsNonDigitCodeJour) {
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(
      R"({"dateJour":"2026-05-20","codeJour":Z})", date, color));
}

TEST(TempoRteLogic, ParseJourTempoTabOnlyBeforeCodeJour) {
  const char *json = "{\"dateJour\":\"2026-05-20\",\"codeJour\"\t\t:\t1}";
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(json, date, color));
  EXPECT_EQ(color, "Bleu");
}

TEST(TempoRteLogic, ParseJourTempoRejectsTruncatedStringValue) {
  std::string date;
  std::string color;
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"dateJour":"2026-05-20")", date, color));
  EXPECT_FALSE(tempo_rte_logic_parse_jour_tempo(R"({"dateJour":)", date, color));
}

TEST(TempoRteLogic, ParseJourTempoSpacesOnlyBeforeQuotedDate) {
  const char *json = R"({"dateJour"   "2026-05-20","codeJour":1})";
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(json, date, color));
  EXPECT_EQ(date, "2026-05-20");
}

TEST(TempoRteLogic, ShouldPollWhenColorsMissingAndPollClockNotAdvanced) {
  TempoRteState st;
  st.enabled = true;
  st.ltarf.clear();
  st.tomorrow_stge_hex = kStgeTomorrowUnset;
  st.last_poll_time_decihours = -1;
  EXPECT_TRUE(tempo_rte_logic_should_poll(st, 500, true, true));
}

TEST(TempoRteLogic, ShouldNotPollSameSlotWhenComplete) {
  TempoRteState st;
  st.enabled = true;
  st.ltarf = kTempoColorBlue;
  st.tomorrow_stge_hex = kStgeTomorrowBlue;
  st.last_poll_time_decihours = 500;
  EXPECT_FALSE(tempo_rte_logic_should_poll(st, 500, true, true));
}

TEST(TempoRteLogic, ApplyJourTempoBleuFixtureFile) {
  std::ifstream in("firmware/test/fixtures/tempo/jour_tempo_bleu.json");
  ASSERT_TRUE(in.good());
  std::stringstream ss;
  ss << in.rdbuf();
  std::string date;
  std::string color;
  ASSERT_TRUE(tempo_rte_logic_parse_jour_tempo(ss.str(), date, color));
  EXPECT_EQ(date, "2026-05-20");
  EXPECT_EQ(color, "Bleu");
  TempoRteState st;
  st.enabled = true;
  ASSERT_TRUE(tempo_rte_logic_apply_jour_tempo_responses(ss.str(), "", st));
  EXPECT_EQ(st.ltarf, kTempoColorBlue);
  EXPECT_EQ(st.today_color_label, kTempoColorBlue);
  EXPECT_FALSE(tempo_rte_logic_has_tomorrow_stge_color(st.tomorrow_stge_hex));
}
