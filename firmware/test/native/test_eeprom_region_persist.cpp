#include <gtest/gtest.h>

#include "balansun_ha_site_persist.h"
#include "balansun_regulation_persist.h"
#include "storage_eeprom_layout.h"
#include "storage_eeprom_ram.h"

TEST(RegulationPersist, MissingMagicLeavesAbsent) {
  RamEepromBackend ram;
  EepromExtensionFields fields;
  fields.regulationPersistPresent = true;
  const int end = balansun_regulation_persist_read(100, ram, fields);
  EXPECT_EQ(end, 100);
  EXPECT_FALSE(fields.regulationPersistPresent);
}

TEST(RegulationPersist, RoundTripAndGainClamp) {
  RamEepromBackend ram;
  const int base = 300;
  EepromExtensionFields in;
  in.expertRegulationMode = 2;
  in.regulationGain = 0;
  in.actionRegCoeffs[0].kp = 10;
  in.actionRegCoeffs[0].ki = 11;
  in.actionRegCoeffs[0].kd = 12;
  in.actionRegCoeffs[0].pid = true;

  const int endWrite = balansun_regulation_persist_write(base, ram, in);
  EXPECT_GT(endWrite, base);

  EepromExtensionFields out;
  const int endRead = balansun_regulation_persist_read(base, ram, out);
  EXPECT_EQ(endRead, endWrite);
  EXPECT_TRUE(out.regulationPersistPresent);
  EXPECT_EQ(out.expertRegulationMode, 2);
  EXPECT_EQ(out.regulationGain, 1);
  EXPECT_EQ(out.actionRegCoeffs[0].kp, 10);
  EXPECT_TRUE(out.actionRegCoeffs[0].pid);
}

TEST(HaSitePersist, MissingMagicLeavesAbsent) {
  RamEepromBackend ram;
  EepromExtensionFields fields;
  fields.haSitePersistPresent = true;
  const int end = balansun_ha_site_persist_read(400, ram, fields);
  EXPECT_EQ(end, 400);
  EXPECT_FALSE(fields.haSitePersistPresent);
}

TEST(HaSitePersist, RoundTrip) {
  RamEepromBackend ram;
  const int base = 500;
  EepromExtensionFields in;
  in.vacationEnabled = true;
  in.vacationEndEpoch = 1'700'000'000u;
  in.maxRoutedW = 9000;
  in.mqttJsonCommands = true;
  in.triacOffWhenSourceStale = true;
  in.triacBackoffWhenHeaterIdle = false;
  in.actionDailyCapWh[0] = 5000;

  const int endWrite = balansun_ha_site_persist_write(base, ram, in);
  EXPECT_GT(endWrite, base);

  EepromExtensionFields out;
  const int endRead = balansun_ha_site_persist_read(base, ram, out);
  EXPECT_EQ(endRead, endWrite);
  EXPECT_TRUE(out.haSitePersistPresent);
  EXPECT_TRUE(out.vacationEnabled);
  EXPECT_EQ(out.vacationEndEpoch, in.vacationEndEpoch);
  EXPECT_EQ(out.maxRoutedW, in.maxRoutedW);
  EXPECT_EQ(out.actionDailyCapWh[0], 5000u);
}
