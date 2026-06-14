import type { TempoTariffStatus } from "../api/types";
import { getStrings } from "../i18n";

const TARIFF_COLOR_MAP: Record<string, string> = {
  BLEU: "var(--c-tariff-bleu)",
  BLANC: "var(--c-tariff-blanc)",
  ROUGE: "var(--c-tariff-rouge)",
  CREUSE: "var(--c-tariff-creuse)",
  PLEINE: "var(--c-tariff-pleine)",
};

export interface TariffDisplay {
  label: string;
  color: string;
}

/** Map LTARF / TEMPO_* strings to localized label and CSS color token. */
export function resolveTariffDisplay(text: string): TariffDisplay | null {
  const trimmed = text.trim();
  if (!trimmed || trimmed === "UNDEFINED") return null;

  const tariffs = getStrings().home.tariffs;
  for (const key of Object.keys(tariffs) as (keyof typeof tariffs)[]) {
    if (trimmed.indexOf(key) >= 0) {
      return {
        label: tariffs[key],
        /* v8 ignore next -- every tariffs key has a color entry */
        color: TARIFF_COLOR_MAP[key] ?? "var(--c-fg-faint)",
      };
    }
  }
  return { label: trimmed, color: "var(--c-fg-faint)" };
}

const TEMPO_TARIFF_CODES: Record<number, string> = {
  17: "TEMPO_BLEU",
  18: "TEMPO_BLANC",
  19: "TEMPO_ROUGE",
};

/** Prefer ltarf, then today_color, then firmware tariff_code (17–19). */
export function tempoTariffText(
  status: {
    ltarf?: string;
    today_color?: string;
    tomorrow_color?: string;
    tariff_code?: number;
  },
  opts?: { allowTomorrow?: boolean },
): string | undefined {
  const ltarf = status.ltarf?.trim();
  if (ltarf && ltarf !== "UNDEFINED") return ltarf;
  const today = status.today_color?.trim();
  if (today && today !== "UNDEFINED") return today;
  const code = status.tariff_code;
  if (code != null && TEMPO_TARIFF_CODES[code]) return TEMPO_TARIFF_CODES[code];
  if (opts?.allowTomorrow) {
    const tomorrow = status.tomorrow_color?.trim();
    if (tomorrow && tomorrow !== "UNDEFINED") return tomorrow;
  }
  return undefined;
}

export interface TempoTariffDisplay {
  text: string;
  isTomorrow: boolean;
}

export function resolveTempoTariffDisplay(
  status: Parameters<typeof tempoTariffText>[0],
): TempoTariffDisplay | null {
  const today = tempoTariffText(status);
  if (today) return { text: today, isTomorrow: false };
  const tomorrow = status.tomorrow_color?.trim();
  if (tomorrow && tomorrow !== "UNDEFINED") {
    return { text: tomorrow, isTomorrow: true };
  }
  return null;
}

export function mergeTempoTariffStatus(
  status: TempoTariffStatus,
  measurements?: { linky_tariff?: string; ltarf?: string } | null,
): TempoTariffStatus {
  if (tempoTariffText(status)) return status;
  const linky = measurements?.linky_tariff?.trim() ?? "";
  const ltarfM = measurements?.ltarf?.trim() ?? "";
  const fromM = linky || ltarfM;
  if (fromM && fromM !== "UNDEFINED") {
    return { ...status, ltarf: fromM };
  }
  return status;
}
