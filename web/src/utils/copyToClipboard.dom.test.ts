import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { copyTextToClipboard, copyTextToClipboardSync } from "./copyToClipboard";

describe("copyTextToClipboard", () => {
  const writeText = vi.fn();
  const execCommand = vi.fn();

  beforeEach(() => {
    writeText.mockReset();
    execCommand.mockReset();
    Object.defineProperty(navigator, "clipboard", {
      configurable: true,
      value: { writeText },
    });
    document.execCommand = execCommand;
  });

  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("uses Clipboard API in a secure context", async () => {
    vi.stubGlobal("isSecureContext", true);
    writeText.mockResolvedValue(undefined);

    const ok = await copyTextToClipboard("secret-token");

    expect(ok).toBe(true);
    expect(writeText).toHaveBeenCalledWith("secret-token");
    expect(execCommand).not.toHaveBeenCalled();
  });

  it("falls back to execCommand when Clipboard API throws", async () => {
    vi.stubGlobal("isSecureContext", true);
    writeText.mockRejectedValue(new Error("denied"));
    execCommand.mockReturnValue(true);

    const ok = await copyTextToClipboard("lan-token");

    expect(ok).toBe(true);
    expect(execCommand).toHaveBeenCalledWith("copy");
    expect(document.querySelector("textarea")).toBeNull();
  });

  it("uses execCommand on non-secure HTTP contexts", async () => {
    vi.stubGlobal("isSecureContext", false);
    execCommand.mockReturnValue(true);

    const ok = await copyTextToClipboard("http-only");

    expect(ok).toBe(true);
    expect(writeText).not.toHaveBeenCalled();
    expect(execCommand).toHaveBeenCalledWith("copy");
  });

  it("returns false when execCommand fails", async () => {
    vi.stubGlobal("isSecureContext", false);
    execCommand.mockReturnValue(false);

    expect(await copyTextToClipboard("x")).toBe(false);
  });
});

describe("copyTextToClipboardSync", () => {
  const execCommand = vi.fn();

  beforeEach(() => {
    execCommand.mockReset();
    document.execCommand = execCommand;
    vi.stubGlobal("isSecureContext", false);
  });

  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("copies from a visible source input before using a hidden textarea", () => {
    execCommand.mockReturnValue(true);
    const input = document.createElement("input");
    input.value = "pat-token-abc";
    document.body.append(input);

    const ok = copyTextToClipboardSync("pat-token-abc", input);

    expect(ok).toBe(true);
    expect(execCommand).toHaveBeenCalledWith("copy");
    expect(document.querySelector("textarea")).toBeNull();
    input.remove();
  });

  it("uses hidden textarea when no source element is provided", () => {
    execCommand.mockReturnValue(true);

    expect(copyTextToClipboardSync("fallback-text")).toBe(true);
    expect(execCommand).toHaveBeenCalledWith("copy");
    expect(document.querySelector("textarea")).toBeNull();
  });

  it("falls back to hidden textarea when source element copy fails", () => {
    execCommand.mockReturnValueOnce(false).mockReturnValueOnce(true);
    const input = document.createElement("input");
    input.value = "pat-token-abc";
    document.body.append(input);

    expect(copyTextToClipboardSync("pat-token-abc", input)).toBe(true);
    expect(execCommand).toHaveBeenCalledTimes(2);
    input.remove();
  });

  it("returns false for empty text", () => {
    expect(copyTextToClipboardSync("")).toBe(false);
    expect(execCommand).not.toHaveBeenCalled();
  });

  it("sets clipboard payload via copy event listener", () => {
    const setData = vi.fn();
    execCommand.mockImplementation((cmd: string) => {
      if (cmd !== "copy") return false;
      document.dispatchEvent(
        new ClipboardEvent("copy", {
          clipboardData: { setData, preventDefault: vi.fn() } as unknown as DataTransfer,
        }),
      );
      return true;
    });

    expect(copyTextToClipboardSync("event-payload")).toBe(true);
    expect(setData).toHaveBeenCalledWith("text/plain", "event-payload");
  });

  it("returns false when execCommand throws", () => {
    execCommand.mockImplementation(() => {
      throw new Error("blocked");
    });
    expect(copyTextToClipboardSync("x")).toBe(false);
  });

  it("still copies when setSelectionRange throws on readonly input", () => {
    execCommand.mockReturnValue(true);
    const input = document.createElement("input");
    input.value = "readonly-token";
    input.setSelectionRange = () => {
      throw new Error("readonly");
    };
    document.body.append(input);

    expect(copyTextToClipboardSync("readonly-token", input)).toBe(true);
    input.remove();
  });
});
