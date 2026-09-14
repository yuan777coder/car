#ifndef __UI_H
#define __UI_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  ui.h  —  OLED 显示界面
 *
 *  分页显示，K2 短按翻页。刷新做节流（默认 100ms），因为 SSD1306 走
 *  软件 I2C 刷一整屏约需 20ms，每个控制周期都刷会拖慢主循环。
 *  控制周期是 5ms，显示 100ms 刷新一次完全够看。
 *
 *  页面：
 *    MAIN   电感强度条 + 偏差 + 舵机 PWM       —— 跑车时看这一页
 *    ADC    5 路原始 ADC + 归一化值            —— 标定/查通道时看
 *    MOTOR  目标速度 / 实际输出 / 编码器计数   —— 调速度环时看
 *    STATUS 模式 / 元素 / 运行时间 / 参数来源  —— 系统状态
 * ========================================================================== */

typedef enum {
    UI_PAGE_MAIN = 0,
    UI_PAGE_ADC,
    UI_PAGE_MOTOR,
    UI_PAGE_STATUS,
    UI_PAGE_COUNT
} ui_page_t;

void Ui_Init(void);

/* 主循环每圈调用，内部按时间节流刷新 */
void Ui_Tick(void);

void Ui_NextPage(void);
void Ui_SetPage(ui_page_t p);
ui_page_t Ui_GetPage(void);

/* 临时提示（例如 "CALIB" / "SAVED"），持续 ms 毫秒后恢复正常页面 */
void Ui_ShowMessage(const char *line1, const char *line2, uint16 ms);

#endif /* __UI_H */
