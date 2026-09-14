#ifndef __CAR_H
#define __CAR_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  car.h  —  整车应用层：模式管理与控制流程
 *
 *  Car_Loop() 是主循环的全部内容，内部按顺序处理：
 *      控制周期到？ → 采样 → 归一化 → 偏差 → 元素 → 舵机/电机 → 显示 → 调试
 *  非控制周期则只跑按键扫描和调试收包，保证串口和按键响应及时。
 * ========================================================================== */

void Car_Init(void);

/* 主循环体，main() 里 while(1) 反复调用 */
void Car_Loop(void);

sys_mode_t Car_GetMode(void);
void Car_SetMode(sys_mode_t m);

void Car_Start(void);
void Car_Stop(void);

/* 开始一次归一化标定（进入 MODE_CALIB，结束后自动回 MODE_STOP） */
void Car_StartCalib(void);
uint8 Car_IsCalibrating(void);

/* 上电以来执行过的控制周期数（显示/统计用） */
uint32 Car_GetCycleCount(void);

/* --- 供 app/ui.c 与 app/debug.c 取用的实时数据 --- */
/* 注意：返回的是内部数组指针，长度均为 IND_CH_NUM，只读，不要改写 */
const uint16 *Car_GetAdcRaw(void);    /* 滤波后的原始 ADC 值 */
const uint16 *Car_GetNorm(void);      /* 归一化值 0..NORM_MAX */

#endif /* __CAR_H */
