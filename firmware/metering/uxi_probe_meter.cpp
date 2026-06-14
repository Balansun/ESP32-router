/*
 * uxi_probe_meter.cpp — Source Analog: analog ADC incomer measurement (calib U/I).
 * See: /en/build/pinout/sources/analog — source_analog; GUIDE A.3.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_ANALOG
#include "balansun_diag.h"
#include "balansun_globals.h"
#include "balansun_mains_profile.h"
#include "uxi_adc_clip_logic.h"
float EASfloat = 0;
float EAIfloat=0;
void uxi_probe_setup() {
  for (int i = 0; i < 100; i++) {  //Reset table measurements
    voltM[i] = 0;
    ampM[i] = 0;
  }
}
void MeasurePower(void);
void ComputePower(void);
void uxi_probe_poll() {
  MeasurePower();
  ComputePower();
}
void MeasurePower() {
  int iStore;
  value0 = analogRead(g_pins.analog0);
  unsigned long MeasureMillis = millis();
  const uint32_t hz = balansun_mains_effective_frequency_hz();
  const uint32_t period_us = 1000000UL / (hz > 0 ? hz : 50UL);
  const unsigned int window_ms = (unsigned int)(period_us / 1000UL) + 1U;

  int peak_v = 0;
  int peak_a = 0;
  while (millis() - MeasureMillis < window_ms) {
    iStore = (int)((micros() % period_us) / (period_us / 100UL));
    if (iStore < 0) iStore = 0;
    if (iStore > 99) iStore = 99;
    const int rv = analogRead(g_pins.analog1) - value0;
    const int ra = analogRead(g_pins.analog2) - value0;
    volt[iStore] = rv;
    amp[iStore] = ra;
    const int av = rv < 0 ? -rv : rv;
    const int aa = ra < 0 ? -ra : ra;
    if (av > peak_v) peak_v = av;
    if (aa > peak_a) peak_a = aa;
  }
  uxi_adc_clip_logic_update(g_uxi_adc_clip, peak_v, peak_a);
}
void ComputePower() {
  float PWcal = 0;  //Computation Power in Watt
  float V;
  float I;
  float Uef2 = 0;
  float Ief2 = 0;
  for (int i = 0; i < 100; i++) {
    voltM[i] = (19 * voltM[i] + float(volt[i])) / 20;  //Mean value. First Order Filter. Short Integration
    V = kV * voltM[i];
    Uef2 += sq(V);
    ampM[i] = (19 * ampM[i] + float(amp[i])) / 20;  //Mean value. First Order Filter
    I = kI * ampM[i];
    Ief2 += sq(I);
    PWcal += V * I;
  }
  Uef2 = Uef2 / 100;         //square of voltage
  house_voltage_v = sqrt(Uef2);    //RMS voltage
  Ief2 = Ief2 / 100;         //square of current
  house_current_a = sqrt(Ief2);  // RMS current
  PWcal = PWcal / 100;
  float PVA = floor(house_voltage_v * house_current_a);
  float PowerFactor = 0;
  if (PVA > 0) {
    PowerFactor = floor(100 * PWcal / PVA) / 100;
  }
  house_power_factor = PowerFactor;
  if (PWcal >= 0) {
    EASfloat += PWcal / 90000;  // Wh integrator ~40 ms
    house_energy_import_wh = int(EASfloat);
    house_active_import_w = floor(PWcal);
    house_active_export_w = 0;
    house_apparent_import_va = PVA;
    house_apparent_export_va = 0;
  } else {
    EAIfloat += -PWcal / 90000;
    house_energy_export_wh =int(EAIfloat);
    house_active_import_w = 0;
    house_active_export_w = -floor(PWcal);
    house_apparent_import_va = 0;
    house_apparent_export_va = PVA;
  }
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
  meter_reading_valid = true;
  esp_task_wdt_reset();
}

#endif /* BALANSUN_ENABLE_SOURCE_ANALOG */
