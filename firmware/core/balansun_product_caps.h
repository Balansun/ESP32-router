#pragma once

#include "balansun_product_caps_logic.h"

/** Compile-time product capabilities (firmware). */
BalansunProductCaps balansun_product_caps_compile_time();

inline bool balansun_product_has_cap(BalansunCap cap) {
  return balansun_product_caps_has(balansun_product_caps_compile_time(), cap);
}
