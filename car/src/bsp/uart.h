#ifndef __UART_H
#define __UART_H

#include "common.h"
#include "config.h"

/* ==========================================================================
 *  uart.h  —  调试串口（UART1）
 *
 *  两个用途：
 *   1) 文本模式：人可读的实时参数，任何串口助手都能看（默认打开）
 *   2) 示波器模式：定长二进制帧，给上位机画波形用（tools/scope.py）
 *
 *  发送全部采用"查询 TX 完成标志"的阻塞方式：波特率 115200 下一个字节约
 *  87us，一帧几十字节最多几毫秒，放在主循环里可以接受。
 *  接收用中断 + 环形缓冲，保证不丢上位机下发的命令。
 * ========================================================================== */

void Uart_Init(void);

/* --- 发送 --- */
void Uart_PutChar(char c);
void Uart_Send(const char *s);
void Uart_SendBuf(const uint8 *buf, uint16 len);

/* 简易格式化输出：刻意不引入标准 printf，节省 C251 的代码空间 */
void Uart_PrintStr(const char *s);
void Uart_PrintInt(int32 v);
void Uart_PrintUInt(uint32 v);
void Uart_PrintHex8(uint8 v);
void Uart_PrintFix(int32 v, uint8 decimals);   /* 定点显示，如 PrintFix(-1234,2) → -12.34 */

/* --- 接收（中断驱动） --- */
uint8 Uart_Available(void);                    /* 缓冲区里还有几个字节 */
uint8 Uart_ReadChar(char *c);                  /* 非阻塞取一个字节，返回 1 表示取到 */

/* 中断服务函数 */
void Uart_Isr(void);

#endif /* __UART_H */
