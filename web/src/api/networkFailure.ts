/** Detect browser `fetch()` failures (device unreachable, offline, blocked GitHub API). */

export type FirmwareFetchContext = "github_check" | "device";

export class ApiNetworkError extends Error {
  readonly context: FirmwareFetchContext;

  constructor(context: FirmwareFetchContext) {
    super("network");
    this.name = "ApiNetworkError";
    this.context = context;
  }
}

function errorMessage(e: unknown): string {
  if (typeof e === "string") return e;
  if (e && typeof e === "object" && "message" in e) {
    const m = (e as { message: unknown }).message;
    if (typeof m === "string") return m;
  }
  return "";
}

function errorName(e: unknown): string {
  if (e && typeof e === "object" && "name" in e) {
    const n = (e as { name: unknown }).name;
    if (typeof n === "string") return n;
  }
  return "";
}

export function isBrowserNetworkFailure(e: unknown): boolean {
  if (e instanceof ApiNetworkError) return true;
  const msg = errorMessage(e);
  if (/failed to fetch|networkerror|load failed|network error|fetch failed/i.test(msg)) {
    return true;
  }
  const name = errorName(e);
  if (name === "TypeError" || name === "NetworkError" || name === "TimeoutError") {
    return true;
  }
  if (e instanceof TypeError) return true;
  if (e instanceof DOMException) {
    return (
      name === "AbortError" || name === "NetworkError" || name === "TimeoutError"
    );
  }
  return false;
}
