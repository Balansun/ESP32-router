#include "balansun_fixed_str.h"
#include "balansun_globals.h"

void balansun_fixed_str_boot_init() {
  // Lazy-allocate BalansunPsramStr on first assign(); avoid ~40 KB internal heap at boot on WROOM32.
}
