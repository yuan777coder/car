// Minimal PDF stream text dumper (Latin + CIDs), no external deps.
const fs = require('fs');
const zlib = require('zlib');

const file = process.argv[2];
const buf = fs.readFileSync(file);
const raw = buf.toString('latin1');

// collect all stream ... endstream chunks
const out = [];
let idx = 0;
const re = /stream\r?\n/g;
let m;
while ((m = re.exec(raw)) !== null) {
  const start = m.index + m[0].length;
  const end = raw.indexOf('endstream', start);
  if (end < 0) continue;
  const chunk = buf.subarray(start, end);
  let data;
  try {
    data = zlib.inflateSync(chunk);
  } catch (e) {
    try { data = zlib.inflateRawSync(chunk); } catch (e2) { continue; }
  }
  out.push(data.toString('latin1'));
}
fs.writeFileSync(process.argv[3] || 'pdf_streams.txt', out.join('\n%%%STREAM%%%\n'), 'latin1');
console.error('streams: ' + out.length + ' -> ' + (process.argv[3] || 'pdf_streams.txt'));
