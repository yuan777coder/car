#ifndef __EEPROM_H
#define __EEPROM_H

#include "common.h"

/* ==========================================================================
 *  eeprom.h  —  片上 EEPROM（IAP）读写，用于保存参数与归一化标定值
 *
 *  地址说明：STC 的 IAP 地址是"相对于 EEPROM 起始地址"的偏移，
 *  起始地址由 STC-ISP 下载软件里的 "EEPROM 设置" 决定（例如 0x0400）。
 *  代码里只需要用偏移量（0 开始），不要用 Flash 的绝对地址。
 *
 *  STC 的一个 EEPROM 扇区是 512 字节，擦除以扇区为单位，
 *  所以"改一个字节"也要整扇区擦除后重写 —— 这就是 Param_Save()
 *  一次性写整个参数结构体的原因。
 * ========================================================================== */

void Eeprom_Init(void);

/* 读 len 个字节，返回 0 成功 */
uint8 Eeprom_Read(uint16 addr, uint8 *buf, uint16 len);

/* 写 len 个字节（写之前必须确保该扇区已擦除），返回 0 成功 */
uint8 Eeprom_Write(uint16 addr, const uint8 *buf, uint16 len);

/* 擦除 addr 所在的 512 字节扇区，返回 0 成功 */
uint8 Eeprom_EraseSector(uint16 addr);

#endif /* __EEPROM_H */
