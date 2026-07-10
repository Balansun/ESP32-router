#include <gtest/gtest.h>

#include "balansun_source_logic.h"

TEST(BalansunSourceLogic, RegistryCount) { EXPECT_EQ(balansun_source_logic_registry_count(), 13u); }

TEST(BalansunSourceLogic, ParseKnownWires) {
  EXPECT_EQ(balansun_source_logic_parse_wire("JsyMk194"), SourceId::JsyMk194);
  EXPECT_EQ(balansun_source_logic_parse_wire("Linky"), SourceId::Linky);
  EXPECT_EQ(balansun_source_logic_parse_wire("BalansunPeer"), SourceId::BalansunPeer);
  EXPECT_EQ(balansun_source_logic_parse_wire("bad"), SourceId::Unknown);
}

TEST(BalansunSourceLogic, RejectLegacyWires) {
  EXPECT_EQ(balansun_source_logic_parse_wire("UxIx2"), SourceId::Unknown);
  EXPECT_EQ(balansun_source_logic_parse_wire("UxIx3"), SourceId::Unknown);
  EXPECT_EQ(balansun_source_logic_parse_wire("UxI"), SourceId::Unknown);
  EXPECT_EQ(balansun_source_logic_parse_wire("Ext"), SourceId::Unknown);
}

TEST(BalansunSourceLogic, NormalizeWirePassthrough) {
  EXPECT_STREQ(balansun_source_logic_normalize_wire_cstr("JsyMk194"), "JsyMk194");
  EXPECT_STREQ(balansun_source_logic_normalize_wire_cstr("UxIx2"), "UxIx2");
  EXPECT_STREQ(balansun_source_logic_normalize_wire_cstr("Linky"), "Linky");
}

TEST(BalansunSourceLogic, EffectiveIdForBalansunPeer) {
  EXPECT_EQ(balansun_source_logic_effective_id(SourceId::BalansunPeer, "Linky"), SourceId::Linky);
  EXPECT_EQ(balansun_source_logic_effective_id(SourceId::BalansunPeer, "JsyMk194"), SourceId::JsyMk194);
  EXPECT_EQ(balansun_source_logic_effective_id(SourceId::Analog, "ignored"), SourceId::Analog);
}

TEST(BalansunSourceLogic, WireAtAllRegistryRows) {
  for (size_t i = 0; i < balansun_source_logic_registry_count(); ++i) {
    EXPECT_GT(strlen(balansun_source_logic_wire_at(i)), 0u);
  }
}

TEST(BalansunSourceLogic, SecondChannelSnapshotVisible) {
  EXPECT_FALSE(balansun_source_logic_second_channel_snapshot_visible(0.0f, 0, 0, 0.0f));
  EXPECT_TRUE(balansun_source_logic_second_channel_snapshot_visible(235.0f, 0, 0, 0.0f));
  EXPECT_TRUE(balansun_source_logic_second_channel_snapshot_visible(0.0f, 879, 0, 0.0f));
  EXPECT_TRUE(balansun_source_logic_second_channel_snapshot_visible(0.0f, 0, 500, 0.0f));
  EXPECT_TRUE(balansun_source_logic_second_channel_snapshot_visible(0.0f, 0, 0, 0.5f));
  EXPECT_FALSE(balansun_source_logic_cap_mqtt_triac_channel_block_for(SourceId::Pmqtt));
  EXPECT_TRUE(balansun_source_logic_second_channel_snapshot_visible(0.0f, 879, 0, 0.0f));
}

TEST(BalansunSourceLogic, CapabilityMatrix) {
  EXPECT_TRUE(balansun_source_logic_cap_mqtt_triac_channel_block_for(SourceId::JsyMk194));
  EXPECT_FALSE(balansun_source_logic_cap_mqtt_triac_channel_block_for(SourceId::Linky));
  EXPECT_TRUE(balansun_source_logic_cap_mqtt_linky_tariff_for(SourceId::Linky, false));
  EXPECT_TRUE(balansun_source_logic_cap_mqtt_linky_tariff_for(SourceId::Analog, true));
  EXPECT_FALSE(balansun_source_logic_cap_mqtt_linky_tariff_for(SourceId::Analog, false));
  EXPECT_TRUE(balansun_source_logic_cap_serial_adc_gpio_restrict_for(SourceId::Analog));
}

TEST(BalansunSourceLogic, BasePollPeriodJsyMk333) {
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::JsyMk194, 9600), 400);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::JsyMk333, 9600), 800);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::JsyMk333, 19200), 500);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::ShellyEm, 0), 300);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::Enphase, 0), 600);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::BalansunPeer, 0), 800);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::Unknown, 0), 1000);
}

TEST(BalansunSourceLogic, WireRegistryAndNullWire) {
  EXPECT_STREQ(balansun_source_logic_wire_at(0), "JsyMk194");
  EXPECT_STREQ(balansun_source_logic_wire_at(99), "");
  EXPECT_EQ(balansun_source_logic_parse_wire(nullptr), SourceId::Unknown);
  EXPECT_EQ(balansun_source_logic_parse_wire("Pmqtt"), SourceId::Pmqtt);
  EXPECT_TRUE(balansun_source_logic_cap_mqtt_triac_channel_block_for(SourceId::JsyMk333));
  EXPECT_TRUE(balansun_source_logic_cap_mqtt_triac_channel_block_for(SourceId::ShellyEm));
  EXPECT_TRUE(balansun_source_logic_cap_serial_adc_gpio_restrict_for(SourceId::JsyMk333));
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::Analog, 0), 40);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::Linky, 0), 2);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::SmartG, 0), 300);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::NotDef, 0), 600);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::Pmqtt, 0), 600);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::ShellyPro, 0), 300);
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::HomeW, 0), 300);
  EXPECT_FALSE(balansun_source_logic_cap_mqtt_linky_tariff_for(SourceId::Enphase, false));
  EXPECT_EQ(balansun_source_logic_parse_wire("ShellyPro"), SourceId::ShellyPro);
  EXPECT_EQ(balansun_source_logic_parse_wire("HomeW"), SourceId::HomeW);
  EXPECT_EQ(balansun_source_logic_parse_wire("Analog"), SourceId::Analog);
  EXPECT_EQ(balansun_source_logic_parse_wire("JsyMk333"), SourceId::JsyMk333);
  EXPECT_STREQ(balansun_source_logic_normalize_wire_cstr("unknown"), "unknown");
  EXPECT_STREQ(balansun_source_logic_normalize_wire_cstr(""), "");
  EXPECT_EQ(balansun_source_logic_base_poll_period_ms(SourceId::Unknown, 0), 1000);
}

TEST(BalansunSourceLogic, MeterPinGroups) {
  EXPECT_TRUE(balansun_source_logic_meter_uart_pins_active(SourceId::JsyMk194));
  EXPECT_TRUE(balansun_source_logic_meter_uart_pins_active(SourceId::JsyMk333));
  EXPECT_TRUE(balansun_source_logic_meter_uart_pins_active(SourceId::Linky));
  EXPECT_FALSE(balansun_source_logic_meter_uart_pins_active(SourceId::Analog));
  EXPECT_TRUE(balansun_source_logic_meter_analog_pins_active(SourceId::Analog));
  EXPECT_FALSE(balansun_source_logic_meter_analog_pins_active(SourceId::JsyMk194));
}

TEST(BalansunSourceLogic, PinFieldAllowed) {
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed(nullptr, SourceId::JsyMk194));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("not_a_pin_key", SourceId::JsyMk194));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("pin_triac_dim", SourceId::Analog));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("pin_zero_cross", SourceId::Linky));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("pin_temp", SourceId::Analog));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("pin_uart_rx", SourceId::JsyMk194));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("pin_uart_tx", SourceId::Linky));
  EXPECT_FALSE(balansun_source_logic_pin_field_allowed("pin_uart_rx", SourceId::Analog));
  EXPECT_FALSE(balansun_source_logic_pin_field_allowed("pin_uart_tx", SourceId::Analog));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("pin_analog0", SourceId::Analog));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("pin_analog2", SourceId::Analog));
  EXPECT_FALSE(balansun_source_logic_pin_field_allowed("pin_analog1", SourceId::JsyMk194));
  EXPECT_FALSE(balansun_source_logic_pin_field_allowed("pin_analog0", SourceId::JsyMk194));
  EXPECT_TRUE(balansun_source_logic_pin_field_allowed("other_key", SourceId::JsyMk194));
}

TEST(BalansunSourceLogic, WireForId) {
  EXPECT_STREQ(balansun_source_logic_wire_for_id(SourceId::JsyMk194), "JsyMk194");
  EXPECT_STREQ(balansun_source_logic_wire_for_id(SourceId::Linky), "Linky");
  EXPECT_STREQ(balansun_source_logic_wire_for_id(SourceId::BalansunPeer), "BalansunPeer");
  EXPECT_STREQ(balansun_source_logic_wire_for_id(SourceId::Unknown), "Unknown");
}
