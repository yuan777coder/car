#ifndef __OLED_H
#define __OLED_H

#include "common.h"

/* ==========================================================================
 *  oled.h  —  SSD1306 128x64 OLED（I2C）驱动
 *
 *  采用"显存 + 整屏刷新"的方式：所有绘制先写进 1KB 的显存，
 *  再一次性刷到屏上。好处是刷新过程中不会出现半屏撕裂/闪烁，
 *  代价是 1KB RAM（STC32G144K246 完全够用）。
 *
 *  坐标系：x = 0~127（像素列），page = 0~7（每 page 高 8 像素）。
 *  字库：默认 6x8（见 oled_font.c），大号字用 2 倍放大实现 12x16。
 * ========================================================================== */

#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      (OLED_HEIGHT / 8)

void Oled_Init(void);

/* 把显存内容刷到屏幕（I2C 传输约 1KB，115200 下无需担心，
   I2C 100kHz 下约 20ms，所以刷新要节流，见 app/ui.c） */
void Oled_Refresh(void);

void Oled_Clear(void);
void Oled_Fill(uint8 pattern);

/* --- 6x8 小字 --- */
void Oled_ShowChar(uint8 x, uint8 page, char c);
void Oled_ShowString(uint8 x, uint8 page, const char *s);

/* --- 12x16 大字（6x8 两倍放大，用于显示关键数值） --- */
void Oled_ShowCharBig(uint8 x, uint8 page, char c);
void Oled_ShowStringBig(uint8 x, uint8 page, const char *s);

/* --- 数值显示 --- */
/* 右对齐显示有符号数，width 为占用的字符宽度（不足补空格） */
void Oled_ShowInt(uint8 x, uint8 page, int32 v, uint8 width);
void Oled_ShowUInt(uint8 x, uint8 page, uint32 v, uint8 width);
/* 定点显示：PrintFix(-1234, 2) → "-12.34" */
void Oled_ShowFix(uint8 x, uint8 page, int32 v, uint8 decimals, uint8 width);

/* --- 图形 --- */
void Oled_DrawHLine(uint8 x0, uint8 x1, uint8 page);
void Oled_DrawProgress(uint8 x, uint8 page, uint8 width, uint8 percent);

#endif /* __OLED_H */
