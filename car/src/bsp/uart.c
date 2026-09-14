#include "uart.h"
#include "board_config.h"

/* ==========================================================================
 *  UART1：发送用查询（阻塞），接收用中断 + 环形缓冲
 *
 *  波特率发生器用 Timer2（16 位自动重装 + 1T 模式）：
 *      baud = SYSclk / 4 / (65536 - [T2H:T2L])
 *   例：24MHz / 115200 → 重装值 65484（0xFFCC），误差 0.16%
 *
 *  定时器占用：T2 做波特率，因此系统节拍用 T0、编码器用 T1/T3（见 pinmap）。
 * ========================================================================== */

#define RX_BUF_SIZE  64

static uint8 xdata s_rx_buf[RX_BUF_SIZE];
static volatile uint8 s_rx_head;
static volatile uint8 s_rx_tail;

void Uart_Init(void)
{
    uint32 reload;

    s_rx_head = 0;
    s_rx_tail = 0;

    SCON = 0x50;        /* 模式 1（8 位 UART）+ REN 允许接收 */

    /* T2 作为 UART1 波特率发生器，1T 模式 */
    reload = 65536UL - ((uint32)MAIN_FREQ_MHZ * 1000000UL / 4UL / (uint32)UART_BAUD);
    T2L = (uint8)(reload & 0xFF);
    T2H = (uint8)(reload >> 8);

    AUXR |= 0x15;       /* S1BRT(bit0, 串口1用T2) + T2x12(bit2, 1T) + T2R(bit4, 启动) */

    ES = 1;             /* 允许 UART1 中断（接收用） */
}

void Uart_PutChar(char c)
{
    SBUF = c;
    while (!TI) {
        ;
    }
    TI = 0;
}

void Uart_Send(const char *s)
{
    while (*s) {
        Uart_PutChar(*s++);
    }
}

void Uart_SendBuf(const uint8 *buf, uint16 len)
{
    while (len--) {
        Uart_PutChar((char)(*buf++));
    }
}

uint8 Uart_Available(void)
{
    return (uint8)((uint8)(s_rx_head - s_rx_tail) % RX_BUF_SIZE);
}

uint8 Uart_ReadChar(char *c)
{
    if (s_rx_head == s_rx_tail) {
        return 0;
    }
    *c = (char)s_rx_buf[s_rx_tail];
    s_rx_tail = (uint8)((s_rx_tail + 1U) % RX_BUF_SIZE);
    return 1;
}

void Uart_Isr(void) interrupt UART1_VECTOR_NUM
{
    if (RI) {
        RI = 0;
        s_rx_buf[s_rx_head] = SBUF;
        s_rx_head = (uint8)((s_rx_head + 1U) % RX_BUF_SIZE);
        if (s_rx_head == s_rx_tail) {
            /* 缓冲满：丢最旧的一个字节 */
            s_rx_tail = (uint8)((s_rx_tail + 1U) % RX_BUF_SIZE);
        }
    }
    if (TI) {
        TI = 0;
    }
}

/* ------------------------------ 格式化输出 ------------------------------ */

void Uart_PrintStr(const char *s)
{
    Uart_Send(s);
}

void Uart_PrintUInt(uint32 v)
{
    char buf[11];
    uint8 n = 0;
    uint8 i;

    do {
        buf[n++] = (char)('0' + (uint8)(v % 10UL));
        v /= 10UL;
    } while (v && n < 10);

    for (i = n; i > 0; i--) {
        Uart_PutChar(buf[i - 1]);
    }
}

void Uart_PrintInt(int32 v)
{
    if (v < 0) {
        Uart_PutChar('-');
        v = -v;
    }
    Uart_PrintUInt((uint32)v);
}

void Uart_PrintHex8(uint8 v)
{
    static const char code hex[] = "0123456789ABCDEF";

    Uart_PutChar(hex[v >> 4]);
    Uart_PutChar(hex[v & 0x0F]);
}

void Uart_PrintFix(int32 v, uint8 decimals)
{
    if (v < 0) {
        Uart_PutChar('-');
        v = -v;
    }
    {
        int32 ip = v;
        uint8 i;
        uint32 frac = 0;
        uint32 mul = 1;

        for (i = 0; i < decimals; i++) {
            mul *= 10UL;
        }
        frac = (uint32)v % mul;
        ip = v / (int32)mul;

        Uart_PrintInt(ip);
        if (decimals > 0) {
            Uart_PutChar('.');
            for (i = 0; i < decimals; i++) {
                mul /= 10UL;
                Uart_PutChar((char)('0' + (uint8)(frac / mul % 10UL)));
            }
        }
    }
}
