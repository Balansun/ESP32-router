#pragma once

/* linky_tic_logic.h — Linky TIC label checksum helpers (Enedis MOP-CPT). Host-testable. */

#include <string>

/** Linky TIC group checksum (tab-separated label + value + checksum byte). */
int linky_tic_compute_checksum(const std::string &label, const std::string &value);
bool linky_tic_verify_checksum(const std::string &label, const std::string &value, int checksum_byte);
