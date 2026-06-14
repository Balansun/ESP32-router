#include <gtest/gtest.h>

#include <string>

#include "balansun_board.h"
#include "storage_eeprom_extension.h"
#include "storage_eeprom_layout.h"
#include "storage_eeprom_ram.h"

TEST(StorageEepromLayout, RegionsDoNotOverlap) { EXPECT_TRUE(storage_eeprom_layout_validate()); }

TEST(StorageEepromLayout, CleRomInitMatchesBoardHeader) {
  EXPECT_EQ(storage_eeprom_expected_cle_rom_init(), (uint32_t)kEepromLayoutInit);
}

TEST(StorageEepromExtension, RamRoundTripSkipsMainsHook) {
  RamEepromBackend ram;
  const int base = 2000;
  EepromExtensionFields in;
  in.pmqttTopic = "home/energy";
  in.pmqttSchema = "nested";
  in.jsyMk333SerialBaud = 19200;
  in.ext_peer_port = 8080;
  in.ext_peer_path = "/custom/path";
  in.arduinoOtaPassword = "ota-secret";
  in.httpApiPassword = "api-secret";
  in.httpCorsEnabled = true;
  in.apiAccessTokenCount = 1;
  in.apiAccessTokens[0].id = 7;
  in.apiAccessTokens[0].label = "test";
  in.apiAccessTokens[0].token_hex =
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

  const int endWrite = storage_eeprom_extension_write(base, ram, in);
  EXPECT_GT(endWrite, base);

  EepromExtensionFields out;
  const int endRead = storage_eeprom_extension_read(base, ram, out);
  EXPECT_EQ(endRead, endWrite);
  EXPECT_EQ(out.pmqttTopic, in.pmqttTopic);
  EXPECT_EQ(out.pmqttSchema, in.pmqttSchema);
  EXPECT_EQ(out.jsyMk333SerialBaud, in.jsyMk333SerialBaud);
  EXPECT_EQ(out.ext_peer_port, in.ext_peer_port);
  EXPECT_EQ(out.ext_peer_path, in.ext_peer_path);
  EXPECT_EQ(out.arduinoOtaPassword, in.arduinoOtaPassword);
  EXPECT_EQ(out.httpApiPassword, in.httpApiPassword);
  EXPECT_EQ(out.httpCorsEnabled, in.httpCorsEnabled);
  EXPECT_EQ(out.apiAccessTokenCount, in.apiAccessTokenCount);
  EXPECT_EQ(out.apiAccessTokens[0].id, in.apiAccessTokens[0].id);
  EXPECT_EQ(out.apiAccessTokens[0].label, in.apiAccessTokens[0].label);
  EXPECT_EQ(out.apiAccessTokens[0].token_hex, in.apiAccessTokens[0].token_hex);
}

TEST(StorageEepromExtension, DefaultPathWhenMissingSlash) {
  RamEepromBackend ram;
  const int base = 2100;
  EepromExtensionFields in;
  in.ext_peer_path = "no-leading-slash";
  storage_eeprom_extension_write(base, ram, in);
  EepromExtensionFields out;
  storage_eeprom_extension_read(base, ram, out);
  EXPECT_EQ(out.ext_peer_path, "/api/v1/measurements");
}

TEST(StorageEepromExtension, ReportsNoTruncationWhenItFits) {
  RamEepromBackend ram;  // default 4090 bytes
  EepromExtensionFields in;
  in.pmqttTopic = "home/energy";
  bool truncated = true;
  storage_eeprom_extension_write(0, ram, in, nullptr, &truncated);
  EXPECT_FALSE(truncated);
}

TEST(StorageEepromExtension, ReportsTruncationWhenOverCapacity) {
  RamEepromBackend ram(512);  // small capacity to force overflow
  EepromExtensionFields in;
  in.actionsJsonPresent = true;
  in.actionsJson = std::string(2000, 'x');  // far exceeds 512-byte backend
  bool truncated = false;
  const int end = storage_eeprom_extension_write(0, ram, in, nullptr, &truncated);
  EXPECT_TRUE(truncated);
  EXPECT_LE(end, ram.capacity());
}
