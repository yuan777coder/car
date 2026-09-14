#include "gpio.h"
#include "board_config.h"

/* ==========================================================================
 *  GPIO：端口模式与通用位操作
 *  模式编码（PxM1:PxM0）：00 准双向  01 推挽  10 高阻输入  11 开漏
 * ========================================================================== */

/* 把 port 号映射到 Px / PxM0 / PxM1 三组 SFR 的开关宏 */
#define GPIO_PORT_CASE(p, P, PM0, PM1)                    \
    case p:                                                \
        if (mode & 1u) { PM0 |= mask; } else { PM0 &= ~mask; } \
        if (mode & 2u) { PM1 |= mask; } else { PM1 &= ~mask; } \
        break;

void Gpio_Mode(uint8 port, uint8 pin, uint8 mode)
{
    uint8 mask = (uint8)(1u << pin);

    switch (port) {
        GPIO_PORT_CASE(0, P0, P0M0, P0M1)
        GPIO_PORT_CASE(1, P1, P1M0, P1M1)
        GPIO_PORT_CASE(2, P2, P2M0, P2M1)
        GPIO_PORT_CASE(3, P3, P3M0, P3M1)
        GPIO_PORT_CASE(4, P4, P4M0, P4M1)
        GPIO_PORT_CASE(5, P5, P5M0, P5M1)
        GPIO_PORT_CASE(6, P6, P6M0, P6M1)
        GPIO_PORT_CASE(7, P7, P7M0, P7M1)
        default:
            break;
    }
}

void Gpio_Set(uint8 port, uint8 pin)
{
    uint8 mask = (uint8)(1u << pin);

    switch (port) {
        case 0: P0 |= mask; break;
        case 1: P1 |= mask; break;
        case 2: P2 |= mask; break;
        case 3: P3 |= mask; break;
        case 4: P4 |= mask; break;
        case 5: P5 |= mask; break;
        case 6: P6 |= mask; break;
        case 7: P7 |= mask; break;
        default: break;
    }
}

void Gpio_Clr(uint8 port, uint8 pin)
{
    uint8 mask = (uint8)(1u << pin);

    switch (port) {
        case 0: P0 &= (uint8)~mask; break;
        case 1: P1 &= (uint8)~mask; break;
        case 2: P2 &= (uint8)~mask; break;
        case 3: P3 &= (uint8)~mask; break;
        case 4: P4 &= (uint8)~mask; break;
        case 5: P5 &= (uint8)~mask; break;
        case 6: P6 &= (uint8)~mask; break;
        case 7: P7 &= (uint8)~mask; break;
        default: break;
    }
}

void Gpio_Toggle(uint8 port, uint8 pin)
{
    uint8 mask = (uint8)(1u << pin);

    switch (port) {
        case 0: P0 ^= mask; break;
        case 1: P1 ^= mask; break;
        case 2: P2 ^= mask; break;
        case 3: P3 ^= mask; break;
        case 4: P4 ^= mask; break;
        case 5: P5 ^= mask; break;
        case 6: P6 ^= mask; break;
        case 7: P7 ^= mask; break;
        default: break;
    }
}

uint8 Gpio_Read(uint8 port, uint8 pin)
{
    uint8 mask = (uint8)(1u << pin);

    switch (port) {
        case 0: return (uint8)((P0 & mask) ? 1u : 0u);
        case 1: return (uint8)((P1 & mask) ? 1u : 0u);
        case 2: return (uint8)((P2 & mask) ? 1u : 0u);
        case 3: return (uint8)((P3 & mask) ? 1u : 0u);
        case 4: return (uint8)((P4 & mask) ? 1u : 0u);
        case 5: return (uint8)((P5 & mask) ? 1u : 0u);
        case 6: return (uint8)((P6 & mask) ? 1u : 0u);
        case 7: return (uint8)((P7 & mask) ? 1u : 0u);
        default: return 0;
    }
}

void Gpio_Init(void)
{
    /* ---- 电感 ADC：高阻输入（P0.0~P0.6） ---- */
    P0M0 &= 0x80;   /* P0.0~P0.6 → M0=0 */
    P0M1 |= 0x7F;   /* P0.0~P0.6 → M1=1 ⇒ 高阻输入 */

    /* ---- 电机 PWM 与方向：推挽输出（P1.0~P1.3） ---- */
    P1M0 |= 0x0F;
    P1M1 &= 0xF0;

    /* ---- 舵机 PWM：推挽输出（P2.0） ---- */
    P2M0 |= 0x01;
    P2M1 &= 0xFE;

    /* ---- OLED I2C：准双向口（P2.4/P2.5，带外部上拉） ---- */
    P2M0 &= 0xCF;
    P2M1 &= 0xCF;

    /* ---- 按键：准双向口（P3.2/P3.3/P3.6/P3.7），内部上拉，按下接地 ---- */
    P3M0 &= 0x3C;   /* 0x3C = 0011 1100，清 P3.2/3/6/7 的 M0 */
    P3M1 &= 0x3C;   /* 清 M1 ⇒ 准双向 */

    /* ---- 编码器 A/B 相：高阻/准双向输入（P3.4/P3.5、P4.0/P4.1） ---- */
    P3M0 &= 0xCF;
    P3M1 &= 0xCF;
    P4M0 &= 0xFC;
    P4M1 &= 0xFC;

    /* ---- 初值 ---- */
    MOTOR_L_DIR = 0;
    MOTOR_R_DIR = 0;
    OLED_SCL = 1;
    OLED_SDA = 1;
}
