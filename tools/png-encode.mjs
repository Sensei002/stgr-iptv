// -----------------------------------------------------------------------------
// png-encode.mjs - shared PNG encoder + area resampler for the asset
// generators (icons and README banner). Pure Node (stdlib only).
// -----------------------------------------------------------------------------
import { deflateSync } from "node:zlib";

const crcTable = (() => {
    const t = new Int32Array(256);
    for (let n = 0; n < 256; n++) {
        let c = n;
        for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
        t[n] = c;
    }
    return t;
})();

const crc32 = (buf) => {
    let c = 0xffffffff;
    for (let i = 0; i < buf.length; i++) c = crcTable[(c ^ buf[i]) & 0xff] ^ (c >>> 8);
    return (c ^ 0xffffffff) >>> 0;
};

const chunk = (type, data) => {
    const len = Buffer.alloc(4);
    len.writeUInt32BE(data.length, 0);
    const typeBuf = Buffer.from(type, "ascii");
    const crcBuf = Buffer.alloc(4);
    crcBuf.writeUInt32BE(crc32(Buffer.concat([typeBuf, data])), 0);
    return Buffer.concat([len, typeBuf, data, crcBuf]);
};

// Encodes an RGBA Uint8Array (width*height*4) as a PNG buffer.
export function encodePng(width, height, rgba) {
    const ihdr = Buffer.alloc(13);
    ihdr.writeUInt32BE(width, 0);
    ihdr.writeUInt32BE(height, 4);
    ihdr[8] = 8;  // bit depth
    ihdr[9] = 6;  // color type RGBA
    // 10..12 compression/filter/interlace = 0

    const stride = width * 4;
    const raw = Buffer.alloc((stride + 1) * height);
    for (let y = 0; y < height; y++) {
        raw[y * (stride + 1)] = 0; // filter: none
        raw.set(rgba.subarray(y * stride, (y + 1) * stride), y * (stride + 1) + 1);
    }

    return Buffer.concat([
        Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
        chunk("IHDR", ihdr),
        chunk("IDAT", deflateSync(raw, { level: 9 })),
        chunk("IEND", Buffer.alloc(0)),
    ]);
}

// Area resampling (premultiplied alpha, fractional source-pixel coverage).
// Destination dimensions are floored so float sizes can never misalign rows.
export function resampleArea(src, srcW, srcH, dstW, dstH) {
    const dw = Math.floor(dstW);
    const dh = Math.floor(dstH);
    if (dw <= 0 || dh <= 0 || srcW <= 0 || srcH <= 0)
        throw new Error("resampleArea: dimensions must be positive");

    const out = new Uint8Array(dw * dh * 4);
    const sdx = srcW / dw;
    const sdy = srcH / dh;

    for (let dy = 0; dy < dh; dy++) {
        const y0 = dy * sdy;
        const y1 = (dy + 1) * sdy;
        const sy0 = Math.floor(y0);
        const sy1 = Math.min(srcH, Math.ceil(y1));

        for (let dx = 0; dx < dw; dx++) {
            const x0 = dx * sdx;
            const x1 = (dx + 1) * sdx;
            const sx0 = Math.floor(x0);
            const sx1 = Math.min(srcW, Math.ceil(x1));

            let accR = 0, accG = 0, accB = 0, accA = 0, totalW = 0;
            for (let sy = sy0; sy < sy1; sy++) {
                const overlapY = Math.min(sy + 1, y1) - Math.max(sy, y0);
                for (let sx = sx0; sx < sx1; sx++) {
                    const overlapX = Math.min(sx + 1, x1) - Math.max(sx, x0);
                    const w = overlapX * overlapY;
                    const i = (sy * srcW + sx) * 4;
                    const a = src[i + 3];
                    if (a > 0) {
                        accR += src[i] * a * w;
                        accG += src[i + 1] * a * w;
                        accB += src[i + 2] * a * w;
                        accA += a * w;
                    }
                    totalW += w;
                }
            }

            const o = (dy * dw + dx) * 4;
            if (accA > 0) {
                out[o] = Math.round(accR / accA);
                out[o + 1] = Math.round(accG / accA);
                out[o + 2] = Math.round(accB / accA);
                out[o + 3] = Math.round(accA / totalW);
            } else {
                out[o + 3] = 0;
            }
        }
    }
    return out;
}
