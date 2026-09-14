#include "deviation.h"

/* 水平电感权值表，来自 config.h 的 IND_WEIGHT_LIST。
   放在 code 段，不占 RAM（Keil C51/C251 里 const 不一定进 code，必须显式写 code） */
static int16 code s_weight[IND_H_NUM] = IND_WEIGHT_LIST;

static dev_info_t s_info;

void Deviation_Init(void)
{
    s_info.dev   = 0;
    s_info.sum_h = 0;
    s_info.max_h = 0;
    s_info.lost  = 1;
}

void Deviation_Update(const uint16 *norm)
{
    uint8  i;
    uint32 sum = 0;
    uint16 mx  = 0;
    int32  acc = 0;
    int32  dev;
#if (DEV_MODE != 0)
    uint32 lsum = 0;
    uint32 rsum = 0;
#endif

    for (i = 0; i < IND_H_NUM; i++) {
        uint16 v = norm[i];

        sum += (uint32)v;
        if (v > mx) {
            mx = v;
        }
        acc += (int32)v * (int32)s_weight[i];
#if (DEV_MODE != 0)
        if (s_weight[i] < 0) {
            lsum += (uint32)v;
        } else if (s_weight[i] > 0) {
            rsum += (uint32)v;
        }
#endif
    }

    s_info.sum_h = (sum > 0xFFFFUL) ? 0xFFFF : (uint16)sum;
    s_info.max_h = mx;
    s_info.lost  = (sum < (uint32)NORM_VALID_SPAN) ? 1 : 0;

    if (s_info.lost) {
        /* 丢线：保持上一次偏差，让车继续按原方向找线，而不是突然回正 */
        return;
    }

#if (DEV_MODE == 0)
    dev = acc * (int32)DEV_GAIN / (int32)sum;
#else
    {
        uint32 tot = lsum + rsum;

        if (tot < (uint32)NORM_VALID_SPAN) {
            return;
        }
        dev = ((int32)rsum - (int32)lsum) * (int32)DEV_MAX / (int32)tot;
    }
#endif

#if (DEV_CURVE_K != 0)
    /* 非线性修正：两端放大/压缩 */
    dev += (int32)DEV_CURVE_K * dev * ((dev < 0) ? -dev : dev)
           / ((int32)DEV_MAX * 100L);
#endif

    if (dev > (int32)DEV_MAX) {
        dev = (int32)DEV_MAX;
    } else if (dev < -(int32)DEV_MAX) {
        dev = -(int32)DEV_MAX;
    }

#if (DEV_DIR < 0)
    dev = -dev;
#endif

    if ((dev > -(int32)DEV_DEADZONE) && (dev < (int32)DEV_DEADZONE)) {
        dev = 0;
    }

    s_info.dev = (int16)dev;
}

const dev_info_t *Deviation_Get(void)
{
    return &s_info;
}

int16 Deviation_Value(void)
{
    return s_info.dev;
}
