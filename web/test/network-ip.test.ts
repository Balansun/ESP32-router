import { describe, expect, it } from "vitest";
import {
  applyLiveNetworkPrefill,
  displayStoredIp,
  isUnsetIp,
  validLiveIp,
} from "../src/utils/networkIp";

describe("networkIp", () => {
  it("isUnsetIp", () => {
    expect(isUnsetIp("")).toBe(true);
    expect(isUnsetIp(undefined)).toBe(true);
    expect(isUnsetIp("0.0.0.0")).toBe(true);
    expect(isUnsetIp(" 0.0.0.0 ")).toBe(true);
    expect(isUnsetIp("192.168.1.1")).toBe(false);
  });

  it("displayStoredIp", () => {
    expect(displayStoredIp("0.0.0.0")).toBe("");
    expect(displayStoredIp(undefined)).toBe("");
    expect(displayStoredIp("10.0.0.2")).toBe("10.0.0.2");
  });

  it("applyLiveNetworkPrefill only fills unset fields", () => {
    const ipFixed = { v: "0.0.0.0", read() { return this.v; }, write(x: string) { this.v = x; } };
    const gateway = { v: "192.168.1.1", read() { return this.v; }, write(x: string) { this.v = x; } };
    const subnet = { v: "", read() { return this.v; }, write(x: string) { this.v = x; } };
    const dns = { v: "0.0.0.0", read() { return this.v; }, write(x: string) { this.v = x; } };

    applyLiveNetworkPrefill(
      { ipFixed, gateway, subnet, dns },
      {
        ip: "192.168.1.117",
        gateway: "192.168.1.254",
        subnet: "255.255.255.0",
        dns: "192.168.1.254",
      },
    );

    expect(ipFixed.v).toBe("192.168.1.117");
    expect(gateway.v).toBe("192.168.1.1");
    expect(subnet.v).toBe("255.255.255.0");
    expect(dns.v).toBe("192.168.1.254");
    expect(validLiveIp("0.0.0.0")).toBeNull();
    expect(validLiveIp("192.168.1.5")).toBe("192.168.1.5");
  });

  it("displayStoredIp and validLiveIp handle null", () => {
    expect(displayStoredIp(null as unknown as string)).toBe("");
    expect(validLiveIp(null as unknown as string)).toBeNull();
    expect(displayStoredIp("  192.168.1.3  ")).toBe("192.168.1.3");
    expect(isUnsetIp("   ")).toBe(true);
  });

  it("applyLiveNetworkPrefill skips fields that already have an address", () => {
    const ipFixed = { v: "10.0.0.9", read() { return this.v; }, write(x: string) { this.v = x; } };
    const gateway = { v: "0.0.0.0", read() { return this.v; }, write(x: string) { this.v = x; } };
    const subnet = { v: "", read() { return this.v; }, write(x: string) { this.v = x; } };
    const dns = { v: "0.0.0.0", read() { return this.v; }, write(x: string) { this.v = x; } };
    applyLiveNetworkPrefill(
      { ipFixed, gateway, subnet, dns },
      { ip: "192.168.1.200", gateway: "192.168.1.1", subnet: "255.255.255.0", dns: "8.8.8.8" },
    );
    expect(ipFixed.v).toBe("10.0.0.9");
    expect(gateway.v).toBe("192.168.1.1");
  });
});
