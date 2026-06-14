#include <gtest/gtest.h>

#include "balansun_pin_logic.h"

TEST(BalansunPinLogic, DefaultMapValidWroom) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 26;
  m.uart_tx = 27;
  m.temp = 13;
  m.analog0 = 35;
  m.analog1 = 32;
  m.analog2 = 33;
  std::string err;
  EXPECT_TRUE(balansun_pin_map_validate(m, false, err)) << err;
}

TEST(BalansunPinLogic, RejectsDuplicateGpio) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 22;
  m.uart_rx = 26;
  m.uart_tx = 27;
  m.temp = 13;
  m.analog0 = 35;
  m.analog1 = 32;
  m.analog2 = 33;
  std::string err;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
}

TEST(BalansunPinLogic, S3RejectsFlashPin) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 30;
  m.uart_tx = 31;
  m.temp = 21;
  m.analog0 = 1;
  m.analog1 = 2;
  m.analog2 = 3;
  std::string err;
  EXPECT_FALSE(balansun_pin_map_validate(m, true, err));
}

TEST(BalansunPinLogic, TempOptionalDisabled) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 26;
  m.uart_tx = 27;
  m.temp = kBalansunPinTempDisabled;
  m.analog0 = 35;
  m.analog1 = 32;
  m.analog2 = 33;
  std::string err;
  EXPECT_TRUE(balansun_pin_map_validate(m, false, err)) << err;
}

TEST(BalansunPinLogic, IsAllowedPerFunction) {
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::TriacDim, 22, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::TriacDim, 35, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::TriacDim, 12, false));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::ZeroCross, 35, false));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::Temp, kBalansunPinTempDisabled, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartRx, kBalansunPinTempDisabled, false));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 35, false));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 32, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 31, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 22, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::TriacDim, 30, true));
}

TEST(BalansunPinLogic, S3ValidDevKitMap) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 4;
  m.uart_tx = 5;
  m.temp = 21;
  m.analog0 = 1;
  m.analog1 = 2;
  m.analog2 = 3;
  std::string err;
  EXPECT_TRUE(balansun_pin_map_validate(m, true, err)) << err;
}

TEST(BalansunPinLogic, RejectsUartRxEqualsTx) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 26;
  m.uart_tx = 26;
  m.temp = 13;
  m.analog0 = 35;
  m.analog1 = 32;
  m.analog2 = 33;
  std::string err;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
  EXPECT_NE(err.find("uart_rx"), std::string::npos);
}

TEST(BalansunPinLogic, RejectsOutOfRangeGpio) {
  BalansunPinMap m;
  m.triac_dim = 99;
  m.zero_cross = 23;
  m.uart_rx = 26;
  m.uart_tx = 27;
  m.temp = 13;
  m.analog0 = 35;
  m.analog1 = 32;
  m.analog2 = 33;
  std::string err;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
}

TEST(BalansunPinLogic, S3RejectsSpiFlashBand) {
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartTx, 8, true));
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 8;
  m.uart_tx = 5;
  m.temp = 21;
  m.analog0 = 1;
  m.analog1 = 2;
  m.analog2 = 3;
  std::string err;
  EXPECT_FALSE(balansun_pin_map_validate(m, true, err));
}

TEST(BalansunPinLogic, TempDuplicateWithTriacRejected) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 26;
  m.uart_tx = 27;
  m.temp = 22;
  m.analog0 = 35;
  m.analog1 = 32;
  m.analog2 = 33;
  std::string err;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
}

TEST(BalansunPinLogic, RejectsAnalogDuplicate) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 26;
  m.uart_tx = 27;
  m.temp = 13;
  m.analog0 = 32;
  m.analog1 = 32;
  m.analog2 = 33;
  std::string err;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
}

TEST(BalansunPinLogic, RejectsInvalidZeroCross) {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 50;
  m.uart_rx = 26;
  m.uart_tx = 27;
  m.temp = 13;
  m.analog0 = 35;
  m.analog1 = 32;
  m.analog2 = 33;
  std::string err;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
}

TEST(BalansunPinLogic, WroomRejectsNonAdcAnalog) {
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog2, 22, false));
}

TEST(BalansunPinLogic, AllowMatrixEdges) {
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::TriacDim, kBalansunPinTempDisabled, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(static_cast<BalansunPinFunction>(99), 22, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartRx, 0, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartRx, -5, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartTx, 2, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartRx, 15, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 20, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 1, false));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 1, true));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 5, true));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 8, true));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 0, true));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 39, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog0, 40, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::TriacDim, 26, true));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::TriacDim, 6, true));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::ZeroCross, 49, false));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::Temp, 26, false));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::Temp, 13, false));
}

namespace {

BalansunPinMap validWroomMap() {
  BalansunPinMap m;
  m.triac_dim = 22;
  m.zero_cross = 23;
  m.uart_rx = 26;
  m.uart_tx = 27;
  m.temp = 13;
  m.analog0 = 35;
  m.analog1 = 32;
  m.analog2 = 33;
  return m;
}

void setSlot(BalansunPinMap &m, int slot, int8_t v) {
  switch (slot) {
    case 0:
      m.triac_dim = v;
      break;
    case 1:
      m.zero_cross = v;
      break;
    case 2:
      m.uart_rx = v;
      break;
    case 3:
      m.uart_tx = v;
      break;
    case 4:
      m.temp = v;
      break;
    case 5:
      m.analog0 = v;
      break;
    case 6:
      m.analog1 = v;
      break;
    case 7:
      m.analog2 = v;
      break;
    default:
      break;
  }
}

}  // namespace

TEST(BalansunPinLogic, TempDisabledValid) {
  BalansunPinMap m = validWroomMap();
  m.temp = kBalansunPinTempDisabled;
  std::string err;
  EXPECT_TRUE(balansun_pin_map_validate(m, false, err)) << err;
}

TEST(BalansunPinLogic, PairwiseDuplicateGpioFails) {
  std::string err;
  for (int a = 0; a < 8; a++) {
    for (int b = a + 1; b < 8; b++) {
      BalansunPinMap m = validWroomMap();
      setSlot(m, a, 25);
      setSlot(m, b, 25);
      EXPECT_FALSE(balansun_pin_map_validate(m, false, err)) << "pair " << a << "," << b;
    }
  }
}

TEST(BalansunPinLogic, EachSlotRejectsInvalidGpio) {
  std::string err;
  for (int slot = 0; slot < 8; slot++) {
    BalansunPinMap m = validWroomMap();
    setSlot(m, slot, 99);
    EXPECT_FALSE(balansun_pin_map_validate(m, false, err)) << "slot " << slot;
  }
}

TEST(BalansunPinLogic, MapValidateRejectsStrappingAndS3Bands) {
  std::string err;
  BalansunPinMap m = validWroomMap();
  m.uart_tx = 2;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
  m = validWroomMap();
  m.uart_rx = 37;
  EXPECT_FALSE(balansun_pin_map_validate(m, true, err));
  m = validWroomMap();
  m.triac_dim = -1;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
  m = validWroomMap();
  m.temp = kBalansunPinTempDisabled;
  EXPECT_TRUE(balansun_pin_map_validate(m, false, err)) << err;
  m = validWroomMap();
  m.triac_dim = 35;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
  m = validWroomMap();
  m.uart_rx = 0;
  EXPECT_FALSE(balansun_pin_map_validate(m, false, err));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartRx, 7, true));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartTx, 26, true));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::UartTx, 11, true));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Analog1, 11, true));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::ZeroCross, 49, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::Temp, 49, false));
  EXPECT_FALSE(balansun_pin_logic_is_allowed(BalansunPinFunction::TriacDim, 15, false));
  EXPECT_TRUE(balansun_pin_logic_is_allowed(BalansunPinFunction::ZeroCross, 15, false));
  BalansunPinMap m2 = validWroomMap();
  m2.analog0 = 22;
  m2.analog1 = 22;
  EXPECT_FALSE(balansun_pin_map_validate(m2, false, err));
}

