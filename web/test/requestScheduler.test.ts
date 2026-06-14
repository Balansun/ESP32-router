import { describe, expect, it } from "vitest";
import { scheduleApi } from "../src/api/requestScheduler";

describe("scheduleApi", () => {
  it("runs tasks immediately when under concurrency cap", async () => {
    const order: number[] = [];
    await Promise.all([
      scheduleApi(async () => {
        order.push(1);
      }),
      scheduleApi(async () => {
        order.push(2);
      }),
    ]);
    expect(order).toEqual([1, 2]);
  });

  it("never runs more than two API calls concurrently", async () => {
    let active = 0;
    let peak = 0;
    const gate = (ms: number) => new Promise((r) => setTimeout(r, ms));
    await Promise.all([
      scheduleApi(async () => {
        active++;
        peak = Math.max(peak, active);
        await gate(30);
        active--;
      }),
      scheduleApi(async () => {
        active++;
        peak = Math.max(peak, active);
        await gate(30);
        active--;
      }),
      scheduleApi(async () => {
        active++;
        peak = Math.max(peak, active);
        await gate(30);
        active--;
      }),
    ]);
    expect(peak).toBeLessThanOrEqual(2);
  });
});
