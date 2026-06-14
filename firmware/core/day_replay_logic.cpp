#include "day_replay_logic.h"

#include "balansun_regulation_modes.h"
#include "balansun_regulation_state.h"

int day_replay_house_net_w(int active_import_w, int active_export_w) {
  return active_import_w - active_export_w;
}

int day_replay_triac_open_percent(float triac_delay_percent_f) { return 100 - static_cast<int>(triac_delay_percent_f); }

std::vector<DayReplaySample> day_replay_run(const DayReplayConfig &cfg, const DayReplaySlot *slots, size_t n) {
  std::vector<DayReplaySample> out;
  if (!slots || n == 0) return out;
  out.reserve(n);
  balansun_regulation_state_init();
  g_triac_delay_percent_f[0] = cfg.initial_triac_delay_percent_f;
  float triac_delay_percent_f = cfg.initial_triac_delay_percent_f;
  for (size_t i = 0; i < n; ++i) {
    const DayReplaySlot &sl = slots[i];
    DayReplaySample sample;
    sample.slot_index = static_cast<int>(i);
    sample.net_power_w = day_replay_house_net_w(sl.active_import_w, sl.active_export_w);

    TriacRegulationInput in;
    in.net_power_w = sample.net_power_w;
    in.current_triac_delay_percent_f = triac_delay_percent_f;
    in.actif = cfg.actif != 0 ? cfg.actif : kModeDecoupeOnoff;
    in.wall_decihours = sl.wall_decihours;
    in.temperature = sl.temperature_c;
    in.loop_gain = cfg.loop_gain;
    in.ki = static_cast<uint8_t>(cfg.loop_gain > 0 ? cfg.loop_gain : 4);
    in.itmode_ok = true;
    in.triac_max_percent = cfg.triac_max_percent;
    in.schedule_type_triac =
        actions_logic_active_type_triac(cfg.schedule, sl.wall_decihours, sl.temperature_c);
    in.triac_threshold_min_w = actions_logic_threshold_min(cfg.schedule, sl.wall_decihours);
    sample.schedule_type_triac = in.schedule_type_triac;

    const int loops = sl.loops > 0 ? sl.loops : 1;
    TriacRegulationOutput reg_out;
    for (int k = 0; k < loops; ++k) {
      reg_out = balansun_regulation_compute_triac(in);
      in.current_triac_delay_percent_f = reg_out.triac_delay_percent_f;
    }
    triac_delay_percent_f = reg_out.triac_delay_percent_f;
    sample.triac_delay_percent_f = triac_delay_percent_f;
    sample.triac_open_percent = day_replay_triac_open_percent(triac_delay_percent_f);
    out.push_back(sample);
  }
  return out;
}

int day_replay_check_checkpoints(const std::vector<DayReplaySample> &samples,
                                 const DayReplayCheckpoint *checkpoints, size_t n) {
  if (!checkpoints) return -1;
  for (size_t c = 0; c < n; ++c) {
    const DayReplayCheckpoint &cp = checkpoints[c];
    if (cp.slot_index < 0 || static_cast<size_t>(cp.slot_index) >= samples.size()) return static_cast<int>(cp.slot_index);
    const DayReplaySample &s = samples[static_cast<size_t>(cp.slot_index)];
    if (cp.schedule_active && s.schedule_type_triac < 2) return cp.slot_index;
    if (s.triac_open_percent < cp.triac_open_percent_min || s.triac_open_percent > cp.triac_open_percent_max) {
      return cp.slot_index;
    }
  }
  return -1;
}
