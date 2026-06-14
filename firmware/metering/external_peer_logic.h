#pragma once

/*
 * external_peer_logic.h — Parse Source Ext GET /api/v1/measurements JSON.
 * Host-testable; used by external_peer_meter.cpp on core 0.
 */

#include <string>

/** Parsed house/triac powers and energies from a Ext peer (watts, Wh). */
struct ExternalPeerReading {
  int house_active_import_w = 0;
  int house_active_export_w = 0;
  int house_apparent_import_va = 0;
  int house_apparent_export_va = 0;
  double house_day_energy_import_wh = 0;
  double house_day_energy_export_wh = 0;
  double house_energy_import_wh = 0;
  double house_energy_export_wh = 0;
  int second_active_import_w = 0;
  int second_active_export_w = 0;
  std::string header_ltarf;
  std::string header_stge;
  bool valid = false;
};

/** Parse `GET /api/v1/measurements` JSON (`house` + optional `second`). */
bool external_peer_logic_parse_measurements_json(const std::string &body, ExternalPeerReading &out);
