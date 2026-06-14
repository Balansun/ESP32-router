#include "balansun_regulation_safety.h"

#include "actions_logic.h"
#include "balansun_board.h"
#include "balansun_globals.h"
#include "balansun_regulation_state.h"
#include "balansun_triac_isr.h"

void balansun_regulation_force_outputs_off() {
  for (int i = 0; i < NbActions; i++) {
    g_triac_delay_percent_f[i] = 100.0f;
    g_triac_delay_percent[i] = 100;
    if (i > 0) load_channels[i].StopOutputs();
  }
  load_channels[0].On = false;
  siteCapActive = false;
  heaterLoadBackoffActive = false;
  balansun_triac_set_action0_mode(load_channels[0].Actif);
  balansun_regulation_sync_triac_globals();
  if (triac_delay_percent >= 100) digitalWrite(g_pins.triac_dim, LOW);
}
