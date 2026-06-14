#pragma once

#include <balansun/action_node_control.h>

#include <cstdint>

/** True while manual override is active (permanent when until_ms == 0). Wrap-safe millis compare. */
bool action_node_manual_is_live(bool active, uint32_t until_ms, uint32_t now_ms);

/** True when a timed manual override has elapsed (inactive or permanent manual → false). */
bool action_node_manual_has_expired(bool active, uint32_t until_ms, uint32_t now_ms);

ActionNodeControlMode action_node_control_mode_from_manual(bool manual_live);

const char *action_node_control_mode_wire(ActionNodeControlMode mode);
