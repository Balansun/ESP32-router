#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include "linky_tic_logic.h"

TEST(LinkyFixture, EastGroupChecksum) {
  std::ifstream in("firmware/test/fixtures/meters/linky/east_group.txt");
  std::string line;
  std::getline(in, line);
  const size_t t1 = line.find('\t');
  const size_t t2 = line.find('\t', t1 + 1);
  ASSERT_NE(t1, std::string::npos);
  ASSERT_NE(t2, std::string::npos);
  const std::string label = line.substr(0, t1);
  const std::string value = line.substr(t1 + 1, t2 - t1 - 1);
  const int checksum_byte = static_cast<unsigned char>(line[t2 + 1]);
  EXPECT_TRUE(linky_tic_verify_checksum(label, value, checksum_byte));
}

TEST(LinkyFixture, EastGroupExportChecksum) {
  std::ifstream in("firmware/test/fixtures/meters/linky/east_group_export.txt");
  std::string line;
  std::getline(in, line);
  const size_t t1 = line.find('\t');
  const size_t t2 = line.find('\t', t1 + 1);
  ASSERT_NE(t1, std::string::npos);
  ASSERT_NE(t2, std::string::npos);
  const std::string label = line.substr(0, t1);
  const std::string value = line.substr(t1 + 1, t2 - t1 - 1);
  const int checksum_byte = static_cast<unsigned char>(line[t2 + 1]);
  EXPECT_TRUE(linky_tic_verify_checksum(label, value, checksum_byte));
  EXPECT_EQ(value, "-3500");
}
