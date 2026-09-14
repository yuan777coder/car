#include "ui.h"
#include "oled.h"
#include "car.h"
#include "deviation.h"
#include "element.h"
#include "servo.h"
#include "motor.h"
#include "param.h"
#include "normalize.h"
#include "timer.h"

/* ==========================================================================
 *  OLED 界面
 *
 *  1 个字符 = 6x8 像素，128 宽一行最多 21 个字符；8 行（page 0~7）。
 *  所有文本一律用英文/数字缩写，因为字库只有 ASCII —— 中文字库要 16x16
 *  点阵，一个汉字 32 字节，几十个字就上 KB，对这个小工程不划算。
 * ========================================================================== */

#define UI_REFRESH_MS   100

/* 水平电感名字：L / ML / M / MR / R，用 2D 数组而不是指针数组，
   这样整张表都放在 code 段，不占 RAM */
static char code s_hname[IND_H_NUM][3] = IND_NAME_LIST;

static ui_page_t s_page;
static uint32    s_last_ms;
static uint32    s_msg_end_ms;
static const char *s_msg1;
static const char *s_msg2;

static const char *ui_mode_name(sys_mode_t m)
{
    switch (m) {
        case MODE_RUN:   return "RUN";
        case MODE_CALIB: return "CALIB";
        case MODE_DEBUG: return "DEBUG";
        default:         return "STOP";
    }
}

/* 右对齐显示一个数：width = 占用字符数 */
static void ui_num_right(uint8 page, int32 v, uint8 width)
{
    if ((uint16)width * 6U > OLED_WIDTH) {
        width = (uint8)(OLED_WIDTH / 6U);
    }
    Oled_ShowInt((uint8)(OLED_WIDTH - width * 6U), page, v, width);
}

/* ------------------------------ MAIN ------------------------------ */
static void ui_draw_main(void)
{
    const dev_info_t       *d  = Deviation_Get();
    const element_info_t   *el = Element_Get();
    const uint16           *nr = Car_GetNorm();
    uint8 i;

    Oled_ShowString(0, 0, "IND");
    for (i = 0; i < IND_H_NUM; i++) {
        Oled_ShowString((uint8)(24U + (uint16)i * 20U), 0, s_hname[i]);
    }
    for (i = 0; i < IND_H_NUM; i++) {
        Oled_ShowUInt((uint8)((uint16)i * 24U), 1, nr[i], 4);
    }

    Oled_ShowString(0, 2, "DEV");
    ui_num_right(2, (int32)d->dev, 5);

    Oled_ShowString(0, 3, "SRV");
    ui_num_right(3, (int32)Servo_GetPwm(), 5);

    Oled_ShowString(0, 4, "SPD");
    Oled_ShowInt(48, 4, (int32)Motor_GetSpeedLeft(), 4);
    Oled_ShowInt(96, 4, (int32)Motor_GetSpeedRight(), 4);

    Oled_ShowString(0, 5, "TGT");
    Oled_ShowInt(48, 5, (int32)Motor_GetTargetLeft(), 4);
    Oled_ShowInt(96, 5, (int32)Motor_GetTargetRight(), 4);

    Oled_ShowString(0, 6, "EL ");
    Oled_ShowString(24, 6, Element_Name(el->id));

    Oled_ShowString(0, 7, "MODE");
    Oled_ShowString(30, 7, ui_mode_name(Car_GetMode()));
    if (d->lost) {
        Oled_ShowString(96, 7, "LOST");
    }
}

/* ------------------------------ ADC ------------------------------ */
static void ui_draw_adc(void)
{
    const uint16         *raw = Car_GetAdcRaw();
    const uint16         *nr  = Car_GetNorm();
    const element_info_t *el  = Element_Get();
    uint8 i;
    uint8 page = 1;

    Oled_ShowString(0, 0, "CH    RAW   NRM");

    for (i = 0; i < IND_H_NUM; i++) {
        Oled_ShowString(0, page, s_hname[i]);
        Oled_ShowUInt(30, page, raw[i], 5);
        Oled_ShowUInt(84, page, nr[i], 5);
        page++;
        if (page > 6U) {
            break;
        }
    }

#if VERT_IND_ENABLE
    Oled_ShowString(0, 6, "VL");
    Oled_ShowUInt(24, 6, nr[IND_IDX_V0], 4);
    Oled_ShowString(60, 6, "VR");
    Oled_ShowUInt(84, 6, nr[IND_IDX_V1], 4);
#endif

    Oled_ShowString(0, 7, "SUM");
    Oled_ShowUInt(30, 7, el->total, 5);
    Oled_ShowString(78, 7, "B");
    Oled_ShowUInt(90, 7, el->baseline, 4);
}

/* ------------------------------ MOTOR ------------------------------ */
static void ui_draw_motor(void)
{
    const motor_param_t *mp = Motor_GetParam();

    Oled_ShowString(0, 0, "MOTOR / SPEED");

    Oled_ShowString(0, 1, "TGT L");
    ui_num_right(1, (int32)Motor_GetTargetLeft(), 5);

    Oled_ShowString(0, 2, "TGT R");
    ui_num_right(2, (int32)Motor_GetTargetRight(), 5);

    Oled_ShowString(0, 3, "OUT L");
    ui_num_right(3, (int32)Motor_GetOutLeft(), 5);

    Oled_ShowString(0, 4, "OUT R");
    ui_num_right(4, (int32)Motor_GetOutRight(), 5);

    Oled_ShowString(0, 5, "SPD L");
    ui_num_right(5, (int32)Motor_GetSpeedLeft(), 5);

    Oled_ShowString(0, 6, "SPD R");
    ui_num_right(6, (int32)Motor_GetSpeedRight(), 5);

    Oled_ShowString(0, 7, "KP");
    Oled_ShowUInt(18, 7, (uint32)(uint16)mp->kp, 4);
    Oled_ShowString(54, 7, "KI");
    Oled_ShowUInt(72, 7, (uint32)(uint16)mp->ki, 4);
}

/* ------------------------------ STATUS ------------------------------ */
static void ui_draw_status(void)
{
    const element_info_t *el = Element_Get();
    const servo_param_t  *sp = Servo_GetParam();
    uint32 ms = Timer_GetMs();

    Oled_ShowString(0, 0, "SYSTEM STATUS");

    Oled_ShowString(0, 1, "MODE");
    Oled_ShowString(48, 1, ui_mode_name(Car_GetMode()));

    Oled_ShowString(0, 2, "PARAM");
    Oled_ShowString(48, 2, Param_IsFromEeprom() ? "EEPROM" : "DEFAULT");

    Oled_ShowString(0, 3, "CALIB");
    Oled_ShowString(48, 3, Param_IsFromEeprom() ? "LOADED" : "UNCAL");

    Oled_ShowString(0, 4, "EL");
    Oled_ShowString(48, 4, Element_Name(el->id));

    Oled_ShowString(0, 5, "VDIFF");
    ui_num_right(5, (int32)el->vdiff, 5);

    Oled_ShowString(0, 6, "SKP");
    Oled_ShowUInt(24, 6, (uint32)(uint16)sp->kp, 4);
    Oled_ShowString(54, 6, "SKD");
    Oled_ShowUInt(78, 6, (uint32)(uint16)sp->kd, 4);

    Oled_ShowString(0, 7, "T");
    Oled_ShowFix(12, 7, (int32)(ms / 100UL), 1, 6);   /* 秒，保留 1 位小数 */
}

/* ------------------------------ 标定进度 ------------------------------ */
static void ui_draw_calib(void)
{
    uint8 p = Normalize_CalibProgress();

    Oled_ShowStringBig(24, 0, "CALIB");
    Oled_ShowString(0, 3, "Push the car along");
    Oled_ShowString(0, 4, "the track 6s ...");
    Oled_DrawProgress(0, 6, 128, p);
    Oled_ShowUInt(54, 5, (uint32)p, 3);
    Oled_ShowString(72, 5, "%");
}

/* ------------------------------ 对外接口 ------------------------------ */

void Ui_Init(void)
{
    s_page       = UI_PAGE_MAIN;
    s_last_ms    = 0;
    s_msg_end_ms = 0;
    s_msg1       = 0;
    s_msg2       = 0;
}

void Ui_SetPage(ui_page_t p)
{
    if ((uint8)p < (uint8)UI_PAGE_COUNT) {
        s_page = p;
    }
}

void Ui_NextPage(void)
{
    uint8 p = (uint8)s_page;

    p++;
    if (p >= (uint8)UI_PAGE_COUNT) {
        p = 0;
    }
    s_page = (ui_page_t)p;
}

ui_page_t Ui_GetPage(void)
{
    return s_page;
}

void Ui_ShowMessage(const char *line1, const char *line2, uint16 ms)
{
    s_msg1       = line1;
    s_msg2       = line2;
    s_msg_end_ms = Timer_GetMs() + (uint32)ms;
}

void Ui_Tick(void)
{
    uint32 now = Timer_GetMs();

    if ((uint32)(now - s_last_ms) < (uint32)UI_REFRESH_MS) {
        return;
    }
    s_last_ms = now;

    Oled_Clear();

    if (Car_IsCalibrating()) {
        ui_draw_calib();
    } else if ((s_msg1 != 0) && ((int32)(s_msg_end_ms - now) > 0)) {
        Oled_ShowStringBig(14, 2, (s_msg1 != 0) ? s_msg1 : "");
        Oled_ShowStringBig(14, 4, (s_msg2 != 0) ? s_msg2 : "");
    } else {
        s_msg1 = 0;
        s_msg2 = 0;
        switch (s_page) {
            case UI_PAGE_ADC:    ui_draw_adc();    break;
            case UI_PAGE_MOTOR:  ui_draw_motor();  break;
            case UI_PAGE_STATUS: ui_draw_status(); break;
            default:             ui_draw_main();   break;
        }
    }

    Oled_Refresh();
}
