import { describe, expect, it } from "vitest";
import {
  numberRow,
  passwordRow,
  selectRow,
  textRow,
  validateIp,
} from "../src/routes/settings/formRows";

describe("formRows", () => {
  it("validateIp toggles error state", () => {
    const { ref } = textRow("ip", "IP", "192.168.1.1", "");
    expect(validateIp(ref)).toBe(true);
    ref.write("999.1.1.1");
    expect(validateIp(ref)).toBe(false);
  });

  it("passwordRow and numberRow build fields", () => {
    const pw = passwordRow("pw", "Password", "secret", "hint");
    expect(pw.ref.read()).toBe("secret");
    const num = numberRow("n", "Num", 42, "hint", () => {});
    expect(num.ref.read()).toBe("42");
  });

  it("textRow with help uses FieldHelp label", () => {
    const { el } = textRow("t", "Label", "val", "hint", {
      helpScope: "settings",
      helpKey: "router_name",
    });
    expect(el.querySelector(".field-help")).not.toBeNull();
  });

  it("selectRow builds options and read/write round-trip", () => {
    const { ref, el } = selectRow(
      "mode",
      "Mode",
      "follow_triac",
      [
        { value: "off", label: "Off" },
        { value: "follow_triac", label: "Follow triac" },
        { value: "independent", label: "Independent" },
      ],
      "",
    );
    const select = el.querySelector("select");
    expect(select).not.toBeNull();
    expect(select?.tagName).toBe("SELECT");
    expect(select?.querySelectorAll("option")).toHaveLength(3);
    expect(ref.read()).toBe("follow_triac");
    ref.write("independent");
    expect(ref.read()).toBe("independent");
  });

  it("selectRow falls back to first option for unknown value", () => {
    const { ref } = selectRow(
      "gpio",
      "GPIO",
      "99",
      [
        { value: "-1", label: "Off" },
        { value: "14", label: "GPIO 14" },
      ],
      "",
    );
    expect(ref.read()).toBe("-1");
  });
});
