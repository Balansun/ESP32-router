import type { AppStrings } from "../i18n";

export type LoadProfileWire = "water_heater_triac" | "generic_triac";

export function normalizeLoadProfile(value: string | undefined): LoadProfileWire {
  if (value === "generic_triac") return "generic_triac";
  return "water_heater_triac";
}

export function loadProfileOptions(T: AppStrings): { value: LoadProfileWire; label: string }[] {
  return [
    { value: "water_heater_triac", label: T.settings.loadProfiles.water_heater_triac },
    { value: "generic_triac", label: T.settings.loadProfiles.generic_triac },
  ];
}
