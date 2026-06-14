#pragma once

/* shelly_em_logic.h — Shelly EM Gen1 HTTP JSON parse. Host-testable. */

struct ShellyEmMonoReading {
  float power_w = 0;
  float voltage_v = 0;
  float pf = 0;
  long energy_import_wh = 0;
  long energy_export_wh = 0;
  int active_import_w = 0;
  int active_export_w = 0;
  int apparent_import_va = 0;
  int apparent_export_va = 0;
};

/** Parse Shelly EM monophase JSON body (must contain `"is_valid":true`). */
bool shelly_em_logic_parse_monophase_json(const char *json, ShellyEmMonoReading &out);
