// Brute-force PDF stream dump: try several inflate strategies, report stats.
const fs = require('fs');
const zlib = require('zlib');
const file = process.argv[2];
const buf = fs.readFileSync(file);
let ok = 0, fail = 0, textStreams = 0;
const textParts = [];
const re = /stream\r?\n/g;
let m;
while ((m = re.exec(buf.toString('latin1'))) !== null) {
  const start = m.index + m[0].length;
  const end = buf.indexOf('endstream', start, 'latin1');
  if (end < 0) continue;
  let chunk = buf.subarray(start, end);
  // trim trailing EOL
  let cands = [chunk];
  if (chunk.length && chunk[chunk.length-1] === 0x0a) cands.push(chunk.subarray(0, chunk.length-1));
  if (chunk.length>1 && chunk[chunk.length-2]===0x0d && chunk[chunk.length-1]===0x0a) cands.push(chunk.subarray(0, chunk.length-2));
  let data = null;
  for (const c of cands) {
    try { data = zlib.inflateSync(c); break; } catch(e){}
    try { data = zlib.inflateRawSync(c); break; } catch(e){}
  }
  if (data) { ok++; const s = data.toString('latin1'); if (s.includes('BT') || s.includes('Tj') || s.includes('TJ')) { textStreams++; textParts.push(s); } }
  else fail++;
}
fs.writeFileSync(process.argv[3], textParts.join('\n'), 'latin1');
console.error(`ok=${ok} fail=${fail} textStreams=${textStreams}`);
