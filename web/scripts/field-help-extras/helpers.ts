import { readFileSync } from "node:fs";
import { join } from "node:path";
import type { DocsLang, ExtraBlock } from "./types";

const firmwareRoot = join(import.meta.dirname, "../../..");

export function docsPath(lang: DocsLang, path: string): string {
  return `/${lang}/${path.replace(/^\//, "")}`;
}

export function readMeterFixture(relPath: string): string {
  const full = join(firmwareRoot, "firmware/test/fixtures/meters", relPath);
  const parsed = JSON.parse(readFileSync(full, "utf8")) as unknown;
  return JSON.stringify(parsed, null, 2);
}

export function exampleHeading(lang: DocsLang): string {
  return lang === "fr" ? "### Exemple" : "### Example";
}

export function exampleSubheading(lang: DocsLang, title: string): string {
  return lang === "fr" ? `### Exemple — ${title}` : `### Example — ${title}`;
}

export function exampleJson(lang: DocsLang, title: string | null, body: string): readonly ExtraBlock[] {
  const heading = title ? exampleSubheading(lang, title) : exampleHeading(lang);
  return [heading, "```json", body, "```"];
}

export function exampleTable(
  lang: DocsLang,
  title: string | null,
  rows: readonly (readonly [string, string])[],
): readonly ExtraBlock[] {
  const heading = title
    ? lang === "fr"
      ? `### Exemple (${title})`
      : `### Example (${title})`
    : exampleHeading(lang);
  const lines: ExtraBlock[] = [
    heading,
    "| Field | Value |",
    "|-------|-------|",
    ...rows.map(([k, v]) => `| ${k} | ${v} |`),
  ];
  return lines;
}

export function exampleCode(lang: DocsLang, title: string | null, body: string): readonly ExtraBlock[] {
  const heading = title ? exampleSubheading(lang, title) : exampleHeading(lang);
  return [heading, "```", body, "```"];
}

export function linkToGuide(lang: DocsLang, anchorPath: string): ExtraBlock {
  const label =
    lang === "fr" ? "Voir le guide utilisateur" : "See the user guide";
  return `${label}: [§](${docsPath(lang, anchorPath)}).`;
}

export function linkToFieldHelp(
  lang: DocsLang,
  scopeSlug: string,
  key: string,
  label?: string,
): ExtraBlock {
  const text = label ?? (lang === "fr" ? "Même champ en Réglages" : "Same field in Settings");
  return `${text}: [${key}](${docsPath(lang, `field-help/${scopeSlug}#${key}`)}).`;
}
