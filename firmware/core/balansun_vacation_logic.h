#pragma once

#include <cstdint>

/** True when vacation mode should suspend regulation (end epoch 0 = open-ended until cleared). */
bool balansun_vacation_logic_active(bool enabled, uint32_t end_epoch_utc, uint32_t now_epoch_utc);

/** Clear vacation when end date passed. Returns new enabled flag. */
bool balansun_vacation_logic_tick_enabled(bool enabled, uint32_t end_epoch_utc, uint32_t now_epoch_utc);
