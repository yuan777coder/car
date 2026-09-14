#include "motor.h"
#include "pwm.h"
#include "filter.h"
#include "encoder.h"

static motor_param_t s_p;

static int16 s_tgt_l, s_tgt_r;      /* 目标速度 */
static int16 s_spd_l, s_spd_r;      /* 实测速度 */
static int16 s_out_l, s_out_r;      /* 实际输出 ‰ */
static int32 s_acc_l, s_acc_r;      /* 增量式 PI 累加器 */
static int16 s_err_l, s_err_r;      /* 上一次误差 */

/* 电机启动死区补偿：很小的占空比电机不转，直接抬到死区值 */
static int16 Motor_ApplyDeadzone(int16 v)
{
#if (MOTOR_DEADZONE > 0)
    if ((v > 0) && (v < MOTOR_DEADZONE)) {
        v = MOTOR_DEADZONE;
    } else if ((v < 0) && (v > -MOTOR_DEADZONE)) {
        v = -MOTOR_DEADZONE;
    }
#else
    (void)v;
#endif
    return v;
}

void Motor_Init(void)
{
    s_p.base    = SPEED_BASE_DEFAULT;
    s_p.min_spd = SPEED_MIN_DEFAULT;
    s_p.max_spd = SPEED_MAX_DEFAULT;
    s_p.kp      = MOTOR_PI_KP_DEFAULT;
    s_p.ki      = MOTOR_PI_KI_DEFAULT;

    s_tgt_l = 0;
    s_tgt_r = 0;
    s_spd_l = 0;
    s_spd_r = 0;
    s_out_l = 0;
    s_out_r = 0;
    s_acc_l = 0;
    s_acc_r = 0;
    s_err_l = 0;
    s_err_r = 0;

    Pwm_SetMotorLeft(0);
    Pwm_SetMotorRight(0);
}

void Motor_SetParam(const motor_param_t *p)
{
    s_p.base    = p->base;
    s_p.min_spd = p->min_spd;
    s_p.max_spd = p->max_spd;
    s_p.kp      = p->kp;
    s_p.ki      = p->ki;
}

const motor_param_t *Motor_GetParam(void)
{
    return &s_p;
}

void Motor_SetTarget(int16 left, int16 right)
{
    s_tgt_l = left;
    s_tgt_r = right;
}

void Motor_Stop(void)
{
    s_tgt_l = 0;
    s_tgt_r = 0;
    s_spd_l = 0;
    s_spd_r = 0;
    s_out_l = 0;
    s_out_r = 0;
    s_acc_l = 0;
    s_acc_r = 0;
    s_err_l = 0;
    s_err_r = 0;

    Pwm_SetMotorLeft(0);
    Pwm_SetMotorRight(0);
}

void Motor_SpeedLoop(void)
{
    int16 tgt_l, tgt_r;

    tgt_l = CLAMP(s_tgt_l, -s_p.max_spd, s_p.max_spd);
    tgt_r = CLAMP(s_tgt_r, -s_p.max_spd, s_p.max_spd);

#if ENCODER_ENABLE
    {
        int16 e, de;

        /* --- 左轮 --- */
        s_spd_l = Filter_Lpf(s_spd_l, Encoder_ReadLeft(), SPEED_LPF_SHIFT);
        e       = tgt_l - s_spd_l;
        de      = e - s_err_l;
        s_err_l = e;
        s_acc_l += ((int32)s_p.kp * (int32)de + (int32)s_p.ki * (int32)e)
                   / (int32)MOTOR_PI_DIV;
        if (s_acc_l > (int32)MOTOR_PI_OUT_LIMIT) {
            s_acc_l = (int32)MOTOR_PI_OUT_LIMIT;
        } else if (s_acc_l < -(int32)MOTOR_PI_OUT_LIMIT) {
            s_acc_l = -(int32)MOTOR_PI_OUT_LIMIT;
        }
        s_out_l = (int16)s_acc_l;

        /* --- 右轮 --- */
        s_spd_r = Filter_Lpf(s_spd_r, Encoder_ReadRight(), SPEED_LPF_SHIFT);
        e       = tgt_r - s_spd_r;
        de      = e - s_err_r;
        s_err_r = e;
        s_acc_r += ((int32)s_p.kp * (int32)de + (int32)s_p.ki * (int32)e)
                   / (int32)MOTOR_PI_DIV;
        if (s_acc_r > (int32)MOTOR_PI_OUT_LIMIT) {
            s_acc_r = (int32)MOTOR_PI_OUT_LIMIT;
        } else if (s_acc_r < -(int32)MOTOR_PI_OUT_LIMIT) {
            s_acc_r = -(int32)MOTOR_PI_OUT_LIMIT;
        }
        s_out_r = (int16)s_acc_r;
    }
#else
    /* 开环：目标速度就是占空比 */
    s_out_l = tgt_l;
    s_out_r = tgt_r;
    s_spd_l = tgt_l;
    s_spd_r = tgt_r;
#endif

#if (MOTOR_REVERSE_L)
    Pwm_SetMotorLeft(-Motor_ApplyDeadzone(s_out_l));
#else
    Pwm_SetMotorLeft(Motor_ApplyDeadzone(s_out_l));
#endif

#if (MOTOR_REVERSE_R)
    Pwm_SetMotorRight(-Motor_ApplyDeadzone(s_out_r));
#else
    Pwm_SetMotorRight(Motor_ApplyDeadzone(s_out_r));
#endif
}

int16 Motor_GetSpeedLeft(void)   { return s_spd_l; }
int16 Motor_GetSpeedRight(void)  { return s_spd_r; }
int16 Motor_GetTargetLeft(void)  { return s_tgt_l; }
int16 Motor_GetTargetRight(void) { return s_tgt_r; }
int16 Motor_GetOutLeft(void)     { return s_out_l; }
int16 Motor_GetOutRight(void)    { return s_out_r; }
