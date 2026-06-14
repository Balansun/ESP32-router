/* balansun_regulation_logic.cpp — Integral/PID regulation (host-testable). */
#include "balansun_regulation_logic.h"

#include "actions_logic.h"

#include <algorithm>
#include <cmath>

namespace {

float constrain_f(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

}  // namespace

ActionRegulationOutput balansun_regulation_compute_action(const ActionRegulationInput &in) {
  ActionRegulationOutput out;
  const int i = in.action_index;
  if (i < 0 || i >= kRegulationActions) {
    out.triac_delay_percent_f = 100.0f;
    return out;
  }

  float triac_delay_percent_f = g_triac_delay_percent_f[i];
  float error_pw = 0.0f;

  if (in.regulation_mode == kModeInactif || in.schedule_type <= 1) {
    g_triac_delay_percent_f[i] = 100.0f;
    g_surplus_integrator[i] = 100.0f;
    out.triac_delay_percent_f = 100.0f;
    out.active = false;
    return out;
  }

  const float threshold_min_w = static_cast<float>(in.threshold_min_w);
  const float max_triac_pw = static_cast<float>(in.max_open_percent);

  if (in.schedule_type == 2) {
    triac_delay_percent_f = 100.0f - max_triac_pw;
  } else {
    float ki = static_cast<float>(in.ki) / 10000.0f;
    const float gain = static_cast<float>(in.regulation_gain);
    if (in.net_power_w < threshold_min_w && in.regulation_gain > 1 && in.regulation_gain < 100) {
      ki *= gain;
    }

    if (in.regulation_mode == kModeDecoupeOnoff && i > 0) {
      if (in.net_power_w > max_triac_pw) {
        triac_delay_percent_f = 100.0f;
      }
      if (in.net_power_w < threshold_min_w) {
        triac_delay_percent_f = 0.0f;
      }
    } else {
      error_pw = static_cast<float>(in.net_power_w) - threshold_min_w;
      g_surplus_integrator[i] += 0.0001f;
      g_surplus_integrator[i] += error_pw * ki;
      g_surplus_integrator[i] = constrain_f(g_surplus_integrator[i], 0.0f, 100.0f);

      if (in.pid_enabled && in.expert_regulation_mode == 1) {
        const float kp = static_cast<float>(in.kp) / 1000.0f;
        const float kd = static_cast<float>(in.kd) / 1000.0f;
        g_surplus_proportional[i] = kp * error_pw;
        if (in.ki == 0 && in.kp > 0) {
          g_surplus_integrator[i] = 50.0f;
        }
        const float derive = kd * (error_pw - g_surplus_last_error[i]);
        g_surplus_derivative[i] = 0.2f * derive + 0.8f * g_surplus_derivative[i];
        triac_delay_percent_f = g_surplus_proportional[i] + g_surplus_integrator[i] + g_surplus_derivative[i];
      } else {
        triac_delay_percent_f = g_surplus_integrator[i];
        g_surplus_proportional[i] = 0.0f;
        g_surplus_derivative[i] = 0.0f;
      }
      g_surplus_last_error[i] = error_pw;

      if (triac_delay_percent_f < 100.0f - max_triac_pw) {
        triac_delay_percent_f = 100.0f - max_triac_pw;
      }
      if (!in.itmode_ok && i == 0) {
        triac_delay_percent_f = 100.0f;
      }
    }

    triac_delay_percent_f = constrain_f(triac_delay_percent_f, 0.0f, 100.0f);
  }

  g_triac_delay_percent_f[i] = triac_delay_percent_f;
  g_triac_delay_percent[i] = static_cast<int>(std::lround(triac_delay_percent_f));
  out.triac_delay_percent_f = triac_delay_percent_f;
  out.active = g_triac_delay_percent[i] < 100;
  return out;
}

TriacRegulationOutput balansun_regulation_compute_triac(const TriacRegulationInput &in) {
  TriacRegulationOutput out;
  out.triac_delay_percent_f = in.current_triac_delay_percent_f;
  out.triac_on = false;

  if (in.override_state == kActionOverrideOff) {
    out.triac_delay_percent_f = 100.0f;
    return out;
  }
  if (in.override_state == kActionOverrideOn) {
    float max_triac = static_cast<float>(in.triac_max_percent);
    if (max_triac <= 0 || max_triac > 100) max_triac = 100;
    out.triac_delay_percent_f = 100.0f - max_triac;
    out.triac_on = true;
    return out;
  }
  if (in.override_state == kActionOverrideTriacFixed) {
    out.triac_delay_percent_f = 100.0f - static_cast<float>(in.override_triac_percent);
    out.triac_on = in.override_triac_percent > 0;
    return out;
  }

  if (!action_regulation_enabled(in.actif) || in.schedule_type_triac < 2) {
    out.triac_delay_percent_f = 100.0f;
    return out;
  }

  g_triac_delay_percent_f[0] = in.current_triac_delay_percent_f;
  g_surplus_integrator[0] = in.current_triac_delay_percent_f;
  ActionRegulationInput ain;
  ain.action_index = 0;
  ain.net_power_w = static_cast<float>(in.net_power_w);
  ain.regulation_mode = in.actif;
  ain.schedule_type = in.schedule_type_triac;
  ain.threshold_min_w = in.triac_threshold_min_w;
  ain.max_open_percent = in.triac_max_percent;
  ain.kp = in.kp;
  ain.ki = in.ki > 0 ? in.ki : static_cast<uint8_t>(std::max(1, in.loop_gain));
  ain.kd = in.kd;
  ain.pid_enabled = in.pid_enabled;
  ain.expert_regulation_mode = in.expert_regulation_mode;
  ain.regulation_gain = in.regulation_gain;
  ain.itmode_ok = in.itmode_ok;

  const ActionRegulationOutput aout = balansun_regulation_compute_action(ain);
  out.triac_delay_percent_f = aout.triac_delay_percent_f;
  out.triac_on = aout.active;
  return out;
}
