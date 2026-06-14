#include <gtest/gtest.h>

#include "linky_tic_logic.h"

TEST(LinkyTicLogic, ChecksumRoundTrip) {
  const int cs = linky_tic_compute_checksum("EAST", "12345");
  EXPECT_TRUE(linky_tic_verify_checksum("EAST", "12345", cs));
  EXPECT_FALSE(linky_tic_verify_checksum("EAST", "12346", cs));
}
