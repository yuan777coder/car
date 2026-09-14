#ifndef __DEBUG_H
#define __DEBUG_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  debug.h  —  串口调试：文本参数回传 + 上位机虚拟示波器 + 在线改参
 *
 *  文本模式（默认）：每 DBG_TEXT_PERIOD_MS 打印一行，例如
 *      ADC:1234 1102 0987 1050 1180 | DEV: +123 | SRV:1520 | SPD:400 400
 *  用任何串口助手（115200 8N1）都能看，调试初期最实用。
 *
 *  示波器模式：发送定长二进制帧，给 tools/scope.py 画波形，
 *  帧格式见 debug.c 顶部注释（含帧头、通道数、校验和）。
 *
 *  在线改参命令（文本行，回车结束）：
 *      kp=700      改舵机 Kp
 *      kd=60       改舵机 Kd
 *      base=400    改基础速度
 *      min=220     改弯道最低速度
 *      trim=0      改舵机机械中值
 *      save        保存到 EEPROM
 *      load        从 EEPROM 重新载入
 *      default     恢复出厂参数
 *      cal         开始归一化标定
 *      run / stop  启动 / 停止
 *      page=N      切换显示页面
 *      text / scope 切换回传模式
 *      ?           打印当前参数
 * ========================================================================== */

void Debug_Init(void);

/* 主循环每圈调用一次：处理接收命令 + 按周期输出 */
void Debug_Poll(void);

/* 手动打印一行（例如标定完成后提示） */
void Debug_Print(const char *s);

/* 切换回传模式：1=示波器二进制帧，0=文本 */
void Debug_SetScopeMode(uint8 on);
uint8 Debug_GetScopeMode(void);

#endif /* __DEBUG_H */
