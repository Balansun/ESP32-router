#pragma once

#include <Arduino.h>

/** Apply Shelly Pro EM `/rpc/Shelly.GetStatus` body for shellypro3em three-phase (voie 3). */
bool shelly_pro_logic_apply_three_phase_status(const String &shelly_data, const String &device_name,
                                               int voie, String &raw_out);
