/** Full device version for UI (never strips `_RMS` or prerelease suffixes). */
export function formatFirmwareVersionFull(raw: string | undefined | null): string {
  const s = (raw ?? "").trim();
  return s || "—";
}

/** Normalize tag/device strings for equality (v prefix, `_RMS`, case). */
export function normalizeTagForCompare(v: string): string {
  return v
    .trim()
    .replace(/^v/i, "")
    .replace(/_RMS$/i, "")
    .toLowerCase();
}

/** True when device version and GitHub `tag_name` refer to the same release. */
export function isSameReleaseTag(current: string, releaseTag: string): boolean {
  const a = normalizeTagForCompare(current);
  const b = normalizeTagForCompare(releaseTag);
  return a.length > 0 && a === b;
}

/** Parse firmware tag or device version into comparable numeric segments. */
export function parseVersionParts(v: string): number[] {
  const core = v.trim().replace(/^v/i, "").replace(/_RMS$/i, "");
  if (!core) return [0];
  const parts = core.split(/[._-]+/).map((p) => {
    const n = parseInt(p, 10);
    return Number.isFinite(n) ? n : 0;
  });
  /* v8 ignore next -- split always yields at least one segment */
  return parts.length ? parts : [0];
}

/** Compare device `firmware_version` to a release `tag_name`. */
export function compareFirmwareVersions(
  current: string,
  releaseTag: string,
): -1 | 0 | 1 {
  if (isSameReleaseTag(current, releaseTag)) return 0;
  const pa = parseVersionParts(current);
  const pb = parseVersionParts(releaseTag);
  const len = Math.max(pa.length, pb.length);
  for (let i = 0; i < len; i++) {
    const da = pa[i] ?? 0;
    const db = pb[i] ?? 0;
    if (da < db) return -1;
    if (da > db) return 1;
  }
  return 0;
}

/** True only when `releaseTag` is a strictly newer semver than the device version. */
export function isNewerReleaseAvailable(current: string, releaseTag: string): boolean {
  return compareFirmwareVersions(current, releaseTag) === -1;
}
