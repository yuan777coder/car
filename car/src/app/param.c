#include "param.h"
#include "eeprom.h"
#include "servo.h"
#include "motor.h"
#include "normalize.h"

/* 参数在 EEPROM 中的偏移（一个扇区 512 字节，从 0 开始放） */
#define PARAM_EEPROM_ADDR   0x0000U

static param_t s_p;
static uint8   s_from_eeprom;

/* --------------------------------------------------------------------------
 *  校验和：刻意逐字段求和，而不是把结构体当字节数组求和。
 *  原因：结构体可能有编译器插入的对齐填充字节，按字节求和会把填充内容
 *  也算进去，读回来时一旦填充字节不同就会误判为"数据损坏"。
 *  （这个函数不包含 checksum 字段本身）
 * -------------------------------------------------------------------------- */
static uint16 param_checksum(const param_t *p)
{
    uint16 sum = 0;
    uint8  i;

    sum = (uint16)(sum + p->magic);
    sum = (uint16)(sum + (uint16)p->version);
    sum = (uint16)(sum + (uint16)p->servo_kp);
    sum = (uint16)(sum + (uint16)p->servo_kd);
    sum = (uint16)(sum + (uint16)p->servo_trim);
    sum = (uint16)(sum + (uint16)p->spd_base);
    sum = (uint16)(sum + (uint16)p->spd_min);
    sum = (uint16)(sum + (uint16)p->spd_max);
    sum = (uint16)(sum + (uint16)p->pi_kp);
    sum = (uint16)(sum + (uint16)p->pi_ki);

    for (i = 0; i < IND_CH_NUM; i++) {
        sum = (uint16)(sum + p->calib_min[i]);
        sum = (uint16)(sum + p->calib_max[i]);
    }
    return sum;
}

/* 合法性检查：EEPROM 被写坏时不至于让车失控 */
static uint8 param_sanity(const param_t *p)
{
    if ((p->servo_kp < 0) || (p->servo_kp > 5000)) {
        return 0;
    }
    if ((p->servo_kd < 0) || (p->servo_kd > 5000)) {
        return 0;
    }
    if ((p->servo_trim < -500) || (p->servo_trim > 500)) {
        return 0;
    }
    if ((p->spd_base < 0) || (p->spd_base > 1000)) {
        return 0;
    }
    if ((p->spd_min < 0) || (p->spd_min > 1000)) {
        return 0;
    }
    if ((p->spd_max < 0) || (p->spd_max > 1000)) {
        return 0;
    }
    if ((p->pi_kp < 0) || (p->pi_kp > 1000)) {
        return 0;
    }
    if ((p->pi_ki < 0) || (p->pi_ki > 1000)) {
        return 0;
    }
    return 1;
}

void Param_LoadDefault(void)
{
    uint8 i;

    s_p.magic      = PARAM_MAGIC;
    s_p.version    = PARAM_VERSION;

    s_p.servo_kp   = SERVO_KP_DEFAULT;
    s_p.servo_kd   = SERVO_KD_DEFAULT;
    s_p.servo_trim = SERVO_TRIM;

    s_p.spd_base   = SPEED_BASE_DEFAULT;
    s_p.spd_min    = SPEED_MIN_DEFAULT;
    s_p.spd_max    = SPEED_MAX_DEFAULT;

    s_p.pi_kp      = MOTOR_PI_KP_DEFAULT;
    s_p.pi_ki      = MOTOR_PI_KI_DEFAULT;

    for (i = 0; i < IND_CH_NUM; i++) {
        s_p.calib_min[i] = 0;
        s_p.calib_max[i] = NORM_ADC_FULL;
    }

    s_p.checksum = param_checksum(&s_p);
}

void Param_Init(void)
{
    Param_LoadDefault();
    s_from_eeprom = 0;

#if EEPROM_ENABLE
    {
        param_t tmp;

        if (Eeprom_Read(PARAM_EEPROM_ADDR, (uint8 *)&tmp, (uint16)sizeof(param_t)) == 0) {
            if ((tmp.magic == PARAM_MAGIC) &&
                (tmp.version == PARAM_VERSION) &&
                (tmp.checksum == param_checksum(&tmp)) &&
                param_sanity(&tmp)) {
                s_p = tmp;
                s_from_eeprom = 1;
            }
        }
    }
#endif

    Param_Apply();
}

uint8 Param_Save(void)
{
#if EEPROM_ENABLE
    s_p.magic    = PARAM_MAGIC;
    s_p.version  = PARAM_VERSION;
    s_p.checksum = param_checksum(&s_p);

    if (Eeprom_EraseSector(PARAM_EEPROM_ADDR) != 0) {
        return 1;
    }
    if (Eeprom_Write(PARAM_EEPROM_ADDR, (uint8 *)&s_p, (uint16)sizeof(param_t)) != 0) {
        return 2;
    }
    s_from_eeprom = 1;
    return 0;
#else
    return 3;   /* EEPROM 功能被 config.h 关掉了 */
#endif
}

uint8 Param_IsFromEeprom(void)
{
    return s_from_eeprom;
}

param_t *Param_Get(void)
{
    return &s_p;
}

void Param_Apply(void)
{
    norm_calib_t  c;
    motor_param_t mp;
    uint8 i;

    Servo_SetParam(s_p.servo_kp, s_p.servo_kd);
    Servo_SetTrim(s_p.servo_trim);

    mp.base    = s_p.spd_base;
    mp.min_spd = s_p.spd_min;
    mp.max_spd = s_p.spd_max;
    mp.kp      = s_p.pi_kp;
    mp.ki      = s_p.pi_ki;
    Motor_SetParam(&mp);

    for (i = 0; i < IND_CH_NUM; i++) {
        c.raw_min[i] = s_p.calib_min[i];
        c.raw_max[i] = s_p.calib_max[i];
    }
    Normalize_SetCalib(&c);
}

void Param_Capture(void)
{
    const servo_param_t *sp = Servo_GetParam();
    const motor_param_t *mp = Motor_GetParam();
    const norm_calib_t  *nc = Normalize_GetCalib();
    uint8 i;

    s_p.servo_kp   = sp->kp;
    s_p.servo_kd   = sp->kd;
    s_p.servo_trim = Servo_GetTrim();

    s_p.spd_base   = mp->base;
    s_p.spd_min    = mp->min_spd;
    s_p.spd_max    = mp->max_spd;
    s_p.pi_kp      = mp->kp;
    s_p.pi_ki      = mp->ki;

    for (i = 0; i < IND_CH_NUM; i++) {
        s_p.calib_min[i] = nc->raw_min[i];
        s_p.calib_max[i] = nc->raw_max[i];
    }

    s_p.checksum = param_checksum(&s_p);
}
