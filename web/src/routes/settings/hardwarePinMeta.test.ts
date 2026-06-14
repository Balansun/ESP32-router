import { describe, expect, it } from "vitest";
import {
  formatPinStatus,
  gpioCellText,
  HARDWARE_PIN_SUMMARY_ROWS,
  isPinCustom,
  lookupPresence,
  rowIsCustom,
} from "./hardwarePinMeta";
import { getStrings } from "../../i18n";
import { HARDWARE_PIN_DEFAULTS_WROOM } from "./hardwarePinDefaults";

describe("hardwarePinMeta", () => {
  it("detects custom GPIO vs default", () => {
    expect(isPinCustom(22, 22)).toBe(false);
    expect(isPinCustom(23, 22)).toBe(true);
    expect(isPinCustom(-1, 13)).toBe(true);
  });

  it("finds hardware presence item by id", () => {
    const items = [{ id: "triac_dim", role: "triac_dim", state: "ok" as const }];
    expect(lookupPresence(items, "triac_dim")?.state).toBe("ok");
    expect(lookupPresence(items, "missing")).toBeUndefined();
  });

  it("merges UART row for GPIO display", () => {
    const uart = HARDWARE_PIN_SUMMARY_ROWS.find((r) => r.kind === "uart")!;
    const text = gpioCellText(uart, { ...HARDWARE_PIN_DEFAULTS_WROOM, pin_temp_live: 13 });
    expect(text).toBe("RX 26 · TX 27");
  });

  it("shows live temperature on DS18B20 row when present and ok", () => {
    const T = getStrings();
    const item = { id: "temp_ds18b20", role: "temp_ds18b20", state: "ok" as const };
    expect(
      formatPinStatus(item, undefined, false, T, { entryKind: "temp", tempReadingC: 21.4 }),
    ).toContain("21.4");
    expect(formatPinStatus(item, undefined, false, T, { entryKind: "temp", tempReadingC: -127 })).toBe(
      T.diag.hardwareState.ok,
    );
  });

  it("flags UART row custom when RX or TX differs", () => {
    const uart = HARDWARE_PIN_SUMMARY_ROWS.find((r) => r.kind === "uart")!;
    expect(
      rowIsCustom(
        uart,
        { ...HARDWARE_PIN_DEFAULTS_WROOM, pin_uart_rx: 26, pin_uart_tx: 27, pin_temp_live: 13 },
        HARDWARE_PIN_DEFAULTS_WROOM,
      ),
    ).toBe(false);
    expect(
      rowIsCustom(
        uart,
        { ...HARDWARE_PIN_DEFAULTS_WROOM, pin_uart_rx: 4, pin_uart_tx: 27, pin_temp_live: 13 },
        HARDWARE_PIN_DEFAULTS_WROOM,
      ),
    ).toBe(true);
  });
});
