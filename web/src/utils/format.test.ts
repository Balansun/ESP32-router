import { beforeEach, describe, expect, it } from "vitest";
import { localePref } from "../state/store";
import {
  fmt1,
  fmt2,
  fmtAmps,
  fmtBytes,
  fmtEnergyWh,
  fmtInt,
  fmtPercent,
  fmtPowerW,
  fmtTempC,
  fmtTempCValue,
  fmtVolts,
  isProbeTemperatureReading,
  isValidIp,
} from "./format";

describe("format", () => {
  beforeEach(() => {
    localePref.set("en");
  });

  it("isValidIp accepts dotted quads", () => {
    expect(isValidIp("192.168.1.1")).toBe(true);
    expect(isValidIp("")).toBe(false);
    expect(isValidIp("256.1.1.1")).toBe(false);
    expect(isValidIp("1.2.3")).toBe(false);
  });

  it("isProbeTemperatureReading rejects firmware sentinel", () => {
    expect(isProbeTemperatureReading(-127)).toBe(false);
    expect(isProbeTemperatureReading(22.5)).toBe(true);
  });

  it("fmtInt returns em dash for invalid", () => {
    expect(fmtInt(null)).toBe("—");
    expect(fmtInt(1234)).toBe("1,234");
  });

  it("fmtPowerW fmtVolts fmtAmps follow display policy", () => {
    expect(fmtPowerW(426.7)).toBe("427");
    expect(fmtVolts(231.747)).toBe("231.75");
    expect(fmtAmps(3.8317)).toBe("3.83");
  });

  it("fmtEnergyWh scales units", () => {
    expect(fmtEnergyWh(500)).toEqual({ value: "500", unit: "Wh" });
    expect(fmtEnergyWh(2500)).toEqual({ value: "2.50", unit: "kWh" });
    expect(fmtEnergyWh(3011)).toEqual({ value: "3.01", unit: "kWh" });
    expect(fmtEnergyWh(3011.7)).toEqual({ value: "3.01", unit: "kWh" });
    expect(fmtEnergyWh(500.25)).toEqual({ value: "500.25", unit: "Wh" });
    expect(fmtEnergyWh(2_500_000).unit).toBe("MWh");
    expect(fmtEnergyWh(null).value).toBe("—");
  });

  it("fmt1 fmt2 fmtPercent fmtTempC fmtBytes", () => {
    expect(fmt1(1.23)).toContain("1");
    expect(fmt2(1.234)).toContain("1");
    expect(fmt1(Number.POSITIVE_INFINITY)).toBe("—");
    expect(fmt2(Number.NaN)).toBe("—");
    expect(fmtPercent(50)).toContain("%");
    expect(fmtPercent(Number.NaN)).toBe("—");
    expect(fmtTempC(22)).toContain("°C");
    expect(fmtTempC(19.0)).toBe("19 °C");
    expect(fmtTempC(19.1)).toBe("19.1 °C");
    expect(fmtTempCValue(19.0)).toBe("19");
    expect(fmtTempCValue(19.1)).toBe("19.1");
    expect(fmtTempC(-127)).toBe("—");
    expect(fmtBytes(512)).toContain("B");
    expect(fmtBytes(2048)).toContain("KB");
    expect(fmtBytes(5_000_000)).toContain("MB");
    expect(fmtBytes(-1)).toBe("—");
  });

  it("isValidIp rejects non-numeric octets", () => {
    expect(isValidIp("192.168.1.x")).toBe(false);
  });

  it("uses fr locale when localePref is fr", () => {
    localePref.set("fr");
    expect(fmtInt(1234)).toMatch(/[\s\u202f\u00a0]?/);
    localePref.set("en");
  });
});
