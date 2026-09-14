#include "filter.h"

#if ADC_FILTER_WIN < 1
  #error "ADC_FILTER_WIN 必须 >= 1"
#endif

/* 每个通道一个环形窗口 + 一个运行和，滑动平均是 O(1) 的 */
static uint16 s_buf[IND_CH_NUM][ADC_FILTER_WIN];
static uint32 s_sum[IND_CH_NUM];
static uint8  s_idx;      /* 当前写入位置 */
static uint8  s_primed;   /* 0=窗口还没被真实数据填满 */

void Filter_Init(void)
{
    uint8 ch, i;

    for (ch = 0; ch < IND_CH_NUM; ch++) {
        s_sum[ch] = 0;
        for (i = 0; i < ADC_FILTER_WIN; i++) {
            s_buf[ch][i] = 0;
        }
    }
    s_idx    = 0;
    s_primed = 0;
}

void Filter_ResetAdc(void)
{
    s_idx    = 0;
    s_primed = 0;
}

void Filter_Scan(const uint16 *raw, uint16 *out)
{
    uint8 ch, i;

    if (!s_primed) {
        /* 用第一帧数据把整个窗口填满 */
        for (ch = 0; ch < IND_CH_NUM; ch++) {
            s_sum[ch] = 0;
            for (i = 0; i < ADC_FILTER_WIN; i++) {
                s_buf[ch][i] = raw[ch];
                s_sum[ch] += raw[ch];
            }
            out[ch] = raw[ch];
        }
        s_idx    = 0;
        s_primed = 1;
        return;
    }

    for (ch = 0; ch < IND_CH_NUM; ch++) {
        s_sum[ch] -= s_buf[ch][s_idx];
        s_buf[ch][s_idx] = raw[ch];
        s_sum[ch] += raw[ch];
        out[ch] = (uint16)(s_sum[ch] / ADC_FILTER_WIN);
    }

    s_idx++;
    if (s_idx >= ADC_FILTER_WIN) {
        s_idx = 0;
    }
}

int16 Filter_Lpf(int16 prev, int16 now, uint8 shift)
{
    int32 d;

    if (shift == 0) {
        return now;
    }
    if (shift > 15) {
        shift = 15;
    }
    d = (int32)now - (int32)prev;
    return (int16)((int32)prev + d / (int32)(1UL << shift));
}
