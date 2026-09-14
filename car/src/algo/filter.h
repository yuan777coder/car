#ifndef __FILTER_H
#define __FILTER_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  filter.h  —  数字滤波
 *   - Filter_Scan()   : 对一整个 ADC 扫描结果做逐通道滑动平均
 *   - Filter_Lpf()    : 通用一阶低通，用于速度等慢变量
 * ========================================================================== */

void Filter_Init(void);

/* 对 raw[IND_CH_NUM] 做滑动平均，结果写入 out[IND_CH_NUM]。
   首次调用会用 raw 预填充窗口，避免上电后前几个周期数值偏小。 */
void Filter_Scan(const uint16 *raw, uint16 *out);

/* 清空滑动平均窗口，下次 Filter_Scan 重新预填充 */
void Filter_ResetAdc(void);

/* 一阶低通： out = prev + (now - prev) / 2^shift
   shift 越大越平滑、滞后越大。推荐 1~4。 */
int16 Filter_Lpf(int16 prev, int16 now, uint8 shift);

#endif /* __FILTER_H */
