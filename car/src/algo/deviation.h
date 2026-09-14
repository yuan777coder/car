#ifndef __DEVIATION_H
#define __DEVIATION_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  deviation.h  —  由归一化后的水平电感值计算循迹偏差
 *
 *  两种算法（用 config.h 的 DEV_MODE 选择）：
 *   DEV_MODE = 0  加权平均： dev = Σ(v_i * w_i) * DEV_GAIN / Σv_i
 *                 优点：分辨率高、直线很稳；
 *                 缺点：对整体场强变化敏感（所以必须做归一化）。
 *   DEV_MODE = 1  左右差比和： dev = (R - L) * DEV_MAX / (R + L)
 *                 优点：与场强绝对值无关，抗高度变化；
 *                 缺点：只有 2 个"眼睛"，分辨率低。
 *
 *  符号约定： dev > 0 表示导线在车的右侧 → 需要向右打舵。
 * ========================================================================== */

typedef struct {
    int16  dev;      /* 偏差，范围 ±DEV_MAX，丢线时保持上一次的值 */
    uint16 sum_h;    /* 水平电感归一化值之和（总场强，用于判丢线/元素） */
    uint16 max_h;    /* 水平电感归一化值的最大者 */
    uint8  lost;     /* 1 = 丢线（总场强低于阈值） */
} dev_info_t;

void Deviation_Init(void);

/* norm[] 为水平+竖直全部通道的归一化值，前 IND_H_NUM 个是水平电感 */
void Deviation_Update(const uint16 *norm);

const dev_info_t *Deviation_Get(void);
int16 Deviation_Value(void);

#endif /* __DEVIATION_H */
