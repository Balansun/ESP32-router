import { describe, expect, it } from "vitest";
import { effectiveMqttDeviceName } from "../src/utils/mqttDeviceName";

describe("effectiveMqttDeviceName", () => {
  it("uses device_uid when stored name is empty or factory default", () => {
    expect(effectiveMqttDeviceName("", "abc123")).toBe("abc123");
    expect(effectiveMqttDeviceName("balansun", "device-1")).toBe("device-1");
    expect(effectiveMqttDeviceName("balansun", "abc123")).toBe("abc123");
    expect(effectiveMqttDeviceName("  balansun  ", "abc123")).toBe("abc123");
  });

  it("keeps a custom name", () => {
    expect(effectiveMqttDeviceName("my_router", "abc123")).toBe("my_router");
  });

  it("falls back to balansun without device_uid", () => {
    expect(effectiveMqttDeviceName("", "")).toBe("balansun");
  });

  it("treats whitespace-only stored name as factory default", () => {
    expect(effectiveMqttDeviceName("   ", "uid1")).toBe("uid1");
  });

  it("handles undefined stored and whitespace-only device uid", () => {
    expect(effectiveMqttDeviceName(undefined as unknown as string, "  ")).toBe(
      "balansun",
    );
  });

  it("handles null stored name like empty", () => {
    expect(effectiveMqttDeviceName(null as unknown as string, "uid1")).toBe("uid1");
  });

  it("uses optional chaining when stored is null", () => {
    expect(effectiveMqttDeviceName(null as unknown as string, "dev")).toBe("dev");
    expect(effectiveMqttDeviceName("balansun", "dev")).toBe("dev");
  });

  it("returns custom trimmed name when set", () => {
    expect(effectiveMqttDeviceName("  my_router  ", "dev")).toBe("my_router");
    expect(effectiveMqttDeviceName("custom", "ignored")).toBe("custom");
  });
});
