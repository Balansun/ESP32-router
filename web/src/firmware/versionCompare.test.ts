import { describe, expect, it } from "vitest";
import {
  compareFirmwareVersions,
  formatFirmwareVersionFull,
  isNewerReleaseAvailable,
  isSameReleaseTag,
  normalizeTagForCompare,
  parseVersionParts,
} from "./versionCompare";

describe("versionCompare", () => {
  it("formatFirmwareVersionFull handles empty", () => {
    expect(formatFirmwareVersionFull("")).toBe("—");
    expect(formatFirmwareVersionFull("v0.1.0")).toBe("v0.1.0");
  });

  it("normalizeTagForCompare strips v and _RMS", () => {
    expect(normalizeTagForCompare("V0.1.0_RMS")).toBe("0.1.0");
  });

  it("isSameReleaseTag compares normalized tags", () => {
    expect(isSameReleaseTag("0.1.0", "v0.1.0")).toBe(true);
    expect(isSameReleaseTag("", "v0.1.0")).toBe(false);
  });

  it("parseVersionParts handles empty and non-numeric", () => {
    expect(parseVersionParts("")).toEqual([0]);
    expect(parseVersionParts("v1.a.2")).toEqual([1, 0, 2]);
  });

  it("parseVersionParts returns zero for non-numeric segments only", () => {
    expect(parseVersionParts("v..")).toEqual([0, 0]);
    expect(parseVersionParts("---")).toEqual([0, 0]);
  });

  it("compareFirmwareVersions orders semver segments", () => {
    expect(compareFirmwareVersions("0.1.0", "0.2.0")).toBe(-1);
    expect(compareFirmwareVersions("0.2.0", "0.1.0")).toBe(1);
    expect(compareFirmwareVersions("0.1.0", "v0.1.0")).toBe(0);
    expect(compareFirmwareVersions("0.1.0", "0.1.0.1")).toBe(-1);
    expect(compareFirmwareVersions("1.0", "1.0.0")).toBe(0);
    expect(compareFirmwareVersions("2.0.0", "1.9.9")).toBe(1);
    expect(compareFirmwareVersions("1.2.3", "1.2")).toBe(1);
  });

  it("formatFirmwareVersionFull handles nullish", () => {
    expect(formatFirmwareVersionFull(null)).toBe("—");
    expect(formatFirmwareVersionFull(undefined)).toBe("—");
  });

  it("isNewerReleaseAvailable is true only when GitHub is ahead", () => {
    expect(isNewerReleaseAvailable("0.3.6", "0.4.0")).toBe(true);
    expect(isNewerReleaseAvailable("0.4.0", "0.3.6")).toBe(false);
    expect(isNewerReleaseAvailable("0.3.6", "v0.3.6")).toBe(false);
  });
});
