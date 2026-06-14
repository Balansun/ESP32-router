import { readFileSync, existsSync } from "node:fs";
import { join } from "node:path";
import { describe, expect, it } from "vitest";
import { fieldHelpEn } from "../src/i18n/locales/fieldHelp.en";
import { fieldHelpFr } from "../src/i18n/locales/fieldHelp.fr";
import { fieldHelpExtrasEn } from "../scripts/field-help-extras/en";
import { fieldHelpExtrasFr } from "../scripts/field-help-extras/fr";
import { collectExtraKeys } from "../scripts/field-help-extras/build-extras";

const repoRoot = join(import.meta.dirname, "../..");
const websiteContent = join(repoRoot, "..", "Balansun-Website", "content");
const localFieldHelp = join(repoRoot, "web", ".field-help-docs");
const docsRoot =
  process.env.FIELD_HELP_DOCS_ROOT ??
  (existsSync(websiteContent) ? websiteContent : localFieldHelp);

function scopeSlug(scope: string): string {
  if (scope === "sourceWizard") return "source-wizard";
  if (scope === "httpAuth") return "http-auth";
  return scope;
}

function collectFieldHelpKeys(table: typeof fieldHelpEn): string[] {
  return Object.entries(table).flatMap(([scope, entries]) =>
    Object.keys(entries as Record<string, string>).map((key) => `${scope}/${key}`),
  );
}

function hasHeading(md: string, key: string): boolean {
  return new RegExp(`^## ${key.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")}\\s*$`, "m").test(md);
}

function sectionForKey(md: string, key: string): string {
  const escaped = key.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const re = new RegExp(`^## ${escaped}\\s*\\n([\\s\\S]*?)\\n---\\n(?:\\n|$)`, "m");
  return md.match(re)?.[1] ?? "";
}

function hasExampleBlock(md: string, key: string, lang: "en" | "fr"): boolean {
  const section = sectionForKey(md, key);
  const marker = lang === "fr" ? "### Exemple" : "### Example";
  return section.includes(marker);
}

describe("field-help markdown parity", () => {
  const enKeys = collectFieldHelpKeys(fieldHelpEn).sort();
  const frKeys = collectFieldHelpKeys(fieldHelpFr).sort();
  const extrasEnKeys = collectExtraKeys(fieldHelpExtrasEn).sort();
  const extrasFrKeys = collectExtraKeys(fieldHelpExtrasFr).sort();

  it("EN and FR locale keys match", () => {
    expect(frKeys).toEqual(enKeys);
  });

  it("extras EN keys match fieldHelp keys", () => {
    expect(extrasEnKeys).toEqual(enKeys);
  });

  it("extras FR keys match fieldHelp keys", () => {
    expect(extrasFrKeys).toEqual(frKeys);
  });

  it.each(enKeys)("field-help docs en+fr contain ## heading for %s", (compound) => {
    const [scope, key] = compound.split("/");
    const slug = scopeSlug(scope);
    const enPath = join(docsRoot, "en", "field-help", `${slug}.md`);
    const frPath = join(docsRoot, "fr", "field-help", `${slug}.md`);
    expect(existsSync(enPath)).toBe(true);
    expect(existsSync(frPath)).toBe(true);
    const enMd = readFileSync(enPath, "utf8");
    const frMd = readFileSync(frPath, "utf8");
    expect(hasHeading(enMd, key)).toBe(true);
    expect(hasHeading(frMd, key)).toBe(true);
  });

  it.each(enKeys)("generated field-help en+fr include example block for %s", (compound) => {
    const [scope, key] = compound.split("/");
    const slug = scopeSlug(scope);
    const enMd = readFileSync(join(docsRoot, "en", "field-help", `${slug}.md`), "utf8");
    const frMd = readFileSync(join(docsRoot, "fr", "field-help", `${slug}.md`), "utf8");
    expect(hasExampleBlock(enMd, key, "en")).toBe(true);
    expect(hasExampleBlock(frMd, key, "fr")).toBe(true);
  });
});
