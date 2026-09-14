#include "servo.h"
#include "pwm.h"
#include "board_config.h"

static servo_param_t s_p;
static int16  s_trim;
static int16  s_last_dev;
static uint16 s_pwm;
static int16  s_out;

void Servo_Init(void)
{
    s_p.kp      = SERVO_KP_DEFAULT;
    s_p.kd      = SERVO_KD_DEFAULT;
    s_trim      = SERVO_TRIM;
    s_last_dev  = 0;
    s_out       = 0;
    s_pwm       = (uint16)(Pwm_ServoMid() + s_trim);
}

void Servo_SetParam(int16 kp, int16 kd)
{
    s_p.kp = kp;
    s_p.kd = kd;
}

const servo_param_t *Servo_GetParam(void)
{
    return &s_p;
}

void Servo_SetTrim(int16 trim)
{
    int16 hi = (int16)(Pwm_ServoMax() - Pwm_ServoMid());
    int16 lo = (int16)(Pwm_ServoMin() - Pwm_ServoMid());

    if (trim > hi) {
        trim = hi;
    } else if (trim < lo) {
        trim = lo;
    }
    s_trim = trim;
}

int16 Servo_GetTrim(void)
{
    return s_trim;
}

uint16 Servo_Update(int16 dev)
{
    int16 d;
    int32 out;
    int32 pwm;

    d = dev - s_last_dev;
    s_last_dev = dev;

    out = ((int32)s_p.kp * (int32)dev + (int32)s_p.kd * (int32)d) / (int32)PD_DIV;

#if (SERVO_DIR < 0)
    out = -out;
#endif

    if (out > 32767L) {
        out = 32767L;
    } else if (out < -32768L) {
        out = -32768L;
    }
    s_out = (int16)out;

    pwm = (int32)Pwm_ServoMid() + (int32)s_trim + out;

    if (pwm < (int32)Pwm_ServoMin()) {
        pwm = (int32)Pwm_ServoMin();
    } else if (pwm > (int32)Pwm_ServoMax()) {
        pwm = (int32)Pwm_ServoMax();
    }

#if (SERVO_SLEW_MAX > 0)
    {
        int32 delta = pwm - (int32)s_pwm;

        if (delta > (int32)SERVO_SLEW_MAX) {
            pwm = (int32)s_pwm + (int32)SERVO_SLEW_MAX;
        } else if (delta < -(int32)SERVO_SLEW_MAX) {
            pwm = (int32)s_pwm - (int32)SERVO_SLEW_MAX;
        }
    }
#endif

    s_pwm = (uint16)pwm;
    return s_pwm;
}

void Servo_SetRaw(uint16 pwm)
{
    if (pwm < Pwm_ServoMin()) {
        pwm = Pwm_ServoMin();
    } else if (pwm > Pwm_ServoMax()) {
        pwm = Pwm_ServoMax();
    }
    s_pwm      = pwm;
    s_last_dev = 0;
    s_out      = (int16)((int32)pwm - (int32)Pwm_ServoMid() - (int32)s_trim);
}

uint16 Servo_GetPwm(void)
{
    return s_pwm;
}

int16 Servo_GetOut(void)
{
    return s_out;
}
