#include <balansun/load_profile.h>

#include <cstring>

const char *balansun_load_profile_wire(LoadProfile profile) {
  switch (profile) {
    case LoadProfile::GenericTriac:
      return "generic_triac";
    case LoadProfile::WaterHeaterTriac:
    default:
      return "water_heater_triac";
  }
}

LoadProfile balansun_load_profile_from_wire(const char *wire) {
  if (!wire) return LoadProfile::WaterHeaterTriac;
  if (std::strcmp(wire, "generic_triac") == 0) return LoadProfile::GenericTriac;
  if (std::strcmp(wire, "radiator_pilot_wire") == 0 ||
      std::strcmp(wire, "radiator_pilot_wire_remote") == 0 ||
      std::strcmp(wire, "radiator_triac_remote") == 0) {
    return LoadProfile::WaterHeaterTriac;
  }
  return LoadProfile::WaterHeaterTriac;
}

bool balansun_load_profile_supports_triac(LoadProfile profile) {
  return profile == LoadProfile::WaterHeaterTriac || profile == LoadProfile::GenericTriac;
}
