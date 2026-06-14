/**
 * Regenerates field-help markdown from fieldHelp locale tables.
 * Default output: ../Balansun-Website/content when present, else web/.field-help-docs/.
 * Run: cd web && npx tsx scripts/generate-field-help-docs.ts
 *      cd web && npx tsx scripts/generate-field-help-docs.ts --out ../Balansun-Website/content
 */
import { existsSync, mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { fieldHelpEn } from "../src/i18n/locales/fieldHelp.en";
import { fieldHelpFr } from "../src/i18n/locales/fieldHelp.fr";
import { fieldHelpExtrasEn } from "./field-help-extras/en";
import { fieldHelpExtrasFr } from "./field-help-extras/fr";
import type { ExtraMap } from "./field-help-extras/types";

const SCOPE_TITLES: Record<string, { en: string; fr: string }> = {
  settings: { en: "Settings", fr: "Paramètres" },
  actions: { en: "Actions", fr: "Actions" },
  firmware: { en: "Firmware", fr: "Firmware" },
  wifi: { en: "Wi‑Fi setup", fr: "Configuration Wi‑Fi" },
  sourceWizard: { en: "Measurement source wizard", fr: "Assistant source de mesure" },
  httpAuth: { en: "HTTP login", fr: "Connexion HTTP" },
  install: { en: "Install", fr: "Installation" },
  backup: { en: "Backup", fr: "Sauvegarde" },
  api: { en: "HTTP API", fr: "API HTTP" },
};

function scopeSlug(scope: string): string {
  if (scope === "sourceWizard") return "source-wizard";
  if (scope === "httpAuth") return "http-auth";
  return scope;
}

function renderScope(
  scope: string,
  entries: Record<string, string>,
  extras: ExtraMap,
  lang: "en" | "fr",
): string {
  const title = SCOPE_TITLES[scope]?.[lang] ?? scope;
  const intro =
    lang === "fr"
      ? `Référence des champs (**${scope}**). Chaque titre = clé d'aide \`?\` dans l'app.`
      : `Field reference (**${scope}**). Each heading = \`?\` help key in the app.`;
  let md = `# ${title}\n\n${intro}\n\n[← Index](index.md)\n\n`;
  for (const [key, summary] of Object.entries(entries)) {
    md += `## ${key}\n\n${summary}\n\n`;
    for (const block of extras[scope]?.[key] ?? []) {
      md += `${block}\n\n`;
    }
    md += "---\n\n";
  }
  return md.trimEnd() + "\n";
}

function writeIndex(lang: "en" | "fr", scopes: string[]): string {
  const isFr = lang === "fr";
  let md = isFr
    ? "# Aide des champs (interface web)\n\nDocumentation liée aux icônes **?**. Résumé dans l'app + lien vers ces pages.\n\n"
    : "# Field help (web UI)\n\nDocumentation for **?** icons. Short summary in-app + link here.\n\n";
  md += isFr ? "## Par écran\n\n" : "## By screen\n\n";
  for (const scope of scopes) {
    md += `- [${SCOPE_TITLES[scope]?.[lang] ?? scope}](${scopeSlug(scope)}.md)\n`;
  }
  return md;
}

const repoRoot = join(import.meta.dirname, "../..");
const outArg = process.argv.find((a) => a.startsWith("--out="));
const websiteContent = join(repoRoot, "..", "Balansun-Website", "content");
const contentRoot = outArg
  ? outArg.slice("--out=".length)
  : existsSync(websiteContent)
    ? websiteContent
    : join(repoRoot, "web", ".field-help-docs");

const scopes = Object.keys(fieldHelpEn) as (keyof typeof fieldHelpEn)[];

for (const lang of ["en", "fr"] as const) {
  const table = lang === "fr" ? fieldHelpFr : fieldHelpEn;
  const extras = lang === "fr" ? fieldHelpExtrasFr : fieldHelpExtrasEn;
  const outDir = join(contentRoot, lang, "field-help");
  mkdirSync(outDir, { recursive: true });
  for (const scope of scopes) {
    const entries = table[scope] as Record<string, string>;
    writeFileSync(
      join(outDir, `${scopeSlug(scope)}.md`),
      renderScope(scope, entries, extras, lang),
      "utf8",
    );
  }
  writeFileSync(join(outDir, "index.md"), writeIndex(lang, scopes), "utf8");
}

console.log(`Wrote ${contentRoot}/{en,fr}/field-help/*.md`);
