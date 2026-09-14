#ifndef __NORMALIZE_H
#define __NORMALIZE_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  normalize.h  —  电感通道归一化
 *
 *  为什么要归一化：每个电感、每条运放通道的增益和零点都不同，
 *  同一个磁场强度下 5 个通道的 ADC 值可能差 30% 以上。
 *  归一化把"绝对 ADC 值"变成"该通道的相对强度 0..NORM_MAX"，
 *  这样后面的加权偏差计算才不会被通道差异带偏。
 *
 *  标定方法：把车放在赛道上（关键：要让每个电感都经历"远离导线"和
 *  "正对导线"两种状态），按下标定键，然后沿赛道推车走一圈，
 *  6 秒内每个通道各自记录最小值和最大值。
 * ========================================================================== */

#define NORM_ADC_FULL   4095   /* 未标定时的默认满量程（12 位 ADC） */

typedef struct {
    uint16 raw_min[IND_CH_NUM];
    uint16 raw_max[IND_CH_NUM];
} norm_calib_t;

void Normalize_Init(void);

/* --- 标定值管理（配合 app/param.c 存 EEPROM） --- */
void Normalize_SetCalib(const norm_calib_t *c);
const norm_calib_t *Normalize_GetCalib(void);
void Normalize_SetDefaultCalib(void);   /* min=0, max=NORM_ADC_FULL */

/* --- 归一化换算 --- */
uint16 Normalize_Value(uint8 ch, uint16 raw);   /* 返回 0..NORM_MAX */

/* --- 标定流程 --- */
void Normalize_StartCalib(void);                /* 开始一次标定 */
void Normalize_CalibTick(const uint16 *raw);    /* 每个控制周期调用一次 */
uint8 Normalize_CalibBusy(void);                /* 1=正在标定 */
uint8 Normalize_CalibTakeFinished(void);        /* 读"刚标定完"标志并清零 */
uint8 Normalize_CalibProgress(void);            /* 标定进度 0..100，用于显示 */

#endif /* __NORMALIZE_H */
