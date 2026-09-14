#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include "common.h"

/* ==========================================================================
 *  board_config.h —— 引脚 / 外设分配（改硬件接线只改这个文件）
 *
 *  本文件是"硬件映射表"，把逻辑功能映射到具体引脚 / ADC 通道 / PWM 通道，
 *  其余代码一律通过符号名引用，不直接写引脚号。
 *  默认分配基于 STC32G144K246（100 引脚）+ 常用智能车外围，
 *  请务必对照你自己的主板原理图核对后再烧录。
 *
 *  ┌───────────────────────── 引脚总览（默认）──────────────────────────┐
 *  │ P0.0~P0.6 : 电感 ADC（ADC2 CH0~CH6，5 水平 + 2 竖直）             │
 *  │ P0.7      : 电池电压 ADC（ADC2 CH7，可选，默认关）                │
 *  │ P1.0      : 电机L PWM（PWMA 通道1，20kHz）                        │
 *  │ P1.1      : 电机L 方向                                            │
 *  │ P1.2      : 电机R PWM（PWMA 通道2，20kHz）                        │
 *  │ P1.3      : 电机R 方向                                            │
 *  │ P2.0      : 舵机 PWM（PWMB 通道5，50Hz）                          │
 *  │ P2.4/P2.5 : OLED SCL / SDA（软件 I2C）                            │
 *  │ P3.0/P3.1 : UART1 RX / TX                                         │
 *  │ P3.2/P3.3 : K1 / K2                                               │
 *  │ P3.4/P3.5 : 编码器L / R 的 A 相（T0/T1 计数，可选）              │
 *  │ P3.6/P3.7 : K3 / K4                                               │
 *  │ P4.0/P4.1 : 编码器L / R 的 B 相（判方向，可选）                  │
 *  └──────────────────────────────────────────────────────────────────┘
 * ========================================================================== */

/* ------------------------------ 系统 ------------------------------ */
#define MAIN_FREQ_MHZ       24      /* 必须与 STC-ISP 下载时设置的 IRC 频率一致！ */

/* ------------------------------ 电感（ADC） ------------------------------ */
/* ⚠ STC32G144K246 是双 ADC：ADC1(CH0~7=P1.0~1.7)、ADC2(CH0~7=P0.0~0.7)。
   7 路电感全在 P0.0~P0.6 → 用 ADC2 的 CH0~CH6。
   ⚠ ADC 有独立参考引脚 ADC_VREF+，不可悬空，否则读数漂移/不变。 */
#define IND_ADC_MODULE      2       /* 1=ADC1, 2=ADC2 */
#define IND_ADC_CH          { 0, 1, 2, 3, 4, 5, 6 }

/* 电池电压检测（可选） */
#define BAT_ADC_MODULE      2       /* P0.7 → ADC2 CH7 */
#define BAT_ADC_CH          7
#define BAT_DIVIDER         11      /* 分压比：上/下电阻之和 ÷ 下电阻（100k/10k → 11） */

/* ------------------------------ 电机 ------------------------------ */
/* 两路电机用 PWMA 通道1/2（P1.0 / P1.2），方向用普通 GPIO（P1.1 / P1.3） */
#define MOTOR_PWM_FREQ      20000
sbit MOTOR_L_DIR = P1^1;
sbit MOTOR_R_DIR = P1^3;

/* ------------------------------ 舵机 ------------------------------ */
/* 舵机用 PWMB 通道5（P2.0），50Hz；脉宽单位是微秒，与主频无关，
   换算成 CCR 比较值由 pwm.c 按 MAIN_FREQ_MHZ 自动完成 */
#define SERVO_PWM_FREQ      50
#define SERVO_PULSE_MIN_US  500
#define SERVO_PULSE_MID_US  1500
#define SERVO_PULSE_MAX_US  2500

/* ------------------------------ 编码器（可选） ------------------------------ */
/* A 相进 T0/T1 计数脚（固定 P3.4 / P3.5），B 相读电平判方向 */
sbit ENC_L_B = P4^0;
sbit ENC_R_B = P4^1;

/* ------------------------------ OLED（软件 I2C） ------------------------------ */
sbit OLED_SCL = P2^4;
sbit OLED_SDA = P2^5;
#define OLED_FLIP_X         1       /* 0/1：左右镜像开关 */
#define OLED_FLIP_Y         1       /* 0/1：上下翻转开关 */

/* ------------------------------ 按键 ------------------------------ */
#define KEY_NUM             4
sbit KEY1 = P3^2;
sbit KEY2 = P3^3;
sbit KEY3 = P3^6;
sbit KEY4 = P3^7;

/* ------------------------------ EEPROM ------------------------------ */
/* IAP 视角下 EEPROM 从 0 开始（与 STC-ISP 里分配的"用户 EEPROM 大小"无关）。
   若你的芯片/手册要求用绝对地址（如 0xFE0000），只改这一个宏即可。 */
#define EEPROM_BASE_ADDR    0x0000UL

/* ------------------------------ 中断向量号 ------------------------------ */
/* STC32G（251 内核）中断向量（经 STC32G144K246.H 实测确认）：
   T0=1, T1=3, T2=12, T3=19, T4=20, UART1=4。
   注意：STC32G144K246 没有 14/15 号向量（那是老 STC32G.H 的 BRK/ICEP）。 */
#define T0_VECTOR_NUM       1
#define T1_VECTOR_NUM       3
#define T2_VECTOR_NUM       12
#define T3_VECTOR_NUM       19
#define T4_VECTOR_NUM       20
#define UART1_VECTOR_NUM    4

#endif /* __BOARD_CONFIG_H */
