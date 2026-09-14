#include "delay.h"
#include "board_config.h"

/* ==========================================================================
 *  软件延时（近似值）
 *
 *  这是"大致准确"的延时，精度随编译器优化等级、主频设置变化。
 *  用于 OLED 上电复位、按键消抖、ADC 上电稳定等毫秒级场合，
 *  对时序有硬要求的地方（PWM/串口/ADC 采样）一律用硬件外设。
 *
 *  s_loop_per_us 是一个经验系数：空循环 `while(i--);` 在 C251 下
 *  大约 4~6 个时钟周期一次，取 6 时钟/次，再除以 6 得每微秒循环次数
 *  ≈ MAIN_FREQ_MHZ / 6 * 6 / 6 … 直接取 MAIN_FREQ_MHZ/6。
 *  如果发现实际延时偏小（例如 OLED 初始化不稳），把系数调大。
 * ========================================================================== */

static uint16 s_loop_per_us;

void Delay_Init(void)
{
    s_loop_per_us = (uint16)(MAIN_FREQ_MHZ / 6U);
    if (s_loop_per_us == 0) {
        s_loop_per_us = 1;
    }
}

void Delay_Us(uint16 us)
{
    while (us--) {
        volatile uint16 i = s_loop_per_us;

        while (i--) {
            ;
        }
    }
}

void Delay_Ms(uint16 ms)
{
    while (ms--) {
        Delay_Us(1000);
    }
}
