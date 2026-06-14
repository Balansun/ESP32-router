/*
 * uxi_probe_meter.cpp — Source Analog: analog ADC incomer measurement (calib U/I).
 * See: /en/build/pinout/sources/analog — source_analog; GUIDE A.3.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_ANALOG
#include "uxi_probe_meter.h"

#include "balansun_diag.h"
#include "balansun_globals.h"
#include "balansun_mains_profile.h"
#include "uxi_adc_clip_logic.h"

#include <esp_task_wdt.h>

float EASfloat = 0;
float EAIfloat = 0;

static void measurePower() {
  int iStore;
  value0 = analogRead(g_pins.analog0);
  const unsigned long measure_millis = millis();
  const uint32_t hz = balansun_mains_effective_frequency_hz();
  const uint32_t period_us = 1000000UL / (hz > 0 ? hz : 50UL);
  const unsigned int window_ms = (unsigned int)(period_us / 1000UL) + 1U;

  int peak_v = 0;
  int peak_a = 0;
  while (millis() - measure_millis < window_ms) {
    iStore = (int)((micros() % period_us) / (period_us / 100UL));
    if (iStore < 0) {
      iStore = 0;
    }
    if (iStore > 99) {
      iStore = 99;
    }
    const int rv = analogRead(g_pins.analog1) - value0;
    const int ra = analogRead(g_pins.analog2) - value0;
    volt[iStore] = rv;
    amp[iStore] = ra;
    const int av = rv < 0 ? -rv : rv;
    const int aa = ra < 0 ? -ra : ra;
    if (av > peak_v) {
      peak_v = av;
    }
    if (aa > peak_a) {
      peak_a = aa;
    }
  }
  uxi_adc_clip_logic_update(g_uxi_adc_clip, peak_v, peak_a);
}

static void computePower() {
  float pwcal = 0;
  float v;
  float i;
  float uef2 = 0;
  float ief2 = 0;
  for (int i_idx = 0; i_idx < 100; i_idx++) {
    voltM[i_idx] = (19 * voltM[i_idx] + float(volt[i_idx])) / 20;
    v = kV * voltM[i_idx];
    uef2 += sq(v);
    ampM[i_idx] = (19 * ampM[i_idx] + float(amp[i_idx])) / 20;
    i = kI * ampM[i_idx];
    ief2 += sq(i);
    pwcal += v * i;
  }
  uef2 = uef2 / 100;
  house_voltage_v = sqrt(uef2);
  ief2 = ief2 / 100;
  house_current_a = sqrt(ief2);
  pwcal = pwcal / 100;
  const float pva = floor(house_voltage_v * house_current_a);
  float power_factor = 0;
  if (pva > 0) {
    power_factor = floor(100 * pwcal / pva) / 100;
  }
  house_power_factor = power_factor;
  if (pwcal >= 0) {
    EASfloat += pwcal / 90000;
    house_energy_import_wh = int(EASfloat);
    house_active_import_w = floor(pwcal);
    house_active_export_w = 0;
    house_apparent_import_va = pva;
    house_apparent_export_va = 0;
  } else {
    EAIfloat += -pwcal / 90000;
    house_energy_export_wh = int(EAIfloat);
    house_active_import_w = 0;
    house_active_export_w = -floor(pwcal);
    house_apparent_import_va = 0;
    house_apparent_export_va = pva;
  }
}

void AnalogMeter::setup() {
  for (int i = 0; i < 100; i++) {
    voltM[i] = 0;
    ampM[i] = 0;
  }
}

void AnalogMeter::poll() {
  measurePower();
  computePower();
  markPollSuccess();
}

void AnalogMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  (void)linky_tail_max;
  JsonObject wf = doc["uxi_waveform"].to<JsonObject>();
  JsonArray vArr = wf["volt_m"].to<JsonArray>();
  JsonArray aArr = wf["amp_m"].to<JsonArray>();
  int i0 = 0;
  for (int i = 0; i < 100; i++) {
    const int i1 = (i + 1) % 100;
    if (voltM[i] <= 0 && voltM[i1] > 0) {
      i0 = i1;
      break;
    }
  }
  for (int i = 0; i < 100; i++) {
    const int j = (i + i0) % 100;
    vArr.add(voltM[j]);
    aArr.add(ampM[j]);
  }
}

IMeterDriver *balansun_meter_instance_analog() {
  static AnalogMeter instance;
  return &instance;
}

#endif /* BALANSUN_ENABLE_SOURCE_ANALOG */
