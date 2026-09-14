#include "adc.h"
#include "board_config.h"
#include "delay.h"

/* ==========================================================================
 *  ADC：查询式单通道逐次转换（双 ADC）
 *
 *  STC32G144K246 是双 ADC：
 *    ADC1：ADC_CONTR / ADCCFG / ADC_RES / ADC_RESL 是普通 SFR，
 *          ADCTIM 在 XFR 区（0x7efea8）
 *    ADC2：ADC2_CONTR / ADC2CFG / ADC2_RES / ADC2_RESL / ADC2TIM 全在 XFR 区
 *
 *  12 位结果采用右对齐（ADCCFG.RESFMT=1）：result = (RES << 8) | RESL
 *  访问 XFR 区寄存器前必须 EAXFR = 1。
 * ========================================================================== */

static uint8 code s_ind_ch[IND_CH_NUM] = IND_ADC_CH;

void Adc_Init(void)
{
    EAXFR = 1;              /* 使能 XFR（ADC2 与 ADCTIM 在 XFR 区） */

    /* --- ADC1 --- */
    ADC_CONTR = 0x80;       /* ADC_POWER 上电 */
    ADCCFG    = 0x2F;       /* RESFMT=1 右对齐 + SPEED=0x0F */
    ADCTIM    = 0x3F;       /* 采样时序 */

    /* --- ADC2 --- */
    ADC2_CONTR = 0x80;
    ADC2CFG    = 0x2F;
    ADC2TIM    = 0x3F;

    Delay_Ms(1);            /* 等 ADC 内部上电稳定 */
}

uint16 Adc_Sample(uint8 module, uint8 ch)
{
    uint16 res;

    ch &= 0x0F;

    if (module == 2) {
        EAXFR = 1;
        ADC2_CONTR = (uint8)(0x80 | ch);    /* 上电 + 选择通道 */
        ADC2_CONTR |= 0x40;                  /* 启动转换 */
        while ((ADC2_CONTR & 0x20) == 0) {   /* 等待 ADC_FLAG */
            ;
        }
        ADC2_CONTR &= 0xDF;                  /* 清标志 */
        res = (uint16)(((uint16)ADC2_RES << 8) | (uint16)ADC2_RESL);
    } else {
        ADC_CONTR = (uint8)(0x80 | ch);
        ADC_CONTR |= 0x40;
        while ((ADC_CONTR & 0x20) == 0) {
            ;
        }
        ADC_CONTR &= 0xDF;
        res = (uint16)(((uint16)ADC_RES << 8) | (uint16)ADC_RESL);
    }

    return res;
}

void Adc_ScanAll(uint16 *out)
{
    uint8 i;

    for (i = 0; i < IND_CH_NUM; i++) {
        out[i] = Adc_Sample(IND_ADC_MODULE, s_ind_ch[i]);
    }
}

#if BAT_ADC_ENABLE
uint16 Adc_GetBatteryRaw(void)
{
    return Adc_Sample(BAT_ADC_MODULE, BAT_ADC_CH);
}
#endif
