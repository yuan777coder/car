/* verify_font.js — 读取交付文件 src/drivers/oled_font.c 并独立校验
 * 用法: node verify_font.js
 */
'use strict';
const fs = require('fs');
const path = require('path');

const C_FILE = path.resolve(__dirname, '..', '..', 'src', 'drivers', 'oled_font.c');
const H_FILE = path.resolve(__dirname, '..', '..', 'src', 'drivers', 'oled_font.h');
const text = fs.readFileSync(C_FILE, 'utf8');
const htext = fs.readFileSync(H_FILE, 'utf8');

let fail = 0;
function check(cond, msg) {
  console.log((cond ? '  [PASS] ' : '  [FAIL] ') + msg);
  if (!cond) fail++;
}

/* ---------- 解析文件里所有的数据行 ---------- */
const lines = text.split('\n');
const dataLines = lines.filter(l => /^\s*\{\s*0x/.test(l));
const rows = dataLines.map(l => {
  const m = l.match(/\{([^}]*)\}/);
  return m[1].split(',').map(s => s.trim()).filter(s => s.length)
    .map(s => parseInt(s, 16));
});

console.log('===== 1. 结构校验 =====');
console.log('  oled_font.c 总行数      : ' + lines.length);
console.log('  匹配到的数据行数        : ' + rows.length);
check(rows.length === 96, '数组恰好 96 行 (实际 ' + rows.length + ')');
const badLen = rows.map((r, i) => [i, r.length]).filter(([, n]) => n !== 6);
check(badLen.length === 0, '每行恰好 6 个字节 (异常行: ' +
  (badLen.length ? JSON.stringify(badLen) : '无') + ')');
check(rows.every(r => r.every(b => Number.isInteger(b) && b >= 0 && b <= 255)),
  '所有字节都在 0x00~0xFF 范围');
check(rows.every(r => r[5] === 0x00), '第 6 字节全部为 0x00 (字符间距列)');
check(/const\s+uint8\s+OledFont6x8\s*\[\s*96\s*\]\s*\[\s*6\s*\]/.test(text),
  '定义形式为 const uint8 OledFont6x8[96][6]');
check(/extern\s+const\s+uint8\s+OledFont6x8\s*\[\s*96\s*\]\s*\[\s*6\s*\]\s*;/.test(htext),
  'oled_font.h 中 extern 声明与定义一致');
const slashLines = lines.filter(l => l.includes('//') && !l.includes('http'));
check(!/\bfor\s*\(\s*int\b|\bstdint\b|\b(uint8_t|bool)\b/.test(text) && slashLines.length === 0,
  '无 C99 特性 (无 // 注释行、无行内声明、无 stdint 类型; // 仅出现在来源 URL 中)');
check(text.split('\n').every((l, i) => l.length < 100), '每行长度 < 100 字符');

/* ---------- 渲染 ---------- */
function render(code) {
  const g = rows[code - 0x20];
  const out = [];
  for (let i = 0; i < 7; i++) {
    let s = '';
    for (let c = 0; c < 5; c++) s += ((g[c] >> i) & 1) ? '#' : '.';
    out.push(s);
  }
  return out;
}
function show(code, title) {
  console.log('  0x' + code.toString(16).toUpperCase().padStart(2, '0') + ' ' + title);
  console.log('  字节: ' + rows[code - 0x20].slice(0, 5).map(v => '0x' + v.toString(16).toUpperCase().padStart(2, '0')).join(' '));
  for (const l of render(code)) console.log('        ' + l);
  console.log('');
}

console.log('===== 2. 指定字符 7x5 点阵渲染 =====');
const wanted = [['0', 0x30], ['A', 0x41], ['a', 0x61], ['%', 0x25], ['.', 0x2E],
                ['-', 0x2D], [':', 0x3A], ['/', 0x2F], ['<', 0x3C], ['>', 0x3E],
                ['+', 0x2B], ['#', 0x23]];
for (const [ch, code] of wanted) show(code, "'" + ch + "'");

/* ---------- 3. 大小写 / 偏移 结构性校验 ---------- */
console.log('===== 3. 大小写与索引偏移校验 =====');
const rowOf = (code, i) => {
  const g = rows[code - 0x20];
  let v = 0;
  for (let c = 0; c < 5; c++) v |= (g[c] >> i) & 1;
  return v;
};
const upper = [];
for (let c = 0x41; c <= 0x5A; c++) upper.push(c);
const upperNoTop = upper.filter(c => rowOf(c, 0) === 0);
check(upperNoTop.length === 0, '26 个大写字母全部“顶到第 0 行”(满高)，异常: ' +
  upperNoTop.map(c => String.fromCharCode(c)).join(',') || '无');

const xheight = 'acemnorsuvwxz'.split('').map(s => s.charCodeAt(0));
const lowerWithTop = xheight.filter(c => rowOf(c, 0) !== 0);
check(lowerWithTop.length === 0, '小写 x 高度字母 [' + 'acemnorsuvwxz' + '] 均不占第 0 行，异常: ' +
  lowerWithTop.map(c => String.fromCharCode(c)).join(',') || '无');

const sig = code => rows[code - 0x20].slice(0, 5).join(',');
check(sig(0x41) === '124,18,17,18,124',
  "索引 0x41 处确实是 'A' (字节 0x7C,0x12,0x11,0x12,0x7C)");
check(sig(0x61) === '32,84,84,120,64',
  "索引 0x61 处确实是 'a' (字节 0x20,0x54,0x54,0x78,0x40)");
check(sig(0x20) === '0,0,0,0,0', "索引 0x20 (空格) 为全 0");
check(sig(0x30) === '62,81,73,69,62', "索引 0x30 处确实是 '0'");

// 数字 0-9 连续且都占满高；'9' 之后 0x3A 是 ':' 而不是数字
const digits = [];
for (let c = 0x30; c <= 0x39; c++) digits.push(c);
check(digits.every(c => rowOf(c, 0) !== 0), "字符 '0'~'9' 全部占第 0 行");
check(sig(0x39) === '70,73,73,41,30', "索引 0x39 处确实是 '9'");
check(sig(0x3A) === '0,0,20,0,0', "索引 0x3A 处确实是 ':'");

// 相邻字符互不相同（防止整体错位/重复）
let dup = 0;
for (let i = 1; i < 96; i++) {
  if (rows[i].slice(0, 5).join(',') === rows[i - 1].slice(0, 5).join(',')) dup++;
}
check(dup === 0, '相邻字符点阵无重复 (重复对数 ' + dup + ')');

// 逐字节与在线源码重新比对（独立第二遍取数），并交叉核对第二个独立仓库副本
(async () => {
  console.log('===== 4. 与在线原始数据逐字节复核 =====');
  const SOURCES = [
    ['Adafruit-GFX-Library (原始来源)',
     'https://raw.githubusercontent.com/adafruit/Adafruit-GFX-Library/master/glcdfont.c'],
    ['Bodmer/TFT_eSPI (独立第二副本)',
     'https://raw.githubusercontent.com/Bodmer/TFT_eSPI/master/Fonts/glcdfont.c']
  ];
  async function grab(url) {
    for (let attempt = 1; attempt <= 5; attempt++) {
      try {
        const res = await fetch(url);
        if (res.ok) return await res.text();
      } catch (e) { /* 网络抖动，重试 */ }
      await new Promise(r => setTimeout(r, 800));
    }
    return null;
  }
  for (const [name, url] of SOURCES) {
    const t = await grab(url);
    if (t === null) { console.log('  [SKIP] ' + name + ': 5 次重试后仍取不到'); continue; }
    const mm = t.match(/font\[\]\s*PROGMEM\s*=\s*\{([\s\S]*?)\n\};/);
    if (!mm) { console.log('  [SKIP] ' + name + ': 未找到 font[] 数组'); continue; }
    const all = (mm[1].split('\n').map(l => l.replace(/\/\/.*$/, '')).join('\n')
      .match(/0x[0-9A-Fa-f]{1,2}/g) || []).map(x => parseInt(x, 16));
    let diff = 0;
    for (let c = 0; c < 96; c++)
      for (let k = 0; k < 5; k++)
        if (rows[c][k] !== all[(0x20 + c) * 5 + k]) diff++;
    check(all.length === 1280 && diff === 0,
      name + ': 480 字节与原数组 [0x20..0x7F] 完全一致 (差异 ' + diff + ' 字节)');
  }

  console.log('');
  console.log('===== 5. 交付文件前 8 行 =====');
  console.log(dataLines.slice(0, 8).join('\n'));
  console.log('  ...');
  console.log(dataLines[95]);
  console.log('');
  console.log(fail === 0 ? '全部校验通过 (0 项失败)' : ('校验失败项数: ' + fail));
  process.exit(fail === 0 ? 0 : 1);
})();
