# STC32G144K246 内核级寄存器核查报告
## 系统时钟 / GPIO / 中断向量 / 定时器 / ADC / 类型别名 / 芯片规格

> **目标芯片**：STC32G144K246（STC 32 位 8051/251 内核 MCU，Keil uVision **C251** 工具链）
> **核查方式**：**以本机实际安装物为第一手证据**（STC-ISP v6.96P 安装的官方头文件、STC-ISP 可执行文件内嵌的代码生成模板、两份针对本芯片的第三方开源库源码），再加 web 检索到的 STC 官方/数据手册片段做交叉印证。
> **诚实性声明**：凡本轮**没有拿到一手证据**的条目，一律显式标注 **【未确认】**，不做推测性断言。数据手册形态限制见 §5.1.3。

---

## 0. 首要前提：`STC32G.H` ≠ 本芯片的头文件

**结论**

| 头文件 | 适用芯片 | 本机实际路径 | 大小 |
|---|---|---|---|
| `STC32G.H` | **STC32G12K128 / STC32G12K64 / STC32G8K64** 等老 STC32G | `D:\k5\C251\INC\STC\STC32G.H` | 74 939 B |
| **`STC32G144K246.H`** | **STC32G144K246（本芯片）** | `C:\Keil_v5\C51\INC\STC\STC32G144K246.H`（xdata 版）<br>`D:\k5\C251\INC\STC\STC32G144K246.H`（far 版，180 724 B） | 178 306 B / 180 724 B |
| `stc32g144k246.h` | STC32G144K246（逐飞开源库自带副本，宏卫 `__STC32G144K_H__`） | `…\STC32G144K246_100Pin_Library-master\Example\Coreboard_Demo\libraries\zf_common\stc32g144k246.h` | 170 588 B |

**证据**

* STC-ISP 可执行文件 `C:\Users\y\Desktop\AiCube-ISP-v6.96P\AiCube-ISP-v6.96P.exe` 的字符串表里，**同时**列出它要安装的头文件清单（实测偏移 `1393496` / `1393640` 附近）：
  `… STC8A8K64S4A12.H, STC8.H, STC15.H, AI32G.INC, AI32G.H, AI8C.INC, AI8C.H, … STC32G.INC, **STC32G.H**, … STC8H.INC, **STC32G144K246.INC, STC32G144K246.H**, STC8051U.INC, STC8051U.H, …`
  → 官方把两者当**不同芯片族**分别分发，这是 STC-ISP v6.96P（2026 年版本）的行为。
* 目录实测：`D:\k5\C251\INC\STC\` 下两个文件**并存**。

**必须记住的三处差异（会直接写错代码的地方）**

| 项目 | `STC32G.H`（STC32G12K128） | `STC32G144K246.H`（本芯片） |
|---|---|---|
| T4T3M / T3T4M 地址 | `0xDD` | `0xDD`（**一致**；注意 **STC8H 是 `0xD1`**，别照抄 STC8H） |
| 位定义风格（C251 目录版） | `sbit ADC_POWER = ADC_CONTR^7;` | `sbit ADC_POWER = ADC_CONTR^7;`（**一致**） |
| 位定义风格（C51 目录版） | `sbit …` | `#define ADC_POWER 0x80` 等 **宏**风格 |
| 中断向量 14/15 | `BRK_VECTOR 14`、`ICEP_VECTOR 15` **有** | **没有**（从 `USER_VECTOR 13` 直接跳到 `INT4_VECTOR 16`） |
| ADC 模块 | 1 个 ADC | **双 ADC**（`ADC_CONTR` + `ADC2_CONTR`） |

> ⚠️ **`ADC_POWER` 是 `sbit` 还是 `#define` 会改变写法**：
> `sbit` 版可写 `ADC_POWER = 1;`；`#define` 版只能写 `ADC_CONTR |= ADC_POWER;`。
> 本报告统一用 `ADC_CONTR |= 0x80;` 这种**两种版本都能编过**的写法。
> 建议本工程固定 `#include "STC32G144K246.H"`（C251 目录下的 far 版）。

---

## 1. 系统时钟

### 1.1 结论

| 项目 | 结论 |
|---|---|
| IRC 频率微调寄存器 | **有**：`IRTRIM`（SFR `0x9F`）、`IRCBAND`（SFR `0x9D`）、`LIRTRIM`（SFR `0x9E`，低速 IRC 微调） |
| `IRCBAND` 位域 | `USBCKS 0x80`、`USBCKS2 0x40`、`HIRCSEL1 0x02`、`HIRCSEL0 0x01`（**只有 2 位频段选择**） |
| **`WTST` 等待周期寄存器** | **有**，`sfr WTST = 0xE9;`（STC32G 有，**STC8H 没有**） |
| 时钟源选择 | `CLKSEL`（XFR `0x7EFE00`）、`CLKDIV`（`0x7EFE01`）、`HIRCCR`、`XOSCCR`、`IRC32KCR`、`IRC48MCR`、`HSCLKDIV`、`HPLLCR`、`HPLLPDIV`、`HPLL2CR`、`HPLL2PDIV` —— **全在 XFR，访问前必须 `P_SW2 \|= 0x80`（EAXFR=1）** |
| STC-ISP 可选 IRC 频率 | 见下 §1.3；**24 / 30 / 35 / 40 MHz 均在列表中，用户给的这 4 个值是对的** |
| 运行时改 IRC 频率（24/30/35/40MHz） | **【未确认】** 没拿到官方的运行时 `IRTRIM/IRCBAND` 写入例程 |
| ≥60 MHz 时钟建立（本芯片） | 用 HPLL，方法见 §1.4（来自逐飞库，逐行可查） |
| `WTST` 取值规则 | 分芯片不同：STC-ISP 文档对 STC32G12K128 的规定见 §1.2；**STC32G144K246 的官方 WTST 表【未确认】** |
| STC-ISP 生成的 `STC32G_Init()` 时钟段原文 | **【未确认】** STC-ISP exe 里没有明文的 `STC32G_Init`（实测 `STC32G_Init` 出现 0 次、`IRTRIM` 0 次、`EAXFR` 0 次），范例 C 代码存放在压缩资源里。**不要照抄网上记忆版**。 |

### 1.2 证据 / 来源

**(a) 本机官方头文件（一手）** —— `C:\Keil_v5\C51\INC\STC\STC32G144K246.H`
```c
131: sfr         IRCBAND     =           0x9d;
132:     #define USBCKS                  0x80
133:     #define USBCKS2                 0x40
134:     #define HIRCSEL1                0x02
135:     #define HIRCSEL0                0x01
137: sfr         LIRTRIM     =           0x9e;
138: sfr         IRTRIM      =           0x9f;
449: sfr         WTST        =           0xe9;
450: sfr         CKCON       =           0xea;
451:     #define RAMEXE                  0x80
267: sfr         P_SW2       =           0xba;
268:     #define EAXFR                   0x80
571: #define     CLKSEL      (*(unsigned char volatile xdata *)0xfe00)
572: #define     CLKDIV      (*(unsigned char volatile xdata *)0xfe01)
573: #define     HIRCCR      (*(unsigned char volatile xdata *)0xfe02)
574: #define     XOSCCR      (*(unsigned char volatile xdata *)0xfe03)
575: #define     IRC32KCR    (*(unsigned char volatile xdata *)0xfe04)
576: #define     MCLKOCR     (*(unsigned char volatile xdata *)0xfe05)
577: #define     IRCDB       (*(unsigned char volatile xdata *)0xfe06)
578: #define     IRC48MCR    (*(unsigned char volatile xdata *)0xfe07)
580: #define     IRC48MATRIM (*(unsigned char volatile xdata *)0xfe09)
581: #define     IRC48MBTRIM (*(unsigned char volatile xdata *)0xfe0a)
582: #define     HSCLKDIV    (*(unsigned char volatile xdata *)0xfe0b)
583: #define     HPLLCR      (*(unsigned char volatile xdata *)0xfe0c)
584: #define     HPLLPDIV    (*(unsigned char volatile xdata *)0xfe0d)
585: #define     HPLL2CR     (*(unsigned char volatile xdata *)0xfe0e)
586: #define     HPLL2PDIV   (*(unsigned char volatile xdata *)0xfe0f)
```
对照 `C:\Keil_v5\C51\INC\STC\STC8H.H`：`IRCBAND 0x9d / LIRTRIM 0x9e / IRTRIM 0x9f / CLKSEL 0xfe00 / CLKDIV 0xfe01 / ADCTIM 0xfea8` **完全相同**，但 **STC8H.H 里没有 `WTST`**。→ 与你的预判（STC32G 与 STC8H 高度一致）吻合，唯一新增的是 `WTST`。

*本地官方头文件（一手）*：`C:\Keil_v5\C51\INC\STC\STC32G144K246.H`

**(b) WTST 的 STC-ISP 官方文档文本（一手，来自 exe 内嵌帮助，GBK 解码后偏移 ≈1431286）**
```
… The program space of this MCU produced from 2022-2-24 to 2022-3-21 is 63K,
    and the EEPROM is fixed to 1K, which cannot be modified …
    当 CPU 频率 <= 52MHz 时, WTST 取 1
    当 CPU 频率 >  52MHz 时, WTST 取 2
    WTST = 1 when CPU frequency <= 52MHz;
    WTST = 2 when CPU frequency > 52MHz;
```
> 这段文本的上下文是 **STC32G12K128（63K 程序空间那批芯片）**，**不能直接套到 STC32G144K246**。STC32G144K246 的官方 WTST 表我没有拿到 → **【未确认】**。

**(c) STC32G144K246 厂商运行时时钟代码（一手源码）**
`…\STC32G144K246_100Pin_Library-master\Example\Coreboard_Demo\libraries\zf_common\zf_common_clock.c`
```c
void clock_init (uint32 clock)                                  // 核心时钟初始化
{
    uint8 pll_div = ((clock / 1000000UL - 78) / 3);
    interrupt_global_disable();
    EAXFR = 1;                          // 使能访问 XFR
    if((120*1000*1000UL) <= clock)      { WTST = 4; }
    else if((80*1000*1000UL) <= clock)  { WTST = 3; }
    else if((80*1000*1000UL) <= clock)  { WTST = 2; }   // ← 厂商自己的重复条件，此分支不可达
    else                                { WTST = 4; }
#if(INTERNAL_CRYSTAL == USER_CRTSTAL)
    CLKDIV = 2;                         // 系统时钟 = HPLL/2/2
    VRTRIM = CHIPID22;                  // 载入 27MHz 频段的 VRTRIM 值
    IRTRIM = CHIPID12;                  // 指定当前 HIRC 为 24MHz
    IRCBAND &= ~0x03;                   // 清空频段选择
    IRCBAND |= 0x01;                    // 选择 27MHz 频段
    HPLLCR &= ~0x10;                    // HPLL 输入时钟源 = HIRC
    HPLLPDIV = 4;                       // 24MHz/4 = 6MHz，要求输入 HPLL 时钟在 6MHz 附近
    HPLLCR |= pll_div & 0x0F;           // 设置 PLL1 倍频系数
    HPLLCR |= 0x80;                     // 使能 HPLL
    soft_delay(50000);
    CLKSEL &= ~0x03;                    // BASE_CLK 选择 HIRC
    CLKSEL &= ~0x0c;                    // 清空主时钟源选择
    CLKSEL |= 1<<2;                     // 主时钟源 = 内部 HPLL1 输出/2
#elif(EXTERNAL_CRYSTAL == USER_CRTSTAL)
    CLKDIV = 2;
    CLKSEL &= ~0x03;
    CLKSEL &= ~0x0c;
    XOSCCR = 3<<6 | 1<<2 | 1<<5;        // 外部晶振 24MHz
    soft_delay(50000);
    while (!(XOSCCR & 1));              // 等时钟稳定
    HPLLCR &= ~(1<<4);
    HPLLPDIV = 4;
    HPLLCR |= pll_div & 0x0F;
    HPLLCR |= 1<<7;                     // 使能 HPLL
    CLKSEL = 1<<0 | 1<<2;               // 外部高速晶振 + HPLL1/2
#endif
    ADCCFG = 0;  AUXR = 0;  SCON = 0;  S2CON = 0;  S3CON = 0;  S4CON = 0;
    P_SW1 = 0;   IE2  = 0;  TMOD = 0;
    system_clock = clock;
}
```
该库可选主频常量（`zf_common_clock.h`）：
`SYSTEM_CLOCK_78M / 81 / 84 / 87 / 90 / 93 / 96 / 99 / 102 / 105 / 108 / 111 / 114 / 120 / 124 MHz`
→ **本芯片的正常工作区间是 78~124MHz（3MHz 步进），没有 24/30/35/40MHz 档**。你要的 24/30/35/40MHz 属于 STC-ISP 的 IRC 档位（见下），不是这颗芯片库函数的推荐工作点。

**(d) STC-ISP 内置 IRC 频率档位表（一手，exe 字符串表，GBK 解码后偏移 ≈ 一处连续字符串）**
```
11.0592  60.000  55.296  49.152  56.000  52.000  50.8032  48.000  45.1584
32.000   45.000  44.2368 40.000  36.864  35.000  33.1776  30.000  27.000
24.000   22.1184 20.000  18.432  12.000   6.000   5.5296   1.000
```
→ **24.000 / 30.000 / 35.000 / 40.000 MHz 确实都在 STC-ISP 的可选 IRC 档位里**（这些值由 STC-ISP 在**下载时**写入芯片的 IRC 微调选项，属于 ISP 编程选项，不是运行时寄存器）。

**(e) STC-ISP 生成的 GPIO 代码风格（一手，exe 代码生成模板字符串）**
```
    P%dM0 = 0x%02x; P%dM1 = 0x%02x;      ← 例：P0M0 = 0x00; P0M1 = 0x00;
    P%dM1 = (P%dM1 & ~0x%02x) | 0x%02x;
    P%dM0 |= 0x%02x;
```
→ 说明 STC-ISP 生成的初始化代码就是【整体赋值 + 读改写】两种形式，`WTST`/`EAXFR`/`CKCON` 那几句属于 `STC32G_Init()`，其**原文未取到**。

**来源 URL**
* STC 官方 STC32G144K246 产品页（规格、ADC 通道数、封装）：https://www.stcmicro.com/stc/stc32g144k246.html
* STC32G 数据手册（另有数据库索引到该 PDF 文本）：http://www.stcmcudata.com/STC8F-DATASHEET/STC32G.pdf
* STC-ISP 生成的 STC8H/STC32G 时钟写法讨论（社区）：https://www.stcaimcu.com/forum.php?mod=viewthread&tid=9704
* 本机文件（一手，可自行复核）：
  * `C:\Keil_v5\C51\INC\STC\STC32G144K246.H`（SHA-256 `250942FC…AD1FAF`，178 306 B）
  * `D:\k5\C251\INC\STC\STC32G144K246.H`（C251/far 版，180 724 B）
  * `D:\k5\C251\INC\STC\STC32G.H`（SHA-256 `0CAAA8FA…6E2A6`，74 939 B）
  * `C:\Users\y\Desktop\AiCube-ISP-v6.96P\AiCube-ISP-v6.96P.exe`

### 1.3 可直接使用的 C 代码

```c
/* ==========================================================================
 *  clock.c  ——  STC32G144K246 系统时钟 / 访问速度初始化
 *  编译环境：Keil uVision C251，头文件 STC32G144K246.H（C251 目录版）
 * ========================================================================== */
#include "STC32G144K246.H"

#define FOSC_HZ   24000000UL        /* 24MHz，须与 STC-ISP 下载时选择的 IRC 档位一致 */

/* --------------------------------------------------------------------------
 *  基础初始化：XFR 访问使能 + Flash 等待周期 + XRAM 访问速度
 *
 *  【必读】WTST 是 STC32G 特有的“Flash 等待周期”寄存器（STC8H 没有）。
 *   STC-ISP 对 STC32G12K128 的官方说明是：
 *       CPU 频率 <= 52MHz -> WTST = 1
 *       CPU 频率 >  52MHz -> WTST = 2
 *   本芯片 STC32G144K246 的官方 WTST 表【未确认】；厂商逐飞库在
 *       96MHz 用 WTST=3，120MHz 用 WTST=4。
 *   下面按“<=52MHz 取 1”的官方口径给值，并以 STC-ISP 下载界面显示的
 *   "WTST 初值" 为准（exe 帮助文本里有 "Set WTST initial value" 这一项）。
 * -------------------------------------------------------------------------- */
void Clock_BasicInit(void)
{
    P_SW2 |= 0x80;      /* EAXFR = 1：允许访问 XFR(0x7E:xxxx) 扩展 SFR，
                           否则 CLKSEL/CLKDIV/TM0PS/ADCTIM 等读写无效          */
    CKCON  = 0x00;      /* RAMEXE = 0；STC-ISP 注释：“提高访问 XRAM 速度”        */
    WTST   = 1;         /* 24MHz (<=52MHz) 官方口径取 1                          */
}

/* --------------------------------------------------------------------------
 *  >= 60MHz 的 HPLL 时钟建立（逐飞库验证过的流程，逐行对应）
 *  参数 clock_hz：目标系统频率，库支持 78/81/…/120/124 MHz
 *  要求：HPLL 的输入基准必须落在 6MHz 附近（这里用 HIRC 24MHz / HPLLPDIV=4）
 * -------------------------------------------------------------------------- */
void Clock_InitHpLL(unsigned long clock_hz)
{
    unsigned char pll_div = (unsigned char)((clock_hz / 1000000UL - 78) / 3);

    P_SW2 |= 0x80;                      /* EAXFR = 1 */

    if      (clock_hz >= 120000000UL) WTST = 4;   /* 厂商经验值，非官方表格 */
    else if (clock_hz >=  80*1000*1000UL) WTST = 3;
    else                                  WTST = 2;

    CLKDIV = 2;                         /* 系统时钟 = HPLL/2/2 */

    /* --- 让 HIRC 精确工作在 24MHz / 27MHz 频段（用芯片出厂 CHIPID 里的微调值）--- */
    VRTRIM  = CHIPID22;                 /* 27MHz 频段的 VRTRIM（CHIPID22 由头文件提供）*/
    IRTRIM  = CHIPID12;                 /* 把 HIRC 指定到 24MHz */
    IRCBAND &= (unsigned char)~0x03;    /* 清空 HIRCSEL[1:0] */
    IRCBAND |= 0x01;                    /* 选 27MHz 频段 */

    /* --- 配置并使能 HPLL1 --- */
    HPLLCR  &= (unsigned char)~0x10;    /* HPLL 输入时钟源 = HIRC */
    HPLLPDIV = 4;                       /* 24MHz/4 = 6MHz（务必让输入靠近 6MHz）*/
    HPLLCR  |= (unsigned char)(pll_div & 0x0F);  /* 倍频系数 */
    HPLLCR  |= 0x80;                    /* 使能 HPLL */

    {   unsigned int i; for (i = 0; i < 50000u; i++) { /* soft_delay */ } }

    /* --- 切换主时钟源到 HPLL1/2 --- */
    CLKSEL &= (unsigned char)~0x03;     /* BASE_CLK = HIRC（供 HPLL 用）*/
    CLKSEL &= (unsigned char)~0x0C;     /* 清主时钟源选择 */
    CLKSEL |= (1 << 2);                 /* 主时钟源 = 内部 HPLL1 输出 / 2 */
}
```

> **运行时要 24 / 30 / 35 / 40MHz 怎么办？**
> 这 4 个值在 STC-ISP 里是**下载时**的 IRC 档位选项；我在本机 + web 上**没有**拿到 STC32G144K246「运行时把 IRC 从默认频率改成这 4 个值」的官方 `IRTRIM/IRCBAND` 常量表 → **【未确认】**。
> 实操建议：**下载时选好频率**，然后按 §1.2(b) 的口径设 `WTST`，并用串口/示波器实测校验。若一定要运行时改，请先用 STC-ISP「定时器计算器」反推：同一个定时器在已知频率下的重装值可以反推出真实 SYSclk。

---

## 2. GPIO 模式配置

### 2.1 结论

| `PxM1` | `PxM0` | 模式 | 证据强度 |
|---|---|---|---|
| 0 | 0 | **准双向口**（弱上拉，传统 8051 口） | 官方 4 模式列表确认；两个厂商库都**未**显式使用该组合 → 见下方说明 |
| 0 | 1 | **推挽输出**（强推挽） | **两份独立库源码实测一致** |
| 1 | 0 | **高阻输入** | **两份独立库源码实测一致** |
| 1 | 1 | **开漏输出** | **两份独立库源码实测一致** |

* **端口范围**：SFR 位寻址口 **P0(0x80) ~ P7(0xF8)**，`P0M0/P0M1 … P7M0/P7M1` 也全是 SFR。
  **P8/P9/PA/PB/PC 没有 SFR**，只能通过 XFR 访问（`P8M0/P9M0/PAM0/PBM0` 在 `0x7EF710` 起，`P8M1/…` 在 `0x7EF718` 起；数据口 `P8OUT/P9OUT/PAOUT/PBOUT` 在 `0x7EF700` 起，输入 `P8IN…` 在 `0x7EF708` 起）。
* **官方规格的 GPIO 上限**：`P0.0~P0.7, P1.0~P1.7, … P5.0~P5.4, P6.0~P6.7, P7.0~P7.7, P8…, P9…, PA…, PB…` 共 **最多 91 个**。
* **上电默认模式**：STC 官方英文页写 *“High-impedance input by default upon power-on”*。
  ⚠️ 这与「STC 传统 `PxM0/PxM1` 复位值 = `0x00` ⇒ 准双向口」的通行口径**存在矛盾**，两者我都原文记录，**以实测/STC-ISP 显示为准**。
* 附加控制寄存器（都在 XFR，**必须先 `P_SW2 \|= 0x80`**）：`PxPU`(上拉 `0xFE10+port`)、`PxPD`(下拉 `0xFE40+port`)、`PxSR`(电平转换速度 `0xFE20+port`)、`PxDR`(驱动能力 `0xFE28+port`)、`PxIE`(数字输入使能 `0xFE30+port`)、`PxNCS`(施密特触发 `0xFE18+port`)。
  **ADC 模拟脚必须：高阻输入 + 关闭数字输入（`PxIE` 对应位清 0）**——否则采样值会飘，这是两个厂商库共同的做法。

### 2.2 证据 / 来源

**(a) 逐飞库推挽/开漏/高阻三种模式的原文** —— `…\Coreboard_Demo\libraries\zf_driver\zf_driver_gpio.c`
```c
// 推挽输出配置（PnM0置位，PnM1清零）
case IO_P00: P0M1 &= ~(1 << pin_bit); P0M0 |= (1 << pin_bit); break;
// 开漏输出配置（PnM0置位，PnM1置位）
case IO_P00: P0M1 |= (1 << pin_bit); P0M0 |= (1 << pin_bit); break;
// 统一高阻输入配置（PnM1置位，PnM0清零）
case IO_P00: P0M1 |= (1 << pin_bit); P0M0 &= ~(1 << pin_bit); break;
```
同一文件里 **P8/P9/PA/PB 走 `P8M1/P8M0 … PAM1/PAM0`，读写走 `P8OUT/P8IN …`** —— 与「P8 以上不是 SFR」一致。

**(b) 科宇库同结论** —— `https://raw.giteeusercontent.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary/raw/5569362949cbd2be0b72738855776aa389ce4cba/app_examples/library/drivers/ky_gpio.c`
```c
switch (mode) {
case GPIO_MODE_IN_FLOATING: valM1 = 1; valM0 = 0; break;   /* 高阻输入 */
case GPIO_MODE_IN_PU:       valM1 = 1; valM0 = 0; valPU = 1; break;
case GPIO_MODE_IN_PD:       valM1 = 1; valM0 = 0; valPD = 1; break;
case GPIO_MODE_OUT_PP:      valM1 = 0; valM0 = 1; break;   /* 推挽输出 */
case GPIO_MODE_OUT_OD:      valM1 = 1; valM0 = 1; break;   /* 开漏输出 */
case GPIO_MODE_OUT_OD_PU:   valM1 = 1; valM0 = 1; valPU = 1; break;
}
```
并给出 XFR 地址宏（可用于复核 §2.1 的地址）：
```c
#define ADDR_PU(port) (unsigned char volatile far *)((port < 8) ? (0x7EFE10 + port) : (0x7EF9C0 + (port - 8)))
#define ADDR_PD(port) (unsigned char volatile far *)((port < 8) ? (0x7EFE40 + port) : (0x7EF9F0 + (port - 8)))
#define ADDR_SR(port) (unsigned char volatile far *)((port < 8) ? (0x7EFE20 + port) : (0x7EF9D0 + (port - 8)))
#define ADDR_DR(port) (unsigned char volatile far *)((port < 8) ? (0x7EFE28 + port) : (0x7EF9D8 + (port - 8)))
#define ADDR_IE(port) (unsigned char volatile far *)((port < 8) ? (0x7EFE30 + port) : (0x7EF9E0 + (port - 8)))
```

**(c) 官方 4 模式与 P8~PB 引脚列表** —— https://www.stcmicro.com/stc/stc32g144k246.html
> “Supports 4 modes: Quasi-bidirectional, Push-pull output, Open-drain, High-impedance input (High-impedance input by default upon power-on).”
> “Up to 91 GPIOs: P0.0~P0.7, P1.0~P1.7, P2.0~P2.7, P3.0~P3.7, P4.0~P4.7, P5.0~P5.4, P6.0~P6.7, P7.0~P7.7, P8.0~P8.7, P9.0~P9.7, PA.0~PA.7, PB.0~PB.7.”

**(d) 本地头文件（一手）** —— `STC32G144K246.H`
```c
 90: sfr P1M1 = 0x91;   91: sfr P1M0 = 0x92;
 92: sfr P0M1 = 0x93;   93: sfr P0M0 = 0x94;
 94: sfr P2M1 = 0x95;   95: sfr P2M0 = 0x96;
220: sfr P3M1 = 0xb1;  221: sfr P3M0 = 0xb2;
222: sfr P4M1 = 0xb3;  223: sfr P4M0 = 0xb4;
338: sfr P5M1 = 0xc9;  339: sfr P5M0 = 0xca;
340: sfr P6M1 = 0xcb;  341: sfr P6M0 = 0xcc;
423: sfr P7M1 = 0xe1;  424: sfr P7M0 = 0xe2;
516: sfr P7   = 0xf8;      /* ← SFR 端口到 P7 为止 */
588: #define P0PU (*(unsigned char volatile xdata *)0xfe10)
606: #define P0SR (*(unsigned char volatile xdata *)0xfe20)
615: #define P0DR (*(unsigned char volatile xdata *)0xfe28)
624: #define P0IE (*(unsigned char volatile xdata *)0xfe30)
633: #define P0PD (*(unsigned char volatile xdata *)0xfe40)
1913: #define P8M0 (*(unsigned char volatile xdata *)0xf710)   /* P8 起只能 XFR */
1922: #define P8M1 (*(unsigned char volatile xdata *)0xf718)
```
（注意：头文件里 `P8M0` 写的是 `0xf710`，科宇库写的是 `0x7EF710`——同一地址的两种书写法，`0x7E` 段基址。）

**未确认项**
* 「准双向口 = `M1=0, M0=0`」：官方 4 模式列表确认存在"准双向"，但**两个厂商库都没有给出该组合的代码**，我也没拿到官方寄存器表原文 → 标注为**高置信度推断，非一手确认**。
* 复位默认模式：官方页说高阻，STC 传统口径说准双向 → **两者冲突，未确认**。

### 2.3 可直接使用的 C 代码

```c
/* ==========================================================================
 *  gpio.c  ——  STC32G144K246 端口模式设置
 * ========================================================================== */
#include "STC32G144K246.H"

/* 只需一个引脚时（示例：P1.0 推挽输出，驱动电机方向/PWM 脚） */
#define P10_PUSH_PULL()   do { P1M1 &= (unsigned char)~0x01; P1M0 |= 0x01; } while (0)
#define P10_OPEN_DRAIN()  do { P1M1 |= 0x01;                 P1M0 |= 0x01; } while (0)
#define P10_HIGH_Z()      do { P1M1 |= 0x01;                 P1M0 &= (unsigned char)~0x01; } while (0)
#define P10_QUASI()       do { P1M1 &= (unsigned char)~0x01; P1M0 &= (unsigned char)~0x01; } while (0)

/* 整口设置：P0 全部推挽输出（0x00=准双向，0xFF=推挽，0xFF/0x00=高阻，0xFF/0xFF=开漏） */
static void Port0_AllPushPull(void)
{
    P0M1 = 0x00;
    P0M0 = 0xFF;
}

/* --------------------------------------------------------------------------
 *  ADC 模拟输入脚：高阻输入 + 关闭数字输入使能（否则读数会飘）
 *  PxIE 在 XFR，必须先 P_SW2 |= 0x80
 * -------------------------------------------------------------------------- */
static void Port0_Pin0_AnalogIn(void)
{
    P_SW2 |= 0x80;               /* EAXFR = 1，才能访问 P0IE */
    P0M1 |= 0x01;                /* M1 = 1 */
    P0M0 &= (unsigned char)~0x01;/* M0 = 0  -> 高阻输入 */
    P0IE &= (unsigned char)~0x01;/* 关闭 P0.0 的数字输入 */
}

/* --------------------------------------------------------------------------
 *  P8 以上的口不是 SFR，只能用 XFR 宏（头文件已提供 P8M0/P8M1/P8OUT/P8IN）
 *  示例：P8.0 推挽输出 + 输出高
 * -------------------------------------------------------------------------- */
static void Port8_Pin0_PushPull_High(void)
{
    P_SW2 |= 0x80;               /* EAXFR = 1： 必做！ */
    P8M1  &= (unsigned char)~0x01;
    P8M0  |= 0x01;
    P8OUT |= 0x01;
}
```

---

## 3. 中断向量号

### 3.1 结论

**你列的名字里有 2 个是错的**：
* ❌ `T0_VECTOR` → ✅ **`TMR0_VECTOR`**
* ❌ `T2_VECTOR` → ✅ **`TMR2_VECTOR`**
* `ADC_VECTOR` 正确；本芯片还额外提供别名 `ADC1_VECTOR`、`ADC2_VECTOR`(=106)。
* `xxx_VECTOR` 宏**同时存在于 `STC32G.H` 与 `STC32G144K246.H`**，编号一致。
* C251 的中断语法与 C51 **完全相同**：`void f(void) interrupt N`。

**你要求的 14 个向量 + 本芯片扩充（全部来自本机头文件实测）**

| 你的提问 | 头文件里的确切宏 | 数值 | 向量地址 |
|---|---|---|---|
| INT0 | `INT0_VECTOR` | 0 | 0003H |
| T0 | **`TMR0_VECTOR`** | 1 | 000BH |
| INT1 | `INT1_VECTOR` | 2 | 0013H |
| T1 | `TMR1_VECTOR` | 3 | 001BH |
| UART1 | `UART1_VECTOR` | 4 | 0023H |
| ADC | `ADC_VECTOR` / `ADC1_VECTOR` | 5 | 002BH |
| — | `LVD_VECTOR` | 6 | 0033H |
| — | `PCA_VECTOR` | 7 | 003BH |
| UART2 | `UART2_VECTOR` | 8 | 0043H |
| SPI | `SPI_VECTOR` / `SPI1_VECTOR` | 9 | 004BH |
| — | `INT2_VECTOR` / `INT3_VECTOR` | 10 / 11 | 0053H / 005BH |
| T2 | **`TMR2_VECTOR`** | 12 | 0063H |
| — | `USER_VECTOR` | 13 | 006BH |
| — | ⚠️ **14/15 在本芯片头文件里不存在** | — | （`STC32G.H` 有 `BRK_VECTOR 14` / `ICEP_VECTOR 15`） |
| — | `INT4_VECTOR` | 16 | 0083H |
| — | `UART3_VECTOR` / `UART4_VECTOR` | 17 / 18 | 008BH / 0093H |
| T3 | `TMR3_VECTOR` | 19 | 009BH |
| T4 | `TMR4_VECTOR` | 20 | 00A3H |
| CMP | `CMP_VECTOR` / `CMP1_VECTOR` | 21 | 00ABH |
| I2C | `I2C_VECTOR` / `I2C1_VECTOR` | 24 | 00C3H |
| — | `USB_VECTOR` | 25 | 00CBH |
| PWMA / PWMB | `PWMA_VECTOR` / `PWMB_VECTOR` | 26 / 27 | 00D3H / 00DBH |
| — | `CAN1_VECTOR` / `CAN2_VECTOR` | 28 / 29 | 00E3H / 00EBH |
| — | `LIN1_VECTOR` / `LIN2_VECTOR` | 30 / 31 | 00F3H / 00FBH |
| — | `RTC_VECTOR` | 36 | 0123H |
| — | `P0INT_VECTOR … P9INT_VECTOR` | 37 … 46 | 012BH … 0173H |
| — | `DMA_M2M_VECTOR` | 47 | 017BH |
| — | `DMA_ADC_VECTOR` / `DMA_ADC1_VECTOR` | 48 | 0183H |
| — | `PWMC_VECTOR` / `PWMD_VECTOR` / `PWME_VECTOR` / `PWMF_VECTOR` | 121 / 122 / 123 / 124 | 03CBH…03E3H |
| — | `ADC2_VECTOR` | 106 | 0353H |
| — | `DAC_VECTOR` / `DAC2_VECTOR` | 107 / 108 | 035BH / 0363H |
| — | `CMP2_VECTOR` / `CMP3_VECTOR` / `CMP4_VECTOR` | 114 / 115 / 116 | 0393H / 039BH / 03A3H |
| — | `P AINT_VECTOR` / `PBINT_VECTOR` / `PCINT_VECTOR` … | 90 / 91 / 92 … | 02D3H … |

**中断使能位 / 优先级位**

| 功能 | 使能位 | 优先级位 |
|---|---|---|
| 总中断 | `EA = IE^7` | — |
| ADC (`EADC`) | **`sbit EADC = IE^5;`**（IE = `0xA8`） | `PADC = IP^5`，`PADCH = IPH^5` |
| Timer0 | `ET0 = IE^1` | `PT0` / `PT0H` |
| Timer1 | `ET1 = IE^3` | `PT1` / `PT1H` |
| UART1 | `ES  = IE^4` | `PS` / `PSH` |
| Timer2 | **`ET2 = IE2^2`（`IE2 = 0xAF`，即 `IE2 \|= 0x04`）** | `IP2` 组 |
| Timer3 | **`ET3 = IE2^5`（`IE2 \|= 0x20`）** | `PI2C2/…` 同组 |
| Timer4 | **`ET4 = IE2^6`（`IE2 \|= 0x40`）** | 同上 |
| LVD | `ELVD = IE^6` | `PLVD` / `PLVDH` |
| PWMA / PWMB | `IP2` 优先级域 | `PPWMA = IP2^2`、`PPWMB = IP2^3` |

### 3.2 证据 / 来源

**(a) 本机官方头文件（一手）** —— `C:\Keil_v5\C51\INC\STC\STC32G144K246.H` 第 2733~2865 行（节选）：
```c
#define     INT0_VECTOR             0       //0003H
#define     TMR0_VECTOR             1       //000BH
#define     INT1_VECTOR             2       //0013H
#define     TMR1_VECTOR             3       //001BH
#define     UART1_VECTOR            4       //0023H
#define     ADC_VECTOR              5       //002BH
#define     ADC1_VECTOR             5       //002BH
#define     LVD_VECTOR              6       //0033H
#define     PCA_VECTOR              7       //003BH
#define     UART2_VECTOR            8       //0043H
#define     SPI_VECTOR              9       //004BH
#define     SPI1_VECTOR             9       //004BH
#define     INT2_VECTOR             10      //0053H
#define     INT3_VECTOR             11      //005BH
#define     TMR2_VECTOR             12      //0063H
#define     USER_VECTOR             13      //006BH
#define     INT4_VECTOR             16      //0083H
#define     UART3_VECTOR            17      //008BH
#define     UART4_VECTOR            18      //0093H
#define     TMR3_VECTOR             19      //009BH
#define     TMR4_VECTOR             20      //00A3H
#define     CMP_VECTOR              21      //00ABH
#define     CMP1_VECTOR             21      //00ABH
#define     I2C_VECTOR              24      //00C3H
#define     USB_VECTOR              25      //00CBH
#define     PWMA_VECTOR             26      //00D3H
#define     PWMB_VECTOR             27      //00DBH
#define     LIN1_VECTOR             30      //00F3H
#define     LIN2_VECTOR             31      //00FBH
#define     ADC2_VECTOR             106     //0353H
#define     CMP2_VECTOR             114     //0393H
#define     PWMC_VECTOR             121     //03CBH
```
同样内容在 `D:\k5\C251\INC\STC\STC32G.H` 第 1377~1435 行（编号一致，另有 `BRK_VECTOR 14`、`ICEP_VECTOR 15`）。

**(b) 中断服务函数写法（一手，厂商 ISR 源码）**
`…\Coreboard_Demo\E06_pit_demo\user\isr.c`
```c
void P0_IRQHandler()      interrupt P0INT_VECTOR       { … }
void INT0_IRQHandler()    interrupt INT0_VECTOR        { … }
void INT4_IRQHandler(void) interrupt INT4_VECTOR       { … }
void TM0_IRQHandler()     interrupt TMR0_VECTOR        { TIM0_CLEAR_FLAG; … }
void TM1_IRQHandler()     interrupt TMR1_VECTOR        { … }
void TM2_IRQHandler()     interrupt TMR2_VECTOR        { … }
void TM3_IRQHandler()     interrupt TMR3_VECTOR        { … }
void TM4_IRQHandler()     interrupt TMR4_VECTOR        { … }
void DMA_UART1_IRQHandler(void) interrupt DMA_UR1R_VECTOR { … }
```
→ C251 与 C51 完全同构：`void 名字(参数) interrupt <数字或宏>`，宏只是整数，`void f()` 省略 `void` 也合法。

**(c) STC-ISP 自己的代码生成器模板（一手，exe 内嵌字符串）**
```
void Timer%d_Isr(void) interrupt %d
TIMER%d_ISR:
PORT%d_ISR:
void Port%d_Isr(void) interrupt %d
```
→ 官方生成器也用 `interrupt <宏/常量>` 形式。

**来源 URL**
* 本机头文件（同上）。
* 逐飞 STC32G144K246 开源库 ISR 源码（同 (b)）。
* 官方产品页（中断源数量 >49、4 级优先级）：https://www.stcmicro.com/stc/stc32g144k246.html

### 3.3 可直接使用的 C 代码

```c
/* ==========================================================================
 *  isr.c  ——  中断向量宏的确切用法（C251 与 C51 写法完全一致）
 * ========================================================================== */
#include "STC32G144K246.H"

volatile unsigned int g_ms_tick = 0;

/* Timer0：向量 1 —— 注意名字是 TMR0_VECTOR，不是 T0_VECTOR */
void Tmr0_Isr(void) interrupt TMR0_VECTOR
{
    /* 模式0（16 位自动重装载）硬件自动重装，不需要在此重写 TH0/TL0 */
    g_ms_tick++;
}

/* Timer2：向量 12 —— 注意名字是 TMR2_VECTOR，不是 T2_VECTOR */
void Tmr2_Isr(void) interrupt TMR2_VECTOR
{
    AUXINTIF &= (unsigned char)~0x01;   /* 清 T2IF（AUXINTIF = 0xEF，bit0 = T2IF）*/
}

/* Timer3 / Timer4：向量 19 / 20，使能位在 IE2 */
void Tmr3_Isr(void) interrupt TMR3_VECTOR
{
    AUXINTIF &= (unsigned char)~0x02;   /* 清 T3IF */
}
void Tmr4_Isr(void) interrupt TMR4_VECTOR
{
    AUXINTIF &= (unsigned char)~0x04;   /* 清 T4IF */
}

/* ADC：向量 5（= ADC1_VECTOR） */
void Adc_Isr(void) interrupt ADC_VECTOR
{
    ADC_CONTR &= (unsigned char)~0x20;  /* 清 ADC_FLAG，必须软件清零 */
    /* 读结果 */
}

/* 使能示例：EADC 在 IE 的 bit5；EA 必须最后打开 */
static void Isr_EnableAll(void)
{
    ET0 = 1;            /* IE^1  Timer0 */
    ET1 = 1;            /* IE^3  Timer1 */
    IE2 |= 0x04;        /* ET2  = IE2^2  Timer2 */
    IE2 |= 0x20;        /* ET3  = IE2^5  Timer3 */
    IE2 |= 0x40;        /* ET4  = IE2^6  Timer4 */
    EADC = 1;           /* IE^5  ADC —— 头文件里是 sbit */
    EA  = 1;            /* IE^7  总中断（最后打开） */
}
```

---

## 4. 定时器

### 4.1 结论

| 项目 | 结论 |
|---|---|
| `TMOD` | **有**，SFR `0x89`。位：`T1_GATE 0x80 / T1_CT 0x40 / T1_M1 0x20 / T1_M0 0x10 / T0_GATE 0x08 / T0_CT 0x04 / T0_M1 0x02 / T0_M0 0x01` |
| **`TMOD` 模式 0 的语义** | **本芯片（及 STC32G 系）模式 0 = 16 位自动重装载**，**不是**传统 8051 的 13 位模式！ |
| `AUXR` | **有**，SFR `0x8E`。位见下表 |
| **`T4T3M`** | **有**，SFR `0xDD`（同时有别名 `T3T4M = 0xDD`，同一地址）。位：`T4R 0x80 / T4_CT 0x40 / T4x12 0x20 / T4CLKO 0x10 / T3R 0x08 / T3_CT 0x04 / T3x12 0x02 / T3CLKO 0x01` |
| `ET0` / `TR0` 位名 | **有**：`sbit ET0 = IE^1;`（IE=`0xA8`）、`sbit TR0 = TCON^4;`（TCON=`0x88`）。`TF0 = TCON^5` |
| Timer2 相关 | `T2H = 0xD6`、`T2L = 0xD7`、`T2R` 是 `AUXR^4`、`T2_CT` 是 `AUXR^3`、`T2x12` 是 `AUXR^2`；中断标志 `T2IF = AUXINTIF^0`（AUXINTIF=`0xEF`） |
| Timer3/4 相关 | `T3H/T3L = 0xD4/0xD5`、`T4H/T4L = 0xD2/0xD3`；`T3IF = AUXINTIF^1`、`T4IF = AUXINTIF^2` |
| 预分频 | `TM0PS 0xFEA0`、`TM1PS 0xFEA1`、`TM2PS 0xFEA2`、`TM3PS 0xFEA3`、`TM4PS 0xFEA4` —— **全在 XFR，必须先 `P_SW2 \|= 0x80`** |
| **`AUXR` 的 `T0x12` 含义** | **`T0x12 = 0` → Timer0 工作在 12T（传统 8051，每 12 个时钟计 1）；`T0x12 = 1` → 1T（每个时钟计 1，快 12 倍）** |
| `T3T4M` 别名 | **有**（`sfr T3T4M = 0xdd; sfr T4T3M = 0xdd;`），两个名字都能用 |

**`AUXR`（`0x8E`）位定义（来自本机头文件，与 STC8H 一致）**

| 位 | 宏/位名 | 值 | 含义 |
|---|---|---|---|
| 7 | **`T0x12`** | `0x80` | Timer0 速度：0=12T，1=1T |
| 6 | `T1x12` | `0x40` | Timer1 速度：0=12T，1=1T |
| 5 | `S1M0x6` | `0x20` | 串口1 模式0 波特率 6 分频 |
| 4 | `T2R` | `0x10` | Timer2 启动位（1=启动） |
| 3 | `T2_CT` | `0x08` | Timer2 定时/计数选择（0=定时） |
| 2 | `T2x12` | `0x04` | Timer2 速度：0=12T，1=1T |
| 1 | `EXTRAM` | `0x02` | 片内扩展 RAM 访问控制 |
| 0 | `S1BRT` | `0x01` | 串口1 波特率用 Timer1 还是 Timer2 |

**1ms 定时公式（来自 STC32G 数据手册原文，经 EEPW 转载核对）**
```
T0 对内部系统时钟计数（C/T = 0）时：
  1T  模式（AUXR.7 / T0x12 = 1）：输出时钟 = SYSclk / (TM0PS+1) / (65536 - [RL_TH0,RL_TL0]) / 2
  12T 模式（AUXR.7 / T0x12 = 0）：输出时钟 = SYSclk / (TM0PS+1) / 12 / (65536 - [RL_TH0,RL_TL0]) / 2
溢出率（中断频率）= 上式的 2 倍（去掉末尾的 /2）。
```
即 **1T 模式下：中断周期 = (TM0PS+1) × (65536 − 重装值) / SYSclk**。

### 4.2 证据 / 来源

**(a) 本机官方头文件（一手）** —— `STC32G144K246.H`
```c
37: sfr TCON  = 0x88;  sbit TF1=TCON^7; TR1=TCON^6; TF0=TCON^5; TR0=TCON^4;
47: sfr TMOD  = 0x89;
48:     #define T1_GATE 0x80
49:     #define T1_CT   0x40
50:     #define T1_M1   0x20
51:     #define T1_M0   0x10
52:     #define T0_GATE 0x08
53:     #define T0_CT   0x04
54:     #define T0_M1   0x02
55:     #define T0_M0   0x01
57: sfr TL0 = 0x8a;  TL1 = 0x8b;  TH0 = 0x8c;  TH1 = 0x8d;
62: sfr AUXR = 0x8e;
63:     #define T0x12   0x80
64:     #define T1x12   0x40
65:     #define S1M0x6  0x20
66:     #define T2R     0x10
67:     #define T2_CT   0x08
68:     #define T2x12   0x04
69:     #define EXTRAM  0x02
70:     #define S1BRT   0x01
72: sfr INTCLKO = 0x8f;
73:     #define EX4 0x40  EX3 0x20  EX2 0x10  T2CLKO 0x04  T1CLKO 0x02  T0CLKO 0x01
164: sfr IE = 0xa8;
165:     sbit EA   = IE^7;
166:     sbit ELVD = IE^6;
167:     sbit EADC = IE^5;
168:     sbit ES   = IE^4;
169:     sbit ET1  = IE^3;
170:     sbit EX1  = IE^2;
171:     sbit ET0  = IE^1;
172:     sbit EX0  = IE^0;
191: sfr IE2 = 0xaf;
193:     #define ET4   0x40
194:     #define ET3   0x20
197:     #define ET2   0x04
380: sfr T4H = 0xd2;  T4L = 0xd3;  T3H = 0xd4;  T3L = 0xd5;  T2H = 0xd6;  T2L = 0xd7;
389: sfr T3T4M = 0xdd;
390: sfr T4T3M = 0xdd;
391:     #define T4R    0x80
392:     #define T4_CT  0x40
393:     #define T4x12  0x20
394:     #define T4CLKO 0x10
395:     #define T3R    0x08
396:     #define T3_CT  0x04
397:     #define T3x12  0x02
398:     #define T3CLKO 0x01
467: sfr AUXINTIF = 0xef;
468:     #define INT4IF 0x40  INT3IF 0x20  INT2IF 0x10  T4IF 0x04  T3IF 0x02  T2IF 0x01
727: #define TM0PS (*(unsigned char volatile xdata *)0xfea0)
728: #define TM1PS (*(unsigned char volatile xdata *)0xfea1)
729: #define TM2PS (*(unsigned char volatile xdata *)0xfea2)
730: #define TM3PS (*(unsigned char volatile xdata *)0xfea3)
731: #define TM4PS (*(unsigned char volatile xdata *)0xfea4)
```
C251 目录版把上述 `#define` 换成 `sbit`（例如 `sbit T0x12 = AUXR^7;`、`sbit T4R = T4T3M^7;`），**名字与位号相同**。

**(b) TMOD 模式 0 = 16 位自动重装载（数据手册原文转载）** —— https://forum.eepw.com.cn/thread/387721/1/
> “当定时器 0 工作在模式 0（`TMOD[1:0]/[M1,M0]=00B`）时，`[TH0,TL0]` 的溢出不仅置位 TF0，而且会自动将 `[RL_TH0,RL_TL0]` 的内容重新装入 `[TH0,TL0]`。”
> “定时器 0 有两个隐藏的寄存器 `RL_TH0` 和 `RL_TL0`，与 `TH0/TL0` 共有同一个地址……当 `TR0=1` 时对 `TL0/TH0` 写入实际上写的是隐藏重装寄存器。”
> “T0 的速率由 `AUXR` 中的 `T0x12` 决定：`T0x12=0` 则 12T 模式；`T0x12=1` 则 1T 模式。”
> 输出时钟公式（同上 §4.1）。

**(c) STC-ISP 定时器计算器生成的代码（一手，exe 内嵌模板字符串）**
```
/* C 版 */
    TMOD &= 0xF0;      // Timer0 模式位清零（模式0）
    TMOD |= 0x01;      // 或 0x02 / 0x03
    AUXR |= 0x80;      // T0x12 = 1（1T）
    AUXR &= 0x7F;      // T0x12 = 0（12T）
    TM0PS = 0x%02X;
    TH0 = 0x%02X;  TL0 = 0x%02X;
    TR0 = 1;  TF0 = 0;  ET0 = 1;

    TMOD &= 0x0F;  TMOD |= 0x10;  /* Timer1 模式1 */  TMOD |= 0x20;  /* 模式2 */
    AUXR |= 0x40;   AUXR &= 0xBF;   TM1PS = 0x%02X;   TR1 = 1; TF1 = 0; ET1 = 1;

    AUXR |= 0x10;   /* T2R = 1 启动 Timer2 */
    AUXR |= 0x04;   AUXR &= 0xFB;   /* T2x12 */
    T2H = 0x%02X;   T2L = 0x%02X;   TM2PS = 0x%02X;
    ET2 = 1;  TR2 = 1;  T2CON = 0;  T2MOD = 0;  RCAP2H/RCAP2L;

    P_SW2 |= 0x80;                 /* EAXFR=1：TM4PS 在 XFR */
    IE2 |= 0x40;                   /* ET4 */
    T4T3M |= 0x80;  T4H = …; T4L = …;
    T4T3M |= 0x20;  T4T3M &= 0xDF;  /* T4x12 = 1 / 0 */
    TM4PS = 0x%02X;
    IE2 |= 0x20;                   /* ET3 */
    T4T3M |= 0x08;  T3H = …; T3L = …;
    T4T3M |= 0x02;  T4T3M &= 0xFD;  /* T3x12 = 1 / 0 */
    TM3PS = 0x%02X;
    IE2 |= 0x04;                   /* ET2 */
```
（模板里 Timer3/4 的注释在字符串表中的顺序被打乱，但**位值**与头文件定义一一吻合，可互相验证。）

**(d) 逐飞库 Timer0 1ms 实现（一手源码）** —— `zf_driver_pit.c`
```c
case TIM0_PIT:
    AUXR |= 0x80;        /* 1T */
    TM0PS = freq_div;    /* 预分频 */
    TMOD &= 0xF0;        /* 模式 0 = 16 位自动重装载 */
    TL0 = temp;
    TH0 = temp >> 8;
    TR0 = 1;             /* 启动 */
    ET0 = 1;             /* 使能中断 */
    break;
```
`pit_ms_init(TIM0_PIT, 1000, handler)` → `pit_init(TIM0_PIT, 1000 * (system_clock/1000), handler)`，内部 `temp = 65536 - period/(freq_div+1)`，`freq_div = period >> 16`。

**未确认项**
* `T2CON` / `T2MOD` / `RCAP2H` / `RCAP2L` 在 **STC32G144K246 的 `STC32G144K246.H` 里没有被定义**（`STC32G.H` 有）。STC-ISP 的 A51 模板里有 `MOV RCAP2H,#…`，但那是**给老 STC15/STC8 用的共用模板**。→ 本芯片用 Timer2 请走 `T2H/T2L` + `AUXR` 的路子，**不要用 `RCAP2H/RCAP2L`**；STC32G144K246 是否保留 `RCAP2H/L` 与 `T2CON` → **【未确认】**。

### 4.3 可直接使用的 C 代码（Timer0 精确 1ms 中断）

```c
/* ==========================================================================
 *  timer.c  ——  STC32G144K246 Timer0 1ms 中断（16 位自动重装载，1T 模式）
 *  频率宏必须与 STC-ISP 下载时选定的 IRC 频率一致
 * ========================================================================== */
#include "STC32G144K246.H"

#define FOSC_HZ   24000000UL          /* 24MHz */

volatile unsigned int g_ms_tick = 0;

/* --------------------------------------------------------------------------
 *  Timer0 初始化：1ms 周期
 *   周期 = (TM0PS+1) * (65536 - reload) / SYSclk
 *   -> reload = 65536 - (SYSclk/1000) / (TM0PS+1)
 *  SYSclk/1000 可能超过 65535（例如 96MHz -> 96000），此时用 TM0PS 预分频。
 * -------------------------------------------------------------------------- */
void Timer0_Init_1ms(void)
{
    unsigned long ticks;              /* 1ms 需要的 1T 计数值 = SYSclk/1000 */
    unsigned char presc = 0;
    unsigned int  reload;

    P_SW2 |= 0x80;                    /* EAXFR = 1 —— TM0PS 在 XFR，必须先使能！ */

    ticks = FOSC_HZ / 1000UL;
    while ((ticks / (unsigned long)(presc + 1)) > 65535UL) {
        presc++;                      /* 每档 +1 就是把输入再分频一次 */
    }
    reload = (unsigned int)(65536UL - (ticks / (unsigned long)(presc + 1)));

    AUXR |= 0x80;                     /* T0x12 = 1：1T 模式（传统 8051 是 12T） */
    TMOD &= 0xF0;                     /* 清 T0 模式位 -> 模式0 = 16 位自动重装载 */
    TM0PS = presc;                    /* 预分频：溢出率 = SYSclk/(TM0PS+1)/(65536-reload) */

    TL0 = (unsigned char)reload;      /* TR0=0 时写 TH0/TL0 会同时写入隐藏重装寄存器 */
    TH0 = (unsigned char)(reload >> 8);

    TF0 = 0;                          /* 清溢出标志 */
    ET0 = 1;                          /* IE^1：允许 Timer0 中断 */
    TR0 = 1;                          /* TCON^4：启动 Timer0 */
    EA  = 1;                          /* 总中断 */
}

/* 向量号 1，宏名 TMR0_VECTOR（不是 T0_VECTOR） */
void Tmr0_Isr(void) interrupt TMR0_VECTOR
{
    g_ms_tick++;                      /* 模式0 硬件自动重装，无需重写 TH0/TL0 */
}

/* --------------------------------------------------------------------------
 *  Timer2 1ms（同为 16 位自动重装载，走 AUXR 而不是 TMOD）
 * -------------------------------------------------------------------------- */
void Timer2_Init_1ms(void)
{
    unsigned long ticks;
    unsigned char presc = 0;
    unsigned int  reload;

    P_SW2 |= 0x80;

    ticks = FOSC_HZ / 1000UL;
    while ((ticks / (unsigned long)(presc + 1)) > 65535UL) presc++;
    reload = (unsigned int)(65536UL - (ticks / (unsigned long)(presc + 1)));

    AUXR |= 0x04;                     /* T2x12 = 1：Timer2 1T 模式 */
    AUXR &= (unsigned char)~0x08;     /* T2_CT = 0：定时器（对内部时钟计数）*/
    TM2PS = presc;
    T2L = (unsigned char)reload;
    T2H = (unsigned char)(reload >> 8);
    AUXINTIF &= (unsigned char)~0x01; /* 清 T2IF */
    IE2 |= 0x04;                      /* ET2 = IE2^2 */
    AUXR |= 0x10;                     /* T2R = 1：启动 Timer2 */
    EA = 1;
}
void Tmr2_Isr(void) interrupt TMR2_VECTOR
{
    AUXINTIF &= (unsigned char)~0x01; /* 清 T2IF */
}

/* --------------------------------------------------------------------------
 *  Timer3 / Timer4（用 T4T3M，预分频在 TM3PS/TM4PS，中断使能在 IE2）
 * -------------------------------------------------------------------------- */
void Timer3_Init_1ms(void)
{
    unsigned long ticks; unsigned char presc = 0; unsigned int reload;

    P_SW2 |= 0x80;                                  /* TM3PS 在 XFR */
    ticks = FOSC_HZ / 1000UL;
    while ((ticks / (unsigned long)(presc + 1)) > 65535UL) presc++;
    reload = (unsigned int)(65536UL - (ticks / (unsigned long)(presc + 1)));

    TM3PS = presc;
    T3L = (unsigned char)reload;
    T3H = (unsigned char)(reload >> 8);
    T4T3M |= 0x02;                                  /* T3x12 = 1：1T */
    /* T4T3M &= ~0x02;                              // T3x12 = 0：12T */
    T4T3M &= (unsigned char)~0x04;                  /* T3_CT = 0：定时 */
    AUXINTIF &= (unsigned char)~0x02;               /* 清 T3IF */
    IE2 |= 0x20;                                    /* ET3 = IE2^5 */
    T4T3M |= 0x08;                                  /* T3R = 1：启动 */
    EA = 1;
}
void Tmr3_Isr(void) interrupt TMR3_VECTOR { AUXINTIF &= (unsigned char)~0x02; }

void Timer4_Init_1ms(void)
{
    unsigned long ticks; unsigned char presc = 0; unsigned int reload;

    P_SW2 |= 0x80;
    ticks = FOSC_HZ / 1000UL;
    while ((ticks / (unsigned long)(presc + 1)) > 65535UL) presc++;
    reload = (unsigned int)(65536UL - (ticks / (unsigned long)(presc + 1)));

    TM4PS = presc;
    T4L = (unsigned char)reload;
    T4H = (unsigned char)(reload >> 8);
    T4T3M |= 0x20;                                  /* T4x12 = 1：1T */
    T4T3M &= (unsigned char)~0x40;                  /* T4_CT = 0：定时 */
    AUXINTIF &= (unsigned char)~0x04;               /* 清 T4IF */
    IE2 |= 0x40;                                    /* ET4 = IE2^6 */
    T4T3M |= 0x80;                                  /* T4R = 1：启动 */
    EA = 1;
}
void Tmr4_Isr(void) interrupt TMR4_VECTOR { AUXINTIF &= (unsigned char)~0x04; }
```

**24MHz 的具体数值（可直接对照）**：`FOSC/1000 = 24000`，`presc = 0`，`reload = 65536−24000 = 41536 = 0xA240`，即 `TH0 = 0xA2, TL0 = 0x40`。

---

## 5. ADC（本节最重要，含一处**会直接导致功能错误**的纠正）

### 5.0 ⚠️ 先纠正一个会直接导致功能错误的假设

本工程 `src/board_config.h` 现有假设：
```c
15:  *  │ P0.0~P0.6 : 电感 ADC（ADC8~ADC14，5 水平 + 2 竖直）              │
16:  *  │ P0.7      : 电池电压 ADC（可选，默认关）                          │
37: #define IND_ADC_CH          { 8, 9, 10, 11, 12, 13, 14 }
41: #define BAT_ADC_CH          15      /* ADC15 = P0.7 */
```

**这两条都不成立。** 本芯片是**双 ADC**，通道号必须在「哪个 ADC 模块」语境下才有意义；且两个模块的通道映射**完全不同**：

| 模块 | CH0 | CH1 | CH2 | CH3 | CH4 | CH5 | CH6 | CH7 | CH8 | CH9 | CH10 | CH15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **ADC1** | P1.0 | P1.1 | P1.2 | P1.3 | P1.4 | P1.5 | P1.6 | P1.7 | **P0.0** | **P0.1** | **P0.2** | **内部 1.19V 基准** |
| **ADC2** | **P0.0** | **P0.1** | **P0.2** | **P0.3** | **P0.4** | **P0.5** | **P0.6** | **P0.7** | P6.0 | P6.1 | P6.2 | **内部 1.19V 基准** |

推论：
1. **P0.0~P0.6 七个电感只能用 `ADC2` 的 `CH0~CH6`**。用 ADC1 最多只能到 P0.2（`CH8/CH9/CH10`），`CH11~CH14` 两个厂商库都没有映射。
2. **`BAT_ADC_CH = 15` 是错的**：`CH15` 是内部 BandGap 1.19V 基准，读到的是一个与电源无关的固定值，**不能当电池电压**。若电池分压接在 P0.7，正确值是 **ADC2 的 `CH7`**。
3. 现有 `src/bsp/adc.h` 的 `Adc_Sample(uint8 ch)` **缺少"模块"这一维**，必须扩成 `(module, channel)`。

**推荐改法（与两家库一致的编码方式）**
```c
#define ADC_MK(mod, ch)   ( ((unsigned int)(mod) << 12) | ((unsigned int)(ch) << 8) )
#define ADC_MOD_OF(x)     ((unsigned char)((x) >> 12))
#define ADC_CH_OF(x)      ((unsigned char)(((x) >> 8) & 0x0F))

/* 7 路电感全部放 ADC2 的 P0.0~P0.6，极性/顺序与你原来的电感顺序一一对应 */
#define IND_ADC_CH   { ADC_MK(2,0), ADC_MK(2,1), ADC_MK(2,2), ADC_MK(2,3), \
                       ADC_MK(2,4), ADC_MK(2,5), ADC_MK(2,6) }
#define BAT_ADC_CH   ADC_MK(2,7)      /* 电池分压接 P0.7 -> ADC2 CH7 */
```

> **关于 `ADC1 CH11~CH14` 的引脚**：STC 官方页声明 *“12-bit high-precision 15 channels (Ch 0~14), Channel 15 tests internal reference voltage (1.19V ±1%)”*，**意味着 CH11~CH14 存在**；但两份针对本芯片的第三方库都只映射到 CH10，而本地那份官方《STC32G144K246 芯片手册》是**纯扫描图 PDF（无文字层，26/5400 个流可解压，`BT` 文本算子 0 个）**，STC 官网/论坛在本环境全部 403 → **CH11~CH14 具体引脚 → 【未确认】**。工程上**不要依赖它们**。

### 5.1 SFR 全名与位域（结论）

#### 5.1.1 寄存器地址表（全部来自本机官方头文件实测）

| 寄存器 | 地址 | 所在空间 |
|---|---|---|
| `ADC_CONTR` | `0xBC` | SFR |
| `ADC_RES` | `0xBD` | SFR |
| `ADC_RESL` | `0xBE` | SFR |
| `ADCCFG` | `0xDE` | SFR |
| `ADCTIM` | `0xFEA8` | **XFR**（须 EAXFR=1） |
| `ADC_RESH` | `0xFEAA` | XFR |
| `ADCEXCFG` | `0xFEAD` | XFR |
| `ADCEXCFG2` | `0xFEAF` | XFR |
| `ADC2_CONTR` | `0xFB38` | XFR |
| `ADC2CFG` | `0xFB39` | XFR |
| `ADC2_RES` | `0xFB3A` | XFR |
| `ADC2_RESL` | `0xFB3B` | XFR |
| `ADC2TIM` | `0xFB3C` | XFR |
| `ADC2EXCFG` | `0xFB3D` | XFR |
| `ADC2INTR` | `0xFB3E` | XFR |
| `ADC2EXCFG2` | `0xFB3F` | XFR |
| `ADC2_RESH` | `0xFEAB` | XFR |
| `DMA_ADC_CFG/CR/STA/AMT/DONE/RXAH/RXAL/CFG2/CHSW0/CHSW1/ITVH/ITVL` | `0xFA10…0xFA1F` 等 | XFR |
| `DMA_ADC2_*` | `0xFAE0…0xFAEF` | XFR |

#### 5.1.2 位域

**`ADC_CONTR`（`0xBC`）** —— 与 `ADC2_CONTR` 位定义相同

| 位 | 名字 | 值 | 说明 |
|---|---|---|---|
| 7 | `ADC_POWER` | `0x80` | ADC 电源开关 |
| 6 | `ADC_START` | `0x40` | 启动一次转换（硬件转换完自动清 0） |
| 5 | `ADC_FLAG` | `0x20` | 转换完成标志，**硬件置 1，必须软件清 0** 才能开始下一次 |
| 4 | `ADC_EPWMT` | `0x10` | PWM 触发 ADC 使能 |
| 3..0 | **`ADC_CHS[3:0]`** | 掩码 `0x0F` | 模拟通道选择。⚠️ **头文件没有为它定义宏名**，必须自己用 `0x0F` |

**`ADCCFG`（`0xDE`）**

| 位 | 名字 | 值 | 说明 |
|---|---|---|---|
| 7..6 | — | — | 头文件未定义 |
| 5 | **`RESFMT`** | `0x20` | `1` = 结果**右对齐**；`0` = 结果**左对齐** |
| 4 | — | — | 头文件未定义 |
| 3..0 | **`SPEED[3:0]`** | 掩码 `0x0F` | ADC 时钟分频。⚠️ **头文件没有为它定义宏名** |

**`ADCTIM`（`0xFEA8`，XFR）**

* **已确认**：该寄存器由三个域组成 —— **`CSSETUP`、`CSHOLD`、`SMPDUTY`**。
  一手证据：STC-ISP exe 内嵌的「ADC 转换速度计算器」代码生成模板原文
  ```
  ADCTIM = 0x%02x;			//CSSETUP(%d), CSHOLD(%d), SMPDUTY(%d)
  	MOV		A,#0%02XH			;CSSETUP(%d), CSHOLD(%d), SMPDUTY(%d)
  void AdcSetRate(void)		//%sKSPS@%sMHz
  计算ADCTIM寄存器的值 / Calculate ADCTIM value
  ADCTIM的值必须在0-255之间
  ```
* **【未确认】**：三个域各自的**位边界**、**复位值**、以及「转换时间 = (CSSETUP+1)/(CSHOLD+1)/(SMPDUTY+1) × ADCCLK 周期」这类**具体公式**——本地数据手册是扫描图，官方 PDF 站点在本环境 406/403，没能拿到寄存器表原文。
* **工程结论（重要）**：**两份针对本芯片的第三方库都不写 `ADCTIM`**（逐飞 `zf_driver_adc.c`、科宇 `ky_adc.c` 全文都没有 `ADCTIM =`），即它们都**依赖复位默认值**——这是能跑通的直接证据。**建议沿用同样做法**（不写 `ADCTIM`），除非你实测出采样不稳再按 STC-ISP 的「ADC 转换速度」计算器给出的值填入。**不要照抄网上 STC8H 的 `0x3F` 等数字**（我没能核实它在 STC32G144K246 上的正确性）。

**`ADCEXCFG`（`0xFEAD`，XFR）—— 多通道扫描**
* **已确认**：寄存器存在（头文件给出地址），`ADCEXCFG2`（`0xFEAF`）是同族的第二个。
* **【未确认】**：**位定义（哪个位表示扫描使能、哪些位表示扫描通道）**。头文件只给地址不给位；本地手册无文字层；厂商库未使用该寄存器（它们用 **DMA 通道扫描** `DMA_ADC_CHSW0/CHSW1` 代替）。
  → 本工程如果要多通道扫描，**建议走 DMA**（见 §5.1.4 的 `DMA_ADC_*` 寄存器组），或直接软件逐通道轮询（§5.3 的代码）。

**中断使能 / 向量**
* `sbit EADC = IE^5;`（`IE = 0xA8`）—— **已确认**
* `#define ADC_VECTOR 5`、`ADC1_VECTOR 5`、`ADC2_VECTOR 106` —— **已确认**
* 优先级：`sbit PADC = IP^5;`（`IP = 0xB8`）、`PADCH = IPH^5` —— **已确认**
* ⚠️ ADC1 与 ADC2 共用 `ADC_VECTOR`(=5) 还是各自独立？头文件给了 `ADC2_VECTOR = 106` 但也有 `ADC1_VECTOR = 5` → **ADC2 是否同时触发 5 和 106 号向量、【未确认】**（头文件同时定义了 `ADC2INTR` 寄存器，暗示另有独立中断标志）。用 ADC2 时**建议一律走查询方式**（§5.3），避免踩这个坑。

#### 5.1.3 数据手册获取的限制（为什么有些项只能标未确认）

* 本地官方手册 `…\【文档】说明书 芯片手册等\STC32G144K246芯片手册.pdf`（35.2 MB）经解压检测：可解压流 26 个 / 失败 5374 个，内容流中 **`BT` 文本算子 0 个** → **纯扫描图片，无文字层**；本地 `STC8H.pdf`（25.3 MB）同样是扫描件。系统里也没有可用的 `pdftotext` / Python PDF 库（Python 是未安装的 WindowsApps 存根，`pdftotext` 不存在），网络下载被 TLS 阻断（`curl: (35) schannel: SEC_E_NO_CREDENTIALS`）。
* 官方 PDF 站点 `stcmcudata.com` / `stcmicro.com` / `akizukidenshi.com` 的 `.pdf` 直链一律返回 **HTTP 406**（IIS 静态文件 MIME 拒绝），`web_fetch` 对 `application/pdf` 直接报不支持；STC 官方论坛 `stcaimcu.com`、`112.74.51.126` 镜像对本文所有请求返回 **403**；`manualslib.com` 返回 Cloudflare 拦截页。
* 因此**寄存器位表一律以本机 STC-ISP 安装的官方头文件 + STC-ISP 可执行文件内嵌模板 + 两份本芯片专用厂商库源码为准**；凡是这三者都没有的，标 **【未确认】**。

#### 5.1.4 12 位结果拼接（**已确认，两种对齐都给出**）

* `ADCCFG.RESFMT = 1` → **右对齐**：`ADC_RES[3:0] = D[11:8]`，`ADC_RESL[7:0] = D[7:0]`
  → `value = ((uint16)ADC_RES << 8) | ADC_RESL;`（取 `& 0x0FFF` 更稳）
* `ADCCFG.RESFMT = 0` → **左对齐**：`ADC_RES[7:0] = D[11:4]`，`ADC_RESL[7:4] = D[3:0]`，`ADC_RESL[3:0] = 0`
  → `value = ((uint16)ADC_RES << 4) | (ADC_RESL >> 4);`

**证据（三源一致）**
1. 逐飞 `zf_driver_adc.c`（`ADCCFG |= 1<<5` 右对齐 + `ADC_RES<<8 | ADC_RESL`）
2. 科宇 `ky_adc.c`：`ADCCFG = (clock & 0x0F); if (12bit) ADCCFG |= (1<<5);` 然后 `adc_value = (ADC_RES << 8) | ADC_RESL;`
3. STC32G ADC 应用文章原文：*“可配置为左对齐（高 8 位存储在高位寄存器 ADC_RES 中，低四位存储在低位寄存器 ADC_RESL 的高四位中），可配置为右对齐（高 4 位存储在高位寄存器 ADC_RES 的低 4 位中，低 8 位存储在低位寄存器 ADC_RESL 中）”*
   https://blog.csdn.net/billliu66/article/details/129965525

#### 5.1.5 ADC 时钟与转换时间（**公式已确认**）

```
ADC 工作时钟 = SYSclk / 2 / (ADCCFG.SPEED + 1)        SPEED = 0..15  ->  分频比 2,4,6,…,32
```
**证据**
* STC 数据手册片段（搜索索引到 `STC32G.pdf`）：*“…统频率 2 分频再经过用户设置的分频系数进行再次分频（ADC 的工作时钟频率范围为…”*
* STC8A8K64S4A12 官方数据手册 `ADCCFG` 表（lcsc 镜像）行：`| 0001 | SYSclk/2/2 |`
* 两份厂商库的分频枚举 **16 档、命名 `ADC_SYSclk_DIV_2 … ADC_SYSclk_DIV_32` / `ADC_CLK_DIV_2 … ADC_CLK_DIV_32`**，与 `SPEED = 0..15 → div = 2(SPEED+1)` 完全对应。
* STC32G ADC 应用文章：*“ADC 的最高时钟频率为系统频率的 1/2”*（即 SPEED=0 的上限）。

**取值建议**
| SYSclk | 建议 `SPEED` | `ADCCFG[3:0]` | 实际 ADC 时钟 |
|---|---|---|---|
| 24 MHz | 15 | `0x0F` | 24/32 = **0.75 MHz** |
| 30 MHz | 15 | `0x0F` | 30/32 = 0.94 MHz |
| 35 MHz | 15 | `0x0F` | 35/32 = 1.09 MHz |
| 40 MHz | 15 | `0x0F` | 40/32 = 1.25 MHz |
| 96 MHz | 15 | `0x0F` | 96/32 = **3 MHz**（厂商库默认档） |
| 120 MHz | 15 | `0x0F` | 120/32 = 3.75 MHz |

> 说明：厂商库对 96MHz 就用最大分频（`ADC_SYSclk_DIV_32`），说明**分频比越大越稳**。电感采样对速度要求不高（一次转换量级 ~10µs），**直接用 SPEED=15（分频 32）最稳**。
> **单次转换总时间的精确公式（含 `ADCTIM` 三项）→ 【未确认】**；STC 论坛有「一次完整的 A/D 时间含【ADC 输入通道的切换时间 + 采样的时间 + 关闭采样的时间 + 固定转换…】」的讨论帖，但正文 403 无法取证。

#### 5.1.6 ⚠️ 硬件硬约束：`ADC_VREF+` 不可悬空

STC32G 的 ADC **有独立参考电压输入引脚 `ADC_VREF+`**（不同于 STC15 以电源为参考）。
* STC32G ADC 应用文章原文：*“STC32G 的 ADC 模块则与之不同，它有单独的参考电压源引脚，可以接入精准的参考电压（0~5V 皆可）……**注意：STC32G ADC 模块的参考电压输入引脚不可悬空。**”*
* 官方论坛实例帖标题即为 *“STC32G 显示电压一直不变 | **已解决, ADC_VREF+ 没接**”*
  https://www.stcaimcu.com/thread-20355-1-7.html
→ **画板/接线时务必确认 `ADC_VREF+` 已接到稳定的参考（或直接接 MCU 电源）**，否则 ADC 读数无意义。

### 5.2 证据 / 来源（汇总）

**(a) 本机官方头文件（一手）** —— `C:\Keil_v5\C51\INC\STC\STC32G144K246.H`
```c
286: sfr ADC_CONTR = 0xbc;
287:     #define ADC_POWER   0x80
288:     #define ADC_START   0x40
289:     #define ADC_FLAG    0x20
290:     #define ADC_EPWMT   0x10
292: sfr ADC_RES  = 0xbd;
293: sfr ADC_RESL = 0xbe;
400: sfr ADCCFG = 0xde;
401:     #define RESFMT  0x20
732: #define ADCTIM    (*(unsigned char volatile xdata *)0xfea8)
734: #define ADC_RESH  (*(unsigned char volatile xdata *)0xfeaa)
735: #define ADC2_RESH (*(unsigned char volatile xdata *)0xfeab)
737: #define ADCEXCFG  (*(unsigned char volatile xdata *)0xfead)
739: #define ADCEXCFG2 (*(unsigned char volatile xdata *)0xfeaf)
1086: #define ADC2_CONTR (*(unsigned char volatile xdata *)0xfb38)
1087: #define ADC2CFG    (*(unsigned char volatile xdata *)0xfb39)
1088: #define ADC2_RES   (*(unsigned char volatile xdata *)0xfb3a)
1089: #define ADC2_RESL  (*(unsigned char volatile xdata *)0xfb3b)
1090: #define ADC2TIM    (*(unsigned char volatile xdata *)0xfb3c)
1091: #define ADC2EXCFG  (*(unsigned char volatile xdata *)0xfb3d)
1092: #define ADC2INTR   (*(unsigned char volatile xdata *)0xfb3e)
1093: #define ADC2EXCFG2 (*(unsigned char volatile xdata *)0xfb3f)
167:     sbit EADC = IE^5;
258:     sbit PADC = IP^5;
1227: #define DMA_ADC_CFG  (*(unsigned char volatile xdata *)0xfa10)
1234: #define DMA_ADC_CFG2 (*(unsigned char volatile xdata *)0xfa19)
```
`D:\k5\C251\INC\STC\STC32G144K246.H` 用 `sbit ADC_POWER = ADC_CONTR^7;` 等形式（位号相同）。

**(b) 逐飞 STC32G144K246 库（一手源码，芯片专用）**
* `…\Coreboard_Demo\libraries\zf_driver\zf_driver_adc.h`：通道/分频/分辨率枚举（ADC1 CH0..CH10、ADC2 CH0..CH10）
* `…\Coreboard_Demo\libraries\zf_driver\zf_driver_adc.c`：查询式单次转换完整实现
* 备注（来自 `Motherboard_Demo\E04_02_battery_voltage_detection_demo\user\main.c` 第 70 行注释）：
  `adc_init(ADC1_CH0_P10, ADC_12BIT); // 建议电磁传感器用 ADC1 电池检测用 ADC2`

**(c) 科宇科技 STC32G144K246 开源库（一手源码，芯片专用，含 CH15 = BGV）**
* `https://raw.giteeusercontent.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary/raw/5569362949cbd2be0b72738855776aa389ce4cba/app_examples/library/drivers/ky_adc.h`
  ```c
  ADC1_CH0_P10 = 1 << 12 | 0 << 8 | GPIO_P10,
  …
  ADC1_CH8_P00 = 1 << 12 | 8 << 8 | GPIO_P00,
  ADC1_CH9_P01 = 1 << 12 | 9 << 8 | GPIO_P01,
  ADC1_CH10_P02= 1 << 12 |10 << 8 | GPIO_P02,
  ADC1_CH15_BGV= 1 << 12 |15 << 8 | 0xFF,       /* 内部 BandGap */
  …
  ADC2_CH0_P00 = 2 << 12 | 0 << 8 | GPIO_P00,
  ADC2_CH7_P07 = 2 << 12 | 7 << 8 | GPIO_P07,
  ADC2_CH8_P60 = 2 << 12 | 8 << 8 | GPIO_P60,
  ADC2_CH10_P62= 2 << 12 |10 << 8 | GPIO_P62,
  ADC2_CH15_BGV= 2 << 12 |15 << 8 | 0xFF,
  ```
* `https://raw.giteeusercontent.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary/raw/5569362949cbd2be0b72738855776aa389ce4cba/app_examples/library/drivers/ky_adc.c`
  ```c
  gpio_init(pin, GPIO_MODE_IN_FLOATING, GPIO_LOW);   /* 模拟脚必须高阻输入 */
  gpio_set_digital(pin, GPIO_DIGITAL_DISABLE);       /* 且关闭数字输入使能 */
  ADC_CONTR &= 0xF0;  ADC_CONTR |= (channel & 0x0F);
  ADCCFG = (clock & 0x0F);
  if (resolution == ADC_RES_12BIT) ADCCFG |= (1 << 5);
  ADC_CONTR |= (1 << 7);                             /* 上电 */
  …
  ADC_CONTR &= ~0x20;  ADC_CONTR &= 0xF0;  ADC_CONTR |= ch;
  ADC_CONTR |= 0x40;
  while (!(ADC_CONTR & 0x20));
  adc_value = (uint16_t)(ADC_RES << 8);  adc_value |= ADC_RESL;
  ```
  → 注意：**该库全文没有写 `ADCTIM`**，依赖复位默认值。

**(d) STC-ISP 内嵌模板（一手）** —— `AiCube-ISP-v6.96P.exe`，见 §5.1.2 的 `ADCTIM = 0x%02x; //CSSETUP(%d), CSHOLD(%d), SMPDUTY(%d)`。

**(e) 官方规格页 / 应用文章**
* https://www.stcmicro.com/stc/stc32g144k246.html （双 ADC、15 通道 + CH15 内部 1.19V、LQFP100/64/48/44）
* https://blog.csdn.net/billliu66/article/details/129965525 （左/右对齐描述、独立参考引脚不可悬空、最高 ADC 时钟 = 1/2 SYSclk）
* https://www.stcaimcu.com/thread-20355-1-7.html （ADC_VREF+ 没接导致读数不变）
* https://www.stcaimcu.com/forum.php?mod=viewthread&tid=3911 （A/D 时间构成讨论，正文 403 无取证）

### 5.3 可直接使用的 C 代码（查询方式单通道采样，完整可编译）

```c
/* ==========================================================================
 *  adc.c  ——  STC32G144K246 查询式单通道 ADC（双 ADC 模块，12 位，右对齐）
 *  编译环境：Keil uVision C251 + STC32G144K246.H
 *
 *  已验证的关键点：
 *    1) 模拟脚必须“高阻输入 + 关闭数字输入使能”，否则读数会飘；
 *    2) ADC_CONTR/ADC2_CONTR 的 CHS 在 bit3..0，头文件没给宏，用 0x0F；
 *    3) 每次转换前必须软件清 ADC_FLAG（bit5），否则第二次会死等；
 *    4) ADCCFG 的 SPEED 在 bit3..0，RESFMT 在 bit5；
 *    5) 两块 ADC 各有自己的 CONTR/RES/RESL，别混用。
 * ========================================================================== */
#include "STC32G144K246.H"

/* ---------------- 通道编码：高 4 位 = 模块号(1/2)，次 4 位 = 通道号 ---------------- */
#define ADC_MOD_1        1u
#define ADC_MOD_2        2u
#define ADC_MK(mod, ch)  ( (unsigned int)(((unsigned int)(mod) << 12) | ((unsigned int)(ch) << 8)) )
#define ADC_MOD_OF(x)    ((unsigned char)((x) >> 12))
#define ADC_CH_OF(x)     ((unsigned char)(((x) >> 8) & 0x0F))

/* ADC 时钟分频：SPEED[3:0] = 15  ->  ADCCLK = SYSclk/2/16 = SYSclk/32（最稳档）*/
#define ADC_SPEED_DIV_32 0x0F
#define ADC_RESFMT_RIGHT 0x20          /* ADCCFG.bit5 = 1 -> 右对齐 */

/* 供外部使用：本工程 7 路电感走 ADC2 的 P0.0~P0.6，电池走 ADC2 的 P0.7 */
#define ADC_CH_IND0      ADC_MK(ADC_MOD_2, 0)   /* P0.0 */
#define ADC_CH_IND1      ADC_MK(ADC_MOD_2, 1)   /* P0.1 */
#define ADC_CH_IND2      ADC_MK(ADC_MOD_2, 2)   /* P0.2 */
#define ADC_CH_IND3      ADC_MK(ADC_MOD_2, 3)   /* P0.3 */
#define ADC_CH_IND4      ADC_MK(ADC_MOD_2, 4)   /* P0.4 */
#define ADC_CH_IND5      ADC_MK(ADC_MOD_2, 5)   /* P0.5 */
#define ADC_CH_IND6      ADC_MK(ADC_MOD_2, 6)   /* P0.6 */
#define ADC_CH_BATTERY   ADC_MK(ADC_MOD_2, 7)   /* P0.7 */
#define ADC_CH_BGV       ADC_MK(ADC_MOD_2, 15)  /* 内部 1.19V 基准（自检用）*/

static void Adc_DelayMs(unsigned int ms)
{
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 1200u; j++) { /* 24MHz 下约 1ms，仅用于 ADC 上电等待 */ }
}

/* --------------------------------------------------------------------------
 *  把一个 P0 引脚配置成 ADC 模拟输入：高阻输入 + 关闭数字输入
 * -------------------------------------------------------------------------- */
static void Adc_PinAnalogIn_P0(unsigned char bit_mask)
{
    P_SW2 |= 0x80;                        /* EAXFR = 1：P0IE 在 XFR */
    P0M1 |= bit_mask;                     /* M1 = 1 */
    P0M0 &= (unsigned char)~bit_mask;     /* M0 = 0  -> 高阻输入 */
    P0IE &= (unsigned char)~bit_mask;     /* 关闭该位数字输入（模拟口必须）*/
}

/* --------------------------------------------------------------------------
 *  ADC 初始化：上电 + 选通道 + 设分频与对齐
 *  注意：此处按要求“每个通道采样前重选通道”，所以初始化只做上电与配置
 * -------------------------------------------------------------------------- */
void Adc_Init(void)
{
    unsigned char i;

    P_SW2 |= 0x80;                        /* 下面要碰 XFR（ADCTIM 虽不写，PxIE 也要）*/

    /* P0.0~P0.7 全部设为模拟输入（7 电感 + 电池）*/
    Adc_PinAnalogIn_P0(0xFF);

    /* 只给 ADC2 上电（本工程电感与电池都在 ADC2）*/
    ADC2_CONTR &= 0xF0;                   /* 清 CHS */
    ADC2_CONTR |= 0x00;                   /* 通道 0 */
    ADC2CFG     = (unsigned char)(ADC_RESFMT_RIGHT | ADC_SPEED_DIV_32);
    ADC2_CONTR |= 0x80;                   /* ADC2_POWER = 1 */

    /* 若同时要用 ADC1，这里一并上电 */
    /* ADC_CONTR &= 0xF0; ADC_CONTR |= 0x00;
       ADCCFG = (unsigned char)(ADC_RESFMT_RIGHT | ADC_SPEED_DIV_32);
       ADC_CONTR |= 0x80; */

    /* ADC 内部模拟电路需要稳定时间，厂商例程用 >=1ms */
    for (i = 0; i < 1; i++) { Adc_DelayMs(2); }
}

/* --------------------------------------------------------------------------
 *  查询式单次采样，返回 0~4095（右对齐，12 位）
 *  chn 用 ADC_MK(module, channel) 构造
 * -------------------------------------------------------------------------- */
unsigned int Adc_SampleOnce(unsigned int chn)
{
    unsigned char mod = ADC_MOD_OF(chn);
    unsigned char ch  = ADC_CH_OF(chn);
    unsigned int  v;

    P_SW2 |= 0x80;                        /* 保险起见，确保 XFR 可访问 */

    if (mod == ADC_MOD_1)
    {
        ADC_CONTR &= (unsigned char)~0x20;          /* 1) 清 ADC_FLAG（必须软件清）*/
        ADC_CONTR &= 0xF0;                          /* 2) 清 ADC_CHS[3:0] */
        ADC_CONTR |= (unsigned char)(ch & 0x0F);    /* 3) 选通道 */
        ADC_CONTR |= 0x40;                          /* 4) ADC_START = 1 启动 */
        while (!(ADC_CONTR & 0x20)) { }             /* 5) 等 ADC_FLAG 置 1 */
        ADC_CONTR &= (unsigned char)~0x20;          /* 6) 清标志，准备下次 */

        /* 7) 右对齐：ADC_RES[3:0]=D[11:8]，ADC_RESL[7:0]=D[7:0] */
        v = (unsigned int)(((unsigned int)ADC_RES << 8) | (unsigned int)ADC_RESL);
        ADC_RES = 0;  ADC_RESL = 0;
        return (unsigned int)(v & 0x0FFF);
    }
    else /* ADC_MOD_2 */
    {
        ADC2_CONTR &= (unsigned char)~0x20;
        ADC2_CONTR &= 0xF0;
        ADC2_CONTR |= (unsigned char)(ch & 0x0F);
        ADC2_CONTR |= 0x40;
        while (!(ADC2_CONTR & 0x20)) { }
        ADC2_CONTR &= (unsigned char)~0x20;

        v = (unsigned int)(((unsigned int)ADC2_RES << 8) | (unsigned int)ADC2_RESL);
        ADC2_RES = 0;  ADC2_RESL = 0;
        return (unsigned int)(v & 0x0FFF);
    }
}

/* --------------------------------------------------------------------------
 *  左对齐（RESFMT = 0）时的拼法 —— 仅在你改 ADCCFG/ADC2CFG 时为 0x00 才用
 *    ADC_RES[7:0] = D[11:4]，ADC_RESL[7:4] = D[3:0]，ADC_RESL[3:0] = 0
 * -------------------------------------------------------------------------- */
unsigned int Adc_ReadLeftAligned_Adc2(void)
{
    return (unsigned int)((((unsigned int)ADC2_RES << 4) | (ADC2_RESL >> 4)) & 0x0FFF);
}

/* --------------------------------------------------------------------------
 *  多通道扫描（本工程 7 路电感）：查询式逐通道
 * -------------------------------------------------------------------------- */
void Adc_ScanAll(unsigned int *out)
{
    out[0] = Adc_SampleOnce(ADC_CH_IND0);
    out[1] = Adc_SampleOnce(ADC_CH_IND1);
    out[2] = Adc_SampleOnce(ADC_CH_IND2);
    out[3] = Adc_SampleOnce(ADC_CH_IND3);
    out[4] = Adc_SampleOnce(ADC_CH_IND4);
    out[5] = Adc_SampleOnce(ADC_CH_IND5);
    out[6] = Adc_SampleOnce(ADC_CH_IND6);
}

/* --------------------------------------------------------------------------
 *  用内部 1.19V 基准反推 VDDA（可选自检手段）
 *    VDDA = 4095 * 1.19V / ADC(CH15)      —— 前提是参考电压就是 VDDA
 * -------------------------------------------------------------------------- */
unsigned int Adc_ReadBandGap(void)
{
    return Adc_SampleOnce(ADC_CH_BGV);
}
```

---

## 6. `STC32G.H` / `STC32G144K246.H` 中的整数类型别名与 `sfr`/`sbit` 用法

### 6.1 结论

* **`STC32G144K246.H` 和 `STC32G.H` 本身都不定义 `u8`/`u16`/`u32`/`BYTE`**（实测：在这两个文件里 grep `typedef` 只有 PWM 结构体 `TAG_PWM_STRUCT`，没有任何整数别名）。
* 这些别名在**同目录的 `DEF.H`** 里（STC-ISP 一起安装的）。
* 想用 `u8/u16/u32`，必须 **`#include "DEF.H"`** 或自己在工程里 typedef；**`DEF.H` 与 `common.h` 一起包含会重复定义**（C251 对重复 typedef 通常只 warning，但工程里最好只留一处）。
* 本工程 `src/common.h` 目前**刻意使用 `uint8/uint16/uint32` 并注释“避开 STC32G.H 已定义的 u8/u16/u32”** —— 这个说法对 `STC32G.H` 而言**不准确**（`STC32G.H` 没有定义 `u8`），但**做法本身是安全的**（不依赖 `DEF.H`、也不会和任何头冲突）。**建议保持现状**。

**`DEF.H` 的实际内容（一手，本机实测）** —— `C:\Keil_v5\C51\INC\STC\DEF.H`
```c
typedef bit                     BOOL;

typedef unsigned char           BYTE;
typedef unsigned int            WORD;      /* ← C251/C51 中 int = 16 bit */
typedef unsigned long           DWORD;     /* ← long = 32 bit */

typedef signed   char           CHAR;
typedef signed   int            INT;
typedef signed   long           LONG;

typedef unsigned char           uint8_t;
typedef unsigned int            uint16_t;
typedef unsigned long           uint32_t;

typedef signed   char           int8_t;
typedef signed   int            int16_t;
typedef signed   long           int32_t;

typedef unsigned char           uint8;
typedef unsigned int            uint16;
typedef unsigned long           uint32;

typedef signed   char           int8;
typedef signed   int            int16;
typedef signed   long           int32;

typedef unsigned char           u8;
typedef unsigned int            u16;
typedef unsigned long           u32;

typedef signed   char           s8;
typedef signed   int            s16;
typedef signed   long           s32;
```
另含宏：`LOBYTE/HIBYTE/LOWORD/HIWORD/MAKEWORD/MAKELONG/BYTE0..3/WORD0/WORD2`、`BIT0..BIT7`、`BIT(b)`、`PIN_0..PIN_7`、`PIN_ALL`、`CLR_REG_BIT/SET_REG_BIT/CPL_REG_BIT/READ_REG_BIT/READ_REG/WRITE_REG/CLR_REG/MODIFY_REG`、`NULL/LOW/HIGH/FALSE/TRUE/DISABLE/ENABLE/min/max`。

> **关键尺寸提醒（C251，与 ARM 不同）**：`char = 8 bit`、**`int = 16 bit`**、`long = 32 bit`。
> 所以 `typedef unsigned int u16;` 是对的；**不要**把 `unsigned int` 当成 32 位用。

**`sfr` / `sbit` 用法（本机头文件原文）**
```c
sfr   P0      = 0x80;        /* 字节 SFR：只能 8 位直接寻址 */
sbit  P00     = P0^0;        /* 位 SFR：可位寻址的 SFR 的某一位 */
sbit  EA      = IE^7;
sbit  EADC    = IE^5;
sbit  TR0     = TCON^4;
sbit  ET0     = IE^1;
sbit  ADC_FLAG= ADC_CONTR^5; /* C251 目录版写法；C51 目录版是 #define ADC_FLAG 0x20 */
```
**XFR（扩展 SFR，地址 `0x7E:xxxx` / `0x7F:xxxx`）不能用 `sfr`**，官方头文件用带 `volatile` 的指针宏：
```c
/* C51 目录版（xdata 指针）*/
#define ADCTIM  (*(unsigned char volatile xdata *)0xfea8)
/* C251 目录版（far 指针）*/
#define ADCTIM  (*(unsigned char volatile far   *)0x7efea8)
```
**访问 XFR 前必须先置 `EAXFR = 1`（即 `P_SW2 |= 0x80`）**，头文件自带一对宏：
```c
#define EAXSFR()   P_SW2 |= 0x80    /* MOVX 操作对象 = 扩展 SFR */
#define EAXRAM()   P_SW2 &= ~0x80   /* MOVX 操作对象 = 扩展 RAM */
```
（头文件第 555~558 行还有注释：`//访问这些寄存器,需先将 EAXFR 设置为1,才可正常读写 / EAXFR = 1; / P_SW2 |= 0x80;`）

厂商库自己的类型别名（可作为“厂商惯例”参考）：
`…\Coreboard_Demo\libraries\zf_common\zf_common_typedef.h`
```c
typedef unsigned char   uint8;    typedef unsigned int   uint16;   typedef unsigned long  uint32;
typedef signed char     int8;     typedef signed int     int16;    typedef signed long    int32;
typedef uint8  volatile vuint8;   typedef uint16 volatile vuint16; typedef uint32 volatile vuint32;
```

### 6.2 来源

* 本机 `C:\Keil_v5\C51\INC\STC\DEF.H`（171 行，全文见上）
* 本机 `C:\Keil_v5\C51\INC\STC\STC32G144K246.H`（`sfr`/`sbit`/XFR 宏、`EAXSFR()/EAXRAM()` 在第 2869~2870 行）
* 本机 `D:\k5\C251\INC\STC\STC32G144K246.H`（`far` 版）
* 逐飞库 `zf_common_typedef.h`

### 6.3 可直接使用的 C 代码

```c
/* ==========================================================================
 *  common.h  ——  本工程统一类型（不依赖 DEF.H，避免与厂商头文件冲突）
 *  说明：如果同时 #include "DEF.H"，u8/u16/u32/uint8_t/... 会重复 typedef。
 *        本工程选择“只用自己这一套”，因此不要 include DEF.H。
 * ========================================================================== */
#ifndef __COMMON_H
#define __COMMON_H

#include "STC32G144K246.H"     /* 官方头文件：sfr / sbit / XFR 宏 / 中断向量宏 */

typedef unsigned char   uint8;     /*  8 bit */
typedef unsigned int    uint16;    /* 16 bit —— C251 里 int 就是 16 位！ */
typedef unsigned long   uint32;    /* 32 bit */
typedef signed   char   int8;
typedef signed   int    int16;
typedef signed   long   int32;

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))

#endif /* __COMMON_H */
```

```c
/* XFR 访问的三种写法，任选其一，注意都要先 EAXFR = 1 */
#define EAXFR_ON()      (P_SW2 |= 0x80)
#define EAXFR_OFF()     (P_SW2 &= (unsigned char)~0x80)

void Xfr_Access_Demo(void)
{
    EAXFR_ON();                 /* 1) 先使能 */
    TM0PS = 3;                  /* 2) 直接读写 XFR 宏（头文件已声明为 volatile far/xdata 指针）*/
    CLKDIV = 2;
    EAXFR_OFF();                /* 3) 用完可以关（不是必须，但更清晰）*/
}
```

---

## 7. 芯片基本规格

### 7.1 结论（来源：STC 官方产品页 + 官方选型表）

| 项目 | 数值 |
|---|---|
| 型号 | **STC32G144K246** |
| 内核 | 32 位 8051（Intel **C251** 架构），1T 单周期/机器周期，约为传统 8051 的 70 倍 |
| 主频 | **CPU/DSP 100 ~ 120 MHz**；TFPU / PWM 可达 240 MHz（内部双 500MHz PLL） |
| Flash（程序存储器） | **最大 246 KB**（型号里的 “246”） |
| SRAM | **144 KB + 4 KB**：<br>• 16 KB 片内 SRAM（`edata`）<br>• 128 KB 片内扩展 RAM（`xdata`）<br>• 4 KB 高端扩展 RAM（可映射到 `80H:0000H~80H:0FFFH` 以执行程序）<br>合计 **144 KB + 4 KB**（型号里的 “144”） |
| EEPROM / DATA FLASH | IAP，512 B 单页擦除，典型擦写 > 100 000 次 |
| 工作电压 | **1.9 V ~ 5.5 V**（**温度低于 −40 ℃ 时要求 ≥ 3.0 V**），内置 LDO |
| 封装 | **LQFP100 / LQFP64 / LQFP48 / LQFP44**（4 种） |
| 引脚 / GPIO | 最多 **91 个 GPIO**：P0.0~P0.7、P1.0~P1.7、P2.0~P2.7、P3.0~P3.7、P4.0~P4.7、**P5.0~P5.4**、P6.0~P6.7、P7.0~P7.7、P8.0~P8.7、P9.0~P9.7、PA.0~PA.7、PB.0~PB.7 |
| 温度范围 | −20 ℃ ~ +65 ℃（IRC 温漂 −0.76% ~ +0.98%）<br>−40 ℃ ~ +85 ℃（IRC 温漂 ±1.3%）<br>−40 ℃ ~ +125 ℃（**85 ℃ 以上需外部晶振或内部 PLL**） |
| 中断 | 49+ 个中断源，**4 级中断优先级** |
| 定时器 | 14 个 16 位定时器（**T0~T11、T17、T18**）；T0 模式 3 支持 NMI；T17/T18 可同步触发两组 ADC/DAC |
| 串口 | 8 个高速同步/异步串口 **USART1~USART8**（支持 同步/异步/SPI/LIN/IrDA/ISO7816） |
| PWM | 24 通道高级 PWM（**3+3 组** PWMA/PWMC/PWME 支持 4 对互补 + 死区），PWM 时钟最高 240 MHz |
| ADC | **双独立 12 位高速 ADC**，各 **15 通道（Ch0~14）+ Ch15 测内部 1.19 V 基准**，官方选型表写 “16 通道/模块 × 2 = 32 通道”；**有独立参考电压引脚 `ADC_VREF+`（不可悬空）** |
| DAC | 双独立 12 位高速 DAC |
| 比较器 / 运放 | 4 个独立 6P6N 比较器（CMP）、4 个独立运放（OPA） |
| 其他 | 2×CAN-FD、USB 2.0 全速、RTC、2×I2C、3×SPI + QSPI、2×I2S、LCD/TFT 8080/6800 并口驱动、SWD 仿真（P3.0/P3.1）、硬件 USB 下载 + UART ISP 下载 |
| 复位 | 上电复位（1.7~1.9 V）、复位脚（P5.4 可配）、看门狗、LVD（2.0/2.4/2.7/3.0 V 四档）、软件复位 |

> **一个小矛盾**（官方同一页内）：英文 Feature 段写 *“12-bit high-precision 15 channels (Ch 0~14), Channel 15 tests internal reference voltage”*（= 每模块 15 路外部 + 1 路内部），而选型表写 *“2 Independent 12-bit ADCs (16 Channels Each, 32 Channels Total)”*。**两处我都原文保留**；工程上按 §5.0 表中**已由两家厂商库确认的 CH0~CH10 + CH15** 使用最安全。

### 7.2 来源 / 证据 URL

* **官方产品页（英文，本次抓取成功）**：https://www.stcmicro.com/stc/stc32g144k246.html
  > “Up to 246 Kbytes of Flash program memory … SRAM (Total 144K + 4K bytes) — 16K bytes internal SRAM (edata); 128K bytes internal extended RAM (internal xdata); 4K bytes high-end extended RAM …”
  > “Operating voltage: 1.9V ~ 5.5V (requires ≥3.0V when temperature is below -40℃) …”
  > “100MHz~120MHz CPU/DSP, supporting 240MHz TFPU / PWM.”
  > “Package: LQFP100, LQFP64, LQFP48, LQFP44.”
  > “ADC: Dual independent ultra-high-speed ADCs, supporting 12-bit high-precision 15 channels (Ch 0~14), Channel 15 tests internal reference voltage (1.19V ±1%).”
* **官方中文产品页**：https://www.stcmicro.com.cn/stc/stc32g144k246.html （同页中文版）
* **官方 Data Sheet 下载入口（本环境 406 无法下载）**：`https://www.stcmicro.com/datasheet/stc32g-cn.pdf`
* **本地官方手册文件**：`C:\Users\y\Desktop\单片机资料\STC32G144K246_100Pin_Library-master\【文档】说明书 芯片手册等\STC32G144K246芯片手册.pdf`（35.2 MB，**扫描图，无文字层**）
* **本地 100 脚核心板说明书**：同目录 `STC32G144K 100PIN核心板说明书（1.0）.pdf`
* **第三方选型信息（含 LQFP64 实物/封装）**：https://item.szlcsc.com/56102495.html

### 7.3 可直接使用的 C 代码（规格自检 / 版本宏）

```c
/* STC-ISP 生成的头文件本身没有版本号宏，也没有 Flash/SRAM 容量宏。
   要“代码里可编译地”确认芯片，只能用 STC-ISP 提供的 CHIPID 只读常量。
   下面的地址全部是本机官方头文件实测（STC32G144K246.H 第 1032~1065、1164~1197 行）： */

#include "STC32G144K246.H"

/* ✅ 可用：CHIPID 只读常量（XFR，读之前必须 EAXFR=1）
     #define CHIPID    ( (unsigned char volatile xdata *)0xfde0)   // 数组形式
     #define CHIPID0   (*(unsigned char volatile xdata *)0xfde0)   // 逐字节形式
     … CHIPID1(0xfde1) … CHIPID12(0xfdec) … CHIPID22(0xfdf6) … CHIPID31(0xfdff)
     #define CHIPIDX   ( (unsigned char volatile xdata *)0xfbd0)
     #define CHIPIDX0…CHIPIDX31  (0xfbd0 … 0xfbef)
   注意：逐飞库用 CHIPID12 当“HIRC 24MHz 的 IRTRIM 值”、CHIPID22 当“27MHz 频段的 VRTRIM 值”
   （见 zf_common_clock.c），但**每个字节的确切语义应由 STC-ISP「读芯片信息」对照确认**——
   本报告未核实 CHIPID 各字节的官方定义 → 【未确认】。 */
void Chip_ReadId(unsigned char *buf8)
{
    unsigned char i;
    P_SW2 |= 0x80;                       /* EAXFR = 1：CHIPID 在 XFR */
    for (i = 0; i < 8; i++) { buf8[i] = CHIPID[i]; }
}

/* ❌ 不存在：XX_FLASH_SIZE / XX_SRAM_SIZE 之类的宏 —— 头文件里没有，
      不要在代码里写 #if FLASH_SIZE >= 246K 这种判断。 */
```

---

## 8. 对本工程的落地结论（行动清单）

> **写作时对工程现状的实测复核**（`src/board_config.h`、`docs/pinmap.md`）：
> * ✅ **ADC 模块/通道已经是对的**：`board_config.h` 第 15-16 行注释、第 38-43 行
>   （`IND_ADC_MODULE 2` + `IND_ADC_CH {0..6}` + `BAT_ADC_MODULE 2` + `BAT_ADC_CH 7`）
>   与 `docs/pinmap.md` 第 7-8 行**与 §5.0 的结论一致**，无需再改。
> * ❌ **中断向量号 T3/T4 是错的**（`board_config.h` 第 89-90 行、`pinmap.md` 第 44 行）：
>   `T3_VECTOR_NUM 16` 应为 **19**，`T4_VECTOR_NUM 17` 应为 **20**（16=INT4、17=UART3）。
>   见下面行动项 6。

| # | 动作 | 依据 |
|---|---|---|
| 1 | ~~`board_config.h`：`IND_ADC_CH` 由 `{8..14}` 改为 **ADC2 的 CH0~CH6**~~ **已完成** | §5.0 |
| 2 | ~~`board_config.h`：`BAT_ADC_CH` 由 `15` 改为 **`ADC_MK(2,7)`**（P0.7）~~ **已完成** | §5.0（CH15 是内部 1.19V） |
| 3 | `bsp/adc.h`：`Adc_Sample(uint8 ch)` 扩成 `Adc_SampleOnce(uint16 chn)`，`chn = ADC_MK(module, channel)` | §5.0 / §5.3 |
| 4 | `bsp/adc.c`：模拟脚必须 **高阻输入 + 关闭数字输入使能（`P0IE`）**，且 ADC 上电后 **≥1ms** 再采样 | §2.1 / §5.3 |
| 5 | 硬件：确认 **`ADC_VREF+` 未悬空**（接稳定参考或 MCU 电源） | §5.1.6 |
| 6 | **【必修】** `board_config.h` 第 89-90 行：`T3_VECTOR_NUM` **16 → 19**、`T4_VECTOR_NUM` **17 → 20**；同步改 `docs/pinmap.md` 第 44 行 | §3（`TMR3_VECTOR 19` / `TMR4_VECTOR 20`；16=INT4、17=UART3） |
| 7 | 任何 XFR 寄存器（`TM0PS..TM4PS`、`CLKSEL`、`CLKDIV`、`HPLLCR`、`ADCTIM`、`PxPU/PxPD/PxIE/…`、`ADC2_*`）访问前 **必须 `P_SW2 \|= 0x80`** | §1 / §2 / §4 / §5 |
| 8 | 保持 `common.h` 现有的 `uint8/uint16/uint32` 方案；**不要把注释改成“STC32G 已定义 u8”**（不准确），也不要额外 `#include "DEF.H"` | §6 |
| 9 | 时钟：先按 **下载时在 STC-ISP 选 IRC 频率** 的方式做；`WTST` 按 STC-ISP 显示值填（本报告无法给出本芯片官方表） | §1 |
| 10 | 不要用 `RCAP2H/RCAP2L/T2CON/T2MOD` 配 Timer2（本芯片头文件里没有这些） | §4.2 未确认项 |

---

## 附录 A. 一手证据文件索引（均可在本机复核）

| 类型 | 路径 | 用途 |
|---|---|---|
| STC-ISP 安装的官方头文件（本芯片，C51 目录 / xdata 版） | `C:\Keil_v5\C51\INC\STC\STC32G144K246.H` | SFR/XFR 地址、位域、`+_VECTOR` 宏 |
| STC-ISP 安装的官方头文件（本芯片，C251 目录 / far 版） | `D:\k5\C251\INC\STC\STC32G144K246.H` | `sbit` 风格位定义、C251 正式用法 |
| STC-ISP 安装的官方头文件（老 STC32G） | `D:\k5\C251\INC\STC\STC32G.H` | 与 `STC32G144K246.H` 对照 |
| STC-ISP 安装的官方头文件（STC8H，用于交叉印证） | `C:\Keil_v5\C51\INC\STC\STC8H.H` | 确认 ADC/时钟寄存器地址与 STC32G 一致、STC8H **无 WTST** |
| 官方类型别名 | `C:\Keil_v5\C51\INC\STC\DEF.H` | `u8/u16/u32/BYTE/WORD/…` |
| STC-ISP 可执行文件（含代码生成模板与帮助文本） | `C:\Users\y\Desktop\AiCube-ISP-v6.96P\AiCube-ISP-v6.96P.exe` | ADC 计算器模板、定时器计算器模板、WTST 规则、IRC 档位表、头文件清单 |
| 另一版 STC-ISP（可交叉验证） | `D:\k5\stc\AiCube-ISP-v6.96A.exe`、`D:\k5\stc\stc-isp\stc-isp.exe` | 同上 |
| 厂商库 1（逐飞，芯片专用） | `C:\Users\y\Desktop\单片机资料\STC32G144K246_100Pin_Library-master\Example\Coreboard_Demo\libraries\` | `zf_driver_adc.c/.h`、`zf_driver_gpio.c/.h`、`zf_driver_pit.c/.h`、`zf_common_clock.c/.h`、`zf_common_typedef.h`、`E06_pit_demo\user\isr.c` |
| 厂商库 1 的主板例程（含电池检测/Capture 例程） | `…\Example\Motherboard_Demo\E04_01_adc_capture_demo\`、`…\E04_02_battery_voltage_detection_demo\` | ADC 用法与“电磁传感器/电池分模块”建议 |
| 厂商库 2（科宇，芯片专用，Gitee） | `https://gitee.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary` | `ky_adc.c/.h`（含 `CH15_BGV`）、`ky_gpio.c` |
| 官方芯片手册（**扫描图，无文字层**） | `…\【文档】说明书 芯片手册等\STC32G144K246芯片手册.pdf` | 规格（图片形式） |
| 本报告中途生成的中间产物 | `C:\Users\y\Desktop\car\docs\_stc_tmp\` | PDF 流解包脚本、厂商源码 UTF-8 转码副本（可删除） |

## 附录 B. 本报告的【未确认】清单（一条不漏）

1. STC-ISP 生成的 **`STC32G_Init()` 函数原文**（含其中的时钟设置段）。
2. **STC32G144K246 的官方 `WTST` 取值表**。（只拿到 STC32G12K128 的口径：`≤52MHz→1`、`>52MHz→2`）
3. **运行时**把 IRC 改为 24 / 30 / 35 / 40 MHz 的 `IRTRIM/IRCBAND` 具体常量表。
4. `IRCBAND.HIRCSEL[1:0]` 的**档位 ↔ 频率对应关系**。
5. **`ADC1 CH11~CH14` 对应的引脚**（以及它们是否存在）。
6. **`ADCTIM` 三个域 `CSSETUP/CSHOLD/SMPDUTY` 的位边界、复位值、转换时间公式**（域**名称**已确认）。
7. **`ADCEXCFG` / `ADCEXCFG2` 的位定义**（多通道扫描的具体控制位）。
8. **ADC2 中断是只走 `ADC2_VECTOR`(106) 还是也走 `ADC_VECTOR`(5)**（`ADC2INTR` 寄存器存在但位定义未取到）。
9. 「**准双向口 = `M1=0, M0=0`**」的一手依据（官方 4 模式列表确认存在该模式，但没有官方寄存器表原文；两份厂商库都未使用该组合）。
10. STC32G144K246 的 **GPIO 复位默认模式**：官方页说“高阻输入”，STC 传统口径是“准双向（`PxM0/PxM1`=0）”—— 两者冲突。
11. **`TMOD` 模式 3（`M1:M0 = 11B`）在本芯片的确切行为**（只确认了模式 0 = 16 位自动重装载、模式 1/2 按 STC-ISP 模板对应 `0x01/0x02`；官方是否保留了传统 8051 的分裂模式未取证）。
12. **`T2CON` / `T2MOD` / `RCAP2H` / `RCAP2L`** 在 STC32G144K246 上是否存在（头文件里没有）。
13. **Flash 等待周期与主频的官方对应曲线**（用于校验厂商 `WTST=3@96MHz / 4@120MHz` 的经验值）。
14. 本报告的 PDF 证据链缺口：官方 PDF 直链 406、官方论坛 403、本地手册为扫描图 → 所有**需要数据手册寄存器表原文**才能确认的条目都在上面的清单里。

---

*报告完成。所有结论按「一手（本机官方头文件 / STC-ISP 可执行文件内嵌模板 / 本芯片专用厂商库源码） > 官方网页 > 二手转载」的证据等级给出；缺证据的一律标注【未确认】。*
