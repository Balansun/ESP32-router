#!/usr/bin/env node
/**
 * Copy canonical brand SVGs from assets/brand/ to web/public/brand/.
 * Run: node scripts/sync-brand-assets.mjs
 * Invoked automatically before web build (see web/package.json).
 */
import { copyFileSync, mkdirSync, readdirSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const srcDir = join(root, "assets", "brand");
const destDir = join(root, "web", "public", "brand");

mkdirSync(destDir, { recursive: true });

const files = readdirSync(srcDir).filter((f) => f.startsWith("balansun-") && f.endsWith(".svg"));
if (files.length === 0) {
  console.error("sync-brand-assets: no balansun-*.svg in assets/brand/");
  process.exit(1);
}

for (const name of files) {
  copyFileSync(join(srcDir, name), join(destDir, name));
  console.log(`sync-brand-assets: ${name}`);
}
