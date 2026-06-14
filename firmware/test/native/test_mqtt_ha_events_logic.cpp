#include <gtest/gtest.h>

#include "mqtt_ha_events_logic.h"

TEST(MqttHaEvents, SurplusEdges) {
  MqttHaEventInput in;
  in.surplus_active = true;
  in.prev_surplus_active = false;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_TRUE(out.surplus_started);
}

TEST(MqttHaEvents, RegulationHuntingEdge) {
  MqttHaEventInput in;
  in.regulation_hunting = true;
  in.prev_regulation_hunting = false;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_TRUE(out.regulation_hunting_started);
}

TEST(MqttHaEvents, VacationEndedEdge) {
  MqttHaEventInput in;
  in.vacation_active = false;
  in.prev_vacation_active = true;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_TRUE(out.vacation_ended);
}

TEST(MqttHaEvents, ActionCapHitEdge) {
  MqttHaEventInput in;
  in.action_cap_hit = true;
  in.prev_action_cap_hit = false;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_TRUE(out.action_cap_hit);
}

TEST(MqttHaEvents, SurplusEndedAndSourceLost) {
  MqttHaEventInput in;
  in.surplus_active = false;
  in.prev_surplus_active = true;
  in.source_stale = true;
  in.prev_source_stale = false;
  in.site_cap_active = true;
  in.prev_site_cap_active = false;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_TRUE(out.surplus_ended);
  EXPECT_TRUE(out.source_lost);
  EXPECT_TRUE(out.triac_cap_hit);
}

TEST(MqttHaEvents, LinkyTariffChanged) {
  MqttHaEventInput in;
  in.linky_tariff = "HP";
  in.prev_linky_tariff = "HC";
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_TRUE(out.linky_tariff_changed);
  mqtt_ha_events_logic_detect(in, nullptr);
}

TEST(MqttHaEvents, NoEdgesAllFalse) {
  MqttHaEventInput in;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.surplus_started);
  EXPECT_FALSE(out.surplus_ended);
  EXPECT_FALSE(out.source_lost);
  EXPECT_FALSE(out.triac_cap_hit);
  EXPECT_FALSE(out.regulation_hunting_started);
  EXPECT_FALSE(out.vacation_ended);
  EXPECT_FALSE(out.action_cap_hit);
  EXPECT_FALSE(out.linky_tariff_changed);
}

TEST(MqttHaEvents, LinkyNullPrevNoChange) {
  MqttHaEventInput in;
  in.linky_tariff = "HP";
  in.prev_linky_tariff = nullptr;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.linky_tariff_changed);
}

TEST(MqttHaEvents, LinkySameTariffNoChange) {
  MqttHaEventInput in;
  in.linky_tariff = "HP";
  in.prev_linky_tariff = "HP";
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.linky_tariff_changed);
}

TEST(MqttHaEvents, SurplusStartedFalseWhenAlreadyActive) {
  MqttHaEventInput in;
  in.surplus_active = true;
  in.prev_surplus_active = true;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.surplus_started);
}

TEST(MqttHaEvents, SurplusEndedFalseWhenStillInactive) {
  MqttHaEventInput in;
  in.surplus_active = false;
  in.prev_surplus_active = false;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.surplus_ended);
}

TEST(MqttHaEvents, SourceLostFalseWhenStillStale) {
  MqttHaEventInput in;
  in.source_stale = true;
  in.prev_source_stale = true;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.source_lost);
}

TEST(MqttHaEvents, TriacCapFalseWhenStillCapped) {
  MqttHaEventInput in;
  in.site_cap_active = true;
  in.prev_site_cap_active = true;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.triac_cap_hit);
}

TEST(MqttHaEvents, RegulationHuntingFalseWhenStillHunting) {
  MqttHaEventInput in;
  in.regulation_hunting = true;
  in.prev_regulation_hunting = true;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.regulation_hunting_started);
}

TEST(MqttHaEvents, VacationEndedFalseWhenStillOnVacation) {
  MqttHaEventInput in;
  in.vacation_active = true;
  in.prev_vacation_active = true;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.vacation_ended);
}

TEST(MqttHaEvents, ActionCapFalseWhenStillHit) {
  MqttHaEventInput in;
  in.action_cap_hit = true;
  in.prev_action_cap_hit = true;
  MqttHaEventOutput out;
  mqtt_ha_events_logic_detect(in, &out);
  EXPECT_FALSE(out.action_cap_hit);
}
