// -----------------------------------------------------------------------------
// png-decode.mjs - minimal PNG decoder (no interlace / no 16-bit downsample
// support is handled by the caller). Exports decodePng(buffer) -> { width,
// height, rgba: Uint8Array } and analyzePng for diagnostics.
// -----------------------------------------------------------------------------
import { inflateSync } from "node:zlib";

const SIG = [0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a];

export function decodePng(buffer) {
  for (let i = 0; i < 8; i++) {
    if (buffer[i] !== SIG[i]) throw new Error("Not a PNG file");
  }

  let pos = 8;
  let width = 0;
  let height = 0;
  let bitDepth = 8;
  let colorType = 0;
  let interlace = 0;
  const idat = [];
  let palette = null;
  let trns = null;

  while (pos < buffer.length) {
    const length = buffer.readUInt32BE(pos);
    const type = buffer.toString("ascii", pos + 4, pos + 8);
    const data = buffer.subarray(pos + 8, pos + 8 + length);

    if (type === "IHDR") {
      width = data.readUInt32BE(0);
      height = data.readUInt32BE(4);
      bitDepth = data[8];
      colorType = data[9];
      interlace = data[12];
      if (interlace !== 0) throw new Error("Adam7 interlacing not supported");
    } else if (type === "PLTE") {
      palette = data;
    } else if (type === "tRNS") {
      trns = data;
    } else if (type === "IDAT") {
      idat.push(data);
    } else if (type === "IEND") {
      break;
    }

    pos += 12 + length;
  }

  const channels = colorType === 6 ? 4 : colorType === 2 ? 3 : colorType === 4 ? 2 : colorType === 0 ? 1 : 0;
  if (channels === 0) throw new Error(`Unsupported color type ${colorType}`);
  if (bitDepth !== 8 && bitDepth !== 16) throw new Error(`Unsupported bit depth ${bitDepth}`);

  const bpp = channels * (bitDepth === 16 ? 2 : 1);
  const stride = width * bpp;
  const raw = inflateSync(Buffer.concat(idat));
  const rgba = new Uint8Array(width * height * 4);

  const paeth = (a, b, c) => {
    const p = a + b - c;
    const pa = Math.abs(p - a), pb = Math.abs(p - b), pc = Math.abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
  };

  let prev = Buffer.alloc(stride);
  for (let y = 0; y < height; y++) {
    const filter = raw[y * (stride + 1)];
    const line = Buffer.from(raw.subarray(y * (stride + 1) + 1, (y + 1) * (stride + 1)));
    for (let x = 0; x < stride; x++) {
      const left = x >= bpp ? line[x - bpp] : 0;
      const up = prev[x];
      const upLeft = x >= bpp ? prev[x - bpp] : 0;
      let val;
      switch (filter) {
        case 0: val = line[x]; break;
        case 1: val = (line[x] + left) & 0xff; break;
        case 2: val = (line[x] + up) & 0xff; break;
        case 3: val = (line[x] + ((left + up) >> 1)) & 0xff; break;
        case 4: val = (line[x] + paeth(left, up, upLeft)) & 0xff; break;
        default: throw new Error(`Unknown PNG filter ${filter}`);
      }
      line[x] = val;
    }
    prev = line;

    for (let x = 0; x < width; x++) {
      const si = x * bpp;
      const di = (y * width + x) * 4;
      if (bitDepth === 16) {
        // Downsample 16-bit to 8-bit.
        if (channels === 4) {
          rgba[di] = line[si]; rgba[di + 1] = line[si + 2]; rgba[di + 2] = line[si + 4]; rgba[di + 3] = line[si + 6];
        } else if (channels === 3) {
          rgba[di] = line[si]; rgba[di + 1] = line[si + 2]; rgba[di + 2] = line[si + 4]; rgba[di + 3] = 255;
        } else if (channels === 2) {
          const g = line[si]; rgba[di] = g; rgba[di + 1] = g; rgba[di + 2] = g; rgba[di + 3] = line[si + 2];
        } else {
          rgba[di] = line[si]; rgba[di + 1] = line[si]; rgba[di + 2] = line[si]; rgba[di + 3] = 255;
        }
      } else {
        if (channels === 4) {
          rgba[di] = line[si]; rgba[di + 1] = line[si + 1]; rgba[di + 2] = line[si + 2]; rgba[di + 3] = line[si + 3];
        } else if (channels === 3) {
          rgba[di] = line[si]; rgba[di + 1] = line[si + 1]; rgba[di + 2] = line[si + 2]; rgba[di + 3] = 255;
        } else if (channels === 2) {
          const g = line[si]; rgba[di] = g; rgba[di + 1] = g; rgba[di + 2] = g; rgba[di + 3] = line[si + 1];
        } else {
          rgba[di] = line[si]; rgba[di + 1] = line[si]; rgba[di + 2] = line[si]; rgba[di + 3] = 255;
        }
      }
      if (colorType === 3 && palette) {
        const idx = line[si];
        rgba[di] = palette[idx * 3];
        rgba[di + 1] = palette[idx * 3 + 1];
        rgba[di + 2] = palette[idx * 3 + 2];
        rgba[di + 3] = trns && idx < trns.length ? trns[idx] : 255;
      }
    }
  }

  return { width, height, rgba };
}

// Diagnostic summary: dimensions, transparency bounding box, mean color.
export function analyzePng(buffer) {
  const { width, height, rgba } = decodePng(buffer);
  let minX = width, minY = height, maxX = -1, maxY = -1;
  let sumR = 0, sumG = 0, sumB = 0, opaque = 0, semi = 0;
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      const a = rgba[i + 3];
      if (a > 0) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
        sumR += rgba[i]; sumG += rgba[i + 1]; sumB += rgba[i + 2];
        opaque++;
        if (a < 255) semi++;
      }
    }
  }
  return {
    width, height,
    contentBox: [minX, minY, maxX, maxY],
    opaquePixels: opaque,
    semiTransparentPixels: semi,
    meanColor: opaque ? [Math.round(sumR / opaque), Math.round(sumG / opaque), Math.round(sumB / opaque)] : null,
    hasAlphaChannel: true,
  };
}
