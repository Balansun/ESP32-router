import { describe, expect, it } from "vitest";
import { en } from "../../i18n/locales/en";
import { sourceFriendlyTitle, sourceKeyForSummary } from "./measurementSourceSummary";

describe("sourceKeyForSummary", () => {
  it("uses configured label when present", () => {
    expect(sourceKeyForSummary("BalansunPeer", "JsyMk194")).toBe("balansunpeer");
  });

  it("falls back to cfg.source when configured empty", () => {
    expect(sourceKeyForSummary("", "BalansunPeer")).toBe("balansunpeer");
    expect(sourceKeyForSummary("", undefined)).toBe("notdef");
  });

  it("maps BalansunPeer to localized title", () => {
    expect(sourceFriendlyTitle("BalansunPeer", en)).toBe(en.settings.sourceSummary.titles.balansunPeer);
  });
});
