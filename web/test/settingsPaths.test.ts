import { describe, expect, it } from "vitest";
import {
  normalizeSettingsRedirect,
  parseSettingsSection,
  settingsPath,
} from "../src/routes/settings/settingsPaths";

describe("settingsPaths", () => {
  it("parseSettingsSection maps paths to sections", () => {
    expect(parseSettingsSection("/settings")).toBe("general");
    expect(parseSettingsSection("/settings/general")).toBe("general");
    expect(parseSettingsSection("/settings/metering")).toBe("metering");
    expect(parseSettingsSection("/settings/network")).toBe("network");
    expect(parseSettingsSection("/settings/led")).toBe("led");
    expect(parseSettingsSection("/settings/advanced")).toBe("advanced");
    expect(parseSettingsSection("/settings/unknown")).toBe("general");
  });

  it("settingsPath builds canonical URLs", () => {
    expect(settingsPath("network")).toBe("/settings/network");
    expect(settingsPath("led")).toBe("/settings/led");
  });

  it("normalizeSettingsRedirect canonicalizes bare and invalid paths", () => {
    expect(normalizeSettingsRedirect("/settings")).toBe("/settings/general");
    expect(normalizeSettingsRedirect("/settings/")).toBe("/settings/general");
    expect(normalizeSettingsRedirect("/settings/bad")).toBe("/settings/general");
    expect(normalizeSettingsRedirect("/settings/network")).toBe(null);
    expect(normalizeSettingsRedirect("/history")).toBe(null);
  });
});
