# Balansun — SVG wordmark specification (draft)

**Status:** Draft v0.1 — review before copying into `ESP32-router/assets/brand/`.

**Draft files:** [`draft/`](draft/) (`balansun-logo-light.svg`, `balansun-logo-dark.svg`, `balansun-favicon.svg`, icon variants).

---

## 1. Brand name

| Rule | Value |
|------|--------|
| Display name | **Balansun** (one word, capital B) |
| Wordmark split | `Balan` + `sun` |
| Slug / filenames | `balansun` (lowercase, hyphen in asset names: `balansun-logo-light.svg`) |
| Do not use | “Balansun Zero”, “Balan-Sun”, “BALANSUN” in prose UI |

---

## 2. Logo lockup (horizontal)

```
[ Mark ]  Balansun
          ^^^^  ^^^
          body  accent
```

- **Mark** (left): sun disc + 8 rays + horizontal balance bar — same *layout* as legacy Balansun mark, new colors.
- **Wordmark** (right): single line, baseline-aligned with mark vertical center.

### Canvas

| Property | Value |
|----------|--------|
| `viewBox` | `0 0 400 100` |
| Mark center | `(40, 50)` |
| Text origin | `x="90"` `y="65"` (Inter caps baseline) |
| Min clear space | ½ mark radius (9 px) on all sides of full lockup |

Drop-in compatible with existing Balansun logo slots (README width 420, app bar `appbar__logo`).

---

## 3. Color palette

### Light theme (`balansun-logo-light.svg`)

| Token | Hex | Use |
|-------|-----|-----|
| `slate-800` | `#1E293B` | Wordmark `Balan`, favicon tile (optional) |
| `teal-600` | `#0D9488` | Wordmark `sun`, balance bar on mark |
| `amber-500` | `#F59E0B` | Sun disc gradient start, ray strokes |
| `amber-600` | `#D97706` | Sun disc gradient end |

### Dark theme (`balansun-logo-dark.svg`)

| Token | Hex | Use |
|-------|-----|-----|
| `white` | `#FFFFFF` | Wordmark `Balan`, balance bar on mark |
| `teal-400` | `#2DD4BF` | Wordmark `sun` (readable on dark UI) |
| `amber-300` | `#FCD34D` | Ray strokes |
| `amber-500` | `#F59E0B` | Sun disc gradient end |

**Color roles**

- **Teal** = brand + balance (meter equilibrium, “sun” syllable).
- **Amber** = solar energy (disc + rays only — not the whole UI).
- **Slate / white** = neutral wordmark body.

---

## 4. Mark geometry (icon)

All coordinates in logo `viewBox` units.

| Element | Spec |
|---------|------|
| Sun disc | `circle` `cx="40"` `cy="50"` `r="18"` |
| Disc fill | `linearGradient` id `balansunSolarGrad` (amber stops per theme) |
| Rays | 8 segments, `stroke-width="3"` `stroke-linecap="round"` — cardinal + diagonal, inner gap 10 px from center, outer extent as Balansun reference |
| Balance bar | `rect` `x="30"` `y="48"` `width="20"` `height="4"` `rx="2"` — **teal** (light) / **white** (dark) |

Semantic: disc + rays = surplus sun; bar = incomer held level.

### Favicon / app icon (`balansun-favicon.svg`)

| Property | Value |
|----------|--------|
| `viewBox` | `0 0 100 100` |
| Background | `#0D9488` rounded rect `rx="20"` (teal tile — distinct from Balansun slate tile) |
| Mark scale | Disc `cx="50"` `cy="50"` `r="25"`; bar `x="40"` `y="48"` `width="20"` `height="4"` |
| Bar fill | `#FFFFFF` on teal background |

### Square icon (`balansun-icon-light.svg` / `-dark.svg`)

- `viewBox="0 0 100 100"`
- Mark only (no wordmark), transparent background — for HACS PNG generation.
- Light: amber disc, teal bar, amber rays.
- Dark: lighter amber rays, white bar, amber gradient disc (for dark HA UI).

---

## 5. Typography

| Property | Value |
|----------|--------|
| Font family | `Inter, sans-serif` |
| Weight | `700` |
| Size | `42` (logo lockup) |
| `Balan` | base fill (slate or white) |
| `sun` | `<tspan>` accent fill (teal) |

**Implementation:** SVG `<text>` with `<tspan>` — same pattern as Balansun (no outlined paths in v1). If Inter is unavailable, system sans fallback is acceptable for README; firmware SPA bundles Inter via existing stack.

**Optional v2:** convert wordmark to paths for pixel-perfect export without font dependency.

---

## 6. SVG authoring rules

1. `role="img"` and `aria-label="Balansun"` on root `<svg>`.
2. `<title>Balansun</title>` for tooltip / a11y.
3. Unique gradient ids per file (`balansunSolarGrad`, `balansunSolarGradDark`, `balansunFavGrad`) — avoid collisions when inlining multiple SVGs.
4. No embedded raster images.
5. No `foreignObject`.
6. Prefer attributes over inline CSS except gradient stops.
7. Files are **UTF-8**, LF line endings.

---

## 7. File inventory (canonical paths after import)

| File | Purpose |
|------|---------|
| `assets/brand/balansun-logo-light.svg` | README, docs light, SPA light theme |
| `assets/brand/balansun-logo-dark.svg` | README dark, docs dark, SPA dark theme |
| `assets/brand/balansun-favicon.svg` | Browser tab, PWA source |
| `assets/brand/balansun-icon-light.svg` | HACS / HA brand PNG source |
| `assets/brand/balansun-icon-dark.svg` | HACS dark icon PNG source |
| `assets/brand/README.md` | Pointer to this spec |

---

## 8. Consumption (post-import)

| Consumer | Mechanism |
|----------|-----------|
| ESP32 SPA | `web/src/brand/brandAssets.ts` imports `?raw` from `assets/brand/` |
| Docs build | `assets/brand/` copied to `dist/assets/brand/` on build |
| HACS | `npm run prepare:brand` from icon SVGs → `custom_components/balansun/brand/*.png` |
| PWA | `generate-pwa-icons.mjs` renders PNG from `balansun-favicon.svg` |

---

## 9. Acceptance checklist

- [ ] Lockup readable at 120 px width (app bar).
- [ ] `sun` accent distinguishable from `Balan` at 1× and 2× DPI.
- [ ] Dark logo on `#1E293B` UI background passes WCAG contrast for `Balan` (white) and `sun` (teal-400).
- [ ] Favicon recognizable at 16×16 (simplified bar + disc reads as “sun with line”).
- [ ] No Balansun / Balansun strings in SVG source.
- [ ] Gradient ids unique when light + dark inlined in same HTML page.

---

## 10. Open decisions

| # | Question | Draft choice |
|---|----------|--------------|
| 1 | `sun` accent: teal vs amber? | **Teal** (`#0D9488` / `#2DD4BF`) — brand-owned |
| 2 | Favicon tile: teal vs slate? | **Teal** `#0D9488` — stronger brand recall |
| 3 | Balance bar on light mark: teal vs slate? | **Teal** — ties mark to wordmark accent |
| 4 | Outlined paths vs live text? | **Live text** v1 (Balansun parity) |

Confirm or adjust before mass rename import.
