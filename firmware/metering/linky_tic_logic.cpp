#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_LINKY

#include "linky_tic_logic.h"

int linky_tic_compute_checksum(const std::string &label, const std::string &value) {
  int checksum = 0;
  for (char c : label) checksum += static_cast<unsigned char>(c);
  for (char c : value) checksum += static_cast<unsigned char>(c);
  checksum += 18;
  checksum &= 63;
  checksum += 32;
  return checksum;
}

bool linky_tic_verify_checksum(const std::string &label, const std::string &value, int checksum_byte) {
  return linky_tic_compute_checksum(label, value) == checksum_byte;
}

#endif
