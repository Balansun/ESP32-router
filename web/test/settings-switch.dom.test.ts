import { describe, expect, it } from "vitest";
import { settingsSwitchLabel } from "../src/utils/settingsSwitch";

describe("settingsSwitchLabel", () => {
  it("includes switch__track for CSS toggle styling", () => {
    const input = document.createElement("input");
    input.type = "checkbox";
    const label = settingsSwitchLabel(input, "Test switch");
    document.body.append(label);
    expect(label.querySelector(".switch__track")).not.toBeNull();
    label.remove();
  });
});
