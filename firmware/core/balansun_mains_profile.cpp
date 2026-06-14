#include "balansun_mains_profile.h"
#include "balansun_install_countries.h"
#include "EEPROM.h"

#include <Arduino.h>
#include <cstring>

#define EEPROM_MAINS_PROFILE_MAGIC 0xE1C0u

static char g_install_country[4] = "FR";
static char g_install_variant[12] = "";
static uint16_t g_nominal_v = 230;
static MainsFrequencyMode g_freq_mode = MainsFrequencyMode::Auto;
static uint8_t g_freq_hz_manual = 50;
static uint8_t g_effective_hz = 50;
static uint32_t g_half_period_us = 10000;
static uint8_t g_triac_max_delay_ticks = 98;
volatile uint8_t g_triac_max_delay_ticks_isr = 98;
static MainsFrequencySource g_freq_source = MainsFrequencySource::Fallback;
static MainsFrequencyWarning g_freq_warning = MainsFrequencyWarning::None;
static unsigned long g_last_valid_meter_freq_ms = 0;
static unsigned long g_mismatch_since_ms = 0;

static void recompute_timing(uint8_t hz) {
  if (hz != 50 && hz != 60) hz = 50;
  g_effective_hz = hz;
  g_half_period_us = 1000000UL / (2UL * (uint32_t)hz);
  g_triac_max_delay_ticks = (uint8_t)(g_half_period_us / 100UL);
  if (g_triac_max_delay_ticks > 2) {
    g_triac_max_delay_ticks -= 2;
  } else {
    g_triac_max_delay_ticks = 1;
  }
  g_triac_max_delay_ticks_isr = g_triac_max_delay_ticks;
}

static void build_lookup_key(char *out, size_t out_len, const char *iso2, const char *variant) {
  out[0] = '\0';
  if (!iso2 || !iso2[0]) {
    strncpy(out, "FR", out_len - 1);
    return;
  }
  if (strcmp(iso2, "ZZ") == 0) {
    strncpy(out, "ZZ", out_len - 1);
    return;
  }
  if (variant && variant[0]) {
    if (strchr(variant, '-')) {
      strncpy(out, variant, out_len - 1);
    } else {
      snprintf(out, out_len, "%s-%s", iso2, variant);
    }
    out[out_len - 1] = '\0';
    if (balansun_install_country_find(out)) return;
  }
  strncpy(out, iso2, out_len - 1);
  out[out_len - 1] = '\0';
}

void balansun_mains_apply_effective_hz(uint8_t hz, MainsFrequencySource source) {
  if (g_effective_hz != hz) {
    recompute_timing(hz);
  } else {
    g_effective_hz = hz;
  }
  g_freq_source = source;
}

void balansun_mains_set_install_country(const char *iso2) {
  if (!iso2 || !iso2[0]) {
    strncpy(g_install_country, "FR", sizeof(g_install_country) - 1);
    return;
  }
  strncpy(g_install_country, iso2, sizeof(g_install_country) - 1);
  g_install_country[sizeof(g_install_country) - 1] = '\0';
}

void balansun_mains_set_install_variant(const char *variant) {
  if (!variant) {
    g_install_variant[0] = '\0';
    return;
  }
  strncpy(g_install_variant, variant, sizeof(g_install_variant) - 1);
  g_install_variant[sizeof(g_install_variant) - 1] = '\0';
}

void balansun_mains_country_apply(const char *iso2, const char *variant) {
  char key[16];
  const char *c = (iso2 && iso2[0]) ? iso2 : "FR";
  if (strcmp(c, "ZZ") == 0) {
    strncpy(g_install_country, "ZZ", sizeof(g_install_country) - 1);
    g_install_variant[0] = '\0';
    return;
  }
  strncpy(g_install_country, c, sizeof(g_install_country) - 1);
  g_install_country[sizeof(g_install_country) - 1] = '\0';
  if (variant) {
    strncpy(g_install_variant, variant, sizeof(g_install_variant) - 1);
    g_install_variant[sizeof(g_install_variant) - 1] = '\0';
  } else {
    g_install_variant[0] = '\0';
  }
  build_lookup_key(key, sizeof(key), g_install_country, g_install_variant);
  const RmsInstallCountryRow *row = balansun_install_country_find(key);
  if (!row) {
    row = balansun_install_country_find(g_install_country);
  }
  if (row && strcmp(g_install_country, "ZZ") != 0) {
    g_nominal_v = row->nominal_v;
    g_freq_hz_manual = row->frequency_hz;
    recompute_timing(g_freq_hz_manual);
    g_freq_source = MainsFrequencySource::Fallback;
  }
}

void balansun_mains_profile_init_from_eeprom(void) {
  g_last_valid_meter_freq_ms = 0;
  g_mismatch_since_ms = 0;
  g_freq_warning = MainsFrequencyWarning::None;
  balansun_mains_country_apply(g_install_country, g_install_variant[0] ? g_install_variant : nullptr);
  if (g_freq_mode == MainsFrequencyMode::Manual) {
    balansun_mains_apply_effective_hz(g_freq_hz_manual, MainsFrequencySource::Manual);
  }
}

void balansun_mains_on_meter_frequency(float freq_hz) {
  const unsigned long now = millis();
  if (g_freq_mode == MainsFrequencyMode::Manual) {
    balansun_mains_apply_effective_hz(g_freq_hz_manual, MainsFrequencySource::Manual);
    g_freq_warning = MainsFrequencyWarning::None;
    return;
  }
  if (freq_hz < 45.0f || freq_hz > 65.0f) {
    if (now - g_last_valid_meter_freq_ms > 30000UL) {
      balansun_mains_apply_effective_hz(g_freq_hz_manual, MainsFrequencySource::Fallback);
    }
    return;
  }
  g_last_valid_meter_freq_ms = now;
  const uint8_t meter_hz = (freq_hz < 55.0f) ? 50 : 60;
  balansun_mains_apply_effective_hz(meter_hz, MainsFrequencySource::Meter);
  if (meter_hz != g_freq_hz_manual) {
    if (g_mismatch_since_ms == 0) g_mismatch_since_ms = now;
    if (now - g_mismatch_since_ms > 60000UL) {
      g_freq_warning = MainsFrequencyWarning::MeterCountryMismatch;
    }
  } else {
    g_mismatch_since_ms = 0;
    g_freq_warning = MainsFrequencyWarning::None;
  }
}

const char *balansun_mains_install_country(void) { return g_install_country; }
const char *balansun_mains_install_variant(void) { return g_install_variant; }
uint16_t balansun_mains_nominal_v(void) { return g_nominal_v; }
MainsFrequencyMode balansun_mains_frequency_mode(void) { return g_freq_mode; }
void balansun_mains_set_frequency_mode(MainsFrequencyMode mode) { g_freq_mode = mode; }
uint8_t balansun_mains_frequency_hz_manual(void) { return g_freq_hz_manual; }
void balansun_mains_set_frequency_hz_manual(uint8_t hz) {
  g_freq_hz_manual = (hz == 60) ? 60 : 50;
}
uint16_t balansun_mains_nominal_v_set(uint16_t v) {
  g_nominal_v = v;
  return g_nominal_v;
}
uint8_t balansun_mains_effective_frequency_hz(void) { return g_effective_hz; }
uint32_t balansun_mains_half_period_us(void) { return g_half_period_us; }
uint8_t balansun_mains_triac_max_delay_ticks(void) { return g_triac_max_delay_ticks; }
MainsFrequencySource balansun_mains_frequency_source(void) { return g_freq_source; }
MainsFrequencyWarning balansun_mains_frequency_warning(void) { return g_freq_warning; }

const char *balansun_mains_frequency_source_string(void) {
  switch (g_freq_source) {
    case MainsFrequencySource::Meter: return "meter";
    case MainsFrequencySource::Manual: return "manual";
    default: return "fallback";
  }
}

const char *balansun_mains_frequency_warning_string(void) {
  if (g_freq_warning == MainsFrequencyWarning::MeterCountryMismatch) {
    return "meter_country_mismatch";
  }
  return nullptr;
}

static const int kEepromSize = 4090;

int balansun_mains_profile_read_from_eeprom(int address) {
  strncpy(g_install_country, "FR", sizeof(g_install_country) - 1);
  g_install_variant[0] = '\0';
  g_nominal_v = 230;
  g_freq_mode = MainsFrequencyMode::Auto;
  g_freq_hz_manual = 50;
  recompute_timing(50);

  if (address < 0 || address + 2 > kEepromSize) return address;
  const uint16_t mag = EEPROM.readUShort(address);
  if (mag != EEPROM_MAINS_PROFILE_MAGIC) return address;
  address += (int)sizeof(uint16_t);

  char cc[3] = {0, 0, 0};
  cc[0] = (char)EEPROM.readByte(address++);
  cc[1] = (char)EEPROM.readByte(address++);
  if (cc[0]) strncpy(g_install_country, cc, sizeof(g_install_country) - 1);

  if (address < kEepromSize) {
    String var = EEPROM.readString(address);
    address += (int)(var.length() + 1);
    var.toCharArray(g_install_variant, sizeof(g_install_variant));
  }

  if (address + 4 <= kEepromSize) {
    g_freq_mode = (EEPROM.readByte(address) == 1) ? MainsFrequencyMode::Manual : MainsFrequencyMode::Auto;
    address++;
    g_freq_hz_manual = EEPROM.readByte(address++);
    if (g_freq_hz_manual != 50 && g_freq_hz_manual != 60) g_freq_hz_manual = 50;
    g_nominal_v = EEPROM.readUShort(address);
    address += (int)sizeof(uint16_t);
  }
  balansun_mains_country_apply(g_install_country, g_install_variant[0] ? g_install_variant : nullptr);
  if (g_freq_mode == MainsFrequencyMode::Manual) {
    balansun_mains_apply_effective_hz(g_freq_hz_manual, MainsFrequencySource::Manual);
  }
  return address;
}

void balansun_mains_profile_persist_to_eeprom(int address, int *out_address) {
  if (address < 0 || address + 2 > kEepromSize) {
    if (out_address) *out_address = address;
    return;
  }
  EEPROM.writeUShort(address, EEPROM_MAINS_PROFILE_MAGIC);
  address += (int)sizeof(uint16_t);
  EEPROM.writeByte(address++, g_install_country[0] ? g_install_country[0] : 'F');
  EEPROM.writeByte(address++, g_install_country[1] ? g_install_country[1] : 'R');
  String var = String(g_install_variant);
  if (var.length() > 11) var = var.substring(0, 11);
  EEPROM.writeString(address, var);
  address += (int)(var.length() + 1);
  EEPROM.writeByte(address++, (g_freq_mode == MainsFrequencyMode::Manual) ? 1 : 0);
  EEPROM.writeByte(address++, g_freq_hz_manual);
  EEPROM.writeUShort(address, g_nominal_v);
  address += (int)sizeof(uint16_t);
  if (out_address) *out_address = address;
}
