/*
 * balansun_regulation_runtime.cpp — Surplus regulation orchestration (triac, actions, caps).
 */
#include "balansun_app.h"
#include "balansun_board.h"
#include "balansun_globals.h"
#include "actions_api.h"
#include "actions_logic.h"
#include "balansun_pulse_modes.h"
#include "balansun_regulation_logic.h"
#include "balansun_vacation_logic.h"
#include "balansun_source_health_logic.h"
#include "balansun_action_cap_logic.h"
#include "balansun_site_cap_logic.h"
#include "balansun_regulation_modes.h"
#include "balansun_regulation_state.h"
#include "balansun_source.h"
#include "balansun_triac_isr.h"
#include "balansun_triac_calibration_logic.h"
#include "balansun_heater_load_feedback_logic.h"
#include "balansun_diag.h"
#include "balansun_api_ready.h"
#include "balansun_output_suspend_logic.h"
#include "balansun_product_caps.h"
#include "balansun_regulation_safety.h"
#include "balansun_self_test.h"
#include "balansun_state_orchestrator.h"
#include "balansun_state_snapshot.h"
#include "metering/pmqtt_bindings.h"

static HeaterLoadFeedbackState g_heater_load_feedback_state;

static void balansun_regulation_refresh_state_snapshot(bool vacation_active, bool source_stale,
                                                    uint8_t triac_schedule_type, int ceiling_percent) {
  BalansunStateTickInput tick{};
  tick.caps = balansun_product_caps_compile_time();
  tick.api_telemetry_ready = balansun_api_telemetry_ready();
  tick.vacation_active = vacation_active;
  tick.source_stale = source_stale;
  tick.triac_off_when_source_stale = triacOffWhenSourceStale;
  tick.self_test_running = balansun_self_test_is_running();
  tick.triac_open_percent = 100 - g_triac_delay_percent[0];
  tick.schedule_allows_surplus = triac_schedule_type > 1;
  tick.ceiling_percent = ceiling_percent;
  balansun_state_orchestrator_tick(tick);
}

void balansun_apply_surplus_regulation(void) {
  const int grid_net_w = house_active_import_w - house_active_export_w;
  const bool itmode_ok = zc_sync_state > 0;
  const uint32_t now_epoch = static_cast<uint32_t>(time(NULL));
  vacationEnabled = balansun_vacation_logic_tick_enabled(vacationEnabled, vacationEndEpoch, now_epoch);
  const bool vacation_active = balansun_vacation_logic_active(vacationEnabled, vacationEndEpoch, now_epoch);
  const bool source_stale = balansun_source_is_stale_now();

  balansun_regulation_refresh_state_snapshot(vacation_active, source_stale, 0, 100);
  if (g_balansun_state_snapshot.output_suspend.active) {
    balansun_regulation_force_outputs_off();
    g_heater_load_feedback_state.backoff_active = false;
    g_heater_load_feedback_state.suspect_active = false;
    g_heater_load_feedback_state.suspect_since_ms = 0;
    return;
  }

  actions_logic_override_tick_all(millis());

  for (int i = 0; i < NbActions; i++) {
    ActionCapInput capIn;
    capIn.daily_cap_wh = actionDailyCapWh[i];
    capIn.routed_wh_today = (i == 0) ? static_cast<uint32_t>(second_day_energy_export_wh > 0 ? second_day_energy_export_wh : 0)
                                     : 0;
    actionCapHit[i] = balansun_action_cap_logic_is_hit(capIn);
  }

  uint8_t triac_schedule_type = 0;
  int triac_ceiling_percent = 100;

  for (int i = 0; i < NbActions; i++) {
    Action &act = load_channels[i];

    if (i > 0 && act.Host != "localhost") {
      act.apply_regulation(float(grid_net_w), wall_clock_decihours, temperature);
      continue;
    }

    if (!action_regulation_enabled(act.Actif)) {
      g_triac_delay_percent_f[i] = 100.0f;
      g_triac_delay_percent[i] = 100;
      if (i > 0) act.StopOutputs();
      continue;
    }
    if (actionCapHit[i]) {
      g_triac_delay_percent_f[i] = 100.0f;
      g_triac_delay_percent[i] = 100;
      if (i > 0) act.StopOutputs();
      continue;
    }

    ActionScheduleConfig sched;
    sched.period_count = act.periodCount;
    for (uint8_t p = 0; p < act.periodCount && p < 8; p++) {
      sched.periods[p].type = act.Type[p];
      sched.periods[p].period_start = act.period_start[p];
      sched.periods[p].period_end = act.period_end[p];
      sched.periods[p].power_min = act.power_min[p];
      sched.periods[p].power_max = act.power_max[p];
      sched.periods[p].temp_min = act.temp_min[p];
      sched.periods[p].temp_max = act.temp_max[p];
    }

    uint8_t schedule_type = act.schedule_type_at(wall_clock_decihours);
    if (i == 0) {
      schedule_type = act.current_triac_schedule_type(wall_clock_decihours, temperature);
      triac_schedule_type = schedule_type;
      triac_ceiling_percent = actions_logic_threshold_max(sched, wall_clock_decihours);
    }

    if (i == 0) {
      const unsigned long pmqttTriacStaleMs =
          poll_period_ms > 0 ? static_cast<unsigned long>(poll_period_ms) * 3U : 15000U;
      const bool pmqttTriacLive = act.OverrideState == ACTION_OVERRIDE_AUTO &&
                                  pmqtt_bindings_triac_open_percent_live(pmqttTriacStaleMs);
      TriacRegulationOutput tout{};
      tout.triac_on = (100 - g_triac_delay_percent[0]) > 0;
      if (!pmqttTriacLive) {
        TriacRegulationInput tin;
        tin.net_power_w = grid_net_w;
        tin.current_triac_delay_percent_f = g_triac_delay_percent_f[0];
        tin.override_state = act.OverrideState;
        tin.override_triac_percent = act.OverrideTriacPercent;
        tin.actif = act.Actif;
        tin.wall_decihours = wall_clock_decihours;
        tin.temperature = temperature;
        tin.loop_gain = act.Ki;
        tin.triac_threshold_min_w = actions_logic_threshold_min(sched, wall_clock_decihours);
        tin.triac_max_percent = actions_logic_threshold_max(sched, wall_clock_decihours);
        tin.schedule_type_triac = schedule_type;
        tin.kp = act.Kp;
        tin.ki = act.Ki;
        tin.kd = act.Kd;
        tin.pid_enabled = act.PID;
        tin.expert_regulation_mode = expert_regulation_mode;
        tin.regulation_gain = regulation_gain;
        tin.itmode_ok = itmode_ok;
        tout = balansun_regulation_compute_triac(tin);
        g_triac_delay_percent_f[0] = tout.triac_delay_percent_f;
        g_triac_delay_percent[0] = static_cast<int>(tout.triac_delay_percent_f + 0.5f);
      }
      if (act.OverrideState == ACTION_OVERRIDE_AUTO) {
        const int triac_open_fb = 100 - g_triac_delay_percent[0];
        const int second_net_w = second_active_import_w - second_active_export_w;
        HeaterLoadFeedbackConfig fb_cfg;
        fb_cfg.enabled = triacBackoffWhenHeaterIdle;
        fb_cfg.source_has_second_channel =
            balansun_cap_mqtt_triac_channel_block_for(balansun_active_source_get());
        fb_cfg.meter_valid = meter_reading_valid;
        const HeaterLoadFeedbackResult fb = balansun_heater_load_feedback_logic_tick(
            g_heater_load_feedback_state, fb_cfg, triac_open_fb, second_net_w, millis());
        heaterLoadBackoffActive = fb.backoff_active;
        if (fb.backoff_active) {
          g_triac_delay_percent_f[0] = 100.0f;
          g_triac_delay_percent[0] = 100;
          g_surplus_integrator[0] = 100.0f;
          act.On = false;
        }
      } else {
        heaterLoadBackoffActive = false;
        g_heater_load_feedback_state.backoff_active = false;
        g_heater_load_feedback_state.suspect_active = false;
        g_heater_load_feedback_state.suspect_since_ms = 0;
      }
      {
        const int triac_open = 100 - g_triac_delay_percent[0];
        const int est_w = (second_active_export_w > 0 && triac_open > 0)
                              ? (second_active_export_w * triac_open) / 100
                              : 0;
        siteCapActive = maxRoutedW > 0 && est_w > maxRoutedW;
        if (siteCapActive) {
          const int clamped = balansun_site_cap_logic_clamp_triac_open(triac_open, est_w, maxRoutedW);
          g_triac_delay_percent[0] = 100 - clamped;
          g_triac_delay_percent_f[0] = static_cast<float>(g_triac_delay_percent[0]);
        }
      }
      if (g_triac_cal.enabled) {
        int open = 100 - g_triac_delay_percent[0];
        open = balansun_triac_calibration_apply_open_percent(g_triac_cal, open);
        if (open < 0) open = 0;
        if (open > 100) open = 100;
        g_triac_delay_percent[0] = 100 - open;
        g_triac_delay_percent_f[0] = static_cast<float>(g_triac_delay_percent[0]);
      }
      act.On = tout.triac_on;
    } else {
      ActionRegulationInput ain;
      ain.action_index = i;
      ain.net_power_w = static_cast<float>(grid_net_w);
      ain.regulation_mode = act.Actif;
      ain.schedule_type = schedule_type;
      ain.threshold_min_w = actions_logic_threshold_min(sched, wall_clock_decihours);
      ain.max_open_percent = actions_logic_threshold_max(sched, wall_clock_decihours);
      ain.kp = act.Kp;
      ain.ki = act.Ki;
      ain.kd = act.Kd;
      ain.pid_enabled = act.PID;
      ain.expert_regulation_mode = expert_regulation_mode;
      ain.regulation_gain = regulation_gain;
      ain.itmode_ok = itmode_ok;
      const ActionRegulationOutput aout = balansun_regulation_compute_action(ain);
      act.On = aout.active;
      if (g_triac_delay_percent[i] >= 100) {
        act.StopOutputs();
      } else {
        balansun_pulse_modes_apply_regulation_output(i, act.Actif, g_triac_delay_percent[i]);
      }
    }
  }

  balansun_triac_set_action0_mode(load_channels[0].Actif);
  balansun_regulation_sync_triac_globals();
  if (triac_delay_percent >= 100) {
    digitalWrite(g_pins.triac_dim, LOW);
  }
  balansun_regulation_refresh_state_snapshot(vacation_active, source_stale, triac_schedule_type,
                                          triac_ceiling_percent);

}
