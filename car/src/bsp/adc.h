#ifndef __ADC_H
#define __ADC_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  adc.h  —  ADC 采样（电感通道 + 可选电池电压）
 *
 *  信号链：电感 → RS824 运放调理（含偏置/放大）→ 主控 ADC
 *  运放输出必须落在 ADC 参考电压范围内（3.3V 系统不要超过 3.3V），
 *  否则会出现削顶，标定出来的 min/max 就不可信。
 *
 *  采样方式是"查询式单通道逐次转换"：每个控制周期扫一遍全部电感通道。
 *  相比 ADC 中断多通道扫描，查询式实现简单、时序可控，
 *  在 5ms 周期内扫 7 个通道（每个约 10us）绰绰有余。
 * ========================================================================== */

void Adc_Init(void);

/* 单次采样，module 为 ADC 模块号（1=ADC1, 2=ADC2），ch 为该模块的通道号，
   返回 0~4095（右对齐 12 位） */
uint16 Adc_Sample(uint8 module, uint8 ch);

/* 扫描全部电感通道，out[] 长度为 IND_CH_NUM，下标顺序与 board_config.h 的
   ADC 通道表一致 */
void Adc_ScanAll(uint16 *out);

#if BAT_ADC_ENABLE
/* 电池电压采样，返回原始 ADC 值；换算公式见 board_config.h */
uint16 Adc_GetBatteryRaw(void);
#endif

#endif /* __ADC_H */
