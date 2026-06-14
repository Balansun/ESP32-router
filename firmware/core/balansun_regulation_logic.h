#pragma once

/*
 * balansun_regulation_logic.h — Per-action triac_delay_percent from net power (balansun_apply_surplus_regulation).
 * Host-testable; triac overrides applied in balansun_app before compute.
 */

#include "balansun_regulation_modes.h"
#include "balansun_regulation_state.h"

#include <cstdint>

struct ActionRegulationInput {
  int action_index = 0;
  float net_power_w = 0.0f;
  uint8_t regulation_mode = kModeInactif;
  /** Period type: 0=NO, 1=OFF, 2=ON, 3=PW, 4=Triac */
  uint8_t schedule_type = 0;
  int threshold_min_w = 0;
  int max_open_percent = 100;
  uint8_t kp = 0;
  uint8_t ki = 4;
  uint8_t kd = 0;
  bool pid_enabled = false;
  uint8_t expert_regulation_mode = 0;
  uint8_t regulation_gain = 1;
  bool itmode_ok = true;
};

struct ActionRegulationOutput {
  float triac_delay_percent_f = 100.0f;
  bool active = false;
};

/** One regulation step for action @p in (mutates g_surplus_integrator etc.). */
ActionRegulationOutput balansun_regulation_compute_action(const ActionRegulationInput &in);

/** Triac (action 0) with MQTT/API overrides — wraps compute_action when AUTO. */
struct TriacRegulationInput {
  int net_power_w = 0;
  float current_triac_delay_percent_f = 100.0f;
  uint8_t override_state = 0;
  uint8_t override_triac_percent = 0;
  uint8_t actif = 0;
  int wall_decihours = 0;
  float temperature = 20.0f;
  int loop_gain = 1;
  int triac_threshold_min_w = 0;
  int triac_max_percent = 100;
  uint8_t schedule_type_triac = 0;
  uint8_t kp = 0;
  uint8_t ki = 4;
  uint8_t kd = 0;
  bool pid_enabled = false;
  uint8_t expert_regulation_mode = 0;
  uint8_t regulation_gain = 1;
  bool itmode_ok = true;
};

struct TriacRegulationOutput {
  float triac_delay_percent_f = 100.0f;
  bool triac_on = false;
};

TriacRegulationOutput balansun_regulation_compute_triac(const TriacRegulationInput &in);

/** Host-testable surplus regulator (integral / optional PID). */
struct SurplusRegulator {
  static ActionRegulationOutput compute_action(const ActionRegulationInput &in) {
    return balansun_regulation_compute_action(in);
  }
  static TriacRegulationOutput compute_triac(const TriacRegulationInput &in) {
    return balansun_regulation_compute_triac(in);
  }
};
