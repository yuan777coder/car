#include "oled.h"
#include "oled_font.h"
#include "soft_i2c.h"
#include "board_config.h"
#include "delay.h"

/* ==========================================================================
 *  SSD1306 128x64 I2C 驱动
 *
 *  显存布局与 SSD1306 一致： s_fb[page][x]，每个字节是该列的 8 个像素，
 *  bit0 在最上面。这样刷新时可以整页直接搬，不需要逐位换算。
 *
 *  如果屏幕上下颠倒 → 改 OLED_INIT 里的 0xC8 为 0xC0；
 *  如果屏幕左右镜像 → 改 0xA1 为 0xA0。这两个都由 board_config.h 的
 *  OLED_FLIP_X / OLED_FLIP_Y 控制。
 * ========================================================================== */

static uint8 xdata s_fb[OLED_PAGES][OLED_WIDTH];

/* SSD1306 从机地址：7 位 0x3C 左移一位 = 0x78（模块背面若标 0x3D 则用 0x7A） */
#define OLED_ADDR       0x78

#define OLED_CMD        0x00
#define OLED_DATA       0x40

static void oled_cmd(uint8 c);
static void oled_cmd2(uint8 c1, uint8 c2);

/* ------------------------------ 底层 ------------------------------ */

static void oled_cmd(uint8 c)
{
    if (I2c_StartWrite(OLED_ADDR) == 0) {
        I2c_WriteByte(OLED_CMD);
        I2c_WriteByte(c);
    }
    I2c_Stop();
}

static void oled_cmd2(uint8 c1, uint8 c2)
{
    if (I2c_StartWrite(OLED_ADDR) == 0) {
        I2c_WriteByte(OLED_CMD);
        I2c_WriteByte(c1);
        I2c_WriteByte(c2);
    }
    I2c_Stop();
}

/* ------------------------------ 初始化 ------------------------------ */

void Oled_Init(void)
{
    uint8 page;
    uint8 x;

    /* 上电先把显存清掉，避免随机内容闪一下 */
    for (page = 0; page < OLED_PAGES; page++) {
        for (x = 0; x < OLED_WIDTH; x++) {
            s_fb[page][x] = 0x00;
        }
    }

    Delay_Ms(50);            /* 等 SSD1306 内部上电复位完成 */

    oled_cmd(0xAE);          /* display off */
    oled_cmd2(0xD5, 0x80);   /* 时钟分频 / 振荡频率 */
    oled_cmd2(0xA8, 0x3F);   /* multiplex ratio = 64 行 */
    oled_cmd2(0xD3, 0x00);   /* display offset = 0 */
    oled_cmd(0x40);          /* display start line = 0 */
    oled_cmd2(0x8D, 0x14);   /* 电荷泵使能（模块内部升压，必须开） */
    oled_cmd2(0x20, 0x02);   /* 页寻址模式 */
#if OLED_FLIP_X
    oled_cmd(0xA0);          /* 左右不镜像 */
#else
    oled_cmd(0xA1);          /* 左右镜像（默认接线方向） */
#endif
#if OLED_FLIP_Y
    oled_cmd(0xC0);          /* 上下不翻转 */
#else
    oled_cmd(0xC8);          /* 上下翻转（默认接线方向） */
#endif
    oled_cmd2(0xDA, 0x12);   /* COM pins 配置（128x64） */
    oled_cmd2(0x81, 0xCF);   /* 对比度 */
    oled_cmd2(0xD9, 0xF1);   /* 预充电周期 */
    oled_cmd2(0xDB, 0x40);   /* VCOMH 电压 */
    oled_cmd(0xA4);          /* 显示内容跟随显存 */
    oled_cmd(0xA6);          /* 正常显示（非反色） */
    oled_cmd(0xAF);          /* display on */

    Oled_Refresh();
}

/* ------------------------------ 刷新 ------------------------------ */

void Oled_Refresh(void)
{
    uint8  page;
    uint16 i;

    for (page = 0; page < OLED_PAGES; page++) {
        oled_cmd((uint8)(0xB0 + page));   /* 设定页地址 */
        oled_cmd(0x00);                   /* 列地址低 4 位 = 0 */
        oled_cmd(0x10);                   /* 列地址高 4 位 = 0 */

        if (I2c_StartWrite(OLED_ADDR) != 0) {
            I2c_Stop();
            return;                        /* 屏幕没应答就放弃这一帧 */
        }
        I2c_WriteByte(OLED_DATA);
        for (i = 0; i < OLED_WIDTH; i++) {
            I2c_WriteByte(s_fb[page][i]);
        }
        I2c_Stop();
    }
}

/* ------------------------------ 绘图 ------------------------------ */

void Oled_Clear(void)
{
    Oled_Fill(0x00);
}

void Oled_Fill(uint8 pattern)
{
    uint8  page;
    uint8  x;

    for (page = 0; page < OLED_PAGES; page++) {
        for (x = 0; x < OLED_WIDTH; x++) {
            s_fb[page][x] = pattern;
        }
    }
}

void Oled_ShowChar(uint8 x, uint8 page, char c)
{
    uint8 i;
    uint8 idx;

    if (page >= OLED_PAGES) {
        return;
    }
    if ((uint8)c < 0x20U || (uint8)c > 0x7FU) {
        c = ' ';
    }
    idx = (uint8)((uint8)c - 0x20U);

    for (i = 0; i < 6; i++) {
        if ((uint8)(x + i) < OLED_WIDTH) {
            s_fb[page][(uint8)(x + i)] = OledFont6x8[idx][i];
        }
    }
}

void Oled_ShowString(uint8 x, uint8 page, const char *s)
{
    while (*s != '\0') {
        if ((uint8)(x + 6U) > OLED_WIDTH) {
            break;
        }
        Oled_ShowChar(x, page, *s);
        x = (uint8)(x + 6U);
        s++;
    }
}

void Oled_ShowCharBig(uint8 x, uint8 page, char c)
{
    uint8 i, r;
    uint8 idx;
    uint8 b;
    uint8 lo, hi;

    if ((page + 1U) >= OLED_PAGES) {
        return;
    }
    if ((uint8)c < 0x20U || (uint8)c > 0x7FU) {
        c = ' ';
    }
    idx = (uint8)((uint8)c - 0x20U);

    for (i = 0; i < 6; i++) {
        b  = OledFont6x8[idx][i];
        lo = 0;
        hi = 0;
        /* 源 0~3 行 → 目标 0~7 行（上一页）；源 4~7 行 → 目标 0~7 行（下一页） */
        for (r = 0; r < 4; r++) {
            if (b & (uint8)(1U << r)) {
                lo = (uint8)(lo | (uint8)(3U << (r * 2)));
            }
        }
        for (r = 4; r < 8; r++) {
            if (b & (uint8)(1U << r)) {
                hi = (uint8)(hi | (uint8)(3U << ((r - 4) * 2)));
            }
        }

        if ((uint8)(x + i * 2U) < OLED_WIDTH) {
            s_fb[page][(uint8)(x + i * 2U)]       = lo;
            s_fb[(uint8)(page + 1U)][(uint8)(x + i * 2U)] = hi;
        }
        if ((uint8)(x + i * 2U + 1U) < OLED_WIDTH) {
            s_fb[page][(uint8)(x + i * 2U + 1U)]       = lo;
            s_fb[(uint8)(page + 1U)][(uint8)(x + i * 2U + 1U)] = hi;
        }
    }
}

void Oled_ShowStringBig(uint8 x, uint8 page, const char *s)
{
    while (*s != '\0') {
        if ((uint8)(x + 12U) > OLED_WIDTH) {
            break;
        }
        Oled_ShowCharBig(x, page, *s);
        x = (uint8)(x + 12U);
        s++;
    }
}

/* 把 buf 里的 n 个字符（正序）从 x 开始画出来 */
static void oled_puts(uint8 x, uint8 page, const char *buf, uint8 n)
{
    uint8 i;

    for (i = 0; i < n; i++) {
        Oled_ShowChar((uint8)(x + i * 6U), page, buf[i]);
    }
}

/* 生成右对齐数字串：把 v 按十进制倒序填进 buf，返回总长度 */
static uint8 fmt_uint(uint32 uv, char *buf, uint8 width)
{
    uint8 n = 0;
    uint8 i;

    do {
        buf[n] = (char)('0' + (uint8)(uv % 10UL));
        uv /= 10UL;
        n++;
    } while ((uv != 0UL) && (n < 10U));

    while (n < width && n < 10U) {
        buf[n] = ' ';
        n++;
    }

    /* 反序成正常顺序 */
    for (i = 0; i < (uint8)(n / 2U); i++) {
        char t = buf[i];

        buf[i] = buf[(uint8)(n - 1U - i)];
        buf[(uint8)(n - 1U - i)] = t;
    }
    return n;
}

void Oled_ShowUInt(uint8 x, uint8 page, uint32 v, uint8 width)
{
    char buf[12];
    uint8 n = fmt_uint(v, buf, width);

    oled_puts(x, page, buf, n);
}

void Oled_ShowInt(uint8 x, uint8 page, int32 v, uint8 width)
{
    char   buf[12];
    uint32 uv;
    uint8  n = 0;
    uint8  i;

    uv = (v < 0) ? (uint32)(-v) : (uint32)v;

    /* 按"低位在前"的顺序填，最后整体反序一次 */
    do {
        buf[n] = (char)('0' + (uint8)(uv % 10UL));
        uv /= 10UL;
        n++;
    } while ((uv != 0UL) && (n < 10U));

    if ((v < 0) && (n < 11U)) {
        buf[n] = '-';
        n++;
    }
    while (n < width && n < 11U) {
        buf[n] = ' ';
        n++;
    }

    for (i = 0; i < (uint8)(n / 2U); i++) {
        char t = buf[i];

        buf[i] = buf[(uint8)(n - 1U - i)];
        buf[(uint8)(n - 1U - i)] = t;
    }

    oled_puts(x, page, buf, n);
}

void Oled_ShowFix(uint8 x, uint8 page, int32 v, uint8 decimals, uint8 width)
{
    char   buf[14];
    uint32 uv;
    uint8  n = 0;
    uint8  d;
    uint8  i;

    if (decimals > 4U) {
        decimals = 4U;
    }
    uv = (v < 0) ? (uint32)(-v) : (uint32)v;

    /* 先放小数部分（倒序），再放小数点，再放整数部分 */
    for (d = 0; d < decimals; d++) {
        buf[n] = (char)('0' + (uint8)(uv % 10UL));
        uv /= 10UL;
        n++;
    }
    if (decimals > 0U) {
        buf[n] = '.';
        n++;
    }
    do {
        buf[n] = (char)('0' + (uint8)(uv % 10UL));
        uv /= 10UL;
        n++;
    } while ((uv != 0UL) && (n < 12U));

    if (v < 0) {
        buf[n] = '-';
        n++;
    }
    while (n < width && n < 13U) {
        buf[n] = ' ';
        n++;
    }

    for (i = 0; i < (uint8)(n / 2U); i++) {
        char t = buf[i];

        buf[i] = buf[(uint8)(n - 1U - i)];
        buf[(uint8)(n - 1U - i)] = t;
    }

    oled_puts(x, page, buf, n);
}

void Oled_DrawHLine(uint8 x0, uint8 x1, uint8 page)
{
    uint8 x;

    if (page >= OLED_PAGES) {
        return;
    }
    if (x0 > x1) {
        uint8 t = x0;

        x0 = x1;
        x1 = t;
    }
    if (x1 >= OLED_WIDTH) {
        x1 = OLED_WIDTH - 1U;
    }
    for (x = x0; x <= x1; x++) {
        s_fb[page][x] = (uint8)(s_fb[page][x] | 0x80U);   /* 该 page 最下面一行 */
    }
}

void Oled_DrawProgress(uint8 x, uint8 page, uint8 width, uint8 percent)
{
    uint8 i;
    uint8 fill;

    if (page >= OLED_PAGES) {
        return;
    }
    if (percent > 100U) {
        percent = 100U;
    }
    fill = (uint8)(((uint16)width * (uint16)percent) / 100U);

    for (i = 0; i < width; i++) {
        uint8 col = (uint8)(x + i);

        if (col >= OLED_WIDTH) {
            break;
        }
        /* 0x42 = 上下各一条边线（空心），0x7E = 实心 */
        s_fb[page][col] = (i < fill) ? 0x7EU : 0x42U;
    }
}
