#include "intraday_day_nvs.h"

#include <Arduino.h>
#include <Preferences.h>

namespace {
constexpr const char *kNs = "balansun_iday";
constexpr const char *kKeyDate = "date";
constexpr const char *kKeyH1I = "h1i";
constexpr const char *kKeyH1E = "h1e";
constexpr const char *kKeyCh2I = "ch2i";
constexpr const char *kKeyCh2E = "ch2e";

unsigned long g_last_save_ms = 0;
long g_last_saved_max = 0;

bool date_matches_stored(Preferences &prefs, const char *ddmmyyyy) {
  if (!ddmmyyyy || !ddmmyyyy[0]) return false;
  const String stored = prefs.getString(kKeyDate, "");
  return stored.length() > 0 && stored.equals(ddmmyyyy);
}
}  // namespace

void intraday_day_nvs_clear(void) {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) return;
  prefs.clear();
  prefs.end();
  g_last_save_ms = 0;
  g_last_saved_max = 0;
}

bool intraday_day_nvs_load(const char *ddmmyyyy,
                           long *house_import_wh,
                           long *house_export_wh,
                           long *second_import_wh,
                           long *second_export_wh) {
  if (!house_import_wh || !house_export_wh || !second_import_wh || !second_export_wh) {
    return false;
  }
  *house_import_wh = 0;
  *house_export_wh = 0;
  *second_import_wh = 0;
  *second_export_wh = 0;
  Preferences prefs;
  if (!prefs.begin(kNs, true)) return false;
  if (!date_matches_stored(prefs, ddmmyyyy)) {
    prefs.end();
    return false;
  }
  *house_import_wh = static_cast<long>(prefs.getUInt(kKeyH1I, 0));
  *house_export_wh = static_cast<long>(prefs.getUInt(kKeyH1E, 0));
  *second_import_wh = static_cast<long>(prefs.getUInt(kKeyCh2I, 0));
  *second_export_wh = static_cast<long>(prefs.getUInt(kKeyCh2E, 0));
  prefs.end();
  return true;
}

void intraday_day_nvs_maybe_save(const char *ddmmyyyy,
                                 long house_import_wh,
                                 long house_export_wh,
                                 long second_import_wh,
                                 long second_export_wh) {
  if (!ddmmyyyy || !ddmmyyyy[0]) return;
  if (house_import_wh < 0) house_import_wh = 0;
  if (house_export_wh < 0) house_export_wh = 0;
  if (second_import_wh < 0) second_import_wh = 0;
  if (second_export_wh < 0) second_export_wh = 0;
  const long peak = house_import_wh;
  const long peak2 = house_export_wh;
  const long peak3 = second_import_wh;
  const long peak4 = second_export_wh;
  long max_wh = peak;
  if (peak2 > max_wh) max_wh = peak2;
  if (peak3 > max_wh) max_wh = peak3;
  if (peak4 > max_wh) max_wh = peak4;
  if (max_wh <= 0) return;

  const unsigned long now = millis();
  const bool debounce_ok = (now - g_last_save_ms) >= 300000UL;
  const bool big_jump = (max_wh - g_last_saved_max) >= 50L;
  if (!debounce_ok && !big_jump && max_wh <= g_last_saved_max) return;

  Preferences prefs;
  if (!prefs.begin(kNs, false)) return;
  prefs.putString(kKeyDate, ddmmyyyy);
  prefs.putUInt(kKeyH1I, static_cast<uint32_t>(house_import_wh));
  prefs.putUInt(kKeyH1E, static_cast<uint32_t>(house_export_wh));
  prefs.putUInt(kKeyCh2I, static_cast<uint32_t>(second_import_wh));
  prefs.putUInt(kKeyCh2E, static_cast<uint32_t>(second_export_wh));
  prefs.end();
  g_last_save_ms = now;
  g_last_saved_max = max_wh;
}
