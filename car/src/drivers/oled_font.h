#ifndef __OLED_FONT_H
#define __OLED_FONT_H

#include "common.h"

/* 6x8 ASCII 字库：覆盖 0x20(空格) ~ 0x7F，共 96 个字符，每个字符 6 字节。
   字节排列与 SSD1306 显存一致：每个字节代表同一列的 8 个像素，
   bit0 = 最上面一行，bit7 = 最下面一行（LSB 在上）。
   第 6 个字节通常为 0x00，用作字符间距。 */
extern const uint8 OledFont6x8[96][6];

#endif
