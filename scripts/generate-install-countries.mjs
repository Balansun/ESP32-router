#!/usr/bin/env node
/**
 * Generates web/src/data/install-countries.ts and firmware country lookup tables.
 * Run: node scripts/generate-install-countries.mjs
 */
import { mkdirSync, writeFileSync } from "fs";
import { dirname, join } from "path";
import { fileURLToPath } from "url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");

/** @typedef {{ lookupKey: string, parentIso2?: string, variantId?: string, v: number, hz: 50|60, tz?: string, linky?: boolean, splitNoteKey?: string }} Row */

/** @type {Row[]} */
const ROWS = [];

function add(key, v, hz, tz = "", flags = "") {
  const r = { lookupKey: key, v, hz };
  if (tz) r.tz = tz;
  if (flags.includes("L")) r.linky = true;
  if (flags.includes("S")) r.splitNoteKey = `settings.countrySplit${key}`;
  ROWS.push(r);
}

function variant(parent, id, v, hz, tz) {
  ROWS.push({
    lookupKey: `${parent}-${id}`,
    parentIso2: parent,
    variantId: id,
    v,
    hz,
    tz,
  });
}

// Meta
add("ZZ", 230, 50, "", "S");
ROWS[ROWS.length - 1].splitNoteKey = "settings.countryCustom";

// Split parents + variants
add("JP", 100, 50, "Asia/Tokyo", "S");
ROWS[ROWS.length - 1].splitNoteKey = "settings.countrySplitJapan";
variant("JP", "E", 100, 50, "Asia/Tokyo");
variant("JP", "W", 100, 60, "Asia/Tokyo");

add("BR", 127, 60, "America/Sao_Paulo", "S");
ROWS[ROWS.length - 1].splitNoteKey = "settings.countrySplitBrazil";
variant("BR", "127", 127, 60, "America/Sao_Paulo");
variant("BR", "220", 220, 60, "America/Sao_Paulo");

add("SA", 220, 60, "Asia/Riyadh", "S");
ROWS[ROWS.length - 1].splitNoteKey = "settings.countrySplitSaudi";
variant("SA", "127", 127, 60, "Asia/Riyadh");
variant("SA", "220", 220, 60, "Asia/Riyadh");

add("BZ", 110, 60, "America/Belize", "S");
ROWS[ROWS.length - 1].splitNoteKey = "settings.countrySplitBelize";
variant("BZ", "110", 110, 60, "America/Belize");
variant("BZ", "220", 220, 60, "America/Belize");

add("MG", 220, 50, "Indian/Antananarivo", "S");
ROWS[ROWS.length - 1].splitNoteKey = "settings.countrySplitMadagascar";
variant("MG", "127", 127, 50, "Indian/Antananarivo");
variant("MG", "220", 220, 50, "Indian/Antananarivo");

// 60 Hz Americas / Pacific
const hz60 = [
  ["AS", 120, "Pacific/Pago_Pago"],
  ["AI", 110, "America/Anguilla"],
  ["AG", 230, "America/Antigua"],
  ["AW", 127, "America/Aruba"],
  ["BS", 120, "America/Nassau"],
  ["BM", 120, "Atlantic/Bermuda"],
  ["VG", 110, "America/Tortola"],
  ["KY", 120, "America/Cayman"],
  ["CO", 110, "America/Bogota"],
  ["CR", 120, "America/Costa_Rica"],
  ["CU", 110, "America/Havana"],
  ["DO", 110, "America/Santo_Domingo"],
  ["EC", 120, "America/Guayaquil"],
  ["SV", 115, "America/El_Salvador"],
  ["GU", 120, "Pacific/Guam"],
  ["GT", 120, "America/Guatemala"],
  ["HT", 110, "America/Port-au-Prince"],
  ["HN", 110, "America/Tegucigalpa"],
  ["FM", 120, "Pacific/Chuuk"],
  ["MX", 127, "America/Mexico_City"],
  ["NI", 120, "America/Managua"],
  ["MP", 120, "Pacific/Saipan"],
  ["PA", 110, "America/Panama"],
  ["PE", 220, "America/Lima"],
  ["PR", 120, "America/Puerto_Rico"],
  ["SX", 110, "America/Lower_Princes"],
  ["SR", 127, "America/Paramaribo"],
  ["TT", 115, "America/Port_of_Spain"],
  ["TC", 120, "America/Grand_Turk"],
  ["US", 120, "America/New_York"],
  ["UM", 120, "Pacific/Midway"],
  ["VI", 120, "America/St_Thomas"],
  ["VE", 120, "America/Caracas"],
  ["PW", 120, "Pacific/Palau"],
  ["PF", 110, "Pacific/Tahiti"],
  ["PH", 220, "Asia/Manila"],
  ["TW", 110, "Asia/Taipei"],
  ["KR", 220, "Asia/Seoul"],
  ["LR", 120, "Africa/Monrovia"],
  ["CA", 120, "America/Toronto"],
  ["MH", 120, "Pacific/Majuro"],
  ["MS", 230, "America/Montserrat"],
  ["KN", 230, "America/St_Kitts"],
  ["MF", 230, "America/Marigot"],
  ["BL", 230, "America/St_Barthelemy"],
];
for (const [k, v, tz] of hz60) add(k, v, 60, tz);

add("CW", 127, 50, "America/Curacao");
add("JM", 110, 50, "America/Jamaica");
add("LC", 230, 50, "America/St_Lucia");
add("BB", 115, 50, "America/Barbados");
add("GY", 240, 60, "America/Guyana");

// 50 Hz — bulk
const hz50 = [
  ["AD", 230, "Europe/Andorra"],
  ["AE", 220, "Asia/Dubai"],
  ["AF", 220, "Asia/Kabul"],
  ["AL", 230, "Europe/Tirane"],
  ["AM", 230, "Asia/Yerevan"],
  ["AO", 220, "Africa/Luanda"],
  ["AR", 220, "America/Argentina/Buenos_Aires"],
  ["AT", 230, "Europe/Vienna"],
  ["AU", 240, "Australia/Sydney"],
  ["AZ", 220, "Asia/Baku"],
  ["BA", 230, "Europe/Sarajevo"],
  ["BD", 220, "Asia/Dhaka"],
  ["BE", 230, "Europe/Brussels"],
  ["BF", 220, "Africa/Ouagadougou"],
  ["BG", 230, "Europe/Sofia"],
  ["BH", 230, "Asia/Bahrain"],
  ["BI", 220, "Africa/Bujumbura"],
  ["BJ", 220, "Africa/Porto-Novo"],
  ["BN", 240, "Asia/Brunei"],
  ["BO", 230, "America/La_Paz"],
  ["BQ", 127, "America/Kralendijk"],
  ["BT", 230, "Asia/Thimphu"],
  ["BW", 230, "Africa/Gaborone"],
  ["BY", 220, "Europe/Minsk"],
  ["CD", 220, "Africa/Kinshasa"],
  ["CF", 220, "Africa/Bangui"],
  ["CG", 230, "Africa/Brazzaville"],
  ["CH", 230, "Europe/Zurich"],
  ["CI", 220, "Africa/Abidjan"],
  ["CK", 240, "Pacific/Rarotonga"],
  ["CL", 220, "America/Santiago"],
  ["CM", 220, "Africa/Douala"],
  ["CN", 220, "Asia/Shanghai"],
  ["CV", 220, "Atlantic/Cape_Verde"],
  ["CY", 230, "Asia/Nicosia"],
  ["CZ", 230, "Europe/Prague"],
  ["DE", 230, "Europe/Berlin"],
  ["DJ", 220, "Africa/Djibouti"],
  ["DK", 230, "Europe/Copenhagen"],
  ["DM", 230, "America/Dominica"],
  ["DZ", 230, "Africa/Algiers"],
  ["EE", 230, "Europe/Tallinn"],
  ["EG", 220, "Africa/Cairo"],
  ["EH", 220, "Africa/El_Aaiun"],
  ["ER", 230, "Africa/Asmara"],
  ["ES", 230, "Europe/Madrid"],
  ["ET", 220, "Africa/Addis_Ababa"],
  ["FI", 230, "Europe/Helsinki"],
  ["FJ", 240, "Pacific/Fiji"],
  ["FK", 240, "Atlantic/Stanley"],
  ["FO", 230, "Atlantic/Faroe"],
  ["FR", 230, "Europe/Paris", "L"],
  ["GA", 220, "Africa/Libreville"],
  ["GB", 230, "Europe/London"],
  ["GD", 230, "America/Grenada"],
  ["GE", 220, "Asia/Tbilisi"],
  ["GF", 220, "America/Cayenne"],
  ["GG", 230, "Europe/Guernsey"],
  ["GH", 230, "Africa/Accra"],
  ["GI", 240, "Europe/Gibraltar"],
  ["GL", 230, "America/Godthab"],
  ["GM", 230, "Africa/Banjul"],
  ["GN", 220, "Africa/Conakry"],
  ["GP", 230, "America/Guadeloupe"],
  ["GQ", 220, "Africa/Malabo"],
  ["GR", 230, "Europe/Athens"],
  ["GW", 220, "Africa/Bissau"],
  ["HK", 220, "Asia/Hong_Kong"],
  ["HR", 230, "Europe/Zagreb"],
  ["HU", 230, "Europe/Budapest"],
  ["ID", 230, "Asia/Jakarta"],
  ["IE", 230, "Europe/Dublin"],
  ["IL", 230, "Asia/Jerusalem"],
  ["IM", 230, "Europe/Isle_of_Man"],
  ["IN", 230, "Asia/Kolkata"],
  ["IO", 230, "Indian/Chagos"],
  ["IQ", 230, "Asia/Baghdad"],
  ["IR", 220, "Asia/Tehran"],
  ["IS", 230, "Atlantic/Reykjavik"],
  ["IT", 230, "Europe/Rome"],
  ["JE", 230, "Europe/Jersey"],
  ["JO", 230, "Asia/Amman"],
  ["KE", 240, "Africa/Nairobi"],
  ["KG", 220, "Asia/Bishkek"],
  ["KH", 230, "Asia/Phnom_Penh"],
  ["KI", 240, "Pacific/Tarawa"],
  ["KM", 220, "Indian/Comoro"],
  ["KP", 220, "Asia/Pyongyang"],
  ["KW", 240, "Asia/Kuwait"],
  ["KZ", 220, "Asia/Almaty"],
  ["LA", 230, "Asia/Vientiane"],
  ["LB", 220, "Asia/Beirut"],
  ["LI", 230, "Europe/Vaduz"],
  ["LK", 230, "Asia/Colombo"],
  ["LS", 220, "Africa/Maseru"],
  ["LT", 230, "Europe/Vilnius"],
  ["LU", 230, "Europe/Luxembourg"],
  ["LV", 230, "Europe/Riga"],
  ["LY", 127, "Africa/Tripoli"],
  ["MA", 220, "Africa/Casablanca"],
  ["MC", 230, "Europe/Monaco"],
  ["MD", 230, "Europe/Chisinau"],
  ["ME", 230, "Europe/Podgorica"],
  ["MK", 230, "Europe/Skopje"],
  ["ML", 220, "Africa/Bamako"],
  ["MM", 230, "Asia/Yangon"],
  ["MN", 230, "Asia/Ulaanbaatar"],
  ["MO", 220, "Asia/Macau"],
  ["MQ", 220, "America/Martinique"],
  ["MR", 220, "Africa/Nouakchott"],
  ["MT", 230, "Europe/Malta"],
  ["MU", 230, "Indian/Mauritius"],
  ["MV", 230, "Indian/Maldives"],
  ["MW", 230, "Africa/Blantyre"],
  ["MY", 240, "Asia/Kuala_Lumpur"],
  ["MZ", 220, "Africa/Maputo"],
  ["NA", 220, "Africa/Windhoek"],
  ["NC", 220, "Pacific/Noumea"],
  ["NE", 220, "Africa/Niamey"],
  ["NF", 230, "Pacific/Norfolk"],
  ["NG", 230, "Africa/Lagos"],
  ["NL", 230, "Europe/Amsterdam"],
  ["NO", 230, "Europe/Oslo"],
  ["NP", 230, "Asia/Kathmandu"],
  ["NR", 240, "Pacific/Nauru"],
  ["NU", 230, "Pacific/Niue"],
  ["NZ", 230, "Pacific/Auckland"],
  ["OM", 240, "Asia/Muscat"],
  ["PG", 240, "Pacific/Port_Moresby"],
  ["PK", 230, "Asia/Karachi"],
  ["PL", 230, "Europe/Warsaw"],
  ["PM", 230, "America/Miquelon"],
  ["PN", 230, "Pacific/Pitcairn"],
  ["PS", 230, "Asia/Gaza"],
  ["PT", 230, "Europe/Lisbon"],
  ["PY", 220, "America/Asuncion"],
  ["QA", 240, "Asia/Qatar"],
  ["RE", 230, "Indian/Reunion"],
  ["RO", 230, "Europe/Bucharest"],
  ["RS", 230, "Europe/Belgrade"],
  ["RU", 220, "Europe/Moscow"],
  ["RW", 230, "Africa/Kigali"],
  ["SB", 230, "Pacific/Guadalcanal"],
  ["SC", 240, "Indian/Mahe"],
  ["SD", 230, "Africa/Khartoum"],
  ["SE", 230, "Europe/Stockholm"],
  ["SG", 230, "Asia/Singapore"],
  ["SH", 230, "Atlantic/St_Helena"],
  ["SI", 230, "Europe/Ljubljana"],
  ["SJ", 230, "Arctic/Longyearbyen"],
  ["SK", 230, "Europe/Bratislava"],
  ["SL", 230, "Africa/Freetown"],
  ["SM", 230, "Europe/San_Marino"],
  ["SN", 230, "Africa/Dakar"],
  ["SO", 220, "Africa/Mogadishu"],
  ["SS", 230, "Africa/Juba"],
  ["ST", 220, "Africa/Sao_Tome"],
  ["SY", 220, "Asia/Damascus"],
  ["SZ", 230, "Africa/Mbabane"],
  ["TD", 220, "Africa/Ndjamena"],
  ["TG", 220, "Africa/Lome"],
  ["TH", 230, "Asia/Bangkok"],
  ["TJ", 220, "Asia/Dushanbe"],
  ["TK", 230, "Pacific/Fakaofo"],
  ["TL", 220, "Asia/Dili"],
  ["TM", 220, "Asia/Ashgabat"],
  ["TN", 230, "Africa/Tunis"],
  ["TO", 240, "Pacific/Tongatapu"],
  ["TR", 230, "Europe/Istanbul"],
  ["TV", 230, "Pacific/Funafuti"],
  ["TZ", 230, "Africa/Dar_es_Salaam"],
  ["UA", 230, "Europe/Kyiv"],
  ["UG", 240, "Africa/Kampala"],
  ["UY", 220, "America/Montevideo"],
  ["UZ", 220, "Asia/Tashkent"],
  ["VA", 230, "Europe/Vatican"],
  ["VC", 230, "America/St_Vincent"],
  ["VN", 220, "Asia/Ho_Chi_Minh"],
  ["VU", 230, "Pacific/Efate"],
  ["WF", 220, "Pacific/Wallis"],
  ["WS", 230, "Pacific/Apia"],
  ["XK", 230, "Europe/Belgrade"],
  ["YE", 230, "Asia/Aden"],
  ["YT", 230, "Indian/Mayotte"],
  ["ZA", 230, "Africa/Johannesburg"],
  ["ZM", 230, "Africa/Lusaka"],
  ["ZW", 230, "Africa/Harare"],
  ["AX", 230, "Europe/Mariehamn"],
  ["CC", 230, "Indian/Cocos"],
  ["CX", 240, "Indian/Christmas"],
];
for (const row of hz50) {
  const [k, v, tz, flags = ""] = row;
  add(k, v, 50, tz, flags);
}

// Build parent entries for UI (one per ISO2 in dropdown)
const parents = new Map();
for (const r of ROWS) {
  const parent = r.parentIso2 ?? r.lookupKey.slice(0, 2);
  if (r.lookupKey.length > 2 && r.parentIso2) continue;
  const iso2 = r.lookupKey.length <= 3 ? r.lookupKey : r.lookupKey.slice(0, 2);
  if (!parents.has(iso2) || r.lookupKey === iso2) {
    parents.set(iso2, r);
  }
}
for (const r of ROWS) {
  if (r.parentIso2) {
    const p = parents.get(r.parentIso2);
    if (p) {
      if (!p.splitVariants) p.splitVariants = [];
      p.splitVariants.push({
        variantId: r.variantId,
        lookupKey: r.lookupKey,
        defaultNominalV: r.v,
        defaultFrequencyHz: r.hz,
        suggestedTimeTz: r.tz,
      });
    }
  }
}

const installCountries = [...parents.values()]
  .filter((r) => r.lookupKey === (r.parentIso2 ?? r.lookupKey) || r.lookupKey.length <= 3)
  .map((r) => ({
    iso2: r.lookupKey.length <= 3 ? r.lookupKey : r.lookupKey.slice(0, 2),
    lookupKey: r.lookupKey,
    defaultNominalV: r.v,
    defaultFrequencyHz: r.hz,
    suggestedTimeTz: r.tz || undefined,
    linkyAvailable: r.linky || undefined,
    splitNoteKey: r.splitNoteKey || undefined,
    splitVariants: r.splitVariants || undefined,
  }));

// Dedupe by iso2 for dropdown (prefer exact key match)
const byIso = new Map();
for (const c of installCountries) {
  const existing = byIso.get(c.iso2);
  if (!existing || c.lookupKey === c.iso2) byIso.set(c.iso2, c);
}
const INSTALL_COUNTRIES = [...byIso.values()].sort((a, b) => a.iso2.localeCompare(b.iso2));

// --- emit TS ---
const ts = `/** AUTO-GENERATED by scripts/generate-install-countries.mjs — do not edit */
export type MainsFrequencyHz = 50 | 60;

export interface InstallCountryVariant {
  variantId: string;
  lookupKey: string;
  defaultNominalV: number;
  defaultFrequencyHz: MainsFrequencyHz;
  suggestedTimeTz?: string;
}

export interface InstallCountry {
  iso2: string;
  lookupKey: string;
  defaultNominalV: number;
  defaultFrequencyHz: MainsFrequencyHz;
  suggestedTimeTz?: string;
  linkyAvailable?: boolean;
  splitNoteKey?: string;
  splitVariants?: InstallCountryVariant[];
}

export const INSTALL_COUNTRIES: InstallCountry[] = ${JSON.stringify(INSTALL_COUNTRIES, null, 2)};

export type InstallCountryLookupRow = {
  lookupKey: string;
  defaultNominalV: number;
  defaultFrequencyHz: MainsFrequencyHz;
  suggestedTimeTz?: string;
  linkyAvailable?: boolean;
};

function lookupKeyExists(key: string): boolean {
  for (const c of INSTALL_COUNTRIES) {
    if (c.lookupKey === key) return true;
    if (c.splitVariants?.some((v) => v.lookupKey === key)) return true;
  }
  return false;
}

export function resolveInstallLookupKey(
  country: string,
  variant?: string,
): string {
  const c = (country || "").trim().toUpperCase();
  if (!c || c === "ZZ") return "ZZ";
  const v = (variant || "").trim();
  if (v) {
    const full = v.includes("-") ? v.toUpperCase() : \`\${c}-\${v.toUpperCase()}\`;
    if (lookupKeyExists(full)) return full;
  }
  return c;
}

export function lookupInstallCountry(
  country: string,
  variant?: string,
): InstallCountryLookupRow | undefined {
  const key = resolveInstallLookupKey(country, variant);
  for (const c of INSTALL_COUNTRIES) {
    if (c.splitVariants) {
      const sv = c.splitVariants.find((v) => v.lookupKey === key);
      if (sv) {
        return {
          lookupKey: sv.lookupKey,
          defaultNominalV: sv.defaultNominalV,
          defaultFrequencyHz: sv.defaultFrequencyHz,
          suggestedTimeTz: sv.suggestedTimeTz,
        };
      }
    }
    if (c.lookupKey === key) {
      return {
        lookupKey: c.lookupKey,
        defaultNominalV: c.defaultNominalV,
        defaultFrequencyHz: c.defaultFrequencyHz,
        suggestedTimeTz: c.suggestedTimeTz,
        linkyAvailable: c.linkyAvailable,
      };
    }
  }
  return undefined;
}

export function countryDisplayName(iso2: string, locale: string): string {
  try {
    const dn = new Intl.DisplayNames([locale], { type: "region" });
    return dn.of(iso2) ?? iso2;
  } catch {
    return iso2;
  }
}
`;

mkdirSync(join(root, "web/src/data"), { recursive: true });
writeFileSync(join(root, "web/src/data/install-countries.ts"), ts);

// --- emit firmware ---
const fwRows = ROWS.map((r) => ({
  key: r.lookupKey,
  v: r.v,
  hz: r.hz,
  linky: r.linky ? 1 : 0,
}));

const hFile = `/** AUTO-GENERATED by scripts/generate-install-countries.mjs */
#pragma once
#include <stddef.h>
#include <stdint.h>

struct RmsInstallCountryRow {
  const char *lookup_key;
  uint16_t nominal_v;
  uint8_t frequency_hz;
  uint8_t linky_available;
};

#define RMS_INSTALL_COUNTRY_COUNT ${fwRows.length}

const RmsInstallCountryRow *balansun_install_country_table(void);
size_t balansun_install_country_table_count(void);
/** Resolve lookup key (e.g. FR, JP-W). Returns nullptr if unknown. */
const RmsInstallCountryRow *balansun_install_country_find(const char *lookup_key);
`;

const cppRows = fwRows
  .map(
    (r) =>
      `  {"${r.key}", ${r.v}u, ${r.hz}u, ${r.linky}u}`,
  )
  .join(",\n");

const cppFile = `/** AUTO-GENERATED by scripts/generate-install-countries.mjs */
#include "balansun_install_countries.h"
#include <cstring>

static const RmsInstallCountryRow kTable[] = {
${cppRows}
};

const RmsInstallCountryRow *balansun_install_country_table(void) { return kTable; }
size_t balansun_install_country_table_count(void) { return RMS_INSTALL_COUNTRY_COUNT; }

const RmsInstallCountryRow *balansun_install_country_find(const char *lookup_key) {
  if (!lookup_key || !lookup_key[0]) lookup_key = "FR";
  for (size_t i = 0; i < RMS_INSTALL_COUNTRY_COUNT; i++) {
    if (strcmp(kTable[i].lookup_key, lookup_key) == 0) return &kTable[i];
  }
  return nullptr;
}
`;

writeFileSync(join(root, "firmware/core/balansun_install_countries.h"), hFile);
writeFileSync(join(root, "firmware/core/balansun_install_countries.cpp"), cppFile);

console.log(`Generated ${ROWS.length} lookup rows, ${INSTALL_COUNTRIES.length} UI countries`);
