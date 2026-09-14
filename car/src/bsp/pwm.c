#include "pwm.h"
#include "board_config.h"
#include "gpio.h"

/* ==========================================================================
 *  PWM：PWMA 输出 2 路 20kHz 电机 PWM，PWMB 输出 1 路 50Hz 舵机 PWM
 *
 *  STC32G144K246 的 PWM 寄存器在 XFR 区（far 0x7EFExx），访问前必须 EAXFR=1。
 *  一组 PWM 只有一个 PSCR/ARR（时基共用）⇒ 同组所有通道频率相同，
 *  所以 20kHz 电机与 50Hz 舵机必须分属 PWMA / PWMB 两组。
 *
 *  引脚模式（推挽输出）已在 Gpio_Init() 里统一配好，这里不再重复。
 * ========================================================================== */

static uint16 s_motor_arr;      /* 电机 PWM 的 ARR */
static uint16 s_servo_psc;      /* 舵机 PWM 的 PSC */
static uint16 s_servo_arr;      /* 舵机 PWM 的 ARR */
static uint16 s_servo_min;      /* 0.5ms 对应 CCR */
static uint16 s_servo_mid;      /* 1.5ms 对应 CCR */
static uint16 s_servo_max;      /* 2.5ms 对应 CCR */
static int16  s_duty_l, s_duty_r;
static uint16 s_servo_ccr;

/* 把 clk_hz 分频到 freq，输出 PSC 与 ARR（优先 PSC=0 拿最高分辨率） */
static void pwm_calc(uint32 clk_hz, uint16 freq, uint16 *psc, uint16 *arr)
{
    uint32 total = clk_hz / (uint32)freq;

    if (total <= 65536UL) {
        *psc = 0;
        *arr = (uint16)(total - 1UL);
    } else {
        *psc = (uint16)(total / 65536UL);
        *arr = (uint16)((clk_hz / ((uint32)(*psc) + 1UL) / (uint32)freq) - 1UL);
    }
}

/* 脉宽(us) → CCR：ccr = us * MAIN_FREQ_MHZ / (PSC+1) */
static uint16 servo_ccr(uint16 us)
{
    return (uint16)(((uint32)us * (uint32)MAIN_FREQ_MHZ) / ((uint32)s_servo_psc + 1UL));
}

void Pwm_Init(void)
{
    uint16 psc, arr;
    uint32 clk = (uint32)MAIN_FREQ_MHZ * 1000000UL;

    s_duty_l = 0;
    s_duty_r = 0;

    EAXFR = 1;              /* 使能 XFR 访问（必须） */

    /* ================= PWMA：2 路电机 PWM ================= */
    pwm_calc(clk, MOTOR_PWM_FREQ, &psc, &arr);
    s_motor_arr = arr;

    PWMA_PSCRH = (uint8)(psc >> 8);
    PWMA_PSCRL = (uint8)psc;
    PWMA_ARRH  = (uint8)(arr >> 8);
    PWMA_ARRL  = (uint8)arr;

    PWMA_CCER1 = 0x00;      /* 写 CCMR 前必须先关通道 */
    PWMA_CCER2 = 0x00;
    PWMA_CCMR1 = 0x68;      /* PWM模式1 + 预装载 */
    PWMA_CCMR2 = 0x68;
    PWMA_CCR1H = 0x00;
    PWMA_CCR1L = 0x00;      /* 上电 0% 占空比，安全 */
    PWMA_CCR2H = 0x00;
    PWMA_CCR2L = 0x00;
    PWMA_CCER1 = 0x11;      /* CC1E + CC2E，CCxP=0 高电平有效 */
    PWMA_PS   &= 0xF0;      /* 通道1/2 用第 1 组引脚（P1.0 / P1.2） */
    PWMA_ENO   = 0x05;      /* ENO1P + ENO2P */
    PWMA_BKR  |= 0x80;      /* MOE=1，主输出使能 */
    PWMA_EGR   = 0x01;      /* UG=1，立即载入 PSC/ARR */
    PWMA_CR1   = 0x09;      /* ARPE=1 + CEN=1 */

    /* ================= PWMB：1 路舵机 PWM ================= */
    pwm_calc(clk, SERVO_PWM_FREQ, &psc, &arr);
    s_servo_psc = psc;
    s_servo_arr = arr;

    PWMB_PSCRH = (uint8)(psc >> 8);
    PWMB_PSCRL = (uint8)psc;
    PWMB_ARRH  = (uint8)(arr >> 8);
    PWMB_ARRL  = (uint8)arr;

    PWMB_CCER1 = 0x00;
    PWMB_CCER2 = 0x00;
    PWMB_CCMR1 = 0x68;      /* PWMB 的 CCMR1 对应通道5 */
    PWMB_CCER1 = 0x01;      /* CC5E */
    PWMB_PS   &= 0xFC;      /* 通道5 用第 1 组引脚（P2.0） */
    PWMB_ENO   = 0x01;      /* ENO5P */
    PWMB_BKR  |= 0x80;
    PWMB_EGR   = 0x01;
    PWMB_CR1   = 0x09;

    /* 舵机脉宽 → CCR（min/mid/max） */
    s_servo_min = servo_ccr(SERVO_PULSE_MIN_US);
    s_servo_mid = servo_ccr(SERVO_PULSE_MID_US);
    s_servo_max = servo_ccr(SERVO_PULSE_MAX_US);
    s_servo_ccr = s_servo_mid;

    PWMB_CCR5H = (uint8)(s_servo_ccr >> 8);
    PWMB_CCR5L = (uint8)s_servo_ccr;
}

void Pwm_SetMotorLeft(int16 duty)
{
    uint16 d;
    uint16 ccr;
    uint8  dir = 0;

    if (duty < 0) {
        dir = 1;
        d = (uint16)(-duty);
    } else {
        d = (uint16)duty;
    }
    if (d > 1000) {
        d = 1000;
    }
    s_duty_l = duty;

    MOTOR_L_DIR = dir;
    ccr = (uint16)(((uint32)d * ((uint32)s_motor_arr + 1UL)) / 1000UL);
    PWMA_CCR1H = (uint8)(ccr >> 8);
    PWMA_CCR1L = (uint8)ccr;
}

void Pwm_SetMotorRight(int16 duty)
{
    uint16 d;
    uint16 ccr;
    uint8  dir = 0;

    if (duty < 0) {
        dir = 1;
        d = (uint16)(-duty);
    } else {
        d = (uint16)duty;
    }
    if (d > 1000) {
        d = 1000;
    }
    s_duty_r = duty;

    MOTOR_R_DIR = dir;
    ccr = (uint16)(((uint32)d * ((uint32)s_motor_arr + 1UL)) / 1000UL);
    PWMA_CCR2H = (uint8)(ccr >> 8);
    PWMA_CCR2L = (uint8)ccr;
}

void Pwm_SetServo(uint16 counts)
{
    if (counts < s_servo_min) {
        counts = s_servo_min;
    } else if (counts > s_servo_max) {
        counts = s_servo_max;
    }
    s_servo_ccr = counts;
    PWMB_CCR5H = (uint8)(counts >> 8);
    PWMB_CCR5L = (uint8)counts;
}

uint16 Pwm_ServoMin(void) { return s_servo_min; }
uint16 Pwm_ServoMid(void) { return s_servo_mid; }
uint16 Pwm_ServoMax(void) { return s_servo_max; }

int16 Pwm_GetMotorLeft(void)  { return s_duty_l; }
int16 Pwm_GetMotorRight(void) { return s_duty_r; }
uint16 Pwm_GetServo(void)     { return s_servo_ccr; }
