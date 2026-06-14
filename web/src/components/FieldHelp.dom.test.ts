import { afterEach, describe, expect, it } from "vitest";
import {
  buildFieldLabelRow,
  buildInlineFieldHelp,
  buildSectionTitleWithHelp,
  wrapSwitchWithHelp,
} from "./FieldHelp";
import { buildSwitch } from "../routes/actions/formControls";

describe("FieldHelp DOM", () => {
  afterEach(() => {
    document.body.replaceChildren();
  });

  it("buildInlineFieldHelp renders multiple paragraphs", () => {
    const el = buildInlineFieldHelp(["Line one.", "Line two."]);
    expect(el.querySelectorAll("p").length).toBeGreaterThanOrEqual(2);
  });

  it("buildInlineFieldHelp renders details summary", () => {
    const el = buildInlineFieldHelp("Help text here.");
    expect(el.tagName).toBe("DETAILS");
    expect(el.querySelector("summary")?.textContent).toBe("?");
    expect(el.textContent).toContain("Help text here.");
  });

  it("buildFieldLabelRow includes help for known actions key", () => {
    const row = buildFieldLabelRow({
      label: "Daily cap",
      helpScope: "actions",
      helpKey: "action_daily_cap_wh",
    });
    expect(row.querySelector(".field__label")?.textContent).toBe("Daily cap");
    expect(row.querySelector(".field-help")).not.toBeNull();
    expect(row.textContent).toContain("CH2 daily export");
    const link = row.querySelector(".field-help__doc a") as HTMLAnchorElement;
    expect(link.href).toContain("/en/field-help/actions/#action_daily_cap_wh");
    expect(link.target).toBe("_blank");
  });

  it("wrapSwitchWithHelp and buildSectionTitleWithHelp", () => {
    const sw = buildSwitch(true, "On", () => {});
    expect(
      wrapSwitchWithHelp(sw, "actions", "action_daily_cap_wh").querySelector(
        ".field-help",
      ),
    ).not.toBeNull();
    const title = buildSectionTitleWithHelp("Section", "backup", "import_file");
    expect(title.querySelector(".section__title")?.textContent).toBe("Section");
  });

  it("closes open help on pointerdown outside", () => {
    const help = buildInlineFieldHelp("Outside dismiss.");
    document.body.append(help);
    help.open = true;

    document.body.dispatchEvent(new PointerEvent("pointerdown", { bubbles: true }));
    expect(help.open).toBe(false);
  });

  it("keeps open help on pointerdown inside", () => {
    const help = buildInlineFieldHelp("Stay open.");
    document.body.append(help);
    help.open = true;

    help
      .querySelector(".field-help__body")!
      .dispatchEvent(new PointerEvent("pointerdown", { bubbles: true }));
    expect(help.open).toBe(true);
  });

  it("buildFieldLabelRow uses helpDetail without doc link", () => {
    const row = buildFieldLabelRow({
      label: "Custom",
      helpDetail: "Inline only.",
    });
    expect(row.querySelector(".field-help__doc")).toBeNull();
    expect(row.textContent).toContain("Inline only.");
  });

  it("ignores pointerdown when event target is not a Node", () => {
    const help = buildInlineFieldHelp("stay");
    document.body.append(help);
    help.open = true;
    const ev = new PointerEvent("pointerdown", { bubbles: true });
    Object.defineProperty(ev, "target", { value: {}, configurable: true });
    document.dispatchEvent(ev);
    expect(help.open).toBe(true);
  });

  it("buildFieldLabelRow uses span label when forId omitted", () => {
    const row = buildFieldLabelRow({ label: "No for" });
    expect(row.querySelector("label")).toBeNull();
    expect(row.querySelector("span.field__label")).not.toBeNull();
  });

  it("wrapSwitchWithHelp omits help when key missing", () => {
    const sw = buildSwitch(true, "On", () => {});
    const row = wrapSwitchWithHelp(sw, "actions", "nonexistent_xyz");
    expect(row.querySelector(".field-help")).toBeNull();
  });

  it("buildSectionTitleWithHelp omits help when key missing", () => {
    const row = buildSectionTitleWithHelp("Title", "backup", "nonexistent_xyz");
    expect(row.querySelector(".field-help")).toBeNull();
  });

  it("buildSectionTitleWithHelp attaches help for known backup key", () => {
    const row = buildSectionTitleWithHelp("Title", "backup", "sectionImport");
    expect(row.querySelector(".field-help")).not.toBeNull();
  });

  it("buildFieldLabelRow omits help when key missing", () => {
    const row = buildFieldLabelRow({
      label: "Plain",
      helpScope: "actions",
      helpKey: "nonexistent_key_xyz",
    });
    expect(row.querySelector(".field-help")).toBeNull();
  });
});
