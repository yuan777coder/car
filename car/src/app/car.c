#include "car.h"
#include "board_config.h"
#include "gpio.h"
#include "delay.h"
#include "timer.h"
#include "adc.h"
#include "pwm.h"
#include "uart.h"
#include "eeprom.h"
#include "soft_i2c.h"
#include "oled.h"
#include "filter.h"
#include "normalize.h"
#include "deviation.h"
#include "servo.h"
#include "motor.h"
#include "encoder.h"
#include "element.h"
#include "param.h"
#include "keys.h"
#include "ui.h"
#include "debug.h"

/* ==========================================================================
 *  整车应用层
 *
 *  一个控制周期（CTRL_PERIOD_MS = 5ms）里做的事，顺序不能乱：
 *      ADC 扫描 → 滑动平均 → 标定(可选) → 归一化 → 偏差 → 元素
 *      → 舵机 PD → 速度规划 → 电机速度环
 *
 *  时间分配：控制运算放在主循环里由 Timer_TakeFlag() 触发，
 *  定时器中断只置标志；按键扫描和串口收发在两次控制之间穿插执行。
 * ========================================================================== */

static uint16     s_adc_raw[IND_CH_NUM];   /* ADC 原始值 */
static uint16     s_adc_flt[IND_CH_NUM];   /* 滑动平均后 */
static uint16     s_norm[IND_CH_NUM];      /* 归一化值 */

static sys_mode_t s_mode;
static uint32     s_cycles;
static uint32     s_last_key_ms;

/* --------------------------------------------------------------------------
 *  速度规划：直线按基础速度跑，偏差越大越减速
 *  |dev| <= SPD_CURVE_START           → 基础速度
 *  |dev| >= DEV_MAX                   → 最低速度
 *  中间线性插值
 * -------------------------------------------------------------------------- */
static int16 Car_SpeedPlan(int16 dev)
{
    const motor_param_t *mp = Motor_GetParam();
    int16 base  = mp->base;
    int16 minsp = mp->min_spd;
    int16 adev  = ABS_(dev);
    int16 target;

#if SPD_CURVE_ENABLE
    if (adev <= SPD_CURVE_START) {
        target = base;
    } else if (adev >= DEV_MAX) {
        target = minsp;
    } else {
        target = (int16)((int32)base
                 - ((int32)(base - minsp) * (int32)(adev - SPD_CURVE_START))
                   / (int32)(DEV_MAX - SPD_CURVE_START));
    }
#else
    target = base;
    (void)minsp;
#endif

    if (target > mp->max_spd) {
        target = mp->max_spd;
    }
    if (target < 0) {
        target = 0;
    }
    return target;
}

/* --------------------------------------------------------------------------
 *  一个控制周期
 * -------------------------------------------------------------------------- */
static void Car_Control(void)
{
    uint8 i;

    /* --- 1. 采样与滤波 --- */
    Adc_ScanAll(s_adc_raw);
    Filter_Scan(s_adc_raw, s_adc_flt);

    /* --- 2. 标定中：只采集极值，不控制 --- */
    if (s_mode == MODE_CALIB) {
        Normalize_CalibTick(s_adc_flt);
    }

    /* --- 3. 归一化 --- */
    for (i = 0; i < IND_CH_NUM; i++) {
        s_norm[i] = Normalize_Value(i, s_adc_flt[i]);
    }

    /* --- 4. 偏差与元素 --- */
    Deviation_Update(s_norm);
    Element_Update(s_norm, Deviation_Get());

    /* --- 5. 舵机与电机 --- */
    if (s_mode == MODE_RUN) {
        int16 dev    = Deviation_Value();
        int16 target = Car_SpeedPlan(dev);
        int16 diff   = 0;

#if (MOTOR_DIFF_K != 0)
        /* 差速：dev>0 表示要右转，右轮（内侧）减速 */
        diff = (int16)(((int32)MOTOR_DIFF_K * (int32)dev) / 1000L);
#endif

        Pwm_SetServo(Servo_Update(dev));
        Motor_SetTarget((int16)(target + diff), (int16)(target - diff));
    } else {
        /* 停机 / 标定 / 调试：舵机回中，电机不输出 */
        Pwm_SetServo(Servo_Update(0));
        Motor_SetTarget(0, 0);
    }

    Motor_SpeedLoop();

    s_cycles++;
}

/* --------------------------------------------------------------------------
 *  按键处理
 * -------------------------------------------------------------------------- */
static void Car_HandleKeys(void)
{
    key_event_t ev = Keys_TakeEvent();

    switch (ev) {
        case KEY_EV_K1:                 /* 启动 / 停止 */
            if (s_mode == MODE_RUN) {
                Car_Stop();
            } else if (s_mode != MODE_CALIB) {
                Car_Start();
            }
            break;

        case KEY_EV_K2:                 /* 翻页 */
            Ui_NextPage();
            break;

        case KEY_EV_K3:                 /* 归一化标定 */
            Car_StartCalib();
            break;

        case KEY_EV_K4:                 /* 保存参数 */
            Param_Capture();
            if (Param_Save() == 0) {
                Ui_ShowMessage("PARAM", "SAVED", 1200);
            } else {
                Ui_ShowMessage("PARAM", "SAVE FAIL", 1500);
            }
            break;

        case KEY_EV_K1_LONG:            /* 恢复出厂设置 */
            Param_LoadDefault();
            (void)Param_Save();
            Ui_ShowMessage("PARAM", "DEFAULT", 1500);
            break;

        default:
            break;
    }
}

/* --------------------------------------------------------------------------
 *  对外接口
 * -------------------------------------------------------------------------- */

void Car_Init(void)
{
    uint8 i;

    for (i = 0; i < IND_CH_NUM; i++) {
        s_adc_raw[i] = 0;
        s_adc_flt[i] = 0;
        s_norm[i]    = 0;
    }
    s_mode         = MODE_STOP;
    s_cycles       = 0;
    s_last_key_ms  = 0;

    /* --- 底层外设：顺序有讲究 --- */
    Gpio_Init();        /* 先把所有引脚模式定下来，后面外设才能正常工作 */
    Delay_Init();       /* 软件延时常数依赖主频 */
    Timer_Init();       /* 控制节拍 */
    Uart_Init();        /* 调试口，尽早打开便于看启动信息 */
    Adc_Init();
    Pwm_Init();
    Encoder_Init();
    I2c_Init();
    Oled_Init();

    /* --- 算法与参数 --- */
    Filter_Init();
    Normalize_Init();
    Deviation_Init();
    Servo_Init();
    Motor_Init();
    Element_Init();

    Param_Init();       /* 从 EEPROM 载入参数并下发到各模块（必须在上面的 Init 之后） */

    Keys_Init();
    Ui_Init();
    Debug_Init();

    Motor_Stop();
    Pwm_SetServo(Servo_Update(0));   /* 舵机回中 */

    Debug_Print("== EM CAR READY ==");
    if (Param_IsFromEeprom()) {
        Debug_Print("param: loaded from EEPROM");
    } else {
        Debug_Print("param: DEFAULT (need calibrate & save)");
    }
}

void Car_Loop(void)
{
    uint32 now = Timer_GetMs();

    /* 按键：按 KEY_SCAN_MS 节拍扫描 */
    if ((uint32)(now - s_last_key_ms) >= (uint32)KEY_SCAN_MS) {
        s_last_key_ms = now;
#if KEY_ENABLE
        Keys_Scan();
        Car_HandleKeys();
#endif
    }

    /* 串口：收命令 + 周期回传 */
    Debug_Poll();

    /* 控制周期到：跑一轮控制 */
    if (Timer_TakeFlag()) {
        Car_Control();

        /* 标定结束：把标定值一起存进 EEPROM */
        if (Normalize_CalibTakeFinished()) {
            Param_Capture();
            if (Param_Save() == 0) {
                Ui_ShowMessage("CALIB", "SAVED", 1500);
                Debug_Print("calib done, saved");
            } else {
                Ui_ShowMessage("CALIB", "SAVE FAIL", 2000);
                Debug_Print("calib done, SAVE FAILED");
            }
            s_mode = MODE_STOP;
            Motor_Stop();
        }
    }

    /* 显示：内部按 100ms 节流 */
    Ui_Tick();
}

sys_mode_t Car_GetMode(void)
{
    return s_mode;
}

void Car_SetMode(sys_mode_t m)
{
    if ((m == MODE_RUN) && (s_mode == MODE_CALIB)) {
        return;         /* 标定中不允许直接起跑 */
    }
    s_mode = m;
    if (m != MODE_RUN) {
        Motor_Stop();
    }
}

void Car_Start(void)
{
    if (s_mode == MODE_CALIB) {
        return;
    }
    /* 起跑前把 PD 状态和积分清掉，避免接着上次的残留值 */
    Servo_Update(0);
    Motor_Stop();
    s_mode = MODE_RUN;
    Debug_Print("MODE: RUN");
}

void Car_Stop(void)
{
    s_mode = MODE_STOP;
    Motor_Stop();
    Servo_Update(0);
    Pwm_SetServo(Servo_Update(0));
    Debug_Print("MODE: STOP");
}

void Car_StartCalib(void)
{
    if (s_mode == MODE_RUN) {
        Car_Stop();
    }
    Normalize_StartCalib();
    s_mode = MODE_CALIB;
    Debug_Print("MODE: CALIB (push the car along the track 6s)");
}

uint8 Car_IsCalibrating(void)
{
    return (uint8)(s_mode == MODE_CALIB);
}

uint32 Car_GetCycleCount(void)
{
    return s_cycles;
}

const uint16 *Car_GetAdcRaw(void)
{
    return s_adc_flt;
}

const uint16 *Car_GetNorm(void)
{
    return s_norm;
}
