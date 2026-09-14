#ifndef __MOTOR_H
#define __MOTOR_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  motor.h  —  驱动电机（速度给定 / 速度环 / 输出）
 *
 *  目标速度 target 的单位是"千分比"（‰，0~1000）：
 *    ENCODER_ENABLE = 0  开环模式：target 直接就是 PWM 占空比
 *    ENCODER_ENABLE = 1  闭环模式：target 是每控制周期期望的编码器脉冲数
 *  这样上层（app/car.c）做速度规划时不需要关心底层是开环还是闭环。
 * ========================================================================== */

typedef struct {
    int16 base;      /* 基础目标速度 */
    int16 min_spd;   /* 弯道最低速度 */
    int16 max_spd;   /* 速度上限 */
    int16 kp;        /* 编码器速度环 P */
    int16 ki;        /* 编码器速度环 I */
} motor_param_t;

void Motor_Init(void);
void Motor_SetParam(const motor_param_t *p);
const motor_param_t *Motor_GetParam(void);

/* 设置左右轮目标速度（带符号，正=前进）。只记录，不立即输出 */
void Motor_SetTarget(int16 left, int16 right);

/* 每个控制周期调用一次：读编码器 → 速度环/开环 → 更新 PWM 输出 */
void Motor_SpeedLoop(void);

void Motor_Stop(void);       /* 目标清零并立即输出 0 */

int16 Motor_GetSpeedLeft(void);     /* 实测速度（开环模式下=当前输出） */
int16 Motor_GetSpeedRight(void);
int16 Motor_GetTargetLeft(void);
int16 Motor_GetTargetRight(void);
int16 Motor_GetOutLeft(void);       /* 实际输出占空比 ‰ */
int16 Motor_GetOutRight(void);

#endif /* __MOTOR_H */
