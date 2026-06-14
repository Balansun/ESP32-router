/** Matches firmware `balansun_regulation_modes.h` (stored in `Actif`). */
export const MODE_INACTIF = 0;
export const MODE_DECOUPE_ONOFF = 1;

export function isActionRegulationEnabled(regulationMode: number | undefined): boolean {
  return (regulationMode ?? MODE_INACTIF) !== MODE_INACTIF;
}

/** UI on/off switch: off → inactive; on → on/off mode unless already an expert mode. */
export function setActionRegulationEnabled(
  regulationMode: number | undefined,
  enabled: boolean,
): number {
  const mode = regulationMode ?? MODE_INACTIF;
  if (!enabled) return MODE_INACTIF;
  if (mode === MODE_INACTIF) return MODE_DECOUPE_ONOFF;
  return mode;
}
