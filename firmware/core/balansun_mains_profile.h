#pragma once

#include <stdint.h>

/** Frequency selection: auto = follow meter mains_frequency_hz when valid. */
enum class MainsFrequencyMode : uint8_t { Auto = 0, Manual = 1 };

enum class MainsFrequencySource : uint8_t {
  Manual = 0,
  Meter = 1,
  Fallback = 2,
};

enum class MainsFrequencyWarning : uint8_t {
  None = 0,
  MeterCountryMismatch = 1,
};

void balansun_mains_profile_init_from_eeprom(void);

/** Apply country + optional variant (e.g. JP, "W" -> JP-W). Updates nominal V and manual Hz unless ZZ. */
void balansun_mains_country_apply(const char *iso2, const char *variant);
void balansun_mains_set_install_country(const char *iso2);
void balansun_mains_set_install_variant(const char *variant);

const char *balansun_mains_install_country(void);
const char *balansun_mains_install_variant(void);
uint16_t balansun_mains_nominal_v(void);
MainsFrequencyMode balansun_mains_frequency_mode(void);
void balansun_mains_set_frequency_mode(MainsFrequencyMode mode);
uint8_t balansun_mains_frequency_hz_manual(void);
void balansun_mains_set_frequency_hz_manual(uint8_t hz);
uint16_t balansun_mains_nominal_v_set(uint16_t v);

/** Effective values used by triac ISR and Analog source (may differ from manual when auto). */
uint8_t balansun_mains_effective_frequency_hz(void);
uint32_t balansun_mains_half_period_us(void);
uint8_t balansun_mains_triac_max_delay_ticks(void);
/** ISR-safe copy of triac_max_triac_delay_percent (updated with recompute_timing). */
extern volatile uint8_t g_triac_max_delay_ticks_isr;
MainsFrequencySource balansun_mains_frequency_source(void);
MainsFrequencyWarning balansun_mains_frequency_warning(void);
const char *balansun_mains_frequency_source_string(void);
const char *balansun_mains_frequency_warning_string(void);

/** Call after metering updates mains_frequency_hz (auto-primary path). */
void balansun_mains_on_meter_frequency(float freq_hz);

/** Force effective Hz (manual mode or fallback). */
void balansun_mains_apply_effective_hz(uint8_t hz, MainsFrequencySource source);

void balansun_mains_profile_persist_to_eeprom(int address, int *out_address);
int balansun_mains_profile_read_from_eeprom(int address);
