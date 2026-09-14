#ifndef __ENCODER_H
#define __ENCODER_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  encoder.h  —  编码器测速
 *
 *  ENCODER_ENABLE = 0 时不编译任何硬件代码，所有读数为 0，
 *  速度环退化为开环占空比控制（工程仍可正常编译运行）。
 *
 *  ENCODER_ENABLE = 1 时使用 T0/T1 的计数器模式分别对左右编码器 A 相计数，
 *  每个控制周期读一次并清零，得到"脉冲数/周期"，用它代替真实转速做闭环。
 *  注意：此时 T0/T1 被占用，系统节拍改用 T3（见 bsp/timer.c）。
 * ========================================================================== */

void  Encoder_Init(void);
void  Encoder_Reset(void);

/* 读取并清零本控制周期的脉冲数（带符号，负号表示反转） */
int16 Encoder_ReadLeft(void);
int16 Encoder_ReadRight(void);

#endif /* __ENCODER_H */
