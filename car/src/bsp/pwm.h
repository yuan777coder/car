#ifndef __PWM_H
#define __PWM_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  pwm.h  —  PWM 输出：2 路电机 + 1 路舵机
 *
 *  电机：每路一个 PWM 引脚 + 一个方向 GPIO，占空比用"千分比 ‰"表示，
 *        duty 的正负直接表示转向，上层（motor.c）不用关心方向脚。
 *  舵机：50Hz，比较值就是高电平时间对应的计数，
 *        具体计数值与中值/上下限见 board_config.h 的 SERVO_PWM_* 宏。
 *
 *  ⚠ 舵机与电机频率不同，必须用不同的 PWM 定时器资源，
 *    具体分配见 board_config.h 和 docs/pinmap.md。
 * ========================================================================== */

void Pwm_Init(void);

/* duty: -1000 ~ +1000（‰）。负值 = 反转，0 = 断电滑行 */
void Pwm_SetMotorLeft(int16 duty);
void Pwm_SetMotorRight(int16 duty);

/* 直接设置舵机 PWM 比较值（范围会被夹到 Pwm_ServoMin ~ Pwm_ServoMax） */
void Pwm_SetServo(uint16 counts);

/* 舵机脉宽换算出的 CCR 边界（由 Pwm_Init 按 MAIN_FREQ_MHZ 计算） */
uint16 Pwm_ServoMin(void);
uint16 Pwm_ServoMid(void);
uint16 Pwm_ServoMax(void);

int16 Pwm_GetMotorLeft(void);
int16 Pwm_GetMotorRight(void);
uint16 Pwm_GetServo(void);

#endif /* __PWM_H */
