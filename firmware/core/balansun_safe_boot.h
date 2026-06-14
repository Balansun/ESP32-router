#pragma once

/** GPIO0 long-press at boot: reset stored pin map to firmware defaults. */
void balansun_safe_boot_poll_at_startup();

/** True when startup long-press requested pin reset (apply after EEPROM load). */
bool balansun_safe_boot_pin_reset_requested();
