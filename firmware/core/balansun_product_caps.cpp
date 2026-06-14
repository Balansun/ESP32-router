#include "balansun_product_caps.h"

#include "balansun_meter_pack.h"
#include "balansun_product_profile.h"

BalansunProductCaps balansun_product_caps_compile_time() {
  BalansunProductCaps caps = balansun_product_caps_for_role(BALANSUN_ROLE);
  caps.profile_wire = balansun_product_profile_wire_compile_time();
  return caps;
}
