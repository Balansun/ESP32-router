#pragma once

/** Persist today's day-energy counters across crash/reboot (NVS, separate from midnight J0). */
void intraday_day_nvs_clear(void);

bool intraday_day_nvs_load(const char *ddmmyyyy,
                           long *house_import_wh,
                           long *house_export_wh,
                           long *second_import_wh,
                           long *second_export_wh);

/** Debounced save when any counter increases (ponytail: max ~1 write / 5 min). */
void intraday_day_nvs_maybe_save(const char *ddmmyyyy,
                                 long house_import_wh,
                                 long house_export_wh,
                                 long second_import_wh,
                                 long second_export_wh);
