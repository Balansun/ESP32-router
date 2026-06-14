import { describe, expect, it, vi } from "vitest";
import { downloadJsonFile } from "../src/utils/backupFormat";

describe("backupFormat DOM", () => {
  it("downloadJsonFile triggers anchor click", () => {
    const click = vi.fn();
    const anchor = {
      href: "",
      download: "",
      rel: "",
      click,
      remove: vi.fn(),
    };
    vi.spyOn(document, "createElement").mockReturnValue(anchor as unknown as HTMLElement);
    vi.spyOn(document.body, "append").mockImplementation(() => {});
    vi.spyOn(URL, "createObjectURL").mockReturnValue("blob:x");
    vi.spyOn(URL, "revokeObjectURL").mockImplementation(() => {});
    downloadJsonFile("x.json", { a: 1 });
    expect(click).toHaveBeenCalled();
  });
});
