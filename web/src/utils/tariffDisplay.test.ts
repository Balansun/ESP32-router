import { describe, expect, it } from "vitest";
import {
  mergeTempoTariffStatus,
  resolveTariffDisplay,
  resolveTempoTariffDisplay,
  tempoTariffText,
} from "./tariffDisplay";

describe("resolveTariffDisplay", () => {
  it("maps TEMPO_BLEU to bleu color token", () => {
    const d = resolveTariffDisplay("TEMPO_BLEU");
    expect(d?.label).toBeTruthy();
    expect(d?.color).toBe("var(--c-tariff-bleu)");
  });

  it("returns null for UNDEFINED", () => {
    expect(resolveTariffDisplay("UNDEFINED")).toBeNull();
    expect(resolveTariffDisplay("")).toBeNull();
  });
});

describe("tempoTariffText", () => {
  it("prefers ltarf over today_color", () => {
    expect(
      tempoTariffText({ ltarf: "TEMPO_ROUGE", today_color: "TEMPO_BLEU" }),
    ).toBe("TEMPO_ROUGE");
  });

  it("falls back to today_color", () => {
    expect(tempoTariffText({ today_color: "TEMPO_BLANC" })).toBe("TEMPO_BLANC");
  });

  it("returns undefined when both fields empty or undefined", () => {
    expect(tempoTariffText({})).toBeUndefined();
    expect(tempoTariffText({ ltarf: "UNDEFINED", today_color: "UNDEFINED" })).toBeUndefined();
  });

  it("maps tariff_code 17–19", () => {
    expect(tempoTariffText({ tariff_code: 19 })).toBe("TEMPO_ROUGE");
    expect(tempoTariffText({ tariff_code: 17 })).toBe("TEMPO_BLEU");
  });

  it("uses tomorrow_color when allowTomorrow", () => {
    expect(
      tempoTariffText({ tomorrow_color: "TEMPO_ROUGE" }, { allowTomorrow: true }),
    ).toBe("TEMPO_ROUGE");
  });
});

describe("resolveTempoTariffDisplay", () => {
  it("prefers today ltarf", () => {
    expect(resolveTempoTariffDisplay({ ltarf: "TEMPO_BLEU" })).toEqual({
      text: "TEMPO_BLEU",
      isTomorrow: false,
    });
  });

  it("uses tomorrow_color when today is missing", () => {
    expect(
      resolveTempoTariffDisplay({
        tomorrow_color: "TEMPO_BLANC",
      }),
    ).toEqual({ text: "TEMPO_BLANC", isTomorrow: true });
  });

  it("returns null when no usable tariff", () => {
    expect(resolveTempoTariffDisplay({})).toBeNull();
    expect(resolveTempoTariffDisplay({ tomorrow_color: "UNDEFINED" })).toBeNull();
  });
});

describe("mergeTempoTariffStatus", () => {
  it("uses measurements linky_tariff when tempo endpoint is empty", () => {
    const merged = mergeTempoTariffStatus(
      { enabled: true, ltarf: "" },
      { linky_tariff: "TEMPO_BLEU" },
    );
    expect(tempoTariffText(merged)).toBe("TEMPO_BLEU");
  });

  it("returns status unchanged when no tariff text available", () => {
    const status = { enabled: true, ltarf: "" };
    expect(mergeTempoTariffStatus(status, null)).toBe(status);
    expect(mergeTempoTariffStatus(status, { linky_tariff: "UNDEFINED" })).toBe(status);
  });
});

describe("resolveTariffDisplay fallback", () => {
  it("uses faint color for unknown tariff text", () => {
    const d = resolveTariffDisplay("CUSTOM_TARIFF_X");
    expect(d?.label).toBe("CUSTOM_TARIFF_X");
    expect(d?.color).toBe("var(--c-fg-faint)");
  });

  it("maps CREUSE token to creuse color", () => {
    const d = resolveTariffDisplay("TEMPO_CREUSE");
    expect(d?.color).toBe("var(--c-tariff-creuse)");
  });
});

describe("mergeTempoTariffStatus measurements", () => {
  it("mergeTempoTariffStatus prefers measurements linky_tariff", () => {
    const merged = mergeTempoTariffStatus(
      { enabled: true, ltarf: "" },
      { linky_tariff: "TEMPO_ROUGE" },
    );
    expect(merged.ltarf).toBe("TEMPO_ROUGE");
  });

  it("uses measurements ltarf when linky_tariff absent", () => {
    const merged = mergeTempoTariffStatus(
      { enabled: true, ltarf: "" },
      { ltarf: "TEMPO_BLANC" },
    );
    expect(tempoTariffText(merged)).toBe("TEMPO_BLANC");
  });

  it("tempoTariffText uses tariff_code 18", () => {
    expect(tempoTariffText({ tariff_code: 18 })).toBe("TEMPO_BLANC");
  });

  it("mergeTempoTariffStatus uses measurements ltarf when linky_tariff empty", () => {
    const merged = mergeTempoTariffStatus(
      { enabled: true },
      { linky_tariff: "   ", ltarf: "TEMPO_ROUGE" },
    );
    expect(merged.ltarf).toBe("TEMPO_ROUGE");
  });

  it("mergeTempoTariffStatus uses ltarf when linky_tariff is absent", () => {
    const merged = mergeTempoTariffStatus({ enabled: true }, { ltarf: "TEMPO_BLANC" });
    expect(merged.ltarf).toBe("TEMPO_BLANC");
  });

  it("mergeTempoTariffStatus reads trimmed ltarf when linky_tariff missing", () => {
    const merged = mergeTempoTariffStatus(
      { enabled: true },
      { ltarf: " TEMPO_ROUGE " },
    );
    expect(merged.ltarf).toBe("TEMPO_ROUGE");
  });

  it("mergeTempoTariffStatus prefers linky_tariff from measurements", () => {
    const merged = mergeTempoTariffStatus(
      { enabled: true },
      { linky_tariff: "TEMPO_BLEU" },
    );
    expect(merged.ltarf).toBe("TEMPO_BLEU");
  });

  it("mergeTempoTariffStatus returns same status when tempo already populated", () => {
    const status = { enabled: true, ltarf: "TEMPO_BLEU" };
    expect(mergeTempoTariffStatus(status, { ltarf: "TEMPO_ROUGE" })).toBe(status);
  });
});
