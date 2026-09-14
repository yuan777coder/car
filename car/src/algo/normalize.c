#include "normalize.h"

static norm_calib_t s_calib;

/* 标定过程中的临时极值 */
static uint16 s_cmin[IND_CH_NUM];
static uint16 s_cmax[IND_CH_NUM];
static uint16 s_timer_ms;
static uint8  s_busy;
static uint8  s_finished;

void Normalize_SetDefaultCalib(void)
{
    uint8 ch;

    for (ch = 0; ch < IND_CH_NUM; ch++) {
        s_calib.raw_min[ch] = 0;
        s_calib.raw_max[ch] = NORM_ADC_FULL;
    }
}

void Normalize_Init(void)
{
    Normalize_SetDefaultCalib();
    s_busy     = 0;
    s_finished = 0;
    s_timer_ms = 0;
}

void Normalize_SetCalib(const norm_calib_t *c)
{
    uint8 ch;

    for (ch = 0; ch < IND_CH_NUM; ch++) {
        s_calib.raw_min[ch] = c->raw_min[ch];
        s_calib.raw_max[ch] = c->raw_max[ch];
    }
}

const norm_calib_t *Normalize_GetCalib(void)
{
    return &s_calib;
}

uint16 Normalize_Value(uint8 ch, uint16 raw)
{
    uint16 lo, hi;
    uint32 span;

    if (ch >= IND_CH_NUM) {
        return 0;
    }

    lo = s_calib.raw_min[ch];
    hi = s_calib.raw_max[ch];

    /* 异常标定值（通道没接、标定失败）一律当作 0，交由上层判丢线 */
    if (hi <= lo) {
        return 0;
    }
    span = (uint32)(hi - lo);
    if (span < NORM_VALID_SPAN) {
        return 0;
    }

    if (raw <= lo) {
        return 0;
    }
    if (raw >= hi) {
        return NORM_MAX;
    }
    return (uint16)(((uint32)(raw - lo) * NORM_MAX) / span);
}

void Normalize_StartCalib(void)
{
    uint8 ch;

    for (ch = 0; ch < IND_CH_NUM; ch++) {
        s_cmin[ch] = 0xFFFF;
        s_cmax[ch] = 0;
    }
    s_timer_ms = 0;
    s_busy     = 1;
    s_finished = 0;
}

void Normalize_CalibTick(const uint16 *raw)
{
    uint8 ch;
    uint32 hi;

    if (!s_busy) {
        return;
    }

    for (ch = 0; ch < IND_CH_NUM; ch++) {
        if (raw[ch] < s_cmin[ch]) {
            s_cmin[ch] = raw[ch];
        }
        if (raw[ch] > s_cmax[ch]) {
            s_cmax[ch] = raw[ch];
        }
    }

    s_timer_ms += CTRL_PERIOD_MS;
    if (s_timer_ms < CALIB_TIME_MS) {
        return;
    }

    for (ch = 0; ch < IND_CH_NUM; ch++) {
        /* 最小值给一点余量，避免标定时的抖动导致后续长期为 0 */
        if (s_cmin[ch] == 0xFFFF) {
            s_cmin[ch] = 0;
        }
        s_calib.raw_min[ch] = s_cmin[ch];

        hi = (uint32)s_cmax[ch] * (uint32)CALIB_GAIN / 100UL;
        if (hi > NORM_ADC_FULL) {
            hi = NORM_ADC_FULL;
        }
        s_calib.raw_max[ch] = (uint16)hi;
    }

    s_busy     = 0;
    s_finished = 1;
}

uint8 Normalize_CalibBusy(void)
{
    return s_busy;
}

uint8 Normalize_CalibTakeFinished(void)
{
    uint8 f = s_finished;

    s_finished = 0;
    return f;
}

uint8 Normalize_CalibProgress(void)
{
    uint32 p;

    if (!s_busy) {
        return 100;
    }
    p = (uint32)s_timer_ms * 100UL / (uint32)CALIB_TIME_MS;
    if (p > 100UL) {
        p = 100UL;
    }
    return (uint8)p;
}
