#include "eeprom.h"
#include "board_config.h"
#include <intrins.h>

/* ==========================================================================
 *  EEPROM（IAP）驱动
 *
 *  寄存器（与 STC8H 一致，STC32G144K246 多一个 IAP_ADDRE 高位地址寄存器）：
 *     IAP_DATA / IAP_ADDRH / IAP_ADDRL / IAP_CMD / IAP_TRIG / IAP_CONTR
 *     IAP_TPS(0xF5) / IAP_ADDRE(0xF6)
 *  命令：1=读  2=写  3=512 字节页擦除
 *  触发：先写 0x5A 再写 0xA5；STC32G144K 是流水线内核，触发后须补 4 个 NOP。
 *  擦除一页约 4~6ms。
 * ========================================================================== */

#define IAP_CMD_READ    0x01
#define IAP_CMD_WRITE   0x02
#define IAP_CMD_ERASE   0x03

/* 若你的 STC32G.H 没有定义 IAP_ADDRE（0xF6），取消下面一行注释 */
/* sfr IAP_ADDRE = 0xF6; */

static void iap_trig(void)
{
    IAP_TRIG = 0x5A;
    IAP_TRIG = 0xA5;    /* 写入 0xA5 后命令开始执行，CPU 等待 IAP 完成 */
    _nop_();
    _nop_();
    _nop_();
    _nop_();
}

static void iap_set_addr(uint16 addr)
{
    IAP_ADDRL = (uint8)(addr & 0xFF);
    IAP_ADDRH = (uint8)((addr >> 8) & 0xFF);
    IAP_ADDRE = 0x00;   /* EEPROM 基址 + 偏移都在低 16 位内 */
}

void Eeprom_Init(void)
{
    IAP_CONTR = 0x80;                       /* IAPEN = 1 */
    IAP_TPS   = (uint8)(MAIN_FREQ_MHZ + 1U);/* = Fosc/1MHz + 1（24MHz→25） */
    IAP_CMD   = 0x00;
}

static uint8 eeprom_read_byte(uint16 addr)
{
    uint8 dat;

    IAP_CMD = IAP_CMD_READ;
    iap_set_addr(addr);
    iap_trig();
    dat = IAP_DATA;
    return dat;
}

static void eeprom_write_byte(uint16 addr, uint8 dat)
{
    IAP_CMD = IAP_CMD_WRITE;
    iap_set_addr(addr);
    IAP_DATA = dat;
    iap_trig();
}

uint8 Eeprom_Read(uint16 addr, uint8 *buf, uint16 len)
{
    uint8 ea;

    ea = EA;
    EA = 0;
    while (len--) {
        *buf++ = eeprom_read_byte(addr++);
    }
    EA = ea;
    return 0;
}

uint8 Eeprom_Write(uint16 addr, const uint8 *buf, uint16 len)
{
    uint8 ea;

    ea = EA;
    EA = 0;
    while (len--) {
        eeprom_write_byte(addr++, *buf++);
    }
    EA = ea;
    return 0;
}

uint8 Eeprom_EraseSector(uint16 addr)
{
    uint8 ea;

    addr &= 0xFE00;         /* 512 字节对齐 */

    ea = EA;
    EA = 0;

    IAP_CMD = IAP_CMD_ERASE;
    iap_set_addr(addr);
    iap_trig();

    EA = ea;

    /* 擦除一页约 4~6ms，这里给足余量 */
    {
        uint16 i;
        for (i = 0; i < 600; i++) {
            _nop_();
        }
    }
    return 0;
}
