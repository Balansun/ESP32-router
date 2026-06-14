import { describe, expect, it } from "vitest";
import {
  bindRouteScope,
  createRouteScope,
  routeCleanup,
} from "../src/utils/routeLifecycle";

describe("routeLifecycle", () => {
  it("stops tracked polls when the route abort signal fires", async () => {
    const ac = new AbortController();
    const stops: string[] = [];
    const cleanup = routeCleanup(ac.signal, (scope) => {
      scope.trackPoll({
        stop() {
          stops.push("a");
        },
      });
      scope.trackPoll({
        stop() {
          stops.push("b");
        },
      });
    });
    expect(stops).toEqual([]);
    ac.abort();
    cleanup();
    expect(stops).toEqual(["a", "b"]);
  });

  it("runs custom unmount after scope disposal", () => {
    const ac = new AbortController();
    const order: string[] = [];
    const cleanup = routeCleanup(ac.signal, (scope) => {
      scope.trackPoll({
        stop() {
          order.push("poll");
        },
      });
      return () => {
        order.push("extra");
      };
    });
    cleanup();
    expect(order).toEqual(["poll", "extra"]);
  });

  it("createRouteScope disposes tracked callbacks", () => {
    const scope = createRouteScope(new AbortController().signal);
    let n = 0;
    scope.onUnmount(() => {
      n++;
    });
    scope.dispose();
    expect(n).toBe(1);
  });

  it("bindRouteScope disposes on abort", () => {
    const ac = new AbortController();
    const scope = createRouteScope(ac.signal);
    let disposed = false;
    bindRouteScope(ac.signal);
    ac.abort();
    scope.onUnmount(() => {
      disposed = true;
    });
    expect(disposed).toBe(false);
  });
});
