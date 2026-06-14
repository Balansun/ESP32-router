/*
 * bench_sim_meter.cpp — Source NotDef: synthetic metering for bench / first boot without hardware.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_NOTDEF
#include "balansun_globals.h"

static float bench_energy_import_acc = 0;
static float bench_energy_export_acc = 0;

void bench_sim_poll(void) {
  const float pw = float(int(millis() / 30) % 2000 - 500);
  if (pw >= 0) {
    house_active_import_w = (int)pw;
    house_active_export_w = 0;
    bench_energy_import_acc += pw / 90000.0f;
    house_energy_import_wh = (long)bench_energy_import_acc;
    house_apparent_import_va = (int)(pw + 250);
    house_apparent_export_va = 0;
  } else {
    house_active_import_w = 0;
    house_active_export_w = (int)(-pw);
    bench_energy_export_acc += (-pw) / 90000.0f;
    house_energy_export_wh = (long)bench_energy_export_acc;
    house_apparent_export_va = (int)(-pw + 250);
    house_apparent_import_va = 0;
  }
  house_voltage_v = 230.0f;
  house_current_a = 0;
  house_power_factor = 1.0f;
  meter_reading_valid = true;
  if (cptLEDyellow > 30) {
    cptLEDyellow = 4;
  }
}

#endif /* BALANSUN_ENABLE_SOURCE_NOTDEF */
