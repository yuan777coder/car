#ifndef __SERVO_H
#define __SERVO_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  servo.h  —  舵机转向控制（PD）
 *
 *  为什么用 PD 不用 PID：舵机是位置伺服机构，"转多少度"由 PWM 直接给定，
 *  加积分项只会让舵机在直线上缓慢漂移（积分饱和），没有任何好处。
 *  P 项负责回正力度，D 项负责提前量（抑制入弯过冲）。
 *
 *  Servo_Update() 只做算法，返回舵机 PWM 比较值，
 *  真正写寄存器由上层调用 Pwm_SetServo() 完成 —— 算法层与硬件解耦。
 * ========================================================================== */

typedef struct {
    int16 kp;   /* 比例：偏差 1000 大约对应 kp 个 PWM 计数 */
    int16 kd;   /* 微分：抑制过冲，从 kp/10 起步 */
} servo_param_t;

void Servo_Init(void);

void Servo_SetParam(int16 kp, int16 kd);
const servo_param_t *Servo_GetParam(void);

/* 机械中值微调（PWM 计数）。装舵机时舵盘不可能刚好对中，
   这个值用来补偿；掉电保存，现场调好后不用再动。 */
void  Servo_SetTrim(int16 trim);
int16 Servo_GetTrim(void);

/* 每个控制周期调用一次；dev 为 Deviation 输出。返回舵机 PWM 比较值 */
uint16 Servo_Update(int16 dev);

/* 直接给定 PWM 比较值（调试/机械中值标定时用），内部同时复位 PD 状态 */
void Servo_SetRaw(uint16 pwm);

uint16 Servo_GetPwm(void);
int16  Servo_GetOut(void);      /* 上一次 PD 输出（去掉中值后的舵量），用于显示 */

#endif /* __SERVO_H */
