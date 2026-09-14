#include "element.h"

#if ELEMENT_ENABLE

static element_info_t s_info;
static uint32 s_baseline;

/* 各特征"持续成立"的计数值，单位为毫秒 */
static uint16 s_cross_ms;
static uint16 s_round_ms;
static uint16 s_bif_ms;

/* 有符号绝对值 */
static int16 el_abs16(int16 v)
{
    return (v < 0) ? (int16)(-v) : v;
}

/* 特征计数：成立则累加（饱和），不成立则缓慢衰减（形成滞回） */
static uint16 el_count(uint16 cnt, uint8 cond, uint16 limit)
{
    if (cond) {
        if (cnt < limit) {
            cnt = (uint16)(cnt + CTRL_PERIOD_MS);
            if (cnt > limit) {
                cnt = limit;
            }
        }
    } else {
        if (cnt > CTRL_PERIOD_MS) {
            cnt = (uint16)(cnt - CTRL_PERIOD_MS);
        } else {
            cnt = 0;
        }
    }
    return cnt;
}

void Element_Init(void)
{
    s_info.id       = EL_NONE;
    s_info.active   = 0;
    s_info.hold_ms  = 0;
    s_info.total    = 0;
    s_info.baseline = 0;
    s_info.vdiff    = 0;

    s_baseline = 0;
    s_cross_ms = 0;
    s_round_ms = 0;
    s_bif_ms   = 0;
}

void Element_Update(const uint16 *norm, const dev_info_t *dev)
{
    uint16 sum;
    int16  vdiff = 0;
    uint8  c_cross, c_round, c_bif;
    uint16 limit_cross, limit_round, limit_bif;
    element_id_t id = EL_NONE;
    uint16 hold = 0;

    sum = dev->sum_h;

    /* --- 平时总强度基线：只在没有元素时更新，否则十字会把基线抬上去 --- */
    if (s_baseline == 0UL) {
        s_baseline = (uint32)sum;
    } else if (s_info.id == EL_NONE) {
        s_baseline += ((int32)sum - (int32)s_baseline) / (int32)(1UL << EL_BASELINE_SHIFT);
    }

#if VERT_IND_ENABLE
    vdiff = (int16)((int16)norm[IND_IDX_V1] - (int16)norm[IND_IDX_V0]);
#endif

    /* --- 各特征成立条件 --- */
    /* 十字：水平总强度明显高于基线 */
    c_cross = (uint8)(((uint32)sum * 100UL) >
                      (s_baseline * (uint32)EL_CROSS_STRONG));

    /* 圆环：左右竖直电感出现明显差值 */
    c_round = (uint8)(el_abs16(vdiff) > EL_ROUND_VERT_DIFF);

    /* 三岔：一侧竖直电感变强，且水平总强度掉下来 */
    c_bif = (uint8)((el_abs16(vdiff) > EL_ROUND_VERT_DIFF) &&
                    ((uint32)sum < (uint32)EL_ROUND_LOST_MAX));

    limit_cross = EL_CROSS_MIN_MS;
    limit_round = EL_ROUND_MIN_MS;
    limit_bif   = EL_BIFURCATE_MS;

    s_cross_ms = el_count(s_cross_ms, c_cross, (uint16)(limit_cross * 4));
    s_round_ms = el_count(s_round_ms, c_round, (uint16)(limit_round * 4));
    s_bif_ms   = el_count(s_bif_ms,   c_bif,   (uint16)(limit_bif * 4));

    /* --- 决策：优先级 圆环 > 三岔 > 十字 --- */
    if (s_round_ms >= limit_round) {
        id   = (vdiff > 0) ? EL_ROUND_RIGHT : EL_ROUND_LEFT;
        hold = s_round_ms;
    } else if (s_bif_ms >= limit_bif) {
        id   = EL_BIFURCATE;
        hold = s_bif_ms;
    } else if (s_cross_ms >= limit_cross) {
        id   = EL_CROSS;
        hold = s_cross_ms;
    }

    s_info.id       = id;
    s_info.active   = (uint8)(id != EL_NONE);
    s_info.hold_ms  = hold;
    s_info.total    = sum;
    s_info.baseline = (uint16)((s_baseline > 0xFFFFUL) ? 0xFFFF : s_baseline);
    s_info.vdiff    = vdiff;
}

const element_info_t *Element_Get(void)
{
    return &s_info;
}

#else  /* ELEMENT_ENABLE == 0 */

void Element_Init(void) { }
void Element_Update(const uint16 *norm, const dev_info_t *dev) { (void)norm; (void)dev; }
const element_info_t *Element_Get(void)
{
    static const element_info_t e = { EL_NONE, 0, 0, 0, 0, 0 };

    return &e;
}

#endif /* ELEMENT_ENABLE */

const char *Element_Name(element_id_t id)
{
    switch (id) {
        case EL_CROSS:        return "CROSS";
        case EL_ROUND_LEFT:   return "RND-L";
        case EL_ROUND_RIGHT:  return "RND-R";
        case EL_BIFURCATE:    return "BIFUR";
        default:              return "NONE";
    }
}
