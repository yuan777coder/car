#ifndef __SOFT_I2C_H
#define __SOFT_I2C_H

#include "common.h"

/* ==========================================================================
 *  soft_i2c.h  —  GPIO 模拟 I2C 主机（只做写，够 OLED 用）
 *
 *  为什么用软件 I2C 而不是硬件 I2C：
 *    - 引脚随便挑，布线方便；
 *    - OLED 只写不读，速率要求低（100kHz 足够），软件模拟完全够用；
 *    - 不占用硬件 I2C 外设，也不会因硬件 I2C 的状态机踩坑。
 *
 *  引脚在 board_config.h 里配置，要求 SCL/SDA 都带 4.7k~10k 上拉。
 *  时序里用 Delay_Us 产生约 100kHz 的时钟。
 * ========================================================================== */

void I2c_Init(void);

/* 发起一次写传输：START + 从机地址(写) ，返回 0 表示从机应答 */
uint8 I2c_StartWrite(uint8 dev_addr);

/* 发送一个字节，返回 0 表示从机应答 */
uint8 I2c_WriteByte(uint8 dat);

void I2c_Stop(void);

#endif /* __SOFT_I2C_H */
