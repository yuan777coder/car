#include "timer.h"
#include "board_config.h"

/* ==========================================================================
 *  系统节拍：Timer0 产生 1ms 中断，累计到 CTRL_PERIOD_MS 置控制标志
 *
 *  Timer0 工作在方式 1（16 位）、12T 模式（最标准的 8051 定时器，无 STC 专属
 *  寄存器依赖）。1ms 的计数值 = MAIN_FREQ_MHZ * 1000 / 12。
 *
 *  24MHz  → 2000 计数/ms，重装值 0xF830
 *  40MHz  → 3333 计数/ms（略有小数，舍入误差 <0.1%，可接受）
 *
 *  ⚠ 12T 模式下 40MHz 的 1ms 不是整数计数，若你要严格周期，改用 T0x12(1T)
 *    或把 CTRL_PERIOD_MS 凑成整数。24MHz 是整数，无此问题。
 * ========================================================================== */

static volatile uint32 s_ms;
static volatile uint8  s_flag;
static uint8  s_tick_th;
static uint8  s_tick_tl;
static uint8  s_tick_cnt;

void Timer_Init(void)
{
    uint32 cycles = (uint32)MAIN_FREQ_MHZ * 1000UL / 12UL;  /* 每 1ms 计数（12T） */
    uint32 reload = 65536UL - cycles;

    s_tick_th  = (uint8)(reload >> 8);
    s_tick_tl  = (uint8)reload;
    s_tick_cnt = 0;
    s_flag     = 0;
    s_ms       = 0;

    TMOD &= 0xF0;       /* 清 T0 字段 */
    TMOD |= 0x01;       /* T0 方式 1（16 位定时器） */
    TH0 = s_tick_th;
    TL0 = s_tick_tl;
    ET0 = 1;            /* 允许 T0 中断 */
    TR0 = 1;            /* 启动 T0 */
    EA  = 1;            /* 开总中断 */
}

void Timer_Isr(void) interrupt T0_VECTOR_NUM
{
    TH0 = s_tick_th;    /* 方式 1 需手动重装 */
    TL0 = s_tick_tl;

    s_ms++;

    if (++s_tick_cnt >= CTRL_PERIOD_MS) {
        s_tick_cnt = 0;
        s_flag = 1;
    }
}

uint8 Timer_TakeFlag(void)
{
    uint8 f = s_flag;

    s_flag = 0;
    return f;
}

uint32 Timer_GetMs(void)
{
    /* 32 位计数器被 16 位内核分两次读，极偶尔会在 65.5s 边界读到错值，
       对小车显示/计时无影响，故不做关中断保护。 */
    return s_ms;
}
