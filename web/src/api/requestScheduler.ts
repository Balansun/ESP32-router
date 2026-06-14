/** Limits concurrent fetches to the ESP32 HTTP server (single-threaded). */

/** ESP32 WebServer serves one TCP client at a time; >1 causes connection resets. */
const MAX_CONCURRENT = 1;
let inFlight = 0;
const waitQueue: Array<() => void> = [];

function pumpQueue(): void {
  while (inFlight < MAX_CONCURRENT && waitQueue.length > 0) {
    const next = waitQueue.shift();
    if (next) next();
  }
}

/** Serialize same-origin `fetch` against the ESP32 (bypasses `scheduleApi` in `client.ts`). */
export function scheduleDeviceFetch(
  input: string,
  init?: RequestInit,
): Promise<Response> {
  return scheduleApi(() => fetch(input, init));
}

export function scheduleApi<T>(fn: () => Promise<T>): Promise<T> {
  return new Promise<T>((resolve, reject) => {
    const run = () => {
      inFlight++;
      fn()
        .then(resolve, reject)
        .finally(() => {
          inFlight--;
          pumpQueue();
        });
    };
    if (inFlight < MAX_CONCURRENT) run();
    else waitQueue.push(run);
  });
}
