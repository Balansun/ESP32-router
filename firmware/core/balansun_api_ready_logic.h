#pragma once

#include <cstdint>

/** Host-testable base gate for telemetry REST (no source-specific rules). */
inline bool balansun_api_ready_logic_base(bool boot_complete, uint32_t last_metering_task_ms,
                                       bool meter_reading_valid) {
  return boot_complete && last_metering_task_ms > 0 && meter_reading_valid;
}

enum class PmqttBindingsPowerGroup : uint8_t { None, Signed, Split, Snapshot };

/** True when the configured Pmqtt power group has live metrics (host tests). */
bool balansun_pmqtt_bindings_ready_logic(PmqttBindingsPowerGroup group, bool signed_net_live,
                                      bool import_live, bool export_live, bool snapshot_live);
