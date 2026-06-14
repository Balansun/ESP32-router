import { describe, expect, it } from "vitest";
import {
  fmtHourMinFrom2400,
  from2400ToMinutes,
  minutesTo2400,
} from "./time2400";

describe("time2400", () => {
  it("fmtHourMinFrom2400 formats HH:MM", () => {
    expect(fmtHourMinFrom2400(0)).toBe("00:00");
    expect(fmtHourMinFrom2400(800)).toBe("08:00");
    expect(fmtHourMinFrom2400(2400)).toBe("24:00");
  });

  it("minutesTo2400 and from2400ToMinutes round-trip", () => {
    expect(from2400ToMinutes(minutesTo2400(510))).toBe(510);
    expect(minutesTo2400(from2400ToMinutes(1800))).toBe(1800);
  });

  it("minutesTo2400 clamps to day bounds", () => {
    expect(minutesTo2400(-10)).toBe(0);
    expect(from2400ToMinutes(minutesTo2400(2000))).toBe(1440);
  });
});
