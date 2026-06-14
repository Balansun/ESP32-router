/** True when EEPROM/config has no usable address (empty or 0.0.0.0). */
export function isUnsetIp(v: string | undefined): boolean {
  const s = (v ?? "").trim();
  return !s || s === "0.0.0.0";
}

/** Value for form fields: hide unset placeholders until live prefill runs. */
export function displayStoredIp(v: string | undefined): string {
  if (isUnsetIp(v)) return "";
  return String(v).trim();
}

export function validLiveIp(v: string | undefined): string | null {
  const s = (v ?? "").trim();
  if (isUnsetIp(s)) return null;
  return s;
}

export interface NetworkFieldRef {
  read(): string;
  write(v: string): void;
}

export interface LiveNetworkFields {
  ipFixed: NetworkFieldRef;
  gateway: NetworkFieldRef;
  subnet: NetworkFieldRef;
  dns: NetworkFieldRef;
}

/** Fill inputs from `GET /api/v1/system` when still unset (does not fire input events). */
export function applyLiveNetworkPrefill(
  fields: LiveNetworkFields,
  system: { ip?: string; gateway?: string; subnet?: string; dns?: string },
): void {
  const pairs: [NetworkFieldRef, string | undefined][] = [
    [fields.ipFixed, system.ip],
    [fields.gateway, system.gateway],
    [fields.subnet, system.subnet],
    [fields.dns, system.dns],
  ];
  for (const [field, live] of pairs) {
    const ip = validLiveIp(live);
    if (ip && isUnsetIp(field.read())) {
      field.write(ip);
    }
  }
}
