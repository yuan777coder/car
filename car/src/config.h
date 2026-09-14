#ifndef __CONFIG_H
#define __CONFIG_H

#include "common.h"

/* ==========================================================================
 *  config.h  —  算法与功能配置（与引脚无关；引脚/外设分配见 board_config.h）
 *
 *  调参顺序建议：
 *    1) 先在 config.h 里选好电感数量与布局（IND_H_NUM / VERT_IND_ENABLE）
 *    2) 到 board_config.h 里把 ADC 通道与电感一一对应
 *    3) 整车归一化标定（按键触发）
 *    4) 调舵机 PD（SERVO_KP_DEFAULT / SERVO_KD_DEFAULT）
 *    5) 调速度环（SPEED_BASE_DEFAULT 等）
 *  运行期可通过串口命令在线改参并 save 到 EEPROM，详见 docs/tuning.md。
 * ========================================================================== */

/* ------------------------------ 功能开关 ------------------------------ */
#define ENCODER_ENABLE      0    /* 1=编码器闭环调速   0=开环占空比调速 */
#define ELEMENT_ENABLE      1    /* 1=启用十字/圆环/三岔元素识别 */
#define DEBUG_UART_ENABLE   1    /* 1=启用调试串口（文本 + 虚拟示波器帧） */
#define EEPROM_ENABLE       1    /* 1=参数与标定值存入 EEPROM（掉电不丢） */
#define KEY_ENABLE          1    /* 1=启用按键 */
#define BAT_ADC_ENABLE      0    /* 1=启用电池电压采样（分压接 P0.7/ADC2 CH7） */

/* ------------------------------ 电感布局 ------------------------------ */
/* 水平电感：车头方向看，从左到右依次排列，索引 0..IND_H_NUM-1，用于算偏差。
   竖直电感：左右并排两个竖直电感，索引 IND_H_NUM..，用于元素识别。
   若你的车只有水平电感，把 VERT_IND_ENABLE 改成 0 即可。 */
#define IND_H_NUM           5
#define VERT_IND_ENABLE     1
#if VERT_IND_ENABLE
  #define IND_V_NUM         2
#else
  #define IND_V_NUM         0
#endif
#define IND_CH_NUM          (IND_H_NUM + IND_V_NUM)

#define IND_IDX_V0          (IND_H_NUM)       /* 左竖直电感下标（VERT_IND_ENABLE=1 时有效） */
#define IND_IDX_V1          (IND_H_NUM + 1)   /* 右竖直电感下标 */

/* 水平电感权值：单位 = 相邻电感间距的一半。
   下标 0 是最左电感，权值为负；最右为正。 */
#if   IND_H_NUM == 3
  #define IND_WEIGHT_LIST   { -1, 0, 1 }
  #define IND_NAME_LIST     { "L", "M", "R" }
#elif IND_H_NUM == 4
  #define IND_WEIGHT_LIST   { -3, -1, 1, 3 }
  #define IND_NAME_LIST     { "L", "ML", "MR", "R" }
#elif IND_H_NUM == 5
  #define IND_WEIGHT_LIST   { -2, -1, 0, 1, 2 }
  #define IND_NAME_LIST     { "L", "ML", "M", "MR", "R" }
#elif IND_H_NUM == 7
  #define IND_WEIGHT_LIST   { -3, -2, -1, 0, 1, 2, 3 }
  #define IND_NAME_LIST     { "L", "L2", "ML", "M", "MR", "R2", "R" }
#else
  #error "IND_H_NUM 只支持 3 / 4 / 5 / 7，请修改 config.h"
#endif

/* ------------------------------ 偏差计算 ------------------------------ */
/* 符号约定：dev > 0 表示导线在车的右侧，车需要向右打舵。
   若你的车装出来方向相反，把 DEV_DIR 改成 -1，或直接调 SERVO_DIR。 */
#define DEV_MODE            0     /* 0=归一化加权平均   1=左右差比和 */
#define DEV_DIR             1     /* 1 或 -1，整体翻转偏差符号 */
#define DEV_GAIN            500   /* 加权平均法：dev = Σ(v*w)*DEV_GAIN/Σv，满偏约 ±1000 */
#define DEV_MAX             1000  /* 偏差满量程（±） */
#define DEV_DEADZONE        5     /* 死区：|dev| 小于该值按 0 处理，抑制直线抖动 */

/* 偏差非线性修正：dev_out = dev + DEV_CURVE_K * dev * |dev| / (DEV_MAX * 100)
   电磁场强度与位置并非线性关系。0=关闭；
   正值放大两端（大偏差时打舵更狠，弯道响应更快）；
   负值压缩两端（直线更稳，但入弯偏软）。建议范围 -100 ~ +200。 */
#define DEV_CURVE_K         0

/* ------------------------------ 归一化 ------------------------------ */
#define NORM_MAX            1000   /* 归一化后的满量程 */
#define CALIB_TIME_MS       6000   /* 一次标定的持续时长（毫秒） */
#define CALIB_GAIN          100    /* 标定时最大值留的余量，100 = 不留余量，110 = 放大 10% */
#define NORM_VALID_SPAN     80     /* max-min 小于该值的通道视为无效（未接/坏） */

/* ------------------------------ 滤波 ------------------------------ */
#define ADC_FILTER_WIN      8      /* ADC 每通道滑动平均窗口长度 */
#define SPEED_LPF_SHIFT     2      /* 速度一阶低通：prev + (now-prev)/2^shift */

/* ------------------------------ 舵机 ------------------------------ */
#define SERVO_KP_DEFAULT    700    /* P：偏差 1000 对应约 700 个 PWM 计数的舵量 */
#define SERVO_KD_DEFAULT    60     /* D：抑制过冲，先用 Kp 的 1/10 起步再增 */
#define PD_DIV              1000   /* PD 输出除数：(kp*dev + kd*ddev) / PD_DIV */
#define SERVO_DIR           1      /* 1 或 -1：舵机安装方向修正 */
#define SERVO_SLEW_MAX      60     /* 每控制周期 PWM 计数最大变化量，0=不限速 */
#define SERVO_TRIM          0      /* 机械中值微调（PWM 计数，正数向右偏） */

/* ------------------------------ 电机与速度 ------------------------------ */
#define SPEED_BASE_DEFAULT  400    /* 基础速度：开环时是‰占空比，编码器模式时是目标计数 */
#define SPEED_MIN_DEFAULT   220    /* 弯道最低速度 */
#define SPEED_MAX_DEFAULT   900    /* 速度上限 */
#define SPD_CURVE_ENABLE    1      /* 1=按偏差自动减速（弯道减速） */
#define SPD_CURVE_START     150    /* |dev| 超过该值开始减速 */
#define MOTOR_DIFF_K        0      /* 差速系数（‰/偏差单位），0=两轮同速；舵机车一般填 0 */

#define MOTOR_PI_KP_DEFAULT 30     /* 编码器速度环增量式 PI */
#define MOTOR_PI_KI_DEFAULT 4
#define MOTOR_PI_DIV        100    /* 增量式 PI 缩放：out += (kp*Δe + ki*e)/DIV */
#define MOTOR_PI_OUT_LIMIT  1000   /* 速度环输出限幅（‰） */

#define MOTOR_DEADZONE      0      /* 电机启动死区（‰），给很小的占空比补一点 */
#define MOTOR_REVERSE_L     0      /* 1=左电机转向取反（接线反了才需要改） */
#define MOTOR_REVERSE_R     0      /* 1=右电机转向取反 */

/* ------------------------------ 元素识别 ------------------------------ */
/* 说明：元素判据严重依赖电感高度、间距、运放增益和赛道材质，
        config.h 里的阈值只是"能跑起来"的起点，必须现场标定。
        ELEMENT_ACTION_ENABLE = 0 时元素只做识别与显示/回传，不干预控制，
        上路验证判据正确后再改成 1 让它真正参与决策。 */
#define ELEMENT_ACTION_ENABLE 0
#define EL_CROSS_MIN_MS     40     /* 十字特征需连续保持的时间 */
#define EL_CROSS_STRONG     130    /* 总强度超过"平时"的百分比，判为十字 */
#define EL_ROUND_MIN_MS     80     /* 圆环特征最短保持时间 */
#define EL_ROUND_VERT_DIFF  250    /* 左右竖直电感差值阈值（归一化值） */
#define EL_ROUND_LOST_MAX   1500   /* 圆环内圈丢线判据：总强度上限 */
#define EL_BIFURCATE_MS     60     /* 三岔判据保持时间 */
#define EL_BASELINE_SHIFT   6      /* 平时总强度基线的低通系数 */

/* ------------------------------ 按键 ------------------------------ */
#define KEY_LONG_MS         800    /* 长按判定时间 */
#define KEY_SCAN_MS         10     /* 按键扫描周期 */

/* ------------------------------ 调试 ------------------------------ */
#define UART_BAUD           115200
#define DBG_TEXT_PERIOD_MS  100    /* 文本调试信息输出周期 */
/* 虚拟示波器通道：
   0=偏差  1=舵机PWM  2=左速度  3=右速度  4=水平总强度  5..=各水平电感归一化值 */
#define SCOPE_CH_NUM        (5 + IND_H_NUM)

#endif /* __CONFIG_H */
