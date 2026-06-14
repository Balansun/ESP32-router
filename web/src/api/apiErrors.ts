import { ApiError } from "./client";
import { getStrings } from "../i18n";

function apiErrorBodyMessage(body: unknown): string | undefined {
  if (body && typeof body === "object" && "message" in body) {
    const m = (body as { message?: unknown }).message;
    if (typeof m === "string" && m.length > 0) return m;
  }
  return undefined;
}

/** Map known firmware API error messages to localized UI strings. */
export function mapFirmwareApiMessage(message: string): string | undefined {
  const A = getStrings().apiPage.apiErrors;
  const table: Record<string, string> = {
    "HTTP API password is not enabled": A.httpAuthPasswordNotSet,
    "maximum number of tokens reached": A.maxTokensReached,
    "label must be printable ASCII (max 24 chars)": A.invalidLabel,
    "no free token id": A.noFreeTokenId,
    "failed to hash token": A.failedToHashToken,
  };
  return table[message];
}

/** User-facing text for API failures (prefers localized mapping over raw HTTP status). */
export function formatApiError(e: unknown): string {
  const T = getStrings();
  if (e instanceof ApiError) {
    const msg = apiErrorBodyMessage(e.body);
    if (msg) {
      const mapped = mapFirmwareApiMessage(msg);
      if (mapped) return mapped;
      return msg;
    }
    if (e.message && !/^HTTP \d+/.test(e.message)) return e.message;
    return `HTTP ${e.status}`;
  }
  if (e instanceof Error && e.message) return e.message;
  return T.saveError;
}
