#include "keys.h"
#include "board_config.h"
#include "timer.h"

/* ==========================================================================
 *  按键扫描
 *
 *  去抖策略：连续 2 次扫描（2 x KEY_SCAN_MS）电平一致才认。
 *  长短按：按下沿开始计时，到 KEY_LONG_MS 立即产生长按事件（不等松手），
 *          松手时如果没产生过长按，就产生短按事件。
 *  事件用 8 深度的环形缓冲，避免上层还没来得及取就走丢了。
 * ========================================================================== */

#define KEY_RING_SIZE   8
#define KEY_DB_TIMES    2

static key_event_t s_ring[KEY_RING_SIZE];
static uint8 s_head;    /* 写入位置 */
static uint8 s_tail;    /* 读出位置 */

static uint8  s_stable;                 /* 位掩码：1 = 已去抖的按下状态 */
static uint8  s_db_cnt[KEY_NUM];
static uint16 s_press_ms[KEY_NUM];
static uint8  s_long_sent[KEY_NUM];

static void keys_push(key_event_t ev)
{
    uint8 next = (uint8)((s_head + 1U) % KEY_RING_SIZE);

    if (next == s_tail) {
        return;             /* 满了就丢，正常情况不会发生 */
    }
    s_ring[s_head] = ev;
    s_head = next;
}

/* 读一次原始电平，返回位掩码（1 = 按下） */
static uint8 keys_raw_mask(void)
{
    uint8 m = 0;

#if (KEY_NUM > 0)
    if (KEY1 == 0) { m |= 0x01U; }
#endif
#if (KEY_NUM > 1)
    if (KEY2 == 0) { m |= 0x02U; }
#endif
#if (KEY_NUM > 2)
    if (KEY3 == 0) { m |= 0x04U; }
#endif
#if (KEY_NUM > 3)
    if (KEY4 == 0) { m |= 0x08U; }
#endif
    return m;
}

void Keys_Init(void)
{
    uint8 i;

    s_head = 0;
    s_tail = 0;
    s_stable = 0;
    for (i = 0; i < KEY_NUM; i++) {
        s_db_cnt[i]    = 0;
        s_press_ms[i]  = 0;
        s_long_sent[i] = 0;
    }
}

void Keys_Scan(void)
{
    uint8 raw = keys_raw_mask();
    uint8 chg;
    uint8 i;

    chg = (uint8)(raw ^ s_stable);

    for (i = 0; i < KEY_NUM; i++) {
        uint8 bit = (uint8)(1U << i);

        if ((chg & bit) == 0) {
            s_db_cnt[i] = 0;
            continue;
        }

        if (s_db_cnt[i] < KEY_DB_TIMES) {
            s_db_cnt[i]++;
        }
        if (s_db_cnt[i] < KEY_DB_TIMES) {
            continue;
        }

        s_db_cnt[i] = 0;

        if (raw & bit) {
            /* 按下 */
            s_stable = (uint8)(s_stable | bit);
            s_press_ms[i]  = 0;
            s_long_sent[i] = 0;
        } else {
            /* 松开：没发过长按就补一个短按 */
            s_stable = (uint8)(s_stable & (uint8)(~bit));
            if ((!s_long_sent[i]) && (s_press_ms[i] > 0U)) {
                keys_push((key_event_t)((uint8)KEY_EV_K1 + i));
            }
            s_press_ms[i] = 0;
        }
    }

    for (i = 0; i < KEY_NUM; i++) {
        if (s_stable & (uint8)(1U << i)) {
            if (s_press_ms[i] < 60000U) {
                s_press_ms[i] = (uint16)(s_press_ms[i] + KEY_SCAN_MS);
            }
            if ((!s_long_sent[i]) && (s_press_ms[i] >= KEY_LONG_MS)) {
                s_long_sent[i] = 1;
                keys_push((key_event_t)((uint8)KEY_EV_K1_LONG + i));
            }
        }
    }
}

key_event_t Keys_TakeEvent(void)
{
    key_event_t ev;

    if (s_tail == s_head) {
        return KEY_EV_NONE;
    }
    ev = s_ring[s_tail];
    s_tail = (uint8)((s_tail + 1U) % KEY_RING_SIZE);
    return ev;
}

uint8 Keys_GetState(void)
{
    return s_stable;
}
