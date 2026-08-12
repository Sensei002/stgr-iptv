// -----------------------------------------------------------------------------
// generate_icons.mjs - builds STGR IpTV icon assets from the real STEiGER Dojo
// logo (assets/branding/source-logo.png). The source PNG has a transparent
// background; the artwork is cropped to its content box, padded, centered on a
// square canvas and area-resampled to every icon size.
//
//   node tools/generate_icons.mjs [--source path/to/logo.png]
//
// Pure Node (stdlib only). Reuses tools/png-encode.mjs for PNG/ICO data.
// Writes:
//   assets/icons/app-icon.ico      (16..256, PNG-compressed ICO entries)
//   assets/icons/app-icon-256.png  (256x256)
//   assets/branding/logo.png       (512x512, for docs/README)
// -----------------------------------------------------------------------------

import { mkdirSync, writeFileSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { decodePng } from "./png-decode.mjs";
import { encodePng, resampleArea } from "./png-encode.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");

const argSource = process.argv.indexOf("--source");
const sourcePath = argSource >= 0
    ? process.argv[argSource + 1]
    : join(root, "assets", "branding", "source-logo.png");

// Margin around the artwork, as a fraction of the largest content dimension.
const MARGIN = 0.10;

// ---------------------------------------------------------------------------
// Source -> square canvas: crop to the content box and center with margin.
// ---------------------------------------------------------------------------
function normalizeSource({ width, height, rgba }) {
    let minX = width, minY = height, maxX = -1, maxY = -1;
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const i = (y * width + x) * 4;
            if (rgba[i + 3] > 8) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    if (maxX < 0) throw new Error("Source PNG contains no visible content");

    const cw = maxX - minX + 1;
    const ch = maxY - minY + 1;
    const side = Math.ceil(Math.max(cw, ch) * (1 + MARGIN * 2));
    const offX = Math.round((side - cw) / 2) - minX;
    const offY = Math.round((side - ch) / 2) - minY;

    const out = new Uint8Array(side * side * 4);
    for (let y = minY; y <= maxY; y++) {
        const srcStart = (y * width + minX) * 4;
        const dstStart = ((y + offY) * side + minX + offX) * 4;
        out.set(rgba.subarray(srcStart, srcStart + cw * 4), dstStart);
    }
    return { side, rgba: out };
}

// ---------------------------------------------------------------------------
// ICO packing (PNG-compressed entries, supported since Windows Vista)
// ---------------------------------------------------------------------------
function packIco(entries) {
    const header = Buffer.alloc(6);
    header.writeUInt16LE(0, 0);
    header.writeUInt16LE(1, 2);
    header.writeUInt16LE(entries.length, 4);

    const dirSize = 16 * entries.length;
    let offset = 6 + dirSize;
    const buffers = [header];
    const dirs = [];

    for (const e of entries) {
        const dir = Buffer.alloc(16);
        dir[0] = e.size >= 256 ? 0 : e.size;
        dir[1] = e.size >= 256 ? 0 : e.size;
        dir.writeUInt16LE(1, 4); // planes
        dir.writeUInt16LE(32, 6); // bpp
        dir.writeUInt32LE(e.png.length, 8);
        dir.writeUInt32LE(offset, 12);
        dirs.push(dir);
        buffers.push(dir);
        offset += e.png.length;
    }
    for (const e of entries) buffers.push(e.png);
    return Buffer.concat(buffers);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
let source;
try {
    source = decodePng(readFileSync(sourcePath));
} catch (err) {
    console.error(`Cannot load source logo "${sourcePath}": ${err.message}`);
    console.error("Pass --source path/to/logo.png (transparent-background PNG preferred).");
    process.exit(1);
}
console.log(`Source: ${source.width}x${source.height} @ ${sourcePath}`);

const { side, rgba } = normalizeSource(source);
console.log(`Normalized to ${side}x${side} (content cropped + ${Math.round(MARGIN * 100)}% margin)`);

const sizes = [16, 24, 32, 48, 64, 128, 256];
const assetsDir = join(root, "assets", "icons");
const brandingDir = join(root, "assets", "branding");
mkdirSync(assetsDir, { recursive: true });
mkdirSync(brandingDir, { recursive: true });

const icoEntries = [];
for (const size of sizes) {
    const png = encodePng(size, size, resampleArea(rgba, side, side, size, size));
    icoEntries.push({ size, png });
    if (size === 256) {
        writeFileSync(join(assetsDir, "app-icon-256.png"), png);
        writeFileSync(join(brandingDir, "logo.png"), encodePng(512, 512, resampleArea(rgba, side, side, 512, 512)));
    }
    console.log(`  icon ${size}x${size} -> ${png.length} bytes`);
}

writeFileSync(join(assetsDir, "app-icon.ico"), packIco(icoEntries));
console.log("Wrote assets/icons/app-icon.ico, assets/icons/app-icon-256.png, assets/branding/logo.png");
