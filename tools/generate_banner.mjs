// -----------------------------------------------------------------------------
// generate_banner.mjs - builds the dark-background STGR IpTV README banner:
// the STEiGER Dojo logo on a near-black panel with the same subtle dojo
// decoration as the Home page (seigaiha wave field + enso rings).
//
//   node tools/generate_banner.mjs
//
// Writes:
//   assets/branding/logo-dark.svg  (GitHub-native, crisp at any size)
//   assets/branding/logo-dark.png  (1600x400 fallback for other viewers)
// -----------------------------------------------------------------------------

import { readFileSync, writeFileSync, mkdirSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { decodePng } from "./png-decode.mjs";
import { encodePng, resampleArea } from "./png-encode.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const brandingDir = join(root, "assets", "branding");

// Banner geometry (logical units).
const W = 1600;
const H = 400;
const CX = W / 2;
const CY = H / 2;
const LOGO_H = 360; // artwork height on the banner

// Palette (matches the app theme).
const BG_TOP = [20, 20, 26];     // #14141a
const BG_BOTTOM = [13, 13, 16];  // #0d0d10
const CRIMSON = [226, 52, 63];   // #e2343f
const GOLD = [229, 181, 94];     // #e5b55e

// ---------------------------------------------------------------------------
// Source logo: decode + content bounding box
// ---------------------------------------------------------------------------
const logo = decodePng(readFileSync(join(brandingDir, "source-logo.png")));
const { width: LW, height: LH, rgba: LRGBA } = logo;

let minX = LW, minY = LH, maxX = -1, maxY = -1;
for (let y = 0; y < LH; y++) {
    for (let x = 0; x < LW; x++) {
        if (LRGBA[(y * LW + x) * 4 + 3] > 8) {
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
        }
    }
}
const cropW = maxX - minX + 1;
const cropH = maxY - minY + 1;

const scale = LOGO_H / cropH;
const logoW = Math.round(cropW * scale);
const logoX = Math.round(CX - logoW / 2);
const logoY = Math.round(CY - LOGO_H / 2);

// ---------------------------------------------------------------------------
// Shared decoration parameters (identical in SVG and PNG)
// ---------------------------------------------------------------------------
const WAVE_D = 72;              // seigaiha scale (diameter)
const WAVE_R = WAVE_D / 2;      // row spacing
const WAVE_OPACITY = 0.06;
const ENSO = { cx: CX, cy: CY, r: 235, opacity: 0.10, width: 3 };
const ENSO_GOLD = { cx: CX + 12, cy: CY + 6, r: 252, opacity: 0.05, width: 1.5 };

function waveCenters() {
    const arcs = [];
    const rows = Math.ceil(H / WAVE_R) + 1;
    for (let row = 0; row < rows; row++) {
        const cy = row * WAVE_R;
        const off = row % 2 === 0 ? 0 : WAVE_R;
        for (let cx = off - WAVE_R; cx < W + WAVE_R; cx += WAVE_D)
            arcs.push({ cx, cy });
    }
    return arcs;
}

// ---------------------------------------------------------------------------
// SVG banner
// ---------------------------------------------------------------------------
function buildSvg(paths) {
    const arcs = waveCenters();
    const arcPaths = arcs.map(({ cx, cy }) =>
        `<path d="M ${(cx - WAVE_R).toFixed(1)},${cy.toFixed(1)} A ${WAVE_R},${WAVE_R} 0 0 1 ${(cx + WAVE_R).toFixed(1)},${cy.toFixed(1)}"/>`
    ).join("\n    ");

    const gx = CX - (minX + cropW / 2) * scale;
    const gy = CY - (minY + cropH / 2) * scale;

    return `<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     width="${W}" height="${H}" viewBox="0 0 ${W} ${H}"
     role="img" aria-labelledby="bannerTitle bannerDesc">
  <title id="bannerTitle">STGR IpTV by STEiGER Dojo</title>
  <desc id="bannerDesc">STEiGER Dojo logo on a dark banner with a subtle dojo pattern.</desc>
  <defs>
    <linearGradient id="bannerBg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#14141a"/>
      <stop offset="1" stop-color="#0d0d10"/>
    </linearGradient>
  </defs>
  <rect width="${W}" height="${H}" fill="url(#bannerBg)"/>
  <g fill="none" stroke="#e2343f" stroke-opacity="${WAVE_OPACITY}" stroke-width="2">
    ${arcPaths}
  </g>
  <circle cx="${ENSO.cx}" cy="${ENSO.cy}" r="${ENSO.r}"
          fill="none" stroke="#e2343f" stroke-opacity="${ENSO.opacity}" stroke-width="${ENSO.width}"/>
  <circle cx="${ENSO_GOLD.cx}" cy="${ENSO_GOLD.cy}" r="${ENSO_GOLD.r}"
          fill="none" stroke="#e5b55e" stroke-opacity="${ENSO_GOLD.opacity}" stroke-width="${ENSO_GOLD.width}"/>
  <g transform="translate(${gx.toFixed(2)} ${gy.toFixed(2)}) scale(${scale.toFixed(5)})">
    ${paths.join("\n    ")}
  </g>
</svg>
`;
}

// ---------------------------------------------------------------------------
// PNG banner (rasterized with the same geometry)
// ---------------------------------------------------------------------------
function blend(dst, i, [r, g, b], a) {
    // dst[i..i+3] is an RGBA pixel; paint color with alpha (0..255) over it.
    const dr = dst[i], dg = dst[i + 1], db = dst[i + 2], da = dst[i + 3];
    const oa = a + da * (255 - a) / 255;
    if (oa <= 0) return;
    dst[i] = Math.round((r * a + dr * da * (255 - a) / 255) / oa);
    dst[i + 1] = Math.round((g * a + dg * da * (255 - a) / 255) / oa);
    dst[i + 2] = Math.round((b * a + db * da * (255 - a) / 255) / oa);
    dst[i + 3] = Math.round(oa);
}

function buildPng() {
    const px = new Uint8Array(W * H * 4);

    // Background gradient.
    for (let y = 0; y < H; y++) {
        const t = y / (H - 1);
        const r = Math.round(BG_TOP[0] + (BG_BOTTOM[0] - BG_TOP[0]) * t);
        const g = Math.round(BG_TOP[1] + (BG_BOTTOM[1] - BG_TOP[1]) * t);
        const b = Math.round(BG_TOP[2] + (BG_BOTTOM[2] - BG_TOP[2]) * t);
        for (let x = 0; x < W; x++) {
            const i = (y * W + x) * 4;
            px[i] = r; px[i + 1] = g; px[i + 2] = b; px[i + 3] = 255;
        }
    }

    // Seigaiha wave arcs: only pixels inside each arc's bounding box.
    const waveAlpha = Math.round(255 * WAVE_OPACITY);
    const strokeHalf = 1.0; // pen width 2 -> half-width 1
    for (const { cx, cy } of waveCenters()) {
        const x0 = Math.max(0, Math.floor(cx - WAVE_R - strokeHalf));
        const x1 = Math.min(W - 1, Math.ceil(cx + WAVE_R + strokeHalf));
        const y0 = Math.max(0, Math.floor(cy - strokeHalf));
        const y1 = Math.min(H - 1, Math.ceil(cy + WAVE_R + strokeHalf));
        for (let y = y0; y <= y1; y++) {
            for (let x = x0; x <= x1; x++) {
                const dist = Math.hypot(x - cx, y - cy);
                if (Math.abs(dist - WAVE_R) <= strokeHalf)
                    blend(px, (y * W + x) * 4, CRIMSON, waveAlpha);
            }
        }
    }

    // Enso rings.
    const ensoAlpha = Math.round(255 * ENSO.opacity);
    const ensoHalf = ENSO.width / 2;
    for (let y = 0; y < H; y++) {
        for (let x = 0; x < W; x++) {
            const i = (y * W + x) * 4;
            const dC = Math.abs(Math.hypot(x - ENSO.cx, y - ENSO.cy) - ENSO.r);
            if (dC <= ensoHalf) blend(px, i, CRIMSON, ensoAlpha);
            const dG = Math.abs(Math.hypot(x - ENSO_GOLD.cx, y - ENSO_GOLD.cy) - ENSO_GOLD.r);
            if (dG <= ENSO_GOLD.width / 2) blend(px, i, GOLD, Math.round(255 * ENSO_GOLD.opacity));
        }
    }

    // Logo composited over the backdrop.
    const logoImg = resampleArea(cropBuffer(), cropW, cropH, logoW, LOGO_H);
    for (let y = 0; y < LOGO_H; y++) {
        for (let x = 0; x < logoW; x++) {
            const s = (y * logoW + x) * 4;
            const a = logoImg[s + 3];
            if (a <= 0) continue;
            blend(px, ((logoY + y) * W + (logoX + x)) * 4,
                  [logoImg[s], logoImg[s + 1], logoImg[s + 2]], a);
        }
    }

    return encodePng(W, H, px);
}

// Cropped copy of the source logo (content bounding box only).
function cropBuffer() {
    const out = new Uint8Array(cropW * cropH * 4);
    for (let y = minY; y <= maxY; y++) {
        const srcStart = (y * LW + minX) * 4;
        out.set(LRGBA.subarray(srcStart, srcStart + cropW * 4), (y - minY) * cropW * 4);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
mkdirSync(brandingDir, { recursive: true });

// Extract the inline <path> elements from the master logo SVG (they share the
// 2048x1728 viewBox of the source PNG, so one transform scales both).
const logoSvg = readFileSync(join(brandingDir, "logo.svg"), "utf8");
const paths = logoSvg.match(/<path[^>]*\/>/g);
if (!paths || paths.length !== 8)
    throw new Error(`Expected 8 logo paths, found ${paths ? paths.length : 0}`);

const svg = buildSvg(paths);
writeFileSync(join(brandingDir, "logo-dark.svg"), svg);
console.log(`Wrote assets/branding/logo-dark.svg (${svg.length} bytes)`);

const png = buildPng();
writeFileSync(join(brandingDir, "logo-dark.png"), png);
console.log(`Wrote assets/branding/logo-dark.png (${W}x${H}, ${png.length} bytes)`);
