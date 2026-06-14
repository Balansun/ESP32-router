#include <balansun/action_node_manual.h>

bool action_node_manual_is_live(bool active, uint32_t until_ms, uint32_t now_ms) {
  if (!active) return false;
  if (until_ms == 0) return true;
  return static_cast<int32_t>(now_ms - until_ms) < 0;
}

bool action_node_manual_has_expired(bool active, uint32_t until_ms, uint32_t now_ms) {
  return active && until_ms != 0 && static_cast<int32_t>(now_ms - until_ms) >= 0;
}

ActionNodeControlMode action_node_control_mode_from_manual(bool manual_live) {
  return manual_live ? ActionNodeControlMode::Manual : ActionNodeControlMode::Auto;
}

const char *action_node_control_mode_wire(ActionNodeControlMode mode) {
  return mode == ActionNodeControlMode::Manual ? "manual" : "auto";
}
