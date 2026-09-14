// Scan a binary for zlib/gzip streams and inflate them; report hits for keywords.
const fs = require('fs');
const zlib = require('zlib');
const file = process.argv[2];
const keywords = (process.argv[3] || 'STC32G_Init').split(',');
const buf = fs.readFileSync(file);
const hits = [];
let found = 0;
for (let i = 0; i < buf.length - 2; i++) {
  const b0 = buf[i], b1 = buf[i+1];
  if (b0 !== 0x78) continue;              // zlib CMF
  if ((b0 * 256 + b1) % 31 !== 0) continue; // header check
  let out;
  try { out = zlib.inflateSync(buf.subarray(i)); } catch (e) { continue; }
  if (out.length < 200) continue;
  found++;
  const s = out.toString('latin1');
  for (const k of keywords) {
    const idx = s.indexOf(k);
    if (idx >= 0) {
      hits.push({ off: i, len: out.length, kw: k });
      fs.appendFileSync(process.argv[4], `\n===== zlib@${i} len=${out.length} kw=${k} =====\n` + s.substring(Math.max(0, idx - 3000), idx + 12000));
      break;
    }
  }
}
console.error(`inflated=${found} hits=${hits.length}`);
console.error(JSON.stringify(hits.slice(0, 40)));
