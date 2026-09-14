#include "debug.h"
#include "uart.h"
#include "car.h"
#include "deviation.h"
#include "element.h"
#include "servo.h"
#include "motor.h"
#include "param.h"
#include "timer.h"
#include "normalize.h"
#include "ui.h"

/* ==========================================================================
 *  串口调试
 *
 *  ── 虚拟示波器帧格式（tools/scope.py 按此解析）──
 *     字节 0   : 0xAA   帧头
 *     字节 1   : 0x55   帧头
 *     字节 2   : LEN    数据区字节数 = SCOPE_CH_NUM * 2
 *     字节 3.. : 数据，每通道 2 字节，高字节在前（有符号补码）
 *     最后一字节: 前面所有数据字节之和的低 8 位（校验）
 *
 *     通道顺序：0=偏差 1=舵机PWM 2=左速度 3=右速度 4=水平总强度
 *               5..(5+IND_H_NUM-1)=各水平电感归一化值
 *
 *  ── 文本模式 ──
 *     每 DBG_TEXT_PERIOD_MS 打印一行，115200 8N1，任何串口助手可看。
 *     同时接收命令，命令表见 debug.h 顶部注释。
 * ========================================================================== */

#define LINE_BUF_SIZE   40

static char   s_line[LINE_BUF_SIZE];
static uint8  s_len;
static uint8  s_scope_mode;
static uint32 s_last_text_ms;

/* ------------------------------ 工具函数 ------------------------------ */

static int32 debug_atoi(const char *s)
{
    int32 v = 0;
    uint8 neg = 0;

    while ((*s == ' ') || (*s == '\t')) {
        s++;
    }
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while ((*s >= '0') && (*s <= '9')) {
        v = v * 10 + (int32)(*s - '0');
        s++;
    }
    return neg ? -v : v;
}

static uint8 debug_streq(const char *a, const char *b)
{
    while ((*a != '\0') && (*b != '\0')) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return (uint8)(*a == *b);
}

static uint8 debug_startswith(const char *a, const char *b)
{
    while (*b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return 1;
}

/* ------------------------------ 数据回传 ------------------------------ */

static void debug_send_scope(void)
{
    const dev_info_t *d = Deviation_Get();
    const uint16     *nr = Car_GetNorm();
    int16  ch[SCOPE_CH_NUM];
    uint8  i;
    uint8  cs = 0;

    ch[0] = d->dev;
    ch[1] = (int16)Servo_GetPwm();
    ch[2] = Motor_GetSpeedLeft();
    ch[3] = Motor_GetSpeedRight();
    ch[4] = (int16)d->sum_h;
    for (i = 0; i < IND_H_NUM; i++) {
        ch[(uint8)(5U + i)] = (int16)nr[i];
    }

    Uart_PutChar((char)0xAA);
    Uart_PutChar((char)0x55);
    Uart_PutChar((char)(uint8)(SCOPE_CH_NUM * 2U));

    for (i = 0; i < SCOPE_CH_NUM; i++) {
        uint8 hi = (uint8)(((uint16)ch[i] >> 8) & 0x00FFU);
        uint8 lo = (uint8)((uint16)ch[i] & 0x00FFU);

        Uart_PutChar((char)hi);
        Uart_PutChar((char)lo);
        cs = (uint8)(cs + hi + lo);
    }
    Uart_PutChar((char)cs);
}

static void debug_send_text(void)
{
    const dev_info_t *d  = Deviation_Get();
    const element_info_t *el = Element_Get();
    const uint16     *nr = Car_GetNorm();
    uint8 i;

    Uart_PrintStr("DEV ");
    Uart_PrintInt((int32)d->dev);
    Uart_PrintStr(" SRV ");
    Uart_PrintInt((int32)Servo_GetPwm());
    Uart_PrintStr(" SPD ");
    Uart_PrintInt((int32)Motor_GetSpeedLeft());
    Uart_PrintStr(" TGT ");
    Uart_PrintInt((int32)Motor_GetTargetLeft());
    Uart_PrintStr(" N");
    for (i = 0; i < IND_H_NUM; i++) {
        Uart_PutChar(' ');
        Uart_PrintUInt((uint32)nr[i]);
    }
    Uart_PrintStr(" SUM ");
    Uart_PrintUInt((uint32)d->sum_h);
    Uart_PrintStr(" EL ");
    Uart_PrintStr(Element_Name(el->id));
    if (d->lost) {
        Uart_PrintStr(" LOST");
    }
    Uart_PrintStr("\r\n");
}

static void debug_print_params(void)
{
    param_t *p = Param_Get();

    Uart_PrintStr("kp=");
    Uart_PrintInt((int32)p->servo_kp);
    Uart_PrintStr(" kd=");
    Uart_PrintInt((int32)p->servo_kd);
    Uart_PrintStr(" trim=");
    Uart_PrintInt((int32)p->servo_trim);
    Uart_PrintStr(" base=");
    Uart_PrintInt((int32)p->spd_base);
    Uart_PrintStr(" min=");
    Uart_PrintInt((int32)p->spd_min);
    Uart_PrintStr(" max=");
    Uart_PrintInt((int32)p->spd_max);
    Uart_PrintStr(" pikp=");
    Uart_PrintInt((int32)p->pi_kp);
    Uart_PrintStr(" piki=");
    Uart_PrintInt((int32)p->pi_ki);
    Uart_PrintStr(" src=");
    Uart_PrintStr(Param_IsFromEeprom() ? "EEPROM" : "DEFAULT");
    Uart_PrintStr("\r\n");
}

/* ------------------------------ 命令解析 ------------------------------ */

static void debug_ack(const char *name)
{
    Uart_PrintStr("OK ");
    Uart_PrintStr(name);
    Uart_PrintStr("\r\n");
}

static void debug_exec(char *line)
{
    param_t *p = Param_Get();

    if (debug_startswith(line, "kp=")) {
        p->servo_kp = (int16)debug_atoi(line + 3);
        Param_Apply();
        debug_ack("kp");
    } else if (debug_startswith(line, "kd=")) {
        p->servo_kd = (int16)debug_atoi(line + 3);
        Param_Apply();
        debug_ack("kd");
    } else if (debug_startswith(line, "trim=")) {
        p->servo_trim = (int16)debug_atoi(line + 5);
        Param_Apply();
        debug_ack("trim");
    } else if (debug_startswith(line, "base=")) {
        p->spd_base = (int16)debug_atoi(line + 5);
        Param_Apply();
        debug_ack("base");
    } else if (debug_startswith(line, "pikp=")) {
        p->pi_kp = (int16)debug_atoi(line + 5);
        Param_Apply();
        debug_ack("pikp");
    } else if (debug_startswith(line, "piki=")) {
        p->pi_ki = (int16)debug_atoi(line + 5);
        Param_Apply();
        debug_ack("piki");
    } else if (debug_startswith(line, "max=")) {
        p->spd_max = (int16)debug_atoi(line + 4);
        Param_Apply();
        debug_ack("max");
    } else if (debug_startswith(line, "min=")) {
        p->spd_min = (int16)debug_atoi(line + 4);
        Param_Apply();
        debug_ack("min");
    } else if (debug_startswith(line, "page=")) {
        Ui_SetPage((ui_page_t)(uint8)debug_atoi(line + 5));
        debug_ack("page");
    } else if (debug_streq(line, "save")) {
        if (Param_Save() == 0) {
            debug_ack("save");
        } else {
            Uart_PrintStr("ERR save\r\n");
        }
    } else if (debug_streq(line, "load")) {
        Param_Init();
        debug_ack("load");
    } else if (debug_streq(line, "default")) {
        Param_LoadDefault();
        Param_Apply();
        debug_ack("default");
    } else if (debug_streq(line, "cal")) {
        Car_StartCalib();
        debug_ack("cal");
    } else if (debug_streq(line, "run")) {
        Car_Start();
        debug_ack("run");
    } else if (debug_streq(line, "stop")) {
        Car_Stop();
        debug_ack("stop");
    } else if (debug_streq(line, "text")) {
        s_scope_mode = 0;
        debug_ack("text");
    } else if (debug_streq(line, "scope")) {
        s_scope_mode = 1;
        debug_ack("scope");
    } else if (debug_streq(line, "?")) {
        debug_print_params();
    } else {
        Uart_PrintStr("ERR unknown\r\n");
    }
}

/* ------------------------------ 对外接口 ------------------------------ */

void Debug_Init(void)
{
    s_len          = 0;
    s_scope_mode   = 0;
    s_last_text_ms = 0;
}

void Debug_Print(const char *s)
{
#if DEBUG_UART_ENABLE
    Uart_PrintStr(s);
    Uart_PrintStr("\r\n");
#else
    (void)s;
#endif
}

void Debug_SetScopeMode(uint8 on)
{
    s_scope_mode = (on != 0) ? 1 : 0;
}

uint8 Debug_GetScopeMode(void)
{
    return s_scope_mode;
}

void Debug_Poll(void)
{
#if DEBUG_UART_ENABLE
    char   c;
    uint32 now;

    while (Uart_ReadChar(&c)) {
        if ((c == '\r') || (c == '\n')) {
            if (s_len > 0U) {
                s_line[s_len] = '\0';
                debug_exec(s_line);
                s_len = 0;
            }
        } else if (s_len < (LINE_BUF_SIZE - 1U)) {
            s_line[s_len] = c;
            s_len++;
        } else {
            s_len = 0;      /* 行太长，丢掉重新收 */
        }
    }

    now = Timer_GetMs();
    if ((uint32)(now - s_last_text_ms) >= (uint32)DBG_TEXT_PERIOD_MS) {
        s_last_text_ms = now;
        if (s_scope_mode) {
            debug_send_scope();
        } else {
            debug_send_text();
        }
    }
#endif
}
