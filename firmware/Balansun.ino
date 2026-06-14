/*
  Balansun — PV surplus router (ESP32), Balansun multi-source.
  Licensed under the EUPL — see LICENSE in the repository root.
  Entry: setup() → balansun_setup(); loop() → balansun_loop() (core 1). Metering task on core 0.
  See: /en/project-overview/, FIRMWARE_BUILD.md. Logic: firmware/core/, firmware/metering/.
*/
#include "balansun_board.h"
#include "balansun_app.h"

void setup() {
  balansun_setup();
}

void loop() {
  balansun_loop();
}
