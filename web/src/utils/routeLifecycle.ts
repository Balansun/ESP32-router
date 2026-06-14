import type { PollHandle } from "../state/store";

/** Register poll handles and other teardown callbacks when a route unmounts. */
export function createRouteScope(_signal: AbortSignal) {
  const cleanups: Array<() => void> = [];

  return {
    trackPoll(handle: PollHandle) {
      cleanups.push(() => handle.stop());
    },
    onUnmount(fn: () => void) {
      cleanups.push(fn);
    },
    dispose() {
      for (const fn of cleanups) fn();
    },
  };
}

/** Wire scope disposal to the route abort signal. Returns a cleanup fn for the router. */
export function bindRouteScope(signal: AbortSignal): () => void {
  const scope = createRouteScope(signal);
  const onAbort = () => scope.dispose();
  signal.addEventListener("abort", onAbort, { once: true });
  return onAbort;
}

/** Run mount logic and return combined cleanup (polls + custom). */
export function routeCleanup(
  signal: AbortSignal,
  mount: (scope: ReturnType<typeof createRouteScope>) => void | (() => void),
): () => void {
  const scope = createRouteScope(signal);
  const extra = mount(scope);
  return () => {
    scope.dispose();
    extra?.();
  };
}
