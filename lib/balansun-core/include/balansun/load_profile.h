#pragma once

#include <cstdint>

/** Routed load kind on Balansun Router (triac surplus). */
enum class LoadProfile : uint8_t {
  WaterHeaterTriac = 0,
  GenericTriac,
};

const char *balansun_load_profile_wire(LoadProfile profile);
LoadProfile balansun_load_profile_from_wire(const char *wire);

bool balansun_load_profile_supports_triac(LoadProfile profile);
