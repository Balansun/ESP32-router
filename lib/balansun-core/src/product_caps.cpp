#include <balansun/product_caps.h>
#include <balansun/product_role.h>

BalansunProductCaps balansun_product_caps_for_role(int role) {
  BalansunProductCaps out{};

  const uint32_t base = static_cast<uint32_t>(BalansunCap::MqttHaDiscovery) |
                        static_cast<uint32_t>(BalansunCap::VacationMode) |
                        static_cast<uint32_t>(BalansunCap::PwmChannel);

  switch (role) {
    case BALANSUN_ROLE_METER_GATEWAY:
      out.mask = base;
      break;
    case BALANSUN_ROLE_METER_ROUTER:
      out.mask = base | static_cast<uint32_t>(BalansunCap::SurplusRegulation) |
                 static_cast<uint32_t>(BalansunCap::TriacDimming) |
                 static_cast<uint32_t>(BalansunCap::SelfTestTriac) |
                 static_cast<uint32_t>(BalansunCap::StatusLedRgb);
      break;
    case BALANSUN_ROLE_FULL_ROUTER:
    default:
      out.mask = base | static_cast<uint32_t>(BalansunCap::SurplusRegulation) |
                 static_cast<uint32_t>(BalansunCap::TriacDimming) |
                 static_cast<uint32_t>(BalansunCap::MultiAction) |
                 static_cast<uint32_t>(BalansunCap::SourceTestInject) |
                 static_cast<uint32_t>(BalansunCap::SelfTestTriac) |
                 static_cast<uint32_t>(BalansunCap::StatusLedRgb);
      break;
  }
  return out;
}

BalansunProductCaps balansun_product_caps_for_profile(int product_profile) {
  BalansunProductCaps out{};
  switch (product_profile) {
    case BALANSUN_PRODUCT_JSY_MK194_METER:
    case BALANSUN_PRODUCT_METER_ONLY:
      out = balansun_product_caps_for_role(BALANSUN_ROLE_METER_GATEWAY);
      break;
    case BALANSUN_PRODUCT_JSY_MK194_ROUTER:
      out = balansun_product_caps_for_role(BALANSUN_ROLE_METER_ROUTER);
      break;
    case BALANSUN_PRODUCT_FULL_ROUTER:
    default:
      out = balansun_product_caps_for_role(BALANSUN_ROLE_FULL_ROUTER);
      break;
  }
  out.profile_wire = balansun_product_profile_wire(product_profile);
  return out;
}

bool balansun_product_caps_has(const BalansunProductCaps &caps, BalansunCap cap) {
  return (caps.mask & static_cast<uint32_t>(cap)) != 0;
}

const char *balansun_product_profile_wire(int product_profile) {
  switch (product_profile) {
    case BALANSUN_PRODUCT_JSY_MK194_ROUTER:
      return "jsy_mk194_router";
    case BALANSUN_PRODUCT_JSY_MK194_METER:
      return "jsy_mk194_meter";
    case BALANSUN_PRODUCT_METER_ONLY:
      return "meter_only";
    case BALANSUN_PRODUCT_FULL_ROUTER:
    default:
      return "full_router";
  }
}

const char *balansun_product_cap_wire(BalansunCap cap) {
  switch (cap) {
    case BalansunCap::SurplusRegulation:
      return "surplus_regulation";
    case BalansunCap::TriacDimming:
      return "triac";
    case BalansunCap::MultiAction:
      return "multi_action";
    case BalansunCap::MqttHaDiscovery:
      return "mqtt_ha";
    case BalansunCap::SourceTestInject:
      return "source_test_inject";
    case BalansunCap::VacationMode:
      return "vacation";
    case BalansunCap::SelfTestTriac:
      return "self_test_triac";
    case BalansunCap::PwmChannel:
      return "pwm";
    case BalansunCap::StatusLedRgb:
      return "status_led_rgb";
    default:
      return "unknown";
  }
}
