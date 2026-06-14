#include <gtest/gtest.h>

#include <balansun/pilot_wire_full.h>

TEST(PilotWireFull, ThreeRelayOrdersDistinct) {
  const auto confort = balansun_pilot_wire_full_relays_for_order(PilotWireOrder::Confort,
                                                                   PilotWireFullWiring::ThreeRelay);
  EXPECT_FALSE(confort.relay_full_wave);
  EXPECT_FALSE(confort.relay_half_negative);
  EXPECT_FALSE(confort.relay_half_positive);

  const auto eco = balansun_pilot_wire_full_relays_for_order(PilotWireOrder::Eco, PilotWireFullWiring::ThreeRelay);
  EXPECT_TRUE(eco.relay_full_wave);
  EXPECT_FALSE(eco.relay_half_negative);
  EXPECT_FALSE(eco.relay_half_positive);

  const auto hg = balansun_pilot_wire_full_relays_for_order(PilotWireOrder::HorsGel, PilotWireFullWiring::ThreeRelay);
  EXPECT_FALSE(hg.relay_full_wave);
  EXPECT_TRUE(hg.relay_half_negative);
  EXPECT_FALSE(hg.relay_half_positive);

  const auto arret =
      balansun_pilot_wire_full_relays_for_order(PilotWireOrder::Arret, PilotWireFullWiring::ThreeRelay);
  EXPECT_FALSE(arret.relay_full_wave);
  EXPECT_FALSE(arret.relay_half_negative);
  EXPECT_TRUE(arret.relay_half_positive);
}

TEST(PilotWireFull, NeverHgAndArretTogether) {
  for (int o = 0; o < 6; o++) {
    const auto m = balansun_pilot_wire_full_relays_for_order(static_cast<PilotWireOrder>(o),
                                                             PilotWireFullWiring::ThreeRelay);
    EXPECT_FALSE(m.relay_half_negative && m.relay_half_positive);
  }
}

TEST(PilotWireFull, WiringFromWire) {
  EXPECT_EQ(balansun_pilot_wire_full_wiring_from_wire("r2_full_3relay"), PilotWireFullWiring::ThreeRelay);
  EXPECT_EQ(balansun_pilot_wire_full_wiring_from_wire("r2_full_2relay"), PilotWireFullWiring::TwoRelayMatrix);
}

TEST(PilotWireFull, TwoRelayGpioLevelsDistinct) {
  const auto confort = balansun_pilot_wire_full_gpio_levels(
      balansun_pilot_wire_full_relays_for_order(PilotWireOrder::Confort, PilotWireFullWiring::TwoRelayMatrix),
      PilotWireFullWiring::TwoRelayMatrix);
  EXPECT_FALSE(confort.relay_full_wave);
  EXPECT_FALSE(confort.half_selector_active);

  const auto eco = balansun_pilot_wire_full_gpio_levels(
      balansun_pilot_wire_full_relays_for_order(PilotWireOrder::Eco, PilotWireFullWiring::TwoRelayMatrix),
      PilotWireFullWiring::TwoRelayMatrix);
  EXPECT_TRUE(eco.relay_full_wave);
  EXPECT_FALSE(eco.half_selector_active);

  const auto hg = balansun_pilot_wire_full_gpio_levels(
      balansun_pilot_wire_full_relays_for_order(PilotWireOrder::HorsGel, PilotWireFullWiring::TwoRelayMatrix),
      PilotWireFullWiring::TwoRelayMatrix);
  EXPECT_FALSE(hg.relay_full_wave);
  EXPECT_TRUE(hg.half_selector_active);
  EXPECT_FALSE(hg.half_select_arret);

  const auto arret = balansun_pilot_wire_full_gpio_levels(
      balansun_pilot_wire_full_relays_for_order(PilotWireOrder::Arret, PilotWireFullWiring::TwoRelayMatrix),
      PilotWireFullWiring::TwoRelayMatrix);
  EXPECT_FALSE(arret.relay_full_wave);
  EXPECT_TRUE(arret.half_selector_active);
  EXPECT_TRUE(arret.half_select_arret);
}

TEST(PilotWireFull, ThreeRelayGpioMapsMaskDirectly) {
  const auto mask =
      balansun_pilot_wire_full_relays_for_order(PilotWireOrder::HorsGel, PilotWireFullWiring::ThreeRelay);
  const auto levels = balansun_pilot_wire_full_gpio_levels(mask, PilotWireFullWiring::ThreeRelay);
  EXPECT_TRUE(levels.relay_half_negative);
  EXPECT_FALSE(levels.relay_half_positive);
  EXPECT_FALSE(levels.half_selector_active);
}
