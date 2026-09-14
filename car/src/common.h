#ifndef __COMMON_H
#define __COMMON_H

/* ==========================================================================
 *  common.h  —  与具体芯片外设无关的公共定义
 *  工程：基于 STC32G144K246 的电磁感应循迹小车
 *
 *  说明：本工程统一使用 uint8/uint16/uint32/int8/... 这套类型名，
 *        刻意避开 STC32G.H 已定义的 u8/u16/u32，防止重复定义。
 *        Keil C251 数据模型：char=8bit, int=16bit, long=32bit。
 * ========================================================================== */

typedef unsigned char  uint8;
typedef unsigned int   uint16;
typedef unsigned long  uint32;
typedef signed char    int8;
typedef signed int     int16;
typedef signed long    int32;

#define TRUE_   1
#define FALSE_  0

#define ARRAY_SIZE(a)     (sizeof(a) / sizeof((a)[0]))

/* 限幅：注意每个参数只求值一次 */
#define CLAMP(x, lo, hi)  ( (x) < (lo) ? (lo) : ( (x) > (hi) ? (hi) : (x) ) )
#define ABS_(x)           ( (x) < 0 ? -(x) : (x) )
#define MIN_(a, b)        ( (a) < (b) ? (a) : (b) )
#define MAX_(a, b)        ( (a) > (b) ? (a) : (b) )

/* 控制周期：由定时器中断产生，是整车主循环的时间基准 */
#define CTRL_PERIOD_MS    5

/* 运行模式 */
typedef enum {
    MODE_STOP = 0,   /* 停机：电机断电，舵机回中 */
    MODE_RUN,        /* 正常循迹 */
    MODE_CALIB,      /* 归一化标定中 */
    MODE_DEBUG       /* 只显示/回传数据，电机不转 */
} sys_mode_t;

#endif /* __COMMON_H */
