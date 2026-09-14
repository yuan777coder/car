#ifndef __PARAM_H
#define __PARAM_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  param.h  —  可掉电保存的运行参数
 *
 *  分成两部分：
 *   1) 控制参数：舵机 PD、速度、速度环 PI —— 现场调出来的值，必须存住；
 *   2) 归一化标定值：每个电感通道的 raw_min / raw_max。
 *
 *  存储布局：EEPROM 偏移 0 开始，一个扇区（512 字节）够放。
 *  写入流程：擦除扇区 → 写 magic/数据 → 写校验和。
 *  读取流程：读结构体 → 校验 magic 和 checksum → 失败则用默认值。
 * ========================================================================== */

#define PARAM_MAGIC     0x5A5AU
#define PARAM_VERSION   0x01U

typedef struct {
    uint16 magic;
    uint8  version;

    /* --- 舵机 --- */
    int16  servo_kp;
    int16  servo_kd;
    int16  servo_trim;

    /* --- 速度 --- */
    int16  spd_base;
    int16  spd_min;
    int16  spd_max;

    /* --- 速度环 PI（仅 ENCODER_ENABLE=1 时有效） --- */
    int16  pi_kp;
    int16  pi_ki;

    /* --- 归一化标定 --- */
    uint16 calib_min[IND_CH_NUM];
    uint16 calib_max[IND_CH_NUM];

    uint16 checksum;
} param_t;

/* 上电调用：尝试从 EEPROM 载入，失败则用默认值并立即标脏 */
void Param_Init(void);

void Param_LoadDefault(void);

/* 把参数写回 EEPROM，返回 0 成功、非 0 失败 */
uint8 Param_Save(void);

/* 从 EEPROM 读取过参数返回 1，用的是默认值返回 0（显示用） */
uint8 Param_IsFromEeprom(void);

param_t *Param_Get(void);

/* 把结构体里的值下发到 servo / motor / normalize 模块 */
void Param_Apply(void);

/* 从 servo / motor 模块把当前值收回到结构体（准备保存） */
void Param_Capture(void);

#endif /* __PARAM_H */
