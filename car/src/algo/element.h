#ifndef __ELEMENT_H
#define __ELEMENT_H

#include "common.h"
#include "config.h"
#include "deviation.h"

/* ==========================================================================
 *  element.h  —  赛道元素识别（十字 / 圆环 / 三岔）
 *
 *  ⚠ 重要提醒
 *  元素判据的阈值与电感高度、间距、运放增益、赛道线宽强相关，
 *  config.h 里的默认值只是"能识别出变化"的起点，必须现场标定。
 *  建议先用虚拟示波器把竖直电感通道和总强度打到上位机上看波形，
 *  再照着波形改 EL_* 阈值。确认判据可靠后再打开 ELEMENT_ACTION_ENABLE。
 *
 *  判据思路（简化版，便于理解与修改）：
 *    十字   —— 左右和竖直方向同时有导线，水平电感"全都变强"，
 *              总强度相对平时的基线明显上升。
 *    圆环   —— 环岛入口的切向导线只被一侧竖直电感看到，
 *              左右竖直电感出现明显差值。
 *    三岔   —— 一侧竖直电感变强，同时水平总强度下降（导线分叉后各自变弱）。
 * ========================================================================== */

typedef enum {
    EL_NONE = 0,
    EL_CROSS,          /* 十字 */
    EL_ROUND_LEFT,     /* 左圆环 */
    EL_ROUND_RIGHT,    /* 右圆环 */
    EL_BIFURCATE       /* 三岔 */
} element_id_t;

typedef struct {
    element_id_t id;        /* 当前识别到的元素 */
    uint8  active;          /* 1 = 元素特征仍然成立 */
    uint16 hold_ms;         /* 特征已持续的时间（带权，可用于决策） */
    uint16 total;           /* 当前水平电感归一化值之和 */
    uint16 baseline;        /* 平时的总强度基线 */
    int16  vdiff;           /* 右竖直 - 左竖直 */
} element_info_t;

void Element_Init(void);

/* 每个控制周期调用一次 */
void Element_Update(const uint16 *norm, const dev_info_t *dev);

const element_info_t *Element_Get(void);

/* 元素名称，用于 OLED / 串口显示（英文缩写，避免字库问题） */
const char *Element_Name(element_id_t id);

#endif /* __ELEMENT_H */
