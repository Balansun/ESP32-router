#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "balansun_config_store_logic.h"
#include "storage_eeprom_layout.h"

TEST(ConfigStoreLogic, DomainsTileImageWithoutGaps) {
  const ConfigDomain *d = balansun_config_domains();
  // First domain starts at 0, each domain is contiguous, last ends at kEepromSize.
  EXPECT_EQ(d[0].start, 0);
  int expected = 0;
  int total = 0;
  for (int i = 0; i < kBalansunConfigDomainCount; i++) {
    EXPECT_EQ(d[i].start, expected) << "domain " << i << " not contiguous";
    EXPECT_GT(d[i].len, 0);
    expected = d[i].start + d[i].len;
    total += d[i].len;
  }
  EXPECT_EQ(expected, kEepromSize);
  EXPECT_EQ(total, kEepromSize);
}

TEST(ConfigStoreLogic, Crc32KnownVector) {
  const char *s = "123456789";
  EXPECT_EQ(balansun_config_crc32(reinterpret_cast<const uint8_t *>(s), 9), 0xCBF43926u);
}

TEST(ConfigStoreLogic, Crc32EmptyAndStable) {
  uint8_t buf[16] = {0};
  const uint32_t a = balansun_config_crc32(buf, 16);
  const uint32_t b = balansun_config_crc32(buf, 16);
  EXPECT_EQ(a, b);
}

TEST(ConfigStoreLogic, OnlyTouchedDomainChangesCrc) {
  std::vector<uint8_t> img(static_cast<size_t>(kEepromSize), 0);
  const ConfigDomain *d = balansun_config_domains();

  uint32_t before[kBalansunConfigDomainCount];
  for (int i = 0; i < kBalansunConfigDomainCount; i++) {
    before[i] = balansun_config_crc32(img.data() + d[i].start, d[i].len);
  }

  // Mutate one byte inside the "params" domain (index 2).
  img[d[2].start + 5] ^= 0xAA;

  for (int i = 0; i < kBalansunConfigDomainCount; i++) {
    const uint32_t after = balansun_config_crc32(img.data() + d[i].start, d[i].len);
    if (i == 2) {
      EXPECT_NE(after, before[i]) << "params domain CRC should change";
    } else {
      EXPECT_EQ(after, before[i]) << "untouched domain " << i << " CRC should be stable";
    }
  }
}
