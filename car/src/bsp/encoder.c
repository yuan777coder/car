#include "encoder.h"
#include "board_config.h"

/* ==========================================================================
 *  编码器测速（可选，默认 ENCODER_ENABLE=0 不编译）
 *
 *  方案：A 相进 T1/T3 计数器（硬件计数，主循环只需读、清零），B 相读电平判方向。
 *    - T1 计数器：标准 8051（TMOD 的 C/T 位），输入脚 P3.5
 *    - T3 计数器：STC 扩展定时器（T4T3M 配置），输入脚需查 144K246 手册
 *
 *  ⚠ 本模块是整工程里最"软"的部分：
 *    T3 的计数模式寄存器位序【未逐字核实】，且单相计数 + B 相电平判向
 *    只对单向旋转精确，双向来回抖动时会少计。默认关闭，接编码器后请
 *    先用串口回传实测脉冲数验证方向和量程，再打开 ENCODER_ENABLE。
 * ========================================================================== */

#if ENCODER_ENABLE

void Encoder_Init(void)
{
    /* ---- T1：16 位计数器（P3.5 输入） ---- */
    TMOD &= 0x0F;       /* 清 T1 字段 */
    TMOD |= 0x50;       /* T1 计数器、方式 1（16 位） */
    TH1 = 0;
    TL1 = 0;
    TR1 = 1;            /* 启动 T1 计数 */

    /* ---- T3：16 位计数器（STC 扩展，输入脚查手册） ---- */
    T4T3M &= 0xF0;      /* 清 T3 字段（位序需核对） */
    T4T3M |= 0x08;      /* T3R=1 运行（位序需核对） */
    T3L = 0;
    T3H = 0;
}

void Encoder_Reset(void)
{
    TL0 = 0;            /* 左轮若也用 T0 时清；此处保留 T1/T3 */
    TH1 = 0;
    TL1 = 0;
    T3L = 0;
    T3H = 0;
}

int16 Encoder_ReadLeft(void)
{
    int16 v;
    uint8 ea;

    ea = EA;
    EA = 0;
    v = (int16)(((uint16)TH1 << 8) | TL1);
    TH1 = 0;
    TL1 = 0;
    EA = ea;

    if (ENC_L_B) {
        v = (int16)(-v);
    }
    return v;
}

int16 Encoder_ReadRight(void)
{
    int16 v;
    uint8 ea;

    ea = EA;
    EA = 0;
    v = (int16)(((uint16)T3H << 8) | T3L);
    T3L = 0;
    T3H = 0;
    EA = ea;

    if (ENC_R_B) {
        v = (int16)(-v);
    }
    return v;
}

#else  /* ENCODER_ENABLE == 0 */

void Encoder_Init(void)   { }
void Encoder_Reset(void)  { }

int16 Encoder_ReadLeft(void)
{
    return 0;
}

int16 Encoder_ReadRight(void)
{
    return 0;
}

#endif
