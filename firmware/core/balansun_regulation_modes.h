#pragma once

/*
 * Regulation modes — stored in Action::Actif (triac and relay channels).
 */

#include "balansun_board.h"

#include <cstdint>

/** Regulation mode constants — stored in Action::Actif. */
constexpr uint8_t kModeInactif = 0;
constexpr uint8_t kModeDecoupeOnoff = 1;
constexpr uint8_t kModeMultisinus = 2;
constexpr uint8_t kModeTrainsinus = 3;
constexpr uint8_t kModePwm = 4;
constexpr uint8_t kModeDemisinus = 5;

inline bool action_regulation_enabled(uint8_t actif) { return actif != kModeInactif; }
