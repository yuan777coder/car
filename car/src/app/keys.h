#ifndef __KEYS_H
#define __KEYS_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  keys.h  —  按键（低电平有效，内部上拉/外部上拉）
 *
 *  事件模型：Keys_Scan() 去抖 + 判长短按，产生一个"事件"放在队列里，
 *  上层用 Keys_TakeEvent() 取走。这样按键逻辑不会和显示、控制搅在一起。
 *
 *  默认功能分配（见 app/car.c）：
 *      K1 短按：启动 / 停止
 *      K2 短按：切换显示页面
 *      K3 短按：开始归一化标定
 *      K4 短按：保存参数到 EEPROM
 *      K1 长按：清除 EEPROM 参数并恢复默认（重置用）
 * ========================================================================== */

typedef enum {
    KEY_EV_NONE = 0,
    KEY_EV_K1, KEY_EV_K2, KEY_EV_K3, KEY_EV_K4,
    KEY_EV_K1_LONG, KEY_EV_K2_LONG, KEY_EV_K3_LONG, KEY_EV_K4_LONG
} key_event_t;

void Keys_Init(void);

/* 每 KEY_SCAN_MS 调用一次（由 app/car.c 的节拍逻辑保证） */
void Keys_Scan(void);

/* 取走一个事件，没有则返回 KEY_EV_NONE */
key_event_t Keys_TakeEvent(void);

/* 当前按下的按键位掩码：bit0=K1 ... bit3=K4 */
uint8 Keys_GetState(void);

#endif /* __KEYS_H */
