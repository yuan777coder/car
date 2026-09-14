#ifndef __GPIO_H
#define __GPIO_H

#include "common.h"

/* ==========================================================================
 *  gpio.h  —  端口模式与通用位操作
 *
 *  STC32G 每个引脚由 PxM1:PxM0 两位决定工作模式：
 *      00 准双向口   01 推挽输出   10 高阻输入   11 开漏输出
 *  上电默认为准双向口。驱动 PWM、I2C、串口等外设前必须先设好模式。
 *
 *  具体哪个脚做什么由 board_config.h 的 Gpio_Init() 配置表决定。
 * ========================================================================== */

#define GPIO_MODE_QUASI   0   /* 准双向口（可读可写，弱上拉） */
#define GPIO_MODE_PP      1   /* 推挽输出（驱动能力强，不能当输入） */
#define GPIO_MODE_IN      2   /* 高阻输入（做 ADC / 输入必须用这个） */
#define GPIO_MODE_OD      3   /* 开漏输出 */

/* 按 board_config.h 里的配置表初始化所有引脚（模式 + 初值） */
void Gpio_Init(void);

/* port: 0~7 对应 P0~P7；pin: 0~7 */
void  Gpio_Mode(uint8 port, uint8 pin, uint8 mode);
void  Gpio_Set(uint8 port, uint8 pin);
void  Gpio_Clr(uint8 port, uint8 pin);
void  Gpio_Toggle(uint8 port, uint8 pin);
uint8 Gpio_Read(uint8 port, uint8 pin);

#endif /* __GPIO_H */
