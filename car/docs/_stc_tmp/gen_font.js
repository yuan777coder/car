/* gen_font.js — 从 Adafruit GFX 官方 glcdfont.c 生成 SSD1306 6x8 ASCII 字库
 * 用法: node gen_font.js
 * 输出: src/drivers/oled_font.h, src/drivers/oled_font.c
 */
'use strict';
const fs = require('fs');
const path = require('path');

const SRC_URL = 'https://raw.githubusercontent.com/adafruit/Adafruit-GFX-Library/master/glcdfont.c';
const OUT_DIR = path.resolve(__dirname, '..', '..', 'src', 'drivers');

function bail(msg) { console.error('ERROR: ' + msg); process.exit(1); }

async function main() {
  const res = await fetch(SRC_URL);
  if (!res.ok) bail('HTTP ' + res.status);
  const text = await res.text();
  fs.writeFileSync(path.join(__dirname, 'glcdfont_raw.c'), text); // 留档，便于复核

  if (!/Standard ASCII 5x7 font/.test(text)) bail('下载内容不是预期的 glcdfont.c');

  // 取出 font[] 数组主体
  const m = text.match(/font\[\]\s*PROGMEM\s*=\s*\{([\s\S]*?)\n\};/);
  if (!m) bail('未找到 font[] 数组');
  let body = m[1];
  // 去掉 // 行注释，避免注释里的 "25%" 之类干扰
  body = body.split('\n').map(l => l.replace(/\/\/.*$/, '')).join('\n');

  const toks = body.match(/0x[0-9A-Fa-f]{1,2}/g) || [];
  const all = toks.map(t => parseInt(t, 16));
  console.log('原数组字节总数 = ' + all.length + ' (期望 1280 = 256*5)');
  if (all.length !== 1280) bail('字节总数不是 1280，取数区间可能错了');

  // 字符 c 的第 k 字节 = all[c*5 + k]
  function glyph(c) { const o = c * 5; return all.slice(o, o + 5); }

  // ---- 关键取样校验：确认索引没有整体偏移 ----
  const landmarks = [
    [0x20, [0x00,0x00,0x00,0x00,0x00], 'space'],
    [0x21, [0x00,0x00,0x5F,0x00,0x00], '!'],
    [0x30, [0x3E,0x51,0x49,0x45,0x3E], '0'],
    [0x41, [0x7C,0x12,0x11,0x12,0x7C], 'A'],
    [0x61, [0x20,0x54,0x54,0x78,0x40], 'a'],
    [0x7A, [0x44,0x64,0x54,0x4C,0x44], 'z'],
  ];
  let ok = true;
  for (const [c, exp, name] of landmarks) {
    const got = glyph(c);
    const same = got.every((v, i) => v === exp[i]);
    console.log('  landmark 0x' + c.toString(16).toUpperCase() + " '" + name + "' = " +
      got.map(v => '0x' + v.toString(16).toUpperCase().padStart(2, '0')).join(',') +
      (same ? '  [OK]' : '  [MISMATCH 期望 ' + exp.map(v => '0x' + v.toString(16).toUpperCase()).join(',') + ']'));
    if (!same) ok = false;
  }
  if (!ok) bail('关键字符取样不符，取数区间/字节顺序有误');

  // ---- 抽取 0x20~0x7F 共 96 个字符，每字符补 1 个 0x00 作间距 ----
  const rows = [];
  for (let c = 0x20; c <= 0x7F; c++) {
    const g = glyph(c).slice();
    while (g.length < 6) g.push(0x00);
    rows.push({ code: c, bytes: g });
  }
  if (rows.length !== 96) bail('字符数不是 96');

  const hex2 = v => '0x' + v.toString(16).toUpperCase().padStart(2, '0');

  // ---- 写 .c ----
  const lines = [];
  lines.push('#include "oled_font.h"');
  lines.push('');
  lines.push('/* ==========================================================================');
  lines.push(' *  oled_font.c  —  SSD1306 6x8 ASCII 点阵字库');
  lines.push(' *');
  lines.push(' *  数据来源：Adafruit GFX Library 官方 5x7 经典字库 glcdfont.c');
  lines.push(' *  ' + SRC_URL);
  lines.push(' *  取原数组索引 0x20~0x7F（第 32~127 项，共 96 字符），每字符 5 字节，');
  lines.push(' *  末尾补 1 个 0x00 作字符间距，得到 6 字节/字符，共 96*6 = 576 字节。');
  lines.push(' *  原始字节序为“列优先、LSB 在上”，与 SSD1306 页显存一致，未做任何翻转。');
  lines.push(' * ========================================================================== */');
  lines.push('');
  lines.push('const uint8 OledFont6x8[96][6] = {');
  for (const r of rows) {
    const body = r.bytes.map(hex2).join(',');
    let cmt;
    if (r.code === 0x7F) {
      cmt = '/* 0x7F      */';
    } else {
      cmt = "/* 0x" + r.code.toString(16).toUpperCase().padStart(2, '0') +
            " '" + String.fromCharCode(r.code) + "'  */";
    }
    lines.push('    { ' + body + ' }, ' + cmt + (r.code === 0x7F ? '' : ''));
  }
  lines.push('};');
  lines.push('');

  if (!fs.existsSync(OUT_DIR)) fs.mkdirSync(OUT_DIR, { recursive: true });
  fs.writeFileSync(path.join(OUT_DIR, 'oled_font.c'), lines.join('\n'), 'utf8');

  // ---- 写 .h（内容按任务规定，原样） ----
  const hdr = [
    '#ifndef __OLED_FONT_H',
    '#define __OLED_FONT_H',
    '',
    '#include "common.h"',
    '',
    '/* 6x8 ASCII 字库：覆盖 0x20(空格) ~ 0x7F，共 96 个字符，每个字符 6 字节。',
    '   字节排列与 SSD1306 显存一致：每个字节代表同一列的 8 个像素，',
    '   bit0 = 最上面一行，bit7 = 最下面一行（LSB 在上）。',
    '   第 6 个字节通常为 0x00，用作字符间距。 */',
    'extern const uint8 OledFont6x8[96][6];',
    '',
    '#endif',
    ''
  ].join('\n');
  fs.writeFileSync(path.join(OUT_DIR, 'oled_font.h'), hdr, 'utf8');

  console.log('已写出: ' + path.join(OUT_DIR, 'oled_font.h'));
  console.log('已写出: ' + path.join(OUT_DIR, 'oled_font.c'));
  console.log('0x7F 原始 5 字节 = ' + glyph(0x7F).map(hex2).join(','));
}

main().catch(e => bail(e && e.stack || String(e)));
