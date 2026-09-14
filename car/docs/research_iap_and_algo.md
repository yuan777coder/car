# STC32G144K246 电磁循迹智能车 — 资料核查报告

**核查对象**：STC32G144K246（STC 251 内核，Keil C251 / MDK FOR C251，官方头文件 `STC32G.H`）
**核查范围**：(1) IAP / EEPROM 参数存储的寄存器级事实；(2) 电磁循迹算法与工程惯例
**核查方式**：全部结论来自本次实际抓取到的网页 / 数据手册 / 开源工程源码，逐条附来源 URL。
**核查日期**：本次会话

> **可信度约定**
> - ✅ **已核实**：有官方数据手册、芯片头文件或可运行开源工程源码直接支撑。
> - ⚠️ **部分确认 / 存疑**：有来源但不一致，或多来源互相矛盾，已列出各方说法。
> - ❌ **未确认**：本次未能找到可靠来源，明确标注，不做推测填充。
> - 二手博客（尤其 CSDN 上大量 AI 生成的“教程”）仅作旁证，**不作为唯一依据**。

---

## 目录

- [第一部分：IAP / EEPROM 参数存储（寄存器级）](#第一部分iap--eeprom-参数存储寄存器级)
  - [1. IAP 相关 SFR 全名与位域；STC32G 与 STC8H 是否一致](#1-iap-相关-sfr-全名与位域stc32g-与-stc8h-是否一致)
  - [2. IAP_CONTR 使能位、等待时间与 IAP_TPS 计算方法](#2-iap_contr-使能位等待时间与-iap_tps-计算方法)
  - [3. IAP_CMD 的取值](#3-iap_cmd-的取值)
  - [4. 触发序列](#4-触发序列)
  - [5. 可直接编译的 EEPROM 读写函数](#5-可直接编译的-eeprom-读写函数)
  - [6. IAP 可操作地址范围、扇区大小、STC-ISP EEPROM 设置](#6-iap-可操作地址范围扇区大小stc-isp-eeprom-设置与代码的一致性)
- [第二部分：电磁循迹算法与工程参考](#第二部分电磁循迹算法与工程参考)
  - [7. 电磁组电感布置惯例](#7-电磁组电感布置惯例)
  - [8. 归一化](#8-归一化)
  - [9. 偏差计算](#9-偏差计算)
  - [10. 舵机控制](#10-舵机控制)
  - [11. 速度控制](#11-速度控制)
  - [12. 元素识别：十字 / 圆环（环岛）/ 三岔路口](#12-元素识别十字--圆环环岛--三岔路口)
  - [13. 调试手段：虚拟示波器串口协议与 OLED 显示参数](#13-调试手段虚拟示波器串口协议与-oled-显示参数)
- [全文「未确认」事项汇总](#全文未确认事项汇总)

---

## 执行摘要：本报告最重要的 12 条结论

**A. EEPROM / IAP（寄存器级）**

1. STC32G144K246 的 IAP 寄存器组与 STC8H **地址完全一致**（`IAP_DATA`=0xC2 … `IAP_CONTR`=0xC7、`IAP_TPS`=0xF5），但**多一个 `IAP_ADDRE`=0xF6**（24 位地址高字节），`IAP_CONTR` 多 `SWBS2`（bit3）。
2. `IAP_CONTR = 0x80` 即使能（`IAPEN` = bit7）；`IAP_TPS = Fosc/1MHz + 1`（逐飞官方取值，比“Fosc/1MHz”多留 1 个计数余量）。
3. `IAP_CMD`：**0x00 待机 / 0x01 字节读 / 0x02 字节写 / 0x03 页擦除**。**不要用 `_def.h` 里的 IAP_CMD 宏名**（有“与手册不符”的公开反映）。
4. 触发：**`IAP_TRIG = 0x5A;` 然后 `IAP_TRIG = 0xA5;`**，之后 **STC32G144K246 必须补 4 个 `_nop_()`**（该芯片是多级流水线，这是芯片特有要求）。全过程关中断。
5. **页 = 512 字节**，擦除 1 页约 4~6 ms，擦写寿命 > 10 万次（STC 官方页面）。**写之前必须先擦整页**，并做“读-改-擦-写”。
6. **代码与 STC-ISP 的一致性不是换算地址**，而是三条约束：① 代码最大偏移 < STC-ISP 分配的 EEPROM 大小；② 程序 Flash + EEPROM ≤ 246 KB；③ **关闭“下载时擦除用户 EEPROM”**。
7. ⚠️ 唯一存疑点：IAP 访问 EEPROM 究竟用 **0 基址**（逐飞官方例程与 STC32F12K54 手册）还是 **24 位绝对地址**（CSDN 笔记）。报告给出“**先读验证、勿先擦 0 地址**”的安全流程，并把基址做成宏。

**B. 电磁循迹算法**

8. **电感**：主流 **2~6 个**；可确证的典型是「2 横 + 2 竖（四电感）」「3 横 + 2 竖（五电感）」「3 水平 + 2 竖直 + 2 内八（七电感）」。用 **10 mH 工字电感 + 6.8 nF**（谐振 ≈19.3~20 kHz），高度 **10~15 cm**，左右严格对称，**相邻 ≥2 cm 防互感**。⚠️ “水平4+竖直2 / 水平5 / 水平3+竖直2”这三个术语**未在任何来源中原样出现**，建议改用上述可核实的说法。
9. **归一化**：`(x−min)/(max−min)×100` → 每通道 **0~100**；必须**有符号类型**（防减法下溢）、**防 0/0**（竖电感直道≈0）、**防除零 `+1` 会牺牲小信号线性度**。顺序是**先归一化（纵向）再差比和（横向）**。
10. **偏差**：差比和 ×100 → **±100** 是可确证的惯例（不是 ±50）。加权求和 **Σ(ADCᵢ·wᵢ)/Σ(ADCᵢ)** 的分母是 **Σ(ADCᵢ)**（非加权），权重可确证的是 `{1,50,99}`（3 路）与 `{0.2,0.8,1,1,1,1,0.8,0.2}`（8 路）；⚠️ **`{−2,−1,0,1,2}` 未确认**。**非线性修正用开方比值法 / 差比和差 / 分段拟合+分段 Kp；`atan` 在电磁组未确认，不要写。**
11. **舵机**：**位置式 PD、不加 I**（厂商手册明文“位置式 PID 一般用不到积分项”；队伍实测“试过 PID 最后用单 PD”）。±100 口径起点 **Kp ≈ 0.65 / Kd ≈ 1.88**（龙邱 21 届手册）；**D 项必须限幅**（`D_MAX = 225*Kd` 反推）；舵机 **50 Hz / 0.5~2.5 ms / 中值 1.5 ms**；**有陀螺仪就加上**（并级 `−Gyro_Z*0.005` 或 1.3~1.4 倍前馈）。
12. **速度**：**编码器测速（计数差值+清零，可不折算 m/s）+ 差速 + 增量式 PID 5 ms**；起点 **Kp≈3.2 / Ki≈2.8 / Kd≈0.1**（21 届 STC32G144K246 工程，限幅 ±9500、PWM 17 kHz、`Kff1=1.0` 前馈）。目标速度按元素分段 + **加减速滞回** + **加速度限幅（`STEP_ACC=20 / STEP_DEC=40`，减速优先）** + **陀螺仪确认车身稳定才给全速**。⚠️ 位置式与增量式参数**不可互换**。
13. **元素识别**：**环岛**用“左右横电感同时升高 + 中间竖电感之和 > 阈值”提前识别，再用**编码器积分位移**找到打角点；**十字电磁组通常不处理，直接冲**（“补线/打死转向”是摄像头组做法）；**三岔**用“中间电感 < 阈值 + 总电感量落在窄窗口（如 200~500）”，左右岔靠**次数标志位**区分。⚠️ “连续 N 个周期偏差很小”作为元素判据**未确认**。
14. **调试**：逐飞助手协议与上位机**有多个不兼容代次**——本芯片官方库用的是 `zf_components/seekfree_assistant/`（**0xAA 头、8 位累加和、大端 float、最多 8 通道**）；另有 V2 例程包用**小端、8 字节头、16 通道**。**务必以你工程实际目录名选协议、配对应版本上位机。** OLED 只显示摘要量（归一化值、error、舵机输出、速度、状态机状态、Kp/Kd），原始电感值走串口/示波器。

---

# 第一部分：IAP / EEPROM 参数存储（寄存器级）

## 本次核查最关键的 4 个结论（先看这里）

1. **STC32G144K246 的 IAP 寄存器组与 STC8H 基本一致，但多了 1 个寄存器 `IAP_ADDRE`（地址 0xF6）**，用于 24 位地址的第 3 个字节；`IAP_CONTR` 也多了 `SWBS2`（bit3）。见 §1。
2. **`IAP_CONTR = 0x80` 即使能 IAP（IAPEN = IAP_CONTR 的 bit7）**；`IAP_TPS = Fosc / 1MHz`，逐飞官方库额外 **+1** 做余量。见 §2。
3. **`IAP_CMD`：0x00 待机 / 0x01 字节读 / 0x02 字节写 / 0x03 扇区(页)擦除**。论坛有帖子称 `STC32G144K246_def.h` 里这部分宏定义与手册不符——**以逐飞可运行驱动的 1/2/3 为准**。见 §3。
4. **STC32G144K246 是深流水线内核，触发后必须补 4 个 `_nop_()`**，否则 `IAP_DATA` 可能读到未完成的数据。这是逐飞驱动里 chip-specific 的注释，STC8H 代码里没有这一条。见 §4。

---

## 1. IAP 相关 SFR 全名与位域；STC32G 与 STC8H 是否一致

### 结论

**SFR 名称与地址**（左列取自 **逐飞科技针对 STC32G144K246 的官方开源库芯片头文件** `libraries/zf_common/stc32g144k246.h`，即本芯片可运行工程实际使用的定义；右列取自覆盖 STC8F/8A/8G/8H 的寄存器头文件）：

| SFR 名称 | 功能 | STC32G144K246 地址 | STC8F/8A/8G/8H 地址 | 是否一致 |
|---|---|---|---|---|
| `IAP_DATA` | IAP 数据寄存器 | 0xC2 | 0xC2 | ✅ 一致 |
| `IAP_ADDRH` | IAP 地址高 8 位 | 0xC3 | 0xC3 | ✅ 一致 |
| `IAP_ADDRL` | IAP 地址低 8 位 | 0xC4 | 0xC4 | ✅ 一致 |
| `IAP_CMD` | IAP 命令寄存器 | 0xC5 | 0xC5 | ✅ 一致 |
| `IAP_TRIG` | IAP 触发寄存器 | 0xC6 | 0xC6 | ✅ 一致 |
| `IAP_CONTR` | IAP 控制寄存器 | 0xC7 | 0xC7 | ✅ 地址一致，位域**多一位** |
| `IAP_TPS` | EEPROM 操作等待时间控制 | 0xF5 | 0xF5 | ✅ 一致 |
| **`IAP_ADDRE`** | **IAP 地址最高 8 位（bit23:16）** | **0xF6** | **无此寄存器** | ❌ **STC32G144K246 独有** |

`IAP_CONTR` 位域（逐飞 `stc32g144k246.h` 中直接以 `sbit` 给出，✅ 已核实）：

| 位 | 名称 | 地址/掩码 | 含义 |
|---|---|---|---|
| bit7 | `IAPEN` | `IAP_CONTR^7` = **0x80** | IAP/EEPROM 操作使能 |
| bit6 | `SWBS` | `IAP_CONTR^6` = 0x40 | 软复位后启动区选择 |
| bit5 | `SWRST` | `IAP_CONTR^5` = 0x20 | 软复位触发 |
| bit4 | `CMD_FAIL` | `IAP_CONTR^4` = 0x10 | EEPROM 操作失败标志 |
| bit3 | `SWBS2` | `IAP_CONTR^3` = 0x08 | 启动区选择扩展位（STC8H 头文件里没有这一位） |

**结论：STC32G144K246 与 STC8H 的 IAP 寄存器“主体一致、细节有增量”。**
- 一致的：`IAP_DATA / IAP_ADDRH / IAP_ADDRL / IAP_CMD / IAP_TRIG / IAP_CONTR / IAP_TPS` 七个寄存器的**地址完全相同**，`IAP_CONTR` 的 `IAPEN/SWBS/SWRST/CMD_FAIL` 位位置也完全相同。
- STC32G144K246 增量的：多了 `IAP_ADDRE`(0xF6)，`IAP_CONTR` 多了 `SWBS2`(bit3)；并且**触发后需要补 NOP**（见 §4）。
- 因此：**STC8H/STC8G 的 IAP 例程可以照搬寄存器名，但用于 STC32G144K246 时必须补上 `IAP_ADDRE` 赋值与 NOP**，否则在 EEPROM 地址 > 64K 或高速主频下会出错。

### 关于 `IAP_DATA` 是 32 位寄存器这件事（⚠️ 部分确认）

STC32G 英文数据手册的寄存器表里出现了 `IAP_DATA3 | IAP Data Register | ... | DATA[31:24] | 0000,0000`，说明 STC32G 系列的 IAP 数据通路是 32 位宽的（`IAP_DATA` / `IAP_DATA1` / `IAP_DATA2` / `IAP_DATA3`）。CSDN 上的 STC32G12K128 EEPROM 文章也明确说“STC32G12K128 的数据寄存器，**仅 IAP_DATA 有效**”，即字节读/写只需要用低 8 位 `IAP_DATA`。
→ **对字节读/字节写/扇区擦除这三种操作，只用 `IAP_DATA` 即可**；其余数据寄存器本次**未确认**其在 STC32G144K246 上的确切名字与地址，本报告的代码不使用它们。

### 来源

- 逐飞科技 STC32G144K246 官方开源库芯片头文件（`IAP_DATA/IAP_ADDRH/IAP_ADDRL/IAP_CMD/IAP_TRIG/IAP_CONTR/IAPEN/SWBS/SWRST/CMD_FAIL/SWBS2/IAP_TPS/IAP_ADDRE` 定义原文）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_common/stc32g144k246.h>
  仓库主页：<https://gitee.com/seekfree/STC32G144K246_100Pin_Library>
- 覆盖 STC8F/8A/8G/8H 的寄存器头文件（`sfr IAP_DATA=0xc2; ... sfr IAP_TPS=0xf5;`、`#define IAPEN 0x80` 等原文）：
  <https://raw.giteeusercontent.com/ldo-li/modbus/raw/master/example/stc.h>
- STC32G 英文数据手册（`IAP_DATA3 ... DATA[31:24]` 寄存器表）：
  <https://www.mikrocontroller.net/attachment/613402/stc32g-en.pdf>
- STC32G 单片机 EEPROM 操作实例（“STC32G12K128 的数据寄存器，仅 IAP_DATA 有效”）：
  <https://blog.csdn.net/billliu66/article/details/128053591>
- STC32G 官方数据手册（中文，寄存器表在 PDF 第 254 页前后，浏览器直接打开 PDF 检索 `IAP_CONTR`）：
  <https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246.pdf>

---

## 2. IAP_CONTR 使能位、等待时间与 IAP_TPS 计算方法

### 结论

**(a) 使能位**
- `IAP_CONTR` 的 **bit7 = `IAPEN` = 0x80**，**写 1 使能** IAP/EEPROM 操作。
- 逐飞官方驱动原文：`IAP_CONTR = 0x80;  //使能EEPROM操作`；关闭：`IAP_CONTR = 0; //失能EEPROM操作`。
- STC32G 英文数据手册的示例代码注释同样写着：`IAP_CONTR = 0x80. //Enable IAP`。
- **常用但容易踩坑的点**：早期 STC15 的 `IAP_CONTR` 里 `WT2:WT0`（bit4:2）是等待时间位，所以老代码里会出现 `IAP_CONTR = 0x83`（使能 + 等待时间 3）。**STC8/STC32G 已经把等待时间搬到独立的 `IAP_TPS` 寄存器**，因此 STC32G144K246 上**只需** `IAP_CONTR = 0x80`，不要再按 STC15 的写法去拼等待时间位（bit4:2 现在并非等待时间）。
- 读回状态：逐飞驱动提供 `iap_get_cmd_state()`，实现是 `return ((IAP_CONTR & 0x01) == 0x01);`。⚠️ **这条本次未能在数据手册中确认 bit0 的确切定义**，不建议在自己的工程里依赖它做流程控制；用**固定延时**（见 §5）更稳妥。

**(b) `IAP_TPS` 计算方法（与主频的关系）**

两种写法，本次都找到了出处：

| 出处 | 公式 | 例 |
|---|---|---|
| CSDN《STC32G 单片机 EEPROM 操作实例》 | `IAP_TPS = 系统频率 / 1000000` | 30MHz → `IAP_TPS = 30` |
| **逐飞官方驱动（STC32G144K246，可运行）** | `IAP_TPS = 系统频率 / 1000000 + 1` | 96MHz → `IAP_TPS = 97` |

逐飞原文：

```c
void iap_set_tps(void)
{
    uint8 write_time;
    write_time = (system_clock / 1000000) ;
    IAP_TPS = write_time + 1;
}
```

> **建议**：**采用 `IAP_TPS = Fosc(Hz)/1000000 + 1`**。这是官方可运行库在 STC32G144K246 上的实际取值，多留 1 个计数余量，功耗/时间代价可忽略，但能避免主频偏高或 IRC 漂移时 IAP 时序不足导致的偶发写失败。
>
> **顺序要求**：`IAP_TPS` 必须**在系统时钟配置完成之后**再写（必须先 `clock_init()` / 设置 PLL，再调用 `IAP_TPS` 初始化）。若中途改主频，必须**重新写一次 `IAP_TPS`**。
>
> **量级参考**：STC32G144K246 常用主频 40MHz / 96MHz → `IAP_TPS = 41 / 97`。

### 来源

- 逐飞官方驱动 `zf_driver_eeprom.c`（`iap_init()` / `iap_idle()` / `iap_set_tps()` 全部原文）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_driver/zf_driver_eeprom.c>
- CSDN《STC32G 单片机 EEPROM 操作实例》（`IAP_TPS寄存器值 = 系统频率 / 1000000`）：
  <https://blog.csdn.net/billliu66/article/details/128053591>
- STC32G 英文数据手册（`IAP_CONTR = 0x80. //Enable IAP`）：
  <https://www.mikrocontroller.net/attachment/613402/stc32g-en.pdf>

---

## 3. IAP_CMD 的取值

### 结论

| `IAP_CMD` 值 | 命令 | 说明 |
|---|---|---|
| **0x00** | 待机 / 空操作（Standby） | 不执行任何操作 |
| **0x01** | **字节读**（Byte Read） | 读回结果在 `IAP_DATA` |
| **0x02** | **字节写 / 字节编程**（Byte Program） | 待写数据先放入 `IAP_DATA`；**只能把 1 写成 0** |
| **0x03** | **扇区(页)擦除**（Page/Sector Erase） | STC32G144K246 **1 页 = 512 字节**；擦除后该页全为 0xFF |

✅ 已核实：以上 0x00/0x01/0x02/0x03 四个取值**同时**被两处独立来源证实——
1. 逐飞针对本芯片的官方驱动：`IAP_CMD = 1;`(读) / `IAP_CMD = 2;`(写) / `IAP_CMD = 3;`(擦除)；
2. 覆盖 STC8 系列的寄存器头文件：`#define IAP_IDL 0x00 / IAP_READ 0x01 / IAP_WRITE 0x02 / IAP_ERASE 0x03`。

**⚠️ 已知的“头文件与手册不符”问题**：STC 论坛有帖子《`STC32G144K246_def.h` 中的 `IAP_CMD` 部分宏定义好像与手册不符》(tid=25567)，说明官方 `_def.h` 中这组宏在某版本里被写成了别的值（常见是误抄 STC15 的位定义风格）。**本次未能读到该帖正文（论坛对抓取返回 HTTP 403）**，因此：
- 不依赖官方 `_def.h` 的 `IAP_CMD_xxx` 宏名，**直接在代码里写 1/2/3**（本报告 §5 的代码即如此写）；
- 以逐飞**可运行**驱动的 1/2/3 为准。若你手上的头文件给出不同数值，以本表为准并在烧录后实测验证。

**⚠️ 关于“块擦除”CMD7**：STC32G12K128 数据手册（第 291 页附近）出现命令 `111（CMD7）：擦除EEPROM块`，即 STC32G 系列另有块擦除命令（`IAP_CMD = 0x07`）。但 CSDN 的 STC32G EEPROM 文章写的是“STC32G 系列单片机**仅支持命令 CMD0~CMD3**”。
→ **两说矛盾**；**STC32G144K246 是否支持 CMD7 块擦除：❌ 未确认**。本报告不使用块擦除，只用 0x03 页擦除（对参数存储完全够用）。

### 来源

- 逐飞官方驱动 `zf_driver_eeprom.c`（`IAP_CMD = 1 / 2 / 3` 原文）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_driver/zf_driver_eeprom.c>
- STC8 系列寄存器头文件（`#define IAP_IDL 0x00 / IAP_READ 0x01 / IAP_WRITE 0x02 / IAP_ERASE 0x03`）：
  <https://raw.giteeusercontent.com/ldo-li/modbus/raw/master/example/stc.h>
- 官方论坛《STC32G144K246_def.h 中的 IAP_CMD 部分宏定义好像与手册不符》（本次 403，未能读取正文）：
  <https://www.stcaimcu.com/forum.php?mod=viewthread&tid=25567>
- STC32G 数据手册（`111（CMD7）：擦除EEPROM块`，PDF 第 291 页）：
  <https://www.stcaimcu.com/data/download/Datasheet/STC32G.pdf>
- CSDN《STC32G 单片机 EEPROM 操作实例》（“STC32G 系列单片机仅支持命令 CMD0~CMD3”）：
  <https://blog.csdn.net/billliu66/article/details/128053591>

---

## 4. 触发序列

### 结论

**完整操作序列（写操作）**：

1. 保存并关闭总中断：`ea = EA; EA = 0;`
2. 写地址：`IAP_ADDRL = addr & 0xFF; IAP_ADDRH = (addr>>8) & 0xFF; IAP_ADDRE = (addr>>16) & 0xFF;`（**`IAP_ADDRE` 是 STC32G144K246 必须补的一步**）
3. 写数据（仅写操作需要）：`IAP_DATA = dat;`
4. 写命令：`IAP_CMD = 1/2/3;`
5. **触发**：`IAP_TRIG = 0x5A;` 然后 `IAP_TRIG = 0xA5;` — **必须按这个顺序、连续两次写**，且**每一次操作都要重新触发一次**（不能只触发一次然后连续做多次）。
6. **STC32G144K246 专有**：紧接着执行 **4 个 `_nop_()`**（逐飞官方注释：“STC32G144K 是**多级流水线**的指令系统，所以操作完毕后加 **4 个 NOP**，保证 `IAP_DATA` 读出数据准确”）。
7. 读操作：`IAP_DATA` 中即结果，先取出到临时变量。
8. 擦除操作：触发后硬件需要 **约 4~6ms**（1 页 = 512 字节），必须**等够时间**再继续（逐飞用 `system_delay_ms(10)`）。
9. 恢复中断：`EA = ea;`
10. 收尾：`IAP_CONTR = 0;`（或保留使能但把 `IAP_CMD` 清 0）。

**逐飞官方触发函数原文（这是本节最硬的证据）**：

```c
void eeprom_trig(void)
{
    IAP_TRIG = 0x5A;
    IAP_TRIG = 0xA5;                    //先写5AH，后写A5H到IAP触发寄存器，每次都要这样
    //写入A5H，IAP触发寄存器后，IAP命令开始执行
    //CPU等待IAP完成后，才会继续执行程序
    _nop_();   //由于STC32G144K是多级流水的指令系统，所以操作完毕后加4个NOP，保证IAP_DATA数据读出准确
    _nop_();
    _nop_();
    _nop_();
}
```

**为什么必须关中断**：触发序列是“写 0x5A → 写 0xA5”两步，中间若被中断打断且中断里有任何 SFR 访问/耗时操作，可能破坏触发条件；同时擦除期间 CPU 会被硬件挂起约 4~6ms，会打乱中断时序。**并且绝对不要在中断服务程序里做 EEPROM 写/擦除。**

### 来源

- 逐飞官方驱动 `zf_driver_eeprom.c`（`eeprom_trig()` 与 `iap_read_byte/iap_write_byte/iap_erase_page` 全文，含 4 个 NOP 的原始注释）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_driver/zf_driver_eeprom.c>
- STC32G 数据手册（“先送 5Ah，到 ISP/IAP 触发寄存器”、“写触发命令(0x5a)”，PDF 第 312 页示例代码）：
  <https://www.stcaimcu.com/data/download/Datasheet/STC32G.pdf>
- CSDN《STC32G 单片机 EEPROM 操作实例》（“在设置完操作地址与操作命令后必须分别向 IAP_TRIG 分别写入 0x5A、0xA5，才能使命令生效”）：
  <https://blog.csdn.net/billliu66/article/details/128053591>

---

## 5. 可直接编译的 EEPROM 读写函数

### 5.1 代码前置说明（必读）

1. **目标编译器**：MDK FOR C251（Keil C251）。C251 里 `unsigned long` 为 32 位，足够容纳 24 位地址。
2. **头文件**：使用官方 `STC32G.H`。若你的 `STC32G.H` 版本**没有** `IAP_ADDRE` 定义（部分旧版头文件确实没有），把代码里那一行 `sfr IAP_ADDRE = 0xF6;` 的注释去掉即可（代码中已给出）。
3. **`EA` 关中断/开中断**：用变量保存原值再恢复，不用无条件 `EA = 1`（否则会破坏调用者的临界区嵌套）。
4. **`EEPROM_BASE_ADDR` 的含义**（对应题目第 5、6 节的“起始地址怎么在代码里体现”）：
   - 按 STC32G 系列的手册与逐飞官方例程，**IAP 视角下 EEPROM 的地址从 `0000H` 开始**，与 STC-ISP 里分配多大 EEPROM 无关（大小由 STC-ISP 决定，起始永远是 IAP 的 0 地址）。
   - 所以代码里 `EEPROM_BASE_ADDR` 取 `0x000000UL`；**“STC-ISP 里设置的 EEPROM 起始地址”不需要在代码里换算**，代码只需要保证：
     - 访问的偏移量 `< 你在 STC-ISP 里分配的 EEPROM 大小`；
     - **程序占用空间 + EEPROM 大小 ≤ 芯片 Flash 总容量（STC32G144K246 为 246KB）**。
   - 对 STC32G144K246，另有论坛笔记主张 IAP 访问 EEPROM 要用“存储器绝对地址”（详见 §6 的存疑说明），代码里因此把基址做成**一个宏**，便于切换。
5. **延时**：擦除 1 页需要 4~6ms。下面给出一个**自包含的粗延时**（不依赖你工程的延时库），但它**必须按实际主频标定**；如果你工程里已有可靠的 `system_delay_ms()` / 定时器延时，请直接替换 `eeprom_delay_ms()` 的函数体。代码里也给出了“用 `IAP_TPS` 反推循环次数”的近似写法。

### 5.2 `eeprom.h`

```c
/* ============================================================================
 *  eeprom.h   —  STC32G144K246  IAP / DataFlash(EEPROM) 参数存储驱动
 *  适用：STC32G144K246（251 内核），Keil C251 (MDK FOR C251)
 *  依赖：官方 STC32G.H
 *  说明：字节读 / 字节写 / 512 字节页擦除；关中断保护；STC32G144K246 专有
 *        IAP_ADDRE 与触发后 4 个 NOP 均已处理。
 * ==========================================================================*/
#ifndef __EEPROM_H
#define __EEPROM_H

#include "STC32G.H"
#include <intrins.h>

/* ---------- 若你的 STC32G.H 未定义 IAP_ADDRE，请取消下面一行的注释 ---------- */
/* sfr IAP_ADDRE = 0xF6; */

/* ---------------------------------------------------------------------------
 * 配置区：必须与 STC-ISP「硬件选项 / 用户 EEPROM 大小」的设置一致
 * -------------------------------------------------------------------------*/
#define FOSC_HZ             96000000UL  /* 系统主频(Hz)，必须与实际一致！        */
#define EEPROM_BASE_ADDR    0x000000UL  /* IAP 视角下 EEPROM 起始地址(见 5.1-4)  */
#define EEPROM_SIZE_BYTES   (4UL*1024UL) /* STC-ISP 里"用户EEPROM大小"，此处 4KB */
#define EEPROM_PAGE_SIZE    512U        /* STC32G144K246 一页 512 字节(已核实)    */

/* IAP 命令（不要用 _def.h 里的宏名，见 §3 的"头文件与手册不符"说明）*/
#define IAP_CMD_IDLE        0x00
#define IAP_CMD_READ        0x01
#define IAP_CMD_WRITE       0x02
#define IAP_CMD_ERASE       0x03

void            EEPROM_Init(void);
void            EEPROM_Idle(void);
unsigned char   EEPROM_ReadByte(unsigned long addr);
void            EEPROM_WriteByte(unsigned long addr, unsigned char dat);
void            EEPROM_EraseSector(unsigned long addr);
void            EEPROM_ReadBytes(unsigned long addr, unsigned char *buf, unsigned int len);
void            EEPROM_WriteBytes(unsigned long addr, const unsigned char *buf, unsigned int len);
unsigned char   EEPROM_AddrIsValid(unsigned long addr);

#endif /* __EEPROM_H */
```

### 5.3 `eeprom.c`

```c
/* ============================================================================
 *  eeprom.c   —  STC32G144K246  IAP / DataFlash(EEPROM) 参数存储驱动
 * ==========================================================================*/
#include "eeprom.h"

/* ---------------------------------------------------------------------------
 * 内部：粗略毫秒延时（自包含，不依赖工程延时库）
 *   ★★ 必须按实际主频标定！★★
 *   下面用 FOSC_HZ 估算：C251 上一条简单循环约 4~8 个时钟，
 *   这里取每次内层循环约 8 个时钟，再乘 20 倍安全系数，宁慢勿快。
 *   若工程里已有 system_delay_ms()，请把函数体换成 system_delay_ms(ms)。
 * -------------------------------------------------------------------------*/
static void eeprom_delay_ms(unsigned int ms)
{
    unsigned int i;
    unsigned int j;
    /* 每毫秒循环次数 ≈ FOSC_HZ / 1000 / 8 / 20(安全系数) */
    unsigned int per_ms = (unsigned int)(FOSC_HZ / 1000UL / 8UL / 20UL);

    while (ms--)
    {
        for (i = 0; i < per_ms; i++)
        {
            for (j = 0; j < 20; j++)
            {
                _nop_();
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * 内部：触发 IAP 操作
 *   顺序固定：0x5A → 0xA5；之后必须补 4 个 NOP
 *   （STC32G144K 是多级流水线指令系统，见 §4）
 * -------------------------------------------------------------------------*/
static void iap_trig(void)
{
    IAP_TRIG = 0x5A;
    IAP_TRIG = 0xA5;        /* 写入 0xA5 后命令开始执行，CPU 等待 IAP 完成 */
    _nop_();
    _nop_();
    _nop_();
    _nop_();
}

/* ---------------------------------------------------------------------------
 * EEPROM_Init
 *   使能 IAP 并设置等待时间 IAP_TPS。
 *   ★ 必须在系统时钟(含 PLL)配置完成之后调用；改主频后要重新调用。
 *   ★ IAP_TPS = Fosc/1MHz + 1（逐飞官方库取值，见 §2）
 * -------------------------------------------------------------------------*/
void EEPROM_Init(void)
{
    IAP_CONTR = 0x80;                       /* IAPEN = 1，使能 EEPROM 操作 */
    IAP_TPS   = (unsigned char)(FOSC_HZ / 1000000UL) + 1;
    IAP_CMD   = IAP_CMD_IDLE;               /* 清命令，处于待机 */
}

/* ---------------------------------------------------------------------------
 * EEPROM_Idle：失能 IAP（长时间不做参数存储时可调用，降低误触发风险）
 * -------------------------------------------------------------------------*/
void EEPROM_Idle(void)
{
    IAP_CMD   = IAP_CMD_IDLE;
    IAP_CONTR = 0x00;                       /* IAPEN = 0 */
}

/* ---------------------------------------------------------------------------
 * EEPROM_AddrIsValid：地址越界检查
 *   返回 1 = 合法，0 = 越界（越界时不执行任何操作，避免误擦程序区）
 * -------------------------------------------------------------------------*/
unsigned char EEPROM_AddrIsValid(unsigned long addr)
{
    if (addr < EEPROM_BASE_ADDR)                          return 0;
    if (addr >= (EEPROM_BASE_ADDR + EEPROM_SIZE_BYTES))    return 0;
    return 1;
}

/* ---------------------------------------------------------------------------
 * EEPROM_ReadByte：从 EEPROM 读一个字节
 * -------------------------------------------------------------------------*/
unsigned char EEPROM_ReadByte(unsigned long addr)
{
    unsigned char dat;
    unsigned char ea_flag;

    if (!EEPROM_AddrIsValid(addr)) return 0xFF;

    ea_flag = EA;                           /* 保存中断状态 */
    EA = 0;                                 /* 关中断，保护触发序列 */

    IAP_CMD   = IAP_CMD_READ;

    IAP_ADDRL = (unsigned char)(addr & 0xFF);
    IAP_ADDRH = (unsigned char)((addr >> 8)  & 0xFF);
    IAP_ADDRE = (unsigned char)((addr >> 16) & 0xFF);   /* STC32G144K246 专有 */

    iap_trig();

    dat = IAP_DATA;                         /* NOP 之后读，数据已稳定 */

    EA = ea_flag;                           /* 恢复中断状态 */
    return dat;
}

/* ---------------------------------------------------------------------------
 * EEPROM_WriteByte：向 EEPROM 写一个字节
 *   ★ Flash 只能把 1 写成 0。若目标字节当前不是 0xFF，
 *     必须先 EEPROM_EraseSector() 擦整页，再回写整页数据。
 * -------------------------------------------------------------------------*/
void EEPROM_WriteByte(unsigned long addr, unsigned char dat)
{
    unsigned char ea_flag;

    if (!EEPROM_AddrIsValid(addr)) return;

    ea_flag = EA;
    EA = 0;

    IAP_CMD   = IAP_CMD_WRITE;

    IAP_ADDRL = (unsigned char)(addr & 0xFF);
    IAP_ADDRH = (unsigned char)((addr >> 8)  & 0xFF);
    IAP_ADDRE = (unsigned char)((addr >> 16) & 0xFF);

    IAP_DATA  = dat;

    iap_trig();

    EA = ea_flag;
}

/* ---------------------------------------------------------------------------
 * EEPROM_EraseSector：擦除 addr 所在的 1 页（512 字节）
 *   页首地址 = addr & ~(512-1)。擦除后该页全为 0xFF，耗时约 4~6ms。
 * -------------------------------------------------------------------------*/
void EEPROM_EraseSector(unsigned long addr)
{
    unsigned long page_addr;
    unsigned char ea_flag;

    if (!EEPROM_AddrIsValid(addr)) return;

    page_addr = addr & ~((unsigned long)EEPROM_PAGE_SIZE - 1UL);  /* 512 字节对齐 */

    ea_flag = EA;
    EA = 0;

    IAP_CMD   = IAP_CMD_ERASE;

    IAP_ADDRL = (unsigned char)(page_addr & 0xFF);
    IAP_ADDRH = (unsigned char)((page_addr >> 8)  & 0xFF);
    IAP_ADDRE = (unsigned char)((page_addr >> 16) & 0xFF);

    iap_trig();

    EA = ea_flag;

    eeprom_delay_ms(10);        /* 1 页(512B)擦除约 4~6ms，等 10ms 留余量 */
}

/* ---------------------------------------------------------------------------
 * 多字节读 / 写（写不负责擦除，调用者需先擦页）
 * -------------------------------------------------------------------------*/
void EEPROM_ReadBytes(unsigned long addr, unsigned char *buf, unsigned int len)
{
    while (len--)
    {
        *buf++ = EEPROM_ReadByte(addr++);
    }
}

void EEPROM_WriteBytes(unsigned long addr, const unsigned char *buf, unsigned int len)
{
    while (len--)
    {
        EEPROM_WriteByte(addr++, *buf++);
    }
}
```

### 5.4 典型调用（保存一页参数的标准做法）

```c
/* 参数结构：保存 Kp / Kd / 目标速度 等 */
typedef struct
{
    float   kp;
    float   kd;
    int     base_speed;
    unsigned char magic;      /* 首次使用判据：0xA5 表示参数有效 */
} CarParam_t;

#define PARAM_ADDR   0x000000UL      /* 必须 < EEPROM_SIZE_BYTES */
static CarParam_t g_param;

/* 读参数（上电调用） */
void Param_Load(void)
{
    unsigned char i;
    unsigned char *p = (unsigned char *)&g_param;

    EEPROM_Init();
    for (i = 0; i < sizeof(CarParam_t); i++)
    {
        p[i] = EEPROM_ReadByte(PARAM_ADDR + i);
    }
    if (g_param.magic != 0xA5)       /* 第一次上电/被擦除，装载默认值 */
    {
        g_param.magic      = 0xA5;
        g_param.kp         = 1.0f;
        g_param.kd         = 5.0f;
        g_param.base_speed = 600;
        Param_Save();
    }
}

/* 写参数（改完参数后调用，注意整页读-改-擦-写） */
void Param_Save(void)
{
    unsigned char page_buf[512];
    unsigned char i;
    unsigned char *p = (unsigned char *)&g_param;

    /* 1) 读出整页 */
    EEPROM_ReadBytes(PARAM_ADDR & ~0x1FFUL, page_buf, 512);
    /* 2) 修改要改的字节 */
    for (i = 0; i < sizeof(CarParam_t); i++)
    {
        page_buf[(PARAM_ADDR & 0x1FFUL) + i] = p[i];
    }
    /* 3) 擦页 */
    EEPROM_EraseSector(PARAM_ADDR);
    /* 4) 回写整页 */
    EEPROM_WriteBytes(PARAM_ADDR & ~0x1FFUL, page_buf, 512);
}
```

> **⚠️ 重要提醒**：`page_buf[512]` 占 512 字节 RAM。STC32G144K246 有 16K edata + 128K xdata（官方规格页已核实），空间充裕，但若声明在 `data` 区会紧张，建议加 `xdata` 修饰（例如 `static unsigned char xdata page_buf[512];`）。逐飞官方库的 `extern_iap_write_buff()` 也是这样用 512 字节栈/局部数组做“读-改-擦-写”的。

### 来源

- 本节的寄存器名、`IAP_CONTR=0x80`、`IAP_TPS=Fosc/1e6+1`、`IAP_CMD=1/2/3`、`0x5A/0xA5` 触发、4×`_nop_()`、擦除后 `system_delay_ms(10)`、“1 页 512 字节”、`addr & 0xFE00` / `addr & 0x1FF` 的页对齐写法、以及 `extern_iap_write_buff()` 的“读-改-擦-写”流程，**全部来自逐飞科技 STC32G144K246 官方开源库驱动的实际源码**：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_driver/zf_driver_eeprom.c>
  头文件（函数原型）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_driver/zf_driver_eeprom.h>
- 官方例程（`iap_init(); iap_erase_page(0); iap_write_buff(0x00, write_buff, 8); iap_read_buff(0x00, read_buff, 8);`，并注明“使用之前务必查看 MDK 文件夹下面的【必看】STC-ISP设置.png”）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Example/Coreboard_Demo/E08_flash_demo/user/main.c>
- STC32G 数据手册 EEPROM 章节示例（`0x5A` 触发、关中断写法）：
  <https://www.stcaimcu.com/data/download/Datasheet/STC32G.pdf>
- STC 官方英文数据手册（STC32G 系列 IAP 汇编/寄存器示例）：
  <https://www.mikrocontroller.net/attachment/613402/stc32g-en.pdf>

---

## 6. IAP 可操作地址范围、扇区大小、STC-ISP EEPROM 设置与代码的一致性

### 6.1 扇区（页）大小 —— ✅ 已核实为 512 字节

STC 官方英文产品页对 STC32G144K246 的原文：

> “Supports user-configurable part of Flash used as DATA FLASH/EEPROM, **512 bytes single page erased**, typical endurance over 100,000 cycles.”

逐飞官方驱动的注释也一致：

```c
// 擦除地址addr所在的第一页（1页/512字节）
// iap_erase_page(0); // 擦除0x00到0x100的扇区
// addr地址：0-511为一页数据, 512-1023为一页数据, 1024-1535为一页数据……
```

→ **页 = 扇区 = 512 字节**，页对齐掩码 `addr & ~0x1FF`（等价于逐飞源码里的 `addr & 0xFE00`）。
→ 擦写寿命典型 **> 10 万次**（官方页面原文 “typical endurance over 100,000 cycles”）。

### 6.2 可操作地址范围

- **芯片存储规模**（STC 官方英文产品页，✅ 已核实）：**246 KB Flash（程序存储）**，SRAM 合计 **144K + 4K**（16K edata + 128K xdata + 4K 高端扩展 RAM）。
- **EEPROM 容量**：由用户在 STC-ISP 中从 Flash 里划分，**大小可配置**，不是固定值。
- **IAP 视角下的地址**：**从 `0000H` 开始**（见下）。
  - 依据 1（STC 官方手册，同属 251 内核的 STC32F12K54）：*“例如 STC32F12K54 这个型号的 FLASH 为 54K，此时若用户想分出其中的 8K 作为 EEPROM 使用，则 EEPROM 的物理地址则为 54K 的最后 8K，物理地址为 B800h~D7FFh，**当然，用户若使用 IAP 的方式进行访问，目标地址仍然从 0000h 开始**”*。
  - 依据 2（逐飞官方 STC32G144K246 例程）：直接用 `iap_erase_page(0)` 和 `iap_write_buff(0x00, ...)` 读写 EEPROM，**说明传入的就是 EEPROM 相对偏移**。
- 由此可推出**可操作范围**：`0x000000 ~ (STC-ISP 中分配的 EEPROM 大小 - 1)`；代码侧的总约束是
  **程序占用 Flash + EEPROM 大小 ≤ 246 KB**。

> **⚠️ 存疑点（必须知道，但不影响按官方例程开发）**
> CSDN 上一份《STC32G144K246 开发注意事项笔记》第 10 条写着：
> “**用户系统区，IAP 操作 EEPROM 要使用存储器绝对地址**，IAP 不能读取用户系统区空间，但是指针可以：使用 IAP 读取用户程序区起始空间，地址：0xFF0000，这个 0xFF0000 是存储器绝对地址。”
> 而 CSDN 上另一批 STC32G12K128 的文章则说“无论 EEPROM 设置为多少，EEPROM 的地址始终从 **FE:0000h** 开始”。
> **两种说法（IAP 用 0 基址 / IAP 用绝对地址 0xFE0000 一类）互相冲突，本次未能读到官方手册对应章节原文（论坛与 PDF 直取均受限），因此该点标注为 ⚠️ 未完全确认。**
> **本报告的处置方式**：
> 1. 代码里把基址做成宏 `EEPROM_BASE_ADDR`，默认 `0x000000UL`（与逐飞可运行官方例程一致），若要改成绝对地址只需改这一个宏；
> 2. 若怀疑基址不对，**先用“读”去验证**（读不会破坏 Flash）：读 `0x000000` 与读 `0xFE0000`，哪个能稳定读到 EEPROM 里的已知数据，哪个就是对的；
> 3. **在确认之前不要对 `0x000000` 执行擦除**——若基址其实是绝对地址，擦 `0x000000` 会擦掉程序复位向量（这种情况**可以通过重新下载 HEX 恢复**，不是永久损坏，但没必要踩）。
> 4. 依据来源：
>    - CSDN《STC32G144K246 开发注意事项笔记》（第 10 条原文）：
>      <https://blog.csdn.net/czhaii/article/details/157735488>
>    - CSDN《STC32G 库函数实战：EEPROM 数据存储与读取全解析》（“无论设置为 1K/4K/64K，EEPROM 起始地址固定 0xFE0000”，24 位）：
>      <https://blog.csdn.net/weixin_28432777/article/details/158958036>
>    - CSDN《STC32G 库函数（四）——EEPROM》（“EEPROM 的地址始终从 FE:0000h 开始”）：
>      <https://blog.csdn.net/wd0710/article/details/127596098>
>    - STC32F12K54 官方手册（“使用 IAP 的方式访问，目标地址仍然从 0000h 开始”）：
>      <http://www.stcmcudata.com/STC8F-datasheet/STC32F12K54.pdf>

### 6.3 STC-ISP「EEPROM 设置」与代码的一致性 —— 怎么处理

**结论：设置与代码的分工是这样的**

| 谁负责 | 负责什么 |
|---|---|
| **STC-ISP（下载软件，非代码）** | 从 246KB Flash 中**划出多大一块当 EEPROM**（“用户 EEPROM 大小 / Data Flash Size”），并决定下载时是否擦除它 |
| **代码（IAP 驱动）** | 只用 **0 基址的偏移**去读写这块空间；**不关心它在物理 Flash 里的绝对位置** |

因此“STC-ISP 里填的地址需要与代码一致”这件事，**正确的处理方式是三条约束，而不是去换算地址**：

1. **容量约束**：代码里出现的最大偏移量必须 `< STC-ISP 中设置的 EEPROM 大小`。
   例：STC-ISP 设 4KB → 代码里最大只能访问 `0x0000 ~ 0x0FFF`。
   （本报告代码用 `EEPROM_SIZE_BYTES` 宏 + `EEPROM_AddrIsValid()` 做运行时越界拦截，越界直接不执行，防止误擦程序区。）
2. **空间不重叠约束**：`程序实际占用 Flash` + `EEPROM 大小` ≤ 246KB。
   程序较大时（STC32G144K246 很容易超 64KB），要在 STC-ISP / Keil 里同时确认“用户程序区大小”和 EEPROM 空间不重叠。
   ⚠️ STC32G144K246 特有坑（同一份注意事项笔记）：*“xdata 64K 之后区域要使用 far 关键字”*、*“STC32G144K246 程序超过 64K 后报错问题”* —— 程序大于 64K 时 Keil 的 ROM 配置与 far 指针需要额外设置。
   **已核实的一条关键线索**：逐飞官方库版本说明 V3.0.1 明确写着 *“Code Rom Size 默认开启 Huge，方便用户使用 64K 以上的 FLASH 空间”*，即 **Keil C251 工程里 `Code Rom Size` 应选 `Huge`**。
   来源（库版本文件）：
   <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/doc/version.txt>
   （本次核实的库版本为 **V3.2.5 / 2026-06-27**）
   来源：<https://blog.csdn.net/czhaii/article/details/157735488> ，
   <https://www.stcaimcu.com/thread-24643-1-1.html> ，<https://www.stcaimcu.com/thread-22811-1-4.html>
3. **“擦除用户 EEPROM”选项必须先关掉**：
   STC-ISP 下载时若勾选了“擦除用户 EEPROM / 下载时擦除 Data Flash”，**每次重新烧录程序都会把你保存的参数清掉**，这会让人误以为“代码写不进去”。
   来源（该文专门讲这个坑）：<https://blog.csdn.net/iii12/article/details/150542259>

**逐飞官方例程的原始提示**（最直接的“操作指引”）：
> `// 使用之前务必查看 ..\Coreboard_Demo\E08_flash_demo\MDK文件夹下面的【必看】STC-ISP设置.png`

该截图是逐飞给的 STC-ISP 设置示范：
<https://gitee.com/seekfree/STC32G144K246_100Pin_Library/blob/master/Example/Coreboard_Demo/E08_flash_demo/mdk/%E3%80%90%E5%BF%85%E7%9C%8B%E3%80%91STC-ISP%E8%AE%BE%E7%BD%AE.png>
（⚠️ 本次尝试下载该 PNG 逐像素查看，受沙箱网络限制未能取得完整图片，**截图内的具体数值未确认**；上面的三条约束是通行做法，来源见各条注脚。）

### 6.4 上电/掉电保存的工程建议（避免踩坑）

1. **不要每个控制周期都写 EEPROM**。10 万次寿命按 100Hz 写 = 不到 17 分钟就写坏。只在“用户改参数”或“掉电瞬间”写一次。
2. **写之前先判断是否需要写**：读出旧值，相同则直接返回，避免无谓擦写。
3. **掉电保存**：靠低压检测（LVD）中断，在掉电中断里只做**一次**页写；不要在 LVD 里做整页“读-改-擦-写”（时间可能不够，且擦除 4~6ms 可能来不及）。
4. **首次上电判据**：用 magic 字节（如 `0xA5`）判断参数是否有效，无效则装载默认值。
5. **不要在中断里做 EEPROM 擦/写**；擦除期间 CPU 会被硬件挂起约 4~6ms。

### 来源

- STC 官方英文产品页（246KB Flash、512 字节单页擦除、>10 万次擦写、16K edata + 128K xdata + 4K）：
  <https://www.stcmicro.com/stc/stc32g144k246.html>
- 逐飞官方驱动与例程（512 字节页、`addr & 0xFE00` 对齐、擦除后延时、“必看 STC-ISP 设置.png”提示）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_driver/zf_driver_eeprom.c>
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Example/Coreboard_Demo/E08_flash_demo/user/main.c>
- STC32F12K54 官方手册（“使用 IAP 的方式访问，目标地址仍然从 0000h 开始”）：
  <http://www.stcmcudata.com/STC8F-datasheet/STC32F12K54.pdf>
- CSDN《STC32G 的 EEPROM 避坑指南：为什么你的数据总丢失？烧录设置+库函数详解》：
  <https://blog.csdn.net/iii12/article/details/150542259>
- CSDN《STC32G144K246 开发注意事项笔记》（IAP 绝对地址、far 关键字、SPI/PLL 等）：
  <https://blog.csdn.net/czhaii/article/details/157735488>
- STC 官方论坛相关讨论帖（抓取受限，给出链接）：
  <https://www.stcaimcu.com/thread-22811-1-4.html>（程序 >64K 时 EEPROM 等参数如何配置）
  <https://www.stcaimcu.com/thread-24643-1-1.html>（程序超过 64K 后报错）
  <https://www.stcaimcu.com/forum.php?mod=viewthread&tid=7994>（用户 EEPROM 大小的起始地址）

---

# 第二部分：电磁循迹算法与工程参考

> 本部分由专项检索完成，逐条附来源 URL。凡本次未能找到可靠来源的，明确标注 **❌ 未确认**。

## 7. 电磁组电感布置惯例

### 7.1 常用几个电感

- **主流是 2~6 个**。李敏《智能汽车设计与实践基础》原文：“电磁循线中最核心的部分是电感的个数与布局，**常见电感个数为 2~6 个**”，书中给出两/三/四/五/六电感五套安装示意与偏差公式。
  - **2 个**：左右两个横电感。入门够用，直道好；弯道数据不足，“严重的会导致十字直接冲出赛道”。
  - **3 个**：左中右三个横电感。工程上极常见（18/19 届 STC32G 三电感方案都有实例）。
  - **4 个**：典型是「2 横 + 2 竖」（李敏书四电感方案）或「2 横 + 2 斜（八字）」。
  - **5 个**：两种形态——①「横-竖-横-竖-横」；②「3 横 + 2 竖」（李敏书五电感方案）。
  - **6 个**：李敏书六电感方案 = 用两个对称横电感代替中间那一个横电感；实际队伍也有「2 横 + 2 斜 + 2 竖」。
  - **7 个**：18 届负压电磁组有队伍用「**3 水平 + 2 竖直 + 2 内八（斜置）**」，自述为“4 电感循迹 + 3 电感判元”。
- **规则层面不限数量**：第十七届竞速规则明确“传感器的数量由参赛队伍自行确定，不再限制”。

> ⚠️ **重要修正（与任务描述中“水平4+竖直2”“水平5”“水平3+竖直2”的对照）**
> 本次检索**未在任何实际抓取来源中原样出现这三个术语**：
> - **“水平 4 + 竖直 2”（共 6 个）**：功能等价物（4 水平 + 2 竖直，或 2 水平 + 2 竖直 + 2 斜置）有来源支持，但该命名 **❌ 未确认**。
> - **“水平 5”（纯 5 个水平电感）**：**❌ 未确认**。已确认的 5 电感形态**均含竖直电感**（3 横 + 2 竖 或 横竖交替）。
> - **“水平 3 + 竖直 2”**：李敏书“五电感方案”实质相同（3 横 + 2 竖），但书中未用该术语，该命名 **❌ 未确认**。
>
> 建议在方案文档里改用可核实的说法：「3 横 + 2 竖（五电感）」/「2 横 + 2 竖（四电感）」/「3 水平 + 2 竖直 + 2 内八（七电感）」。

### 7.2 水平 / 竖直 / 斜置电感怎么摆、朝哪

**物理前提（多来源一致）**：赛道中心线通 **20 kHz、100 mA** 交变电流；**漆包线与电感轴线垂直时感应电动势最大**；电感越近值越大。

| 类型 | 轴线朝向 | 摆法 | 主要作用 |
|---|---|---|---|
| **水平（横）** | 水平且与前进方向垂直（横向） | 车头前方横杆上左右对称，常取横杆最外侧 | **直道循迹主力**；横向覆盖大，但中心附近有“区分度差的平台” |
| **竖直（竖）** | **垂直地面** | 紧挨外侧水平电感，左右各一 | **元素识别 / 环岛 / 十字**；对正下方导线最敏感、动态范围大；“普通赛道上值几乎很小，只有环岛等特殊元素才会突增” |
| **八字 / 斜置 / 内八** | 水平但与横向约成 **45°** | 最外侧左右各一，或与水平电感成对 | **弯道灵敏度远大于横电感**，“提前嗅到圆环、十字、坡道入口” |

**关键工程约束（均有来源）**
- **左右必须完全对称**：高度、倾角一致，引线尽量等长。“只要有一路电感歪 2 度，停车状态下读出的一排数据就会偏，归一化后表现为固定方向跑偏”。
- **相邻两电感至少隔 2 cm**：否则产生**互感**，“过十字的时候车身会产生振动”。
- **横电感与竖电感不能贴太近**：李敏书原文——“横电感与竖电感距离过近时会发生**谐振现象**，直道上竖电感与通电导线平行，理论上竖电感是没有值的，但如果发生谐振现象，竖电感也会有较大的值，会严重影响正常的偏差计算”。
- **电感支架禁用金属件**；电感阵列下方不宜大面积铺地铜（涡流影响有效 Q 值）。
- **调试方法**：可调角度支架 + 热熔胶临时固定 → 上路跑几圈看信号 → 再环氧固化。“这个调试过程不能省，它直接决定了你传感器性能的上限”。

### 7.3 常见组合优缺点

| 组合 | 优点 | 缺点 |
|---|---|---|
| **水平 2** | 电路最简单、参数最少；直道循迹好 | 弯道数据不足，只能大致判断偏移方向；十字易冲出赛道 |
| **水平 3**（左中右） | 中间电感进入差比和**分母**后，偏差与偏移距离呈**近似二次关系**，过弯更易切内道；可判坡道/十字/环岛 | 中间电感在环岛交叉处输出是普通位置的 **3 倍以上**，必须把它的放大倍数调小；参数更多 |
| **2 横 + 2 竖**（四电感） | 竖电感差比和加入后弯道灵敏度远大于横电感，更好切内道；竖电感可用于环岛循线 | 横竖靠近易谐振；**直道上竖电感≈0，必须限定归一化值不低于 1，否则出现 0/0 程序 BUG** |
| **3 横 + 2 竖**（五电感） | “融合了三电感循线和四电感循线的优点”，坡道/环岛/十字识别更稳定及时，从而提升速度 | “增大了参数调试方面的难度” |
| **六电感** | 不再单靠最外侧横电感，冗余更高 | 电感越多安装位置越近，易互相谐振 |
| **七电感**（3 水平 + 2 竖直 + 2 内八） | 循迹（4 个）+ 判元（3 个）分工明确，元素前瞻性最好 | 结构最复杂、调参量最大 |

### 7.4 电感间距、高度、前瞻的经验值

| 项目 | 已确认的数值 | 来源 |
|---|---|---|
| 四横两斜的横向位置 | 4 个水平线圈在车模前上方左右对称排布于 **−10 cm、−5 cm、+5 cm、+10 cm**；最外侧对称排布**倾角 45°** 的斜线圈 | 《电磁智能车原理》 |
| 相邻电感最小间距 | **≥ 2 cm** | 电子发烧友《工字电感分布认知》 |
| 电磁杆宽度上限 | “要尽量安装于支撑横杆的外侧，但同时要满足**比赛规定宽度不超过 25 cm** 的限制”（该书成书时期规则） | 李敏书 |
| **电感离地高度** | **建议 10~15 cm**；“太高会造成电感值过小，太低会造成电感值过大” | 《智能车竞赛调参实战》/ 李敏书 / 调车日记（“电感高度十厘米多，离地正常高度 2000 为益”） |
| 斜置角度 | **45°**（八字/内八） | 《电磁智能车原理》/《智能车竞赛调参实战》 |

> ⚠️ **未确认**：以“赛道宽度 20 cm”为自变量给出“水平电感间距取 xx cm”的**显式经验公式或对照表**本次未找到。已确认的只是电磁杆上电感分布位置的具体数值（±5 / ±10 cm）。
> ⚠️ **未确认**：**前瞻距离的具体 cm 数值**（20/25/30 cm）未在任何来源中作为推荐值出现。只有定性结论：“为了更精确采集赛道电磁值以至于不失真，需将**电磁前瞻的高度稍微降低**”；“增加两个电感采用四电感，S 型拐弯会遇到问题，**调整前瞻长度**”。
> ⚠️ **未确认**：智能车赛道标准宽度数值（官方规则以图片给出）。

### 7.5 工字电感 vs 空心电感、电感量与谐振电容

**已确认**
- **10 mH 工字电感是绝对主流默认值**：李敏书“磁感应线圈可以自行绕制，也可以直接使用 **10 mH 工字电感**”；“一般采用 **10 mH 工字电感和 6.8 nF 匹配电容**”；“L = 10 mH、C = 6.8 nF 时，谐振频率为 **19.3 kHz**，接近规定的 20 kHz”。
- **谐振电容**：f₀ = 1/(2π√(LC))；20 kHz + 10 mH → 理论 **6.33 nF**，6.2 nF 非标称值，故实际“**5.6 nF 和 560 pF 并联**得到接近 6.2 nF”；要求“与 20 kHz 谐振后频率在 **19.8~20.2 kHz**”。李敏书实测谐振频率 19.992 kHz（误差 0.4‰），峰峰值 2.32 V；“若输出信号峰峰值**大于 2 V**，即可满足使用要求”。电容建议 **C0G/NPO** 材质。
  由公式推出的常用标称对照：10.0 mH→6330 pF→取 **6.8 nF**；8.2 mH→7720 pF→取 8.2 nF；6.8 mH→9310 pF→取 10 nF；5.0 mH→12660 pF→取 12 nF。
- **空心/自绕电感合法**：书籍承认“磁感应线圈可以自行绕制”。
- **150 kHz 新方向的器件现状**（卓晴 2026 年实验，若你走 150 kHz 方案直接相关）：逐飞制作的**正交工字型电感为 1 mH**，在 150 kHz 下最佳谐振电容为 **1 nF**；“等腰”工字电感实际电感量 1.13 mH，匹配电容应为 **1.1 nF**，用 1 nF 会失谐、灵敏度下降、相位偏移、角度解算出现突变；结论是**“细腰”工字型电感更适合作为传感器**。

> ⚠️ **未确认**：**6.8 mH 作为电磁组电感量**——本次未在任何来源中见到。6.8 这个数字在来源中一律是**谐振电容 6.8 nF**，怀疑“6.8 mH”来自对“6.8 nF”的混淆。**请勿在方案里写“6.8 mH 电感”**。

### 7.6 来源

1. 李敏《智能汽车设计与实践基础》2.1.2「电磁传感器」（电感个数 2~6、两/三/四/五/六电感布局、10 mH 工字电感、5.6 nF+560 pF、25 cm 限制、竖电感谐振现象）— <https://m.zhangyue.com/readbook/12594821/8.html?p2=104508>
2. 《电磁智能车的电感排布和运行原理》— <https://m.elecfans.com/article/1957560.html>
3. 《工字电感分布认知 双水平电感排布方案》（相邻 ≥2 cm 防互感）— <https://m.elecfans.com/article/2326555.html>
4. 《八字形和双T形电感排布方案介绍》— <https://www.elecfans.com/d/2326565.html>
5. 《电磁智能车原理》（四横两斜、−10/−5/+5/+10 cm、45°、三次多项式拟合 + 动态加权、T=160）— <https://www.elecfans.com/d/1957753.html>
6. 《十八届智能车负压电磁组（一）：电感布局策略与ADC信号优化实战》（7 电感 = 3 水平 + 2 竖直 + 2 内八）— <https://blog.csdn.net/p4q5r6s7t/article/details/149951037>
7. 《智能车竞赛调参实战：手把手教你调好电磁杆的'差比和差'算法》（八字 45°、高度 10–15 cm）— <https://blog.csdn.net/weixin_30776273/article/details/159591112>
8. 《浅谈自己的电磁智能车调试之路（1）电磁》（6 电感布局与朝向、开方比值法）— <https://blog.csdn.net/abc565846881/article/details/125650728>
9. 《智能车浅谈——硬件篇》（10 mH 工字电感 + 6.8 nF、20 kHz/100 mA）— <https://blog.csdn.net/qq_41954556/article/details/122664107>
10. 卓晴《测量逐飞制作的正交工字型电感》（1 mH 正交工字电感 @150 kHz、1 nF/1.1 nF、细腰 vs 等腰）— <https://zhuoqing.blog.csdn.net/article/details/156796743>
11. 第十七届全国大学生智能汽车竞赛竞速比赛规则（传感器数量不限、外形尺寸不限）— <https://developer.aliyun.com/article/833020>
12. 《十五届恩智浦智能车-四十天做四轮-调车日记》（电感高度 10 cm 多、离地 2000 为宜）— <https://blog.csdn.net/nikohsu/article/details/108086105>
13. 《智能车—电磁循迹》（20 kHz/100 mA 漆包线、工字电感、运放调试）— <https://blog.csdn.net/weixin_52441317/article/details/131872477>

---

## 8. 归一化

### 8.1 为什么要归一化

1. **电感/运放个体差异**：“或许你左右电感的特性可能不同，也许左边电感更灵敏或者更容易变大一点”。
2. **赛道电源电流变化会让固定阈值失效**（最有说服力的论据，原文数值例）：
   > “假设不用归一化处理时，距离中线零偏差时，电感 A 的值是 1000，偏离赛道 20 厘米时，电感 A 值是 200。当赛道电源不准时，比如输出电流由 100 mA 变成了 120 mA，这时电感 A 在零偏差的值和偏离赛道 20 厘米时候的值都会变大，设分别变成了 1200 和 240，这时你设定的阈值会出问题了……本来该判丢线，却没法判丢线了。如果用归一化处理……偏差 20 cm 时，电感 A 归一值也为 0（(240−240)/(1200−240)），即电源变化对你的阈值已经没有影响了。”
3. **跨场地/跨信号源可移植**：“为了车模能够使用不同的信号源与赛道场地，需要对电磁传感器采集到的数据进行归一化处理”。
4. **纵向漂移**：电源电压波动、运放温漂、检波直流电平漂移、赛道潮湿。
5. **消除左右不对称**：“差比和……不能消除不对称，因为传感器本身可能不对称，重新标定后，能极大的改善对称性”。
6. **量纲统一便于设阈值与元素判据**：归一化后每路都在 0~100（或 0~1），元素阈值可跨场地移植。

### 8.2 三种校准方式的取舍

| 方式 | 做法 | 评价 |
|---|---|---|
| **(a) 跑前/上电标定（最推荐）** | 把车摆在赛道正中间，旋转运放模块可调电阻找每路电感的最大/最小值；“最小的一般是 0，而最大值需要在场地里在屏幕上打印裸电感值读取”；**竖着的电感就把车横过来再扫** | 来源明确推荐“**在每次车跑之前，重新快速校准偏差和电感值的对应关系**”；换赛道只重测 max/min，程序框架不动 |
| **(b) 运行中动态更新** | 每周期 `if(x<min) min=x; if(x>max) max=x;` 再代入公式 | 免标定、自适应；**风险是分母可能为 0，必须限幅与保护** |
| **(c) 在线统计 + 最小差值保护（较稳健的现代写法）** | 先做窗口 5 滑动平均；再统计 min/max；当 `max−min < 30`（12 位 ADC）时**沿用上一次归一化结果** | “避免噪声被放大成错误赛道信号”；同源模板指出“固定量程只在硬件增益绝对稳定时考虑” |
| **(d) 固定量程简化法（新手常用）** | 写死上下限（`min=0; max=4095;`）或直接 `adc[i]=(float)dat[i]/4096.0f` | 零标定成本，但失去抗电源漂移能力 |

> ⚠️ **未确认**：本次抓到的来源只谈了“跑前/上电标定”和“运行中动态更新”，**没有任何来源讨论“按键触发校准”的具体实现或优劣**。“按键校准”**❌ 未确认**（工程上当然可以做，但没有来源支撑其作为“惯例”）。

### 8.3 归一化公式与实际代码

通用公式：`y = (x − Min) / (Max − Min)`，再乘 100 映射到 0~100。

```c
/* 形式 1：固定上下限 + 限幅（电子发烧友《智能车中电磁归一化该怎么处理》） */
int AD_val_1_min = 0;
int AD_val_1_max = 4095;
if (AD_VAL1 > AD_val_1_max)  AD_VAL1 = AD_val_1_max;          /* 限幅 */
ad_VAL1 = 100 * (AD_VAL1 - AD_val_1_min) / (AD_val_1_max - AD_val_1_min);

/* 形式 2：加 1 偏置，避免 0 值与整数除法截断（电子发烧友《电磁循迹中什么是归一化》） */
AD_M_Left[0] = (uint16)(99 * (LeftAverage[0] - M_Left_min)
                             / (M_Left_max[0]  - M_Left_min) + 1);   /* 1~100 */

/* 形式 3：运行中动态刷新 max/min + 限幅
   （CSDN《智能车中电感采集的相关问题及解决方案（电磁负压组）》） */
uint16 adc_date[2];               /* 经两次滤波后的左右电感值 */
uint16 adc_max[2] = {3000, 3000}; /* 归一化电感最大值 */
uint16 adc_min[2] = {0, 0};       /* 归一化电感最小值 */
void normalize_date(void)
{
    int16 JSADC_DATE[2];          /* ★ 必须是有符号类型，否则减法下溢 */
    unsigned char i;
    for (i = 0; i < 2; i++)
    {
        if (adc_date[i] < adc_min[i]) adc_min[i] = adc_date[i];
        if (adc_date[i] > adc_max[i]) adc_max[i] = adc_date[i];
        JSADC_DATE[i] = (adc_date[i] - adc_min[i]) * 100
                        / (adc_max[i] - adc_min[i]);   /* 归一化到 0~100 */
        if      (JSADC_DATE[i] <= 0)   JSADC_DATE[i] = 0;      /* 限幅 */
        else if (JSADC_DATE[i] >= 100) JSADC_DATE[i] = 100;
    }
    Left_Adc  = JSADC_DATE[0];
    Right_Adc = JSADC_DATE[1];
}

/* 形式 4：只除以标定最大值（19 届 STC32G 实战代码，逐飞库；五路电感） */
Left_Adc   = (adc_deal_last[LEFT_1]  * 100) / adc_max[0];
Right_Adc  = (adc_deal_last[RIGHT_1] * 100) / adc_max[1];
Middle_Adc = (adc_deal_last[MIDDLE]  * 100) / adc_max[2];
Right_Adc2 = (adc_deal_last[RIGHT_2] * 100) / adc_max[3];
Right_Adc3 = (adc_deal_last[RIGHT_3] * 100) / adc_max[4];

int16 ADC_Limit(int16 in_adc, int8 max, int8 min)   /* 限幅到 0~100 */
{
    uint16 Adc_Input = in_adc;
    if      (Adc_Input >= max) Adc_Input = max;
    else if (Adc_Input <= min) Adc_Input = min;
    return Adc_Input;
}
```

```c
/* 形式 5：在线 min/max + 最小差值保护
   （CSDN《电磁组智能车从电感信号到归一化与 PD 控制全指南》） */
#define SENSOR_COUNT 8
#define FILTER_LEN   5
static uint16_t buf[SENSOR_COUNT][FILTER_LEN];
static uint8_t  idx[SENSOR_COUNT];
static float    last_norm[SENSOR_COUNT];
static float    sensor_norm[SENSOR_COUNT];

uint16_t adc_get_filtered(uint8_t ch)
{
    uint16_t sum = 0; int i;
    buf[ch][idx[ch]] = adc_read_channel(ch);
    idx[ch] = (idx[ch] + 1) % FILTER_LEN;
    for (i = 0; i < FILTER_LEN; i++) sum += buf[ch][i];
    return sum / FILTER_LEN;
}

void normalize_sensors(void)
{
    float minv = 4095, maxv = 0; int i;
    for (i = 0; i < SENSOR_COUNT; i++) {
        float v = adc_get_filtered(i);
        if (v < minv) minv = v;
        if (v > maxv) maxv = v;
    }
    if (maxv - minv < 30) {                 /* 信号整体过小：可能悬空或出界 */
        for (i = 0; i < SENSOR_COUNT; i++) sensor_norm[i] = last_norm[i];
        return;
    }
    for (i = 0; i < SENSOR_COUNT; i++) {
        sensor_norm[i] = (adc_get_filtered(i) - minv) / (maxv - minv + 1e-6f);
        last_norm[i] = sensor_norm[i];
    }
}
```

**代码级工程注意点（来源明确提到）**
- ★ **数据类型必须有符号**：归一化前做减法，用 `uint16` 会**下溢**；原注释“这个数据类型必须是有符号的 用于计算储存”。
- ★ **防 0/0**：李敏书原话——“尤其是在直道上的竖电感的值，若不加限制会出现 **0/0** 的情况，造成程序 BUG”，对策是“**提前限定电感归一化数值乘以 100 之后的值不能小于 1**”（int 型）。
- ★ **防除零的 `+1` 会牺牲小信号线性度**：差比和分母 `+1.0f` 可防除零，但“会影响小信号时的线性度”。
- **归一化前必须滤波**：工程上最常见是**去极值 + 均值双重滤波**（采 5 次或 10 次，冒泡排序后去掉最大最小再平均），再对结果做一次滑动均值。也可用加权滤波（近期数据高权重，示例 `Weight_Ration[10] = {4,6,7,8,10,15,20,30,200,700}`，权值和 1000，结果 = Σ(xᵢ·wᵢ)/Σwᵢ）。

### 8.4 为什么要做“差比和”而不是直接用差值

1. **急弯时两电感都会变小，直接作差得到的偏差反而变小，不符合实际情况**（李敏书原文，最核心）：
   > “如果单纯只计算两个横电感的差值，在某些时候会出现**车头偏转越多，偏差反而越小**的情况。例如，在急弯处，车头会伸出赛道外，所以两个电感的值都会偏小，此时直接作差得到的偏差会很小，不符合实际情况，而差比和的方式可以有效解决这个问题。”
2. **差比和把放大倍数约掉**：“差比和同样可以消除放大率的影响，基础放大倍数已经约掉”，即 (kL−kR)/(kL+kR) = (L−R)/(L+R)。
3. **输出更平滑**：“差比和能够使得获得的 Err 数据更加的平滑……不容易出现在某一位置 Err 发生突变的情况”。
4. **与归一化正交（重要认知）**：“**归一化是纵向处理**（即对各个探头的处理），**差比和是横向处理**（即对探头之间的处理）”，两者可并存；但“差和比应该在电感信号对称/一致的前提下意义才更大”，所以**先归一化再差比和**是标准顺序。

### 8.5 来源

1. 《智能车中电磁归一化该怎么处理》（公式与限幅代码）— <https://m.elecfans.com/article/2326587.html>
2. 《电磁循迹中什么是归一化》（(x−Min)/(Max−Min)、带 +1 偏置代码、0~100）— <https://www.elecfans.com/d/2326423.html>
3. 《智能车中电感采集的相关问题及解决方案（电磁负压组）》（去极值+均值双重滤波、动态刷新 max/min 归一化代码、有符号类型说明）— <https://blog.csdn.net/weixin_73821581/article/details/130370007>
4. 《【STM32 平衡小车】电磁巡线归一化算法（二）》（归一化数值例 1000/200→1200/240、跑前重标定、纵向 vs 横向处理、加权求和例程）— <https://bbs.huaweicloud.com/blogs/333658>
5. 第十六届智能车竞赛技术报告｜新余学院 开放艺术队（5.3.1 归一化 0~1；5.3.2 差比和 + 开平方改进）— <https://bbs.huaweicloud.com/blogs/320220>
6. 《电磁组-19 届智能车电磁组电感处理与循迹代码带元素处理+讲解（开源）》（去极值滤波、归一化、限幅、三电感融合）— <https://damodev.csdn.net/6864dfe1b93e2f417962d4ad.html>
7. 《【智能车】电磁循迹算法（1）——加权滤波的应用》— <https://www.cnblogs.com/ZYQS/p/14579562.html>
8. 《电磁组智能车从电感信号到归一化与 PD 控制全指南》（在线 min/max + 最小差值保护代码；竖直 vs 水平安装特性；谐振电容对照表）— <https://blog.csdn.net/weixin_29169899/article/details/165279952>

---

## 9. 偏差计算

### 9.1 加权求和法 `误差 = Σ(ADCᵢ·wᵢ) / Σ(ADCᵢ)`

**物理含义**：把每路电感的归一化值当作“磁场能量权重”，wᵢ 取该电感在车体横向上的（等效）位置坐标，则 `Σ(ADCᵢ·wᵢ)/Σ(ADCᵢ)` 就是**磁场强度的重心位置**；减去中心索引即为转向误差。这样**间距不等**的电感阵列也能统一融合。

```c
/* 例 1：3 电感，索引化权重 1 / 50 / 99（华为云社区转载《电磁巡线归一化算法（二）》） */
Sensor_Left   = analogRead(1);   /* 左边电感 */
Sensor_Middle = analogRead(2);   /* 中间电感 */
Sensor_Right  = analogRead(3);   /* 右边电感 */
if (Sensor_Left + Sensor_Middle + Sensor_Right > 25)
{
    sum    = Sensor_Left * 1 + Sensor_Middle * 50 + Sensor_Right * 99;
    Sensor = sum / (Sensor_Left + Sensor_Middle + Sensor_Right);   /* 0~99 */
}
Bias  = Sensor - 50;                              /* 提取偏差，中值 50 */
Angle = Bias * 0.65 + (Bias - Last_Bias) * 0.1;   /* 方向 PD */
Last_Bias = Bias;
/* 原文注释给出的等价非线性写法：
   Angle = abs(Bias)*Bias * 0.02 + Bias * 0.074 + (Bias - Last_Bias) * 1; */
```

```c
/* 例 2：8 路阵列，位置权重
   （CSDN《电磁组智能车从电感信号到归一化与 PD 控制全指南》） */
static const float position_weight[SENSOR_COUNT] = {
    0.2f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.2f
};
float calc_weighted_center(const float *norm)
{
    float num = 0.0f, den = 0.0f; int i;
    for (i = 0; i < SENSOR_COUNT; i++) {
        float w = norm[i] * position_weight[i];
        num += w * i;          /* 位置坐标 i = 0..7 */
        den += w + 1e-6f;
    }
    return num / den;          /* 单位：传感器索引；中心线 = 3.5 */
}
/* error = calc_weighted_center(norm) - 3.5f; */
```
原文对权重规律的解释：“权重数组不是随意定的。**中间几路对应车头正前方，权重给 1.0；边缘电感负责远侧信息，权重给 0.2~0.8**，防止其非线性区主导结果。”

**关于 wᵢ = −2, −1, 0, 1, 2**

> ⚠️ **未确认**：本次检索**未找到任何来源使用 {−2, −1, 0, 1, 2} 这一确切整数权重集**。该集合数学上完全合理（对应“5 路等间距、中心为 0”），但**以来源为准应视为未确认**。
> 已确证的权重集是：**{1, 50, 99}**（3 路，减 50 后即 −49/0/+49）、**{0.2, 0.8, 1, 1, 1, 1, 0.8, 0.2}**（8 路），以及一个 AIGC 来源的 {0.3, 0.7, 0, 0.7, 0.3}（**仅作线索，不建议采信**）。
> ⚠️ **分母写法需确认**：任务描述写作 `Σ(ADCᵢ·wᵢ) / Σ(ADCᵢ)`。上述两个已确证来源的分母都是 **Σ(ADCᵢ)**（即权重全为 1 的和），**不是** Σ(ADCᵢ·wᵢ)。若分母也用加权和，输出不再是位置重心而是无量纲比值。**来源中 `Σ(ADCᵢ)` 的写法更常见且可确证，建议采用 `Σ(ADCᵢ)`。**

### 9.2 “左右差比和” `(L−R)/(L+R)`

```c
/* 两电感：偏差 ×100 → 范围约 ±100 */
Err = (L - R) / (L + R);
ad_value[i] = 100 * (ad_value4 - ad_value1) / (ad_value4 + ad_value1);
```
符号约定（来源明确）：车偏左时 L 变小、R 变大 → Err 为**负**；车偏右 → Err 为**正**；Err = 0 表示居中。

```c
/* 三电感融合（把中间电感放进分母，偏差呈近似二次关系）——19 届 STC32G 实战代码 */
AD_Bias = ((Left_Adc - Middle_Adc) * 100 / (Left_Adc + Middle_Adc))
        - ((Right_Adc - Middle_Adc) * 100 / (Right_Adc + Middle_Adc));
/* 两电感融合（对照）
   AD_Bias = (Left_Adc - Right_Adc) * 100 / (Left_Adc + Right_Adc); */
```
原文评价：“经实验，**两电感靠中性**（车身靠赛道中心行驶）**差一点，不容易出赛道**；**三电感靠中性好一点，但容易出赛道**。”

**“差比和加权”与“差比和差”（卓晴体系，被广泛引用）**

```text
# 公式 C1：差比和加权（原始版本）
err = [ A·(L − R) + B·(LM − RM) ] / [ LIMIT + A·(L + R) + B·(LM + RM) ]

# 公式 C4：差比和差加权（最终式，把分母上的 (LM+RM) 换成 (LM−RM)）
err = [ A·(L − R) + B·(LM − RM) ] / [ LIMIT + C·(LM − RM) ]
```
- 电感排布：由左到右为 **L、LM、M、RM、R**（外侧两个水平电感 + 内侧两个八字/斜电感 + 中间一个）。
- **另一被广泛引用的写法**：`err = [A*(L-R) + B*(LM-RM)] / [A*(L+R) + C*|LM-RM|]`（分母含 `A·(L+R)` 且取绝对值）。
  ⚠️ 该写法与卓晴原文 C4 在分母上**不完全一致**（原文分母为 `LIMIT + C·(LM−RM)`，不含 `A·(L+R)`、未取绝对值）；**两种写法都出现在来源中，引用时建议注明版本**。
- **动机（卓晴原文）**：“当小车入弯时，电感差比和加权算法中位于分子上加权的 (LM+RM) 增大，导致 err 产生减小趋势，该减小趋势导致电感差比和对弯道的敏感度下降。而当把分母上的 (LM+RM) 改为 (LM−RM) 后，小车入弯时 err 的分母增大趋势显然下降，err 产生的减小趋势更弱。”并指出差比和加权在弯道内环会出现**负斜率/边缘上卷（斜率翻转）**，失去负反馈；差比和差不会。
- **调参建议（原文）**：先调出一套稳定的差比和加权参数，再改写成差比和差；改后“由于分子的下降，小车在弯道出现小幅度过调。此时**轻微降低比例系数（约为差比和加权算法的 0.7 到 0.8 倍）**即可完全拟合”。
- **代价**：差比和加权无比例系数时输出 <1；差比和差会突破 1，**赛道适应性下降，每次换赛道都要重新调参**。

```c
/* 6 电感（2 横 + 2 斜 + 2 竖）的分模式开方比值法 + 低端模糊 Kp
   CSDN《浅谈自己的电磁智能车调试之路（1）电磁》 */
float Get_Error(unsigned char MODE)
{
    float error;
    switch (MODE) {
    case 1:  /* 斜放一对电感 */
        error = (sqrt(adc[3]) - sqrt(adc[0])) / (adc[0] + adc[3]); break;
    case 2:  /* 水平横放一对电感 */
        error = (sqrt(adc[2]) - sqrt(adc[1])) / (adc[2] + adc[1]); break;
    case 3:  /* 两对电感组合 */
        error = ((sqrt(adc[3]) + sqrt(adc[2]) - sqrt(adc[1]) - sqrt(adc[0]))
                 / (adc[0] + adc[3] + adc[2] + adc[1])); break;
    case 4:  /* 竖放一对电感 */
        error = (sqrt(adc[5]) - sqrt(adc[4])) / (adc[5] + adc[4]); break;
    }
    return error;
}
/* if (xie_error > 1.2 && (chuizhi_error >= 0.23 || chuizhi_error <= -0.23))
       Steer_pid.Kp = xie_error * lstcs1 + lstcs2;       // 右转
   else if (xie_error < -1.2 && (chuizhi_error >= 0.26 || chuizhi_error <= -0.26))
       Steer_pid.Kp = -xie_error * lstcs01 + lstcs02;    // 左转
   else Steer_pid.Kp = 50;                               // 直道 */
```
该来源的布局经验结论：“**直线寻迹时横电感起最主要的作用、弯道寻迹时斜电感起主要作用、十字判断时竖电感最容易识别**”；调运放时“靠外的 2 个电感数值小一些，而靠内的 2 个电感数值调大一些，这个车可能会好调一点”。

### 9.3 两种方法的区别与适用场合

| 维度 | 加权求和法 Σ(ADCᵢ·wᵢ)/Σ(ADCᵢ) | 左右差比和 (L−R)/(L+R) 及变体 |
|---|---|---|
| 最少路数 | **≥3**（2 路时退化为符号版） | **2** 路即可 |
| 输出物理含义 | **位置**（重心/质心索引），可映射为 mm/cm | **无量纲比例**（位置偏差的相对量） |
| 对绝对幅值/电源漂移 | 需先归一化，否则随总场强漂移 | 分子分母约掉公共放大倍数，**天然免疫**（但不消除左右不对称） |
| 对不同间距的适应 | **天然适应**（权重直接代表实际横向坐标） | 隐含假设左右对称等距 |
| 对单路故障 | 敏感：一路饱和/断路会把重心整体拉偏 | 较鲁棒：单路失效会饱和到 ±100，但方向仍正确 |
| 输出范围 | 取决于权重定义（例：0~99，减 50 得 ±49；8 路为 0~7，减 3.5 得 ±3.5） | 不乘 100 为 (−1,+1)；乘 100 为 ±100 |
| 典型场合 | 多电感阵列（5~8 路）、需要输出“横向位置”给状态机/环岛处理 | 2~3 路入门；三/五电感融合；作为 P 项前的“平滑误差” |
| 来源评价 | “更稳的做法是加权中线……不是只用左右两路，而是把一排电感的数据都融合进来，抗单点干扰能力更强” | “差比和比较简单，用的人多”；“差比和不能解决不对称” |

**实务建议（综合来源）**：先做**每通道归一化（0~100）**，再做**差比和**得到“横向相对偏差”；若电感数 ≥3 可用**加权求和**得到“绝对横向位置”用于元素判据。两者不互斥，是纵向/横向两级处理。李敏书对三电感的结论：“**在三电感的循线策略中，将中间电感加入差比和的分母中去，可以有效地优化行驶路径**”（直道中间电感接近最大 → 偏差变小 → 减少抖动；入弯时中间电感迅速减小 → 偏差呈二次关系 → 更好切内道）。

### 9.4 归一化后偏差的典型数值范围

| 量 | 典型范围 | 依据 |
|---|---|---|
| 单通道归一化电感值 | **0 ~ 100**（整数）；或 **0 ~ 1**（浮点） | 电子发烧友归一化两篇写“限制在 0~100”；新余学院技术报告写“转化为 0~1 之间的数值” |
| 单通道归一化（带 +1 偏置写法） | **1 ~ 100** | `(uint16)(99*(...)/(...) + 1)` |
| 原 ADC（STC32G 逐飞库 12 位） | **0 ~ 4095** | 19 届开源代码 `ADC_12BIT` |
| **差比和（不乘 100）** | **(−1, +1)**；卓晴原文：“未加比例系数时，差比和算法会将输出误差限制在 1 以内” | 卓晴 |
| **差比和 ×100** | **−100 ~ +100** | 实战代码 |
| 差比和差 | **会突破 ±1**（乃至更大），需靠方向环比例系数 P 压回来 | 卓晴原文 |
| 加权求和（3 电感，权重 1/50/99） | Sensor **0 ~ 99**，Bias **−50 ~ +49** | 华为云转载代码 |
| 加权中线（8 路） | **0 ~ 7**（传感器索引），中心 3.5，error 约 **−3.5 ~ +3.5** | CSDN 全指南代码 |
| 元素判据阈值示例 | 中间电感 `>0.8` 判直道、`<0.4` 判弯道；环岛饱和阈值 `>0.85`、缺口阈值 `<0.25` | CSDN 全指南/避坑指南 |
| 死区 | `if(abs(error) < 20) error_angle = 0;` | 《智能车—电磁循迹》实战代码 |

> ⚠️ **未确认**：任务中提到的“**±50**”作为归一化后差比和的典型范围，**未在任何来源中出现为该量的常规范围**。±50 出现在**加权求和（权重 1/50/99，Bias = Sensor − 50）**的语境里。若你把差比和乘 50 而不是 100，则 ±50 成立，但那属于**自定标度**，不是来源中的通用惯例。
> ✅ **结论**：**“归一化到 ±100”是来源可确证的惯例**（差比和 ×100）。

### 9.5 是否需要用 atan / 查表做非线性修正

> ⚠️ **重要修正**：**本次检索未在任何可信的智能车电磁循迹来源中见到用 `atan`/`atan2` 做偏差非线性修正**。`atan` 只出现在**疑似 AI 生成、且数据自相矛盾的文档**中。**工程文档中不宜声称“电磁组常用 atan 修正”**。

**为什么需要修正（有明确学术来源）**

张晓峰等《电磁智能车循迹算法》（《计算机系统应用》2014, 23(12): 187-190）摘要原文：

> “提出了一种智能车竞赛电磁组的循迹算法——**比值法**，针对现有的常用循迹算法**差值法和归一化法**中存在**计算所得偏差值和偏离距离的函数存在极值点**并且**在两极值点外距离与偏差值的关系变为负相关**的问题，采用**对电动势分别开根号作差再比上它们的和值**的方法**去除极值点**，使得**距离与电势差在整个取值范围内都呈正相关**，去除了错误判断。通过实验验证该方法比常用方法有更好的效果。”

即：不修正时会出现在大偏移区“偏差反而变小/变号”的**错误判断**（与李敏书“车头偏转越多偏差反而越小”同源）。

**已确证的修正做法（按可信度排序）**

1. **开方比值法（学术论文，最正规）**：`error = (√E₁ − √E₂)/(E₁ + E₂)` 及多路组合形式（见 §9.2 `Get_Error`）。作用：**去除极值点**，使距离与偏差在全范围正相关。
2. **差值开平方再作差**：“在传统的差比和上进行一定的改进，**差值部分为先将两个电磁数据开平方再做差**，和还是两个电感数据之和，这样效果会有一定优化”（第十六届技术报告，新余学院开放艺术队 5.3.2）。
3. **差比和差加权（卓晴 C4）**：分母 (LM+RM) → (LM−RM)，**消除弯道内环“边缘上卷”（斜率翻转）**，使输出误差曲线维持单调、**更贴合一次曲线**。
4. **分段拟合 + 动态加权（论文级，四横两斜）**：按最大电动势所在线圈**分段**——E₂/E₃ 最大（d ∈ [−10,10] cm）用 4 个水平线圈做**三次多项式拟合**；E₁ 或 E₄ 最大时用**离线拟合线性函数**；同时用两侧内八字斜电感做差值法解算 d₂；最终 `d = α·d₁ + (1−α)·d₂`，α 由 (E₂+E₃) 与阈值 **T = 160** 决定。效果：“采用加权算法求得的直角位置偏差**连续性增强**，有效地减弱了由直道进入直角弯的突变性”。
5. **“圆形变换”提高小误差灵敏度**：“采用‘圆形变换’，**提高误差小的时候的灵敏度**，同时对方向控制量的变化量进行限幅（或者与历史值加权平均一下），防止剧烈抖动”。⚠️ 具体公式 **❌ 未确认**。
6. **查表 / 分段线性**：已确证存在离线拟合线性函数、三次多项式、分段 Kp 查表。⚠️ 把“电感值 → 横向偏移”标定曲线做成 Flash 查表（每 mm 一点）的做法只在**疑似 AI 生成、数据自相矛盾**的文档中出现（10 mH 配 62 nF 却称“谐振频率约 202 kHz”），**不建议采信**。

**综合建议**：非线性修正走 **开方比值法 / 差比和差加权 / 分段拟合+分段 Kp** 这三条可核实路线，**不要用 atan**。

### 9.6 来源

1. 卓晴《智能车电感差比和差加权算法研究》（公式 C1~C4、数学建模、调参 0.7~0.8 倍）— <https://zhuoqing.blog.csdn.net/article/details/108993827>
2. 同文 EET-China 版（电感排布 L,LM,M,RM,R；1/3000 偏置算法论述）— <https://www.eet-china.com/mp/a29377.html>
3. 张晓峰等《电磁智能车循迹算法》，《计算机系统应用》2014, 23(12): 187-190（差值法/归一化法存在极值点、比值法去极值点）— <https://www.c-s-a.org.cn/csa/article/abstract/20141233>
4. 《调参不再玄学：手把手教你优化智能车（电磁组）舵机 PD 控制参数》（差比和 +1 防除零与线性度代价、融合方案对比）— <https://blog.csdn.net/weixin_42520025/article/details/159911170>
5. 《四轮电磁——电磁循迹位置式 PID》（采集+滤波+归一化+差比和流程；内外圈电感信任度系数 OUT_rate/IN_rate/OTHER_rate）— <https://blog.csdn.net/m0_65046930/article/details/127749705>
6. 《[智能车问题] 电磁平衡组方向控制》（“圆形变换”提高小误差灵敏度、变化量限幅/历史加权、差比和归一化）— <https://blog.csdn.net/jklongint/article/details/51415383>
7. 《智能车电磁感应自动循迹具体的过程及代码实现》**[AI生成，仅作线索]** — <https://wenku.csdn.net/answer/1x6o2qp7vp>

## 10. 舵机控制

### 10.1 PD 还是 PID？为什么通常不用积分项

**有来源支撑的部分（最关键的一条）**：山东大学（威海）ACE 队在第十五届智能汽车竞赛 AI 电磁组的技术报告中明确写道：

> “PID 参数整定我们采用的是第二种方法……我们尝试了 **PID、分段 PD、模糊 PD、二次 PD、单 PD**，发现**单 PD 就可以达到设计要求**。因此我们使用**单 PD** 方案。”
> 且“方向控制方案采用 AI 融合 PID 进行控制……智能车在直道、弯道、十字元素处使用**不同的 PID 参数**以及不同的横竖斜电感的权值。”

→ 即：**实际队伍是“试过带 I 的完整 PID，最后落地成 PD”**。这是“电磁组常用 PD”最硬的旁证。

**“不加积分项”还有厂商工程师手册的直接表述（✅ 已核实）**。龙邱科技（智能车器件/方案厂商）第 21 届技术手册原文：

> “我们在转向控制中，通常使用**位置式 PID**，而**位置式 PID，一般用不到积分项，所以我们只需要调节 KP 和 KD 即可**。”

**“没有 I”的真实代价也被写进了技术报告（✅ 已核实，第十八届哈工大紫丁香三队）**：

> “由于小车寻赛道本质上是一个随动系统，**因为没有积分项，造成在进入直道时转向不够准确**，跑直道时虽然能跟踪黑线，但是**转向调整往往超调**，导致车身在直道上左右震荡。”

★ **但该队的解法不是加 I，而是动态改变 PD 参数**：
- 低速（2 m/s 以下）：大偏差时 Kp 直接置 100%，小偏差时 Kp 减半；
- 高速（2.5 m/s 以上）：Kd 随速度增大而增大；
- 原文结论：“经过反复调试 PD 参数，我们发现**只调整 PD 参数很难使车在跑 S 弯和长直道时都选择最佳路径**……这就要求系统能够智能地识别出当前赛道是哪种类型”，最终按赛道类型/中心位置**动态改变 PD 参数**。

**其他机制性理由（来源未直接论述，标注为推论）**：
1. **积分会在“丢线”时饱和**：电磁组丢线/大偏差工况频繁，一旦丢线误差长期不为零，积分项持续累积到饱和，重新捕捉赛道时产生大幅回摆（windup）。
   > ⚠️ **未确认**：中文公开资料中**没有**检索到对“电磁组舵机方向环积分饱和”的专门论述。本条属工程推理。
2. 电磁组的“固定偏航”主要来自**机械中位偏差与电感左右不对称**，正确做法是**机械调中 + 重标定**（见 §8.1 第 5 条），而不是用积分去补。
3. **赛道曲率前馈替代积分**：公开资料里的主流替代手段是“前馈 / 查表 / 动态 Kp”。哈工大报告的舵机打角经验公式为 `PWMDTY_PRE = n·L + D·λ`，原文：“二次项的系数越大**贴黑线就越严格**，一次项系数越大**前瞻性就越高**。但是一次项中的人计算的结果极其不精确，所以如果这一项占的比例太大会导致 PWMDTY_PRE 数值的严重抖动”。
4. **查表法（曲率→打角映射）** 是另一条成熟替代路径：让车固定打角推着过弯，记录 6 组以上 (deltax, 打角) 数据，用 MATLAB `polyfit` 拟合成二次/三次曲线查表。
   > ⚠️ 该做法来自**光电组**调参记录，电磁组是否照搬**未确认**。

> ✅ **结论：方向环用【位置式 PD】是电磁组的主流做法——有厂商手册明文“一般用不到积分项”、有队伍实测“试过 PID 最后用单 PD”、也有队伍实测“无 I 会超调但选择动态 PD 而非加 I”。积分项通常不加。**
> ⚠️ 同时要诚实告诉你：**“无 I 会导致入直道不准/超调”是被写入技术报告的真实缺陷**，如果你的车出现这个现象，正确对策是**动态 Kp/Kd（按赛道类型或曲率）**，而不是先加 I。

### 10.2 离散 PD 的实现形式与真实工程参数

**形式（位置式 PD，主流；舵机环）**：

```c
/* 龙邱 21 届手册给出的精确写法（对误差差分，不是对测量差分） */
Output = sptr->KP * sptr->iError
       + sptr->KD * (sptr->iError - sptr->LastError);
```

```c
/* 电磁四轮参考工程同写法 */
My_Direction.Direct_Parameter = My_Direction.KP * poserror
                              + My_Direction.KD * (poserror - My_Direction.PrevError);
My_Direction.PrevError = poserror;
price_PWM = (int)(My_Direction.Direct_Parameter);
```

> **关于“增量式 PD 用在舵机”**：公开工程中**存在但属少数**，且被实践者反馈“**增量式的改变不如位置式来得更快，转弯更加及时，遂将增量式改成了位置式**”（三轮电磁第一手调试记录）。**结论：舵机环用位置式 PD。**

**真实工程代码中的参数（含量纲说明，这非常重要）**：

```c
/* 来源：华为云社区转载《电磁巡线归一化算法（二）》——3 电感加权求和 */
Sensor_Left   = analogRead(1);
Sensor_Middle = analogRead(2);
Sensor_Right  = analogRead(3);
if (Sensor_Left + Sensor_Middle + Sensor_Right > 25)
{
    sum    = Sensor_Left * 1 + Sensor_Middle * 50 + Sensor_Right * 99;
    Sensor = sum / (Sensor_Left + Sensor_Middle + Sensor_Right);   /* 0 ~ 99 */
}
Bias  = Sensor - 50;                              /* ★ 偏差量程 ±50 */
Angle = Bias * 0.65 + (Bias - Last_Bias) * 0.1;   /* ★ Kp=0.65, Kd=0.1 */
Last_Bias = Bias;
/* 原文注释给出的非线性等价写法：
   Angle = abs(Bias)*Bias * 0.02 + Bias * 0.074 + (Bias - Last_Bias) * 1; */
```

```c
/* 来源：CSDN《浅谈自己的电磁智能车调试之路（1）电磁》——开方比值法 */
/* error 量程约 (−1, +1)（未乘 100） */
case 1: error = (sqrt(adc[3]) - sqrt(adc[0])) / (adc[0] + adc[3]); break;  /* 斜电感 */
...
else Steer_pid.Kp = 50;      /* ★ error∈(−1,1) 时 Kp = 50 */
/* 低端模糊：转弯时按偏差线性加大 Kp */
if (xie_error > 1.2 && (chuizhi_error >= 0.23 || chuizhi_error <= -0.23))
    Steer_pid.Kp = xie_error * lstcs1 + lstcs2;      /* 右转 */
else if (xie_error < -1.2 && (chuizhi_error >= 0.26 || chuizhi_error <= -0.26))
    Steer_pid.Kp = -xie_error * lstcs01 + lstcs02;   /* 左转 */
else Steer_pid.Kp = 50;                              /* 直道 */
```

**★ 把所有抓到的真实工程参数放在一起（务必看清各自偏差量纲）**：

| 工程 | 偏差定义 | Kp | Kd | 输出限幅/单位 | 备注 |
|---|---|---|---|---|---|
| **龙邱科技 21 届技术手册**（厂商工程师方案，✅ 最接近“可直接用的起点”） | 赛道偏差 `eleValue` | **0.65** | **1.88** | **转向环输出限幅 ±100** | 手册注释原文“此处参数仅供**数量级参考**” |
| 3 电感加权和代码（华为云转载） | `Bias` ∈ ±50 | 0.65 | 0.1 | 舵机打角增量 | 与上表不同量纲，勿混 |
| 开方比值法代码 | error ∈ (−1,+1) | 50 | — | 舵机 Kp | 分段 Kp |
| **21 届 STC32G144K246 开源工程** | 舵机误差 = `MiddleLine − Det_True`（**图像/中线像素域**） | `kp_base=120` + 模糊表 130~215（÷5）→ 有效 ≈146~263 | `kd_base=0` + 模糊表 0（备选表 20~50，÷5） | 舵机占空比偏移，限幅 **+650 / −550** | 舵机 **4 ms** 周期；另有陀螺仪项 |
| 三轮电磁学习工程 | 左右电感差值 | 0.7 | 0.5 | 电机目标转速差 | 纯 PD，Ki=0 |
| 18 届负压电磁 HIT | 图像中线偏差（像素） | 动态 Kp（低速分段/高速随速） | 高速时随速度增大 | 舵机 PWM 占空比 | **原文未给具体数字** |
| 19 届电磁开源（三轮/四轮） | `AD_Bias`（电感归一化 0~100 后三电感差比和） | `BIAS_KP`（**源码未给数值**） | `BIAS_KD`（**源码未给数值**） | 电机目标速度差 | 直道判据 `|bias| ≤ 5` |

**★ 换算到“偏差归一化到 ±100”的口径（便于起步）**：

| 来源 | 原始偏差量程 | 原始 Kp | 等效 Kp（±100 口径） | 原始 Kd | 等效 Kd |
|---|---|---|---|---|---|
| 龙邱手册 | 与输出同量纲、输出限幅 ±100 | 0.65 | **0.65（本身就是 ±100 口径）** | 1.88 | **1.88** |
| 3 电感加权和代码 | ±50 | 0.65 | ≈ 0.33 | 0.1 | ≈ 0.05 |
| 开方比值法代码 | ±1 | 50 | ≈ 0.5 | — | — |

→ ✅ **在“偏差归一化到 ±100、输出也是 ±100 量纲”的口径下，最可信的起点是【Kp ≈ 0.65、Kd ≈ 1.88】（龙邱 21 届手册）。**
> ⚠️ **必须强调三点**：
> 1. **不存在“通用典型值”**。本次核查**没有**找到任何一份资料明确写“偏差归一化到 ±100 时，Kp 典型为 X、Kd 典型为 Y”。多数工程的 Kp 是“基础值 + 模糊表/分数”的混合结构（如 `120 + 130~215/5`），**无法折算成单一数字**。
> 2. 不同工程的数字**带不同偏差量纲**（像素域 / 0~100 差比和域 / 舵机误差域），**不可直接互抄**。
> 3. Kp/Kd 与**控制周期、舵机速度、车速、归一化口径**强耦合，**必须实车标定**。

**Kd 项为什么要限幅 / 滤波（有来源）**：
- 高速下“**当 KD 不断增加，小车应对快速弯道的能力越来越强，但是随着 KD 增大，电感采集时引入的噪声也被放大，增加了小车在小偏差情况下的不稳定性**”；
- 有专门文章给出**可操作的微分冲击抑制方案**：
  - **D 项输出限幅（核心手段）**：`raw_D = Kd*(error - prev_error)/dt`，对 `raw_D` 做 `±D_MAX` 限幅，再对总输出做物理限幅（示例 `saturate(P+D, -45, 45)` 单位度）；
  - **`D_MAX` 的确定方法（原文）**：“从舵机最大转向速度反推。例：舵机 0°→45° 需时 0.2 秒 → 最大角速度 ≈ 225°/s → **`D_MAX = 225 * Kd`**”；
  - **动态限幅**：随车速提高而**减小** `D_MAX`；
  - **设定值平滑 / 斜率限制**：`filtered_target = α*new_target + (1-α)*prev_target`，**α 取 0.3~0.7**；或斜率限制（例“10ms 内最大角度变化 ≤5°”）；
  - **调参顺序建议**：先固定 Kp 调滤波参数，再调 D 项限幅，最后微调 Kd；
  - 该文明确指出“**电感车通常需要更强的低通滤波**（相比摄像头车）”。
- 该工程还有一处**可选**做法：20 届负压电磁队用“**动态改变 PD 参数**”而非加 I。

**误差死区（⚠️ 存疑，不要当惯例）**：有博客给出 `if(abs(error) < 20) error_angle = 0;`（±100 口径下死区约 ±20）。但在**逐字读取的多份电磁组开源源码/技术报告中（STC32G144K246 工程、19 届开源、18 届 HIT 报告、龙邱 21 届手册）均未出现“死区”实现**，公开代码里更常见的是**斜率限制**（十字处 ±30 平滑）与**输出限幅**。
→ **结论：死区是可选技巧，不是电磁组惯例；优先用斜率限制/输出限幅。**

### 10.3 舵机 PWM 参数（✅ 来自逐飞官方 STC32G144K246 舵机例程，权威）

逐飞针对本芯片的官方舵机例程原文：

```c
#define CHANNEL_NUMBER          (4)
#define SERVO_PWM1              (PWME_CH1P_PA0)     /* 主板上舵机对应引脚 */
#define SERVO_FREQ              (50 )               /* 舵机频率，务必注意范围 50-300 */
#define SERVO_L_MAX             (80 )               /* 舵机活动范围，角度 */
#define SERVO_R_MAX             (100)
#if ((SERVO_FREQ < 50) || (SERVO_FREQ > 300))
    #error "SERVO_MOTOR_FREQ ERROE!"
#endif

/* ------------------ 舵机占空比计算方式 ------------------
 * 舵机对应的 0-180 活动角度对应 控制脉冲的 0.5ms-2.5ms 高电平
 * 不同频率下占空比 = PWM_DUTY_MAX/(1000/freq)*(1+Angle/180)
 * 50Hz 时即 PWM_DUTY_MAX/(1000/50)*(1+Angle/180)
 * 100Hz 下 90 度打角（高电平 1.5ms）：
 *   PWM_DUTY_MAX/(1000/100)*(1+90/180) = PWM_DUTY_MAX/10*1.5
 * ------------------------------------------------------ */
#define SERVO_DUTY(x)  ((float)PWM_DUTY_MAX / (1000.0/(float)SERVO_FREQ) * (0.5 + (float)(x)/90.0))

pwm_init(channel_list[channel_index], SERVO_FREQ, 0);
pwm_set_duty(channel_list[channel_index], SERVO_DUTY(servo_motor_duty));
```

要点：
- **周期 50 Hz（20 ms）**；库允许 **50~300 Hz**。
- **0~180° ↔ 0.5 ms ~ 2.5 ms 高电平**；**中值 90° ↔ 1.5 ms**。验算：`SERVO_DUTY(90) = MAX/20 × 1.5 = 0.075·MAX`（1.5ms/20ms ✓）；`SERVO_DUTY(0)=MAX/20×0.5=0.025·MAX`（0.5ms ✓）；`SERVO_DUTY(180)=MAX/20×2.5=0.125·MAX`（2.5ms ✓）。
- 例程用 80~100° 做来回扫描，是**测试用的小活动范围**以保护机械；实际转向的**打角限幅**请按你的机械连杆行程设定（不要把 0.5~2.5 ms 打满）。
- 官方例程的安全提示（原文）：“**最好在舵机没有装在车上固定连接转向连杆时测试，防止安装位置不对造成堵转烧舵机**”；“检查舵机供电是否正常 **至少 5V 供电，不可以用杜邦线供电**”。

### 10.4 舵机机械安装对控制的影响（有来源）

- **转向是系统中延迟最大的环节**：“舵机转向是整个控制系统中延迟较大的一个环节，为了减小此时间常数，通过改变舵机的安装位置可以提高舵机的响应速度。”
- **力臂效应**：“在相同的舵机转向条件下，转向连杆在舵机一端的连接点**离舵机轴心距离越远**，转向轮的转向变化越快。这相当于增大力臂长度，提高线速度。”
- **代价（重要）**：“功率等于速度与扭矩的乘积，加大转向速度必然减少输出扭矩，扭矩过小会造成迟钝，所以安装时必须考虑到**转向机构的响应速度与舵机扭矩之间的关系**，获得最佳转向效果。”
- **安装方式**：“舵机安装方式有立式和卧式两种，比较两种方式发现，**立式安装效果更好**。舵机安装时要保证**左右对称**，这样可以保证舵机左右转向时力臂相等且处于最大范围。”

### 10.5 陀螺仪辅助转向 —— ✅ **这是成熟做法，且有多种公开实现与确切参数**

**这不是“未确认”，而是已被多份来源证实的主流技巧。** 陀螺仪（偏航角速度 `Gyro_Z`）用于“**减轻超调震荡、抑制甩尾**”，有三种落地形式：

**(1) 并级（并联加阻尼）** —— 龙邱 21 届手册原文与代码：

```c
/* 偏航角速度比例系数根据实际情况微调 */
eleOut = PlacePID_Control(&Turn_PID_msg, eleValue, elemid, Turn_pid) - Gyro_Z * 0.005;
```
> 手册原理说明：“小车在运动过程中的偏航角速度是不可控的……我们要抑制超调所加的 KD 值往往是偏大一点了，导致误差每变一点，PID 控制器的输出量就剧烈波动，从而产生剧烈的高频震荡。我们可以通过**控制小车的偏航角速度，来抑制超调，做到及时收敛，从而减轻 KD 的超调**。”并说明陀螺仪用途之一是“**与转向环并级或串级使用，以减轻超调震荡现象，抑制甩尾**”。

**(2) 串级（偏航角速度作为内环）** —— 外环是赛道偏差环（PD，目标偏差 0），外环输出**变成目标偏航角速度**；内环是偏航角速度环（PD），内环输出才作用于舵机：

```c
double Turn_pid[2] = {0.65, 1.88};   /* 赛道偏差环 PD —— 手册注明“仅供数量级参考” */
double Gyro_pid[2] = {0.007, 1.4};   /* 偏航角速度环 PD */
...
if(++t >= 2)                          /* 外环分频：2 个中断周期跑一次 */
{
    eleOut_0 = PlacePID_Control(&Turn_PID_msg, eleValue, elemid, Turn_pid);
    eleOut_0 = range_protect(eleOut_0, -100.0, 100.0);
}
eleOut_1 = PlacePID_Control(&Gyro_PID_msg, (double)Gyro_Z, eleOut_0, Gyro_pid);
eleOut_1 = range_protect(eleOut_1, -100.0, 100.0);
```

**(3) 陀螺仪前馈（独立加项，并按元素切换系数）** —— 21 届 STC32G144K246 开源工程：

```c
int16 gyro_num = 14;  int16 gyro_den = 10;               /* 普通弯道 → 系数 1.4 */
int16 ring_gyro_num = 13; int16 ring_gyro_den = 10;      /* 环岛内 (状态6~7) → 1.3 */
int16 cross_ring_gyro_num = 14; int16 cross_ring_gyro_den = 10;  /* 十字+环岛 */
...
term_gyro = mul_div_i16((int16)imu660rb_gyro_transition(imu660rb_gyro_z),
                        current_gyro_num, current_gyro_den);
out_temp = term_p + term_d + term_gyro;
```
即陀螺仪项系数 ≈ **1.3 ~ 1.4 倍**原角速度值，**按赛道元素分档切换**。同一工程差速侧也预留了陀螺仪前馈（当前被注释）：

```c
gyro_feedforward = - mul_div_i16(gyro_z, 10, 256);           /* ≈ 0.039 * gyro_z */
gyro_feedforward = (int16)func_limit_ab(gyro_feedforward, -20, 20);
diff_cmd += gyro_feedforward;
```

**转向延迟的机械层面补偿（有来源，第十八届哈工大报告）**：
- “**舵机转向是整个控制系统中延迟较大的一个环节**，为了减小此时间常数，通过改变舵机的**安装位置**可以提高舵机的响应速度。”
- “转向连杆在舵机一端的连接点**离舵机轴心距离越远**，转向轮的转向变化越快”（增大力臂、提高线速度）。
- 权衡：“**功率等于速度与扭矩的乘积，加大转向速度必然减少输出扭矩**，扭矩过小会造成迟钝”，必须折中。
- “**立式安装效果更好**”“**舵机安装时要保证左右对称**，这样可以保证舵机左右转向时力臂相等且处于最大范围。”

**转向延迟的软件层面补偿**：D 项本身就是对相位滞后的提前量；调参者提醒“**kd 值也应该随速度有差别**”。

> ⚠️ **未确认**：**不完全微分**（微分项一阶惯性滤波）在电磁组舵机环中的使用——已抓取的 5 份电磁组源码/报告中**均未发现**。
> ⚠️ **未确认**：**Kd 项独立低通滤波**作为电磁组常规做法——只有原则性建议“**电感车通常需要更强的低通滤波**”。

### 10.6 来源

1. **逐飞官方 STC32G144K246 舵机例程（50Hz、0.5–2.5ms、占空比公式、50–300Hz、安全提示）**：
   <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Example/Motherboard_Demo/E02_03_servo_control_demo/user/main.c>
2. **第十八届智能汽车竞赛技术报告·负压电磁·哈尔滨工业大学（紫丁香三队）**（“转向舵机采用 PD 控制；驱动电机采用 PI 控制”；无 I 导致入直道不准/超调但用动态 PD 解决；舵机安装与延迟、立式与对称；`PWMDTY_PRE = n·L + D·λ`）：
   <https://blog.csdn.net/zhuoqingjoking97298/article/details/132534760?spm=1001.2014.3001.5501>
3. **龙邱科技·第二十一届全国大学生智能汽车竞赛·疯狂电路组技术手册**（“位置式 PID 一般用不到积分项，只需调 KP 和 KD”；转向环 PD `{0.65, 1.88}`、输出限幅 ±100；陀螺仪并级 `-Gyro_Z*0.005`；陀螺仪串级外环 `{0.65,1.88}` / 内环 `{0.007,1.4}`、外环 2 分频；增量式速度环 `{9, 3.5, 0}`、“KP 可加到 KI 的 2~3 倍”；舵机 50–333Hz、0.5–2.5ms）：
   <https://blog.csdn.net/longqiu_LQ/article/details/160896921?spm=1001.2014.3001.5501>
4. **21st-smart-car-yan-guo-liu-hen-8of10-2.6mps（GitHub 开源，主控即 STC32G144K246，用逐飞库）** —— 舵机 4ms 周期 / 300Hz / 占空比 4320-4870-5520 / PID 限幅 +650−550 / `kp_base=120` + 模糊表 130~215(÷5) / `kd_base=0` / 陀螺仪系数 14/10 与 13/10 / 十字处输出斜率限制 ±30：
   <https://github.com/ZORE-dlfd/21st-smart-car-yan-guo-liu-hen-8of10-2.6mps>
5. 山东大学（威海）ACE 队，第十五届 AI 电磁组技术报告（“尝试了 PID、分段 PD、模糊 PD、二次 PD、单 PD，发现单 PD 就可以”；方向环/速度环各放一个 5ms 定时器；KP/KD 高速敏感性与噪声放大；3 m/s 下 PD 极敏感）：
   <https://www.eet-china.com/mp/a27552.html>
6. 华为云社区转载《电磁巡线归一化算法（二）》（3 电感加权和 + `Angle = Bias*0.65 + (Bias-Last_Bias)*0.1`）：
   <https://bbs.huaweicloud.com/blogs/333658>
7. CSDN《浅谈自己的电磁智能车调试之路（1）电磁》（开方比值法 + 分段 Kp；横/斜/竖电感分工）：
   <https://blog.csdn.net/abc565846881/article/details/125650728>
8. CSDN《智能车竞赛舵机抗饱和控制策略》（微分冲击、`D_MAX = 225*Kd` 反推法、目标值一阶低通 α=0.3~0.7、斜率限制 10ms≤5°、调参顺序、电感车需更强低通）：
   <https://blog.csdn.net/woshihonghonga/article/details/149459637>
9. 博客园《“0.5ms–2.5ms”与代码中“1000–2000”的矛盾》（1500µs 恒为机械中点；500–2500µs 是硬件极限、1000–2000µs 是库保守值；建议 `constrain(700, 2300)` 留余量）：
   <https://www.cnblogs.com/54programer/p/19611384>
10. CSDN《智能车—电磁循迹》（误差死区 `if(abs(error)<20) error_angle=0;`，**本次仅此一处出现，已标注存疑**）：
    <https://blog.csdn.net/weixin_52441317/article/details/131872477>
11. 电磁三轮 PID 第一手调试记录（“增量式的改变不如位置式来得更快，转弯更加及时，遂将增量式改成了位置式”）：
    <https://blog.csdn.net/xcy88888888/article/details/130792635?spm=1001.2014.3001.5501>

---

## 11. 速度控制

### 11.1 差速方案

**有来源的方案（四轮电磁，山东大学威海 ACE 队）**：

> “在速度环控制上，我们主要运用了**差速算法**和**赛道记忆算法**。”
> “假设小车在过弯的过程中没有垂直于车身运行方向上的相对移动，由于后轮的角速度相同，内侧后轮和外侧后轮的转弯半径不同，所以它们的线速度不同。如果赋予它们相同的占空比，必然会降低弯道上的转向性能。**通过前轮转角，我们可以通过公式推算出后侧两轮的差速，得到两轮的目标速度，再经过 PI 运算，使内外两轮达到该转弯半径下对应的线速度。**”
> “在测量多组前轮转角和转弯半径的数据后，使用 MATLAB 拟合曲线，发现**前轮转角和差速近似为线性关系**，因此使用**线性关系来代替上述差速公式**。”

即：**四轮差速 = 由前轮转角（舵机打角）推得的左右轮目标速度差，左右轮各一个 PI 速度环**。

**三轮车（第十九届起的主流形态之一，✅ 已核实）**：开源作者原文——

> “在 19 届以前我们电磁车模使用的是**四轮车模**，只需要控制舵机的角度来控制转向就可以了，**但是 19 届使用的是三轮车模，前面一个万向轮，后面两个电机驱动轮，我们需要进行差速转向**，即左转时，左边电机转得慢或者反转，右边电机转得快。”

- 代码形式：`motor_L.set = GO_AHEAD_SPEED - bias_speed;  motor_R.set = GO_AHEAD_SPEED + bias_speed;`（把带符号的偏差量算成差速量，加减到左右电机目标速度上）。
- **三轮差速的机械局限（第一手）**：“考虑到**差速转弯最小半径问题**，即这种两路 PWM 控制的一路电机**其中一路 PWM 最小值只能小到 0，不能到负**，导致此方法的局限性”——该队后来“加入了 `operate_motor()` 使用**正反转转弯**”，效果仍有限。

**直立车（平衡组）结构与竞速组完全不同（✅ 有来源，但为方案性描述）**：
- 直立车是**多环 PID：直立环 + 速度环 + 方向环**；
- 采样率差异明确：**直立环 500 Hz ~ 1 kHz，速度环 50 Hz ~ 100 Hz**；“方向环利用电磁传感器的差分信号，计算车身偏离量，并通过 **PD 控制器控制舵机角度**”；
- 调试顺序要求：**必须先整定直立环 PD 保证静止平衡，再叠加速度环**。
> ⚠️ 该来源为**商业方案页**，具体环结构与参数需以自己车模实测为准。
> ⚠️ **❌ 未确认**：直立车的**具体速度环 PI 参数与直立环 PD 参数**（该来源只给环结构与采样率）。

**“前轮转角 → 差速”的具体拟合系数**：⚠️ **❌ 未确认**（原文只说“近似线性”，未给斜率）。但 21 届 STC32G144K246 工程保留了同一思路的**阿克曼差速**代码（当前被注释掉，实际 `diff_cmd = 0`，即只跑等速）：

```c
steer_angle = mul_div_i16(Servo.Out, 1, 17);   /* 舵机输出 → 打角(度) */
tan = fast_tan(steer_angle);                   /* 查表 tan，限幅 ±45° */
diff_cmd = mul_div_i16(W, tan, M);             /* 阿克曼差速: W/M * tan(δ) */
gyro_feedforward = - mul_div_i16(gyro_z, 10, 256);
gyro_feedforward = func_limit_ab(gyro_feedforward, -20, 20);
diff_cmd += gyro_feedforward;
diff_cmd = func_limit_ab(diff_cmd, -DIFF_SPEED_LIMIT, DIFF_SPEED_LIMIT);
/* 最终：Right.Target = (256+diff_cmd)*base/256 ; Left.Target = (256-diff_cmd)*base/256 */
```
其 `fast_tan` 用 46 项定点查表（0~45°，满度 256）。

**另一种“差速系数”写法（四轮电磁参考工程）**：`Turn_speed_mid ± Steer_out*0.48`，即**差速系数 0.48**；直道/弯道切换用 `|poserror| = 10` 作阈值：

```c
if(poserror>10 || poserror<-10)
    PID_control(Turn_speed_mid - Steer_out*0.48, Turn_speed_mid + Steer_out*0.48);
else
    PID_control(Go_speed_mid  - Steer_out*0.48, Go_speed_mid  + Steer_out*0.48);
```

### 11.2 目标速度动态衰减

**✅ 有确切公式与系数（第十八届哈工大紫丁香三队技术报告，这是本次核查拿到的唯一带数字的“速度随偏差衰减”公式）**：

> **`SetSpeed = (124 - 3 * error) * bas_speed`**

其中 `error` 是“**图像中线与中心值的偏差**”（**像素域**），**124 是该工程的中心位置基准值**。等价改写为“目标速度随偏差衰减”的形式：

```text
SetSpeed = bas_speed * (1 - 3*error/124)
```

→ **k ≈ 3/124 ≈ 0.0242（每像素）**；若把偏差按满量程 124 归一化为 1.0，则 **k ≈ 3**（即偏差打到满量程时，目标速度只保留 25%）。

该报告的其它相关结论（原文）：
- **加/减速曲线要分开**：“车在从弯道入直道时加速和从直道入弯道时减速达不到最好的控制效果，直道入弯道减速不够快速，弯道出直道加速的时机不够及时。因此我们做了进一步的改进，**加速的曲线与减速的曲线分开形成滞回的效果**，结果表明，控制效果更好。”
- **曲线形状**：“将每场图像得到的黑线位置与速度 PI 参考速度值构成**二次曲线**关系。”
- **分段速度决策**：“我们最终采用的速度决策方法是一个简单的**分段函数**。将赛道分为**直道、小半径弯道、大半径弯道、丢失路线**。”
- **长直道加速**：“赛前在程序中人为设定直线速度不够灵活不够合理，所以我们在程序中根据图像识别长直线提高了直线速度 `HighestSpeed`。”

**✅ 另一条带完整代码的形态（21 届 STC32G144K246 开源工程）** —— `base − k·|打角|` 形式：

```c
base_cmd = base_cmd - mul_div_i16(steer_abs, TURN_SLOW_GAIN_NUM, TURN_SLOW_GAIN_DEN);
```
其中 `steer_abs = |Servo.Out|`（舵机 PID 输出绝对值）。**`TURN_SLOW_GAIN_NUM/DEN` 的具体数值在该源码快照中未出现（宏定义体未见）→ ❌ 未确认。**

**✅ 元素分段速度表（同一 STC32G144K246 工程，基准 `BASE_SPEED_DEFAULT = 200`）**：

| 赛道状态 | 目标速度 | 相对基准 |
|---|---|---|
| 直道（`straight_acc==1` 且陀螺仪确认） | `BASE_SPEED_DEFAULT + TURBO_BOOST_VALUE` | 提速（`TURBO_BOOST_VALUE` 未确认） |
| 直道但 `\|gyro_z\| > 35`（车身不够稳） | `BASE_SPEED_DEFAULT + TURBO_BOOST_VALUE/2` | 只加一半 |
| 环岛（左/右） | `BASE_SPEED_DEFAULT − 30` | **−15%** |
| 十字 | `BASE_SPEED_DEFAULT − 30` | **−15%** |
| 普通弯道 | `BASE_SPEED_DEFAULT − 40` | **−20%** |
| 坡道（`Ramp_Flag`） | `base_cmd += 20` | **+10%** |

**其他有代码的衰减/决策方式**：
- **偏差阈值 + 计数判直道**（19 届开源，偏差量纲 0~100）：`if(bias<=5 && bias>=-5 && Accelerate_Count<75)` 判为直道并计数，计数到 75 后切 `STRAIGHT_HIGH_SPEED`；`else if(bias>5 || bias<-5)` 视为弯道并 `Accelerate_Count = 0`。所有分支都同时施加 `±bias_speed` 差速，其中 `bias_speed = BIAS_KP*bias + BIAS_KD*bias_differential`（**`BIAS_KP`/`BIAS_KD` 数值源码未给 → ❌ 未确认**）。
- **差速本身即“入弯减速”的物理等价**：内侧轮目标速度下降，整车实际速度自然下降。

> ✅ **可以确认的数字**：HIT `(124 − 3·error)`（像素域，等效归一化 k ≈ 3）；直道判据阈值 **±5**（0~100 量纲）；弯道降幅 **30~40（基准 200，即 −15%~−20%）**；差速系数 **0.48**；四轮参考直道/弯道切换阈值 **10**。
> ⚠️ **❌ 未确认**：**没有抓到任何材料把 k 直接写成“归一化偏差 ±100 时的 k 值”**。上列数字都带各自的偏差量纲与基准速度。

**如果你要自己写 `target = base − k·|error|`**：可直接借用 HIT 的**结构**（`base*(1 − k·error/满量程)`，满量程偏差时保留约 25% 速度）+ **加减速滞回**；k 的具体值必须按你的偏差量纲与基准速度标定。

### 11.3 速度环形式、控制周期与参数数量级 —— ✅ **本次拿到了真实数字**

**四条独立来源的速度环参数（务必注意“增量式/位置式”与量纲）**：

| 来源 | 形式 | Kp | Ki | Kd | 输出限幅 | 控制周期 | 备注 |
|---|---|---|---|---|---|---|---|
| **21 届 STC32G144K246 开源工程** | **增量式** | **32/10 = 3.2** | **25/9 ≈ 2.78** | **1/10 = 0.1** | **±9500** | **5 ms**（TIM9） | PWM 载波 **10000**、**17 kHz**；另有前馈 `Kff1 = 1.0`（对目标速度）、`Kff2 = 0` |
| **龙邱 21 届技术手册** | **增量式** | **9** | **3.5** | **0** | 未给出 | 未明确 | 原文“增量式 PID 一般只需要调节 **KP 和 KI** 即可”“通常 **KP 可以加到 KI 的 2~3 倍**”；**位置式与增量式的同一组数字不可互换** |
| 三轮电磁学习工程 | 位置式 | 150 | **0（写成 PD）** | 50 | 未给出 | 未明确 | Kp/Kd 大是因为“误差与实际要用的值相差很大” |
| 18 届负压电磁 HIT | **增量式 PI** | 未给出 | 未给出 | — | — | 未给出 | 明确定为“增量式 PI” |

> ★ **龙邱手册的重要提醒（原文）**：“**增量式 PID 中，KP 项对应的是误差的变化量，KI 项对应的是误差**，KD 项对应的是这次与上次误差变化量的差值，**与位置式的对应关系有所不同**”——**同一组数字在位置式与增量式之间不可互换**。这是最容易踩的坑。

**增量式 PI 的逐字实现（STC32G144K246 开源工程，用分子/分母分数避免浮点）**：

```c
/* 参数（分数形式）：Kp = 32/10, Ki = (1/10)/(25/9) = 9/250, Kd = (1/10)/9 = 1/90,
   前馈 Kff1 = 10/10 = 1, Kff2 = 1/10, OutMax = 9500, OutMin = -9500 */
void PID_Update(PID_t *p, int16 speed)
{
    p->Actual = speed;
    p->Error2 = p->Error1;
    p->Error1 = p->Error0;
    p->Error0 = (int16)func_limit_ab((int32)p->Target - p->Actual, -32768L, 32767L);

    error_p = (int32)p->Error0 - p->Error1;                          /* Δe */
    error_d = (int32)p->Error0 - (int32)p->Error1 * 2L + p->Error2;  /* Δ²e */

    term_p = mul_div_i16(error_p, p->Kp_num, p->Kp_den);
    term_i = mul_div_i16(p->Error0, p->Ki_num, p->Ki_den);
    term_d = mul_div_i16(error_d, p->Kd_num, p->Kd_den);

    pid_sum = (int32)p->PidOut + term_p + term_i + term_d;
    p->PidOut = (int16)func_limit_ab(pid_sum, (int32)p->OutMin, (int32)p->OutMax);
    /* 前馈项 */
    term_ff1 = mul_div_i16(p->Target, p->Kff1_num, p->Kff1_den);
    term_ff2 = mul_div_i16((int32)p->Target - p->TargetLast, p->Kff2_num, p->Kff2_den);
    out_sum = (int32)p->PidOut + term_ff1 + term_ff2;
    p->Out = (int16)func_limit_ab(out_sum, (int32)p->OutMin, (int32)p->OutMax);
}
```
★ 两个值得照抄的设计：**① 双重限幅**（既限幅在积分累加值 `PidOut` 上，又限幅在含前馈的最终输出 `Out` 上）；**② 目标速度前馈 `Kff1`** 直接给出基准占空比，显著减轻 PI 负担。

**标准增量式写法（龙邱手册）**：

```c
Increase = sptr->KP * (sptr->iError - sptr->LastError)
         + sptr->KI * sptr->iError
         + sptr->KD * (sptr->iError - 2*sptr->LastError + sptr->PrevError);
```

**控制周期**：
- **21 届 STC32G144K246 开源工程：速度环 5 ms（TIM9）、舵机环 4 ms（TIM10）**，两者分离且中断优先级不同（速度环 priority 1，舵机 priority 2）；
- 山东大学威海 ACE 队：“方向环、速度环**分别放在两个 5ms 的定时器中执行**”；
- 直立车：直立环 500 Hz~1 kHz、速度环 50 Hz~100 Hz。

**编码器测速方式（✅ 最完整的实现，STC32G144K246 开源工程）**：

```c
volatile int16 speed_R = 0;
volatile int16 speed_L = 0;

void Speed_Calculate(void)
{
    speed_R = -encoder_get_count(ENCODER_QUAD_2);   /* 右轮取负，统一符号约定 */
    speed_L =  encoder_get_count(ENCODER_QUAD_1);
    encoder_clear_count(ENCODER_QUAD_1);
    encoder_clear_count(ENCODER_QUAD_2);
}
void Encoder_Init(void)
{
    encoder_quad_init(ENCODER_QUAD_1, ENCODER_QUAD_1_CHA, ENCODER_QUAD_1_CHB);
    encoder_quad_init(ENCODER_QUAD_2, ENCODER_QUAD_2_CHA, ENCODER_QUAD_2_CHB);
}
```
`Speed_Calculate()` 在 5 ms 的 `pit9_handler` 里调用，因此 **speed 的单位是“5 ms 内的正交计数差值”，不折算成 m/s**（该工程 README 明确说明“速度变量使用编码器和 PWM 的内部量纲，代码本身没有把它校准成 m/s”）。

- **是否标定 m/s 的取舍（19 届开源作者原文）**：“在这里**没有进行标量化处理**，优点：控制简单，缺点：不知道自己小车的实际速度；**标量化处理**优点：可以知道小车跑到几米的速度，缺点：计算比较麻烦，容易算错。”
- ★ **极性与左右对应是高频踩坑点**（龙邱手册整段强调）：“在进行电机闭环之前，**务必确认编码器和电机的左右对应关系和极性**！！！且每次更换电机、更改接线，务必要重新检查核对！！！……给左电机一个正的占空比，核对是否是左电机转、左电机是否是正转、是否是左编码器有数据、数据是否是正值”。
- **方向编码器读法（19 届开源，含方向引脚处理）**：
```c
temp_left_pluse  = ctimer_count_read(SPEEDL_PLUSE);
temp_right_pluse = ctimer_count_read(SPEEDR_PLUSE);
ctimer_count_clean(SPEEDL_PLUSE);
ctimer_count_clean(SPEEDR_PLUSE);
if(1 == SPEEDL_DIR) temp_left_pluse  =  temp_left_pluse;   /* 正转 */
else                temp_left_pluse  = -temp_left_pluse;   /* 反转 */
if(1 == SPEEDR_DIR) temp_right_pluse = -temp_right_pluse;  /* 右侧取反 */
else                temp_right_pluse =  temp_right_pluse;
```
### 11.4 限幅

- ✅ **有来源**：对**方向控制量的变化量限幅**（或与历史值加权平均）以防抖；
- ✅ **有来源**：舵机/PWM 输出需要限幅（舵机活动范围 `SERVO_L_MAX`/`SERVO_R_MAX`）；
- ✅ **有来源（具体数字）：占空比输出限幅**。21 届 STC32G144K246 工程 PID 输出限幅 **±9500**，而 PWM 载波（满占空比）为 **10000**（`pwm_init(PWM_3, 17000, 10000)`，即 **17 kHz / 满度 10000**），因此输出被限制在满量程的 **95%**。驱动方式为“一个半桥固定满占空比 + 另一个半桥给 `10000-|Out|`”实现正反转：
```c
if( Right.Out >= 0 ) { pwm_set_duty(PWM_3, 10000); pwm_set_duty(PWM_1, 10000 - Right.Out); }
else                 { pwm_set_duty(PWM_1, 10000); pwm_set_duty(PWM_3, 10000 + Right.Out ); }
```
- ✅ **有来源（具体数字）：加速度限幅（“步长平滑器”）**。同一工程：
```c
const uint8 STEP_ACC = 20;   /* 每个控制周期(5ms)最多加速 20 */
const uint8 STEP_DEC = 40;   /* 每个控制周期(5ms)最多减速 40 */
if (base_speed_current < base_cmd) { base_speed_current += STEP_ACC;
    if (base_speed_current > base_cmd) base_speed_current = base_cmd; }
else if (base_speed_current > base_cmd) { base_speed_current -= STEP_DEC;
    if (base_speed_current < base_cmd) base_speed_current = base_cmd; }
```
  ★ **减速步长（40）是加速步长（20）的 2 倍** —— 典型的“**减速优先**”设计。按 5 ms 周期折算，基准速度 200 时从 0 加到 200 需 10 个周期 = **50 ms**。
- ✅ **有来源：起步保护 / 软启动**：`speed_init = base_speed_default / 2;`（起步半速），并“直到 `speed_R>=20 && speed_L>=20` 连续 3 次才退出起步状态”。
- ✅ **有来源：加速冷却**：`COOL_DOWN_FRAMES = 200`，弯道/环岛/十字都把 `accel_cooldown` 重置为 200 帧，**冷却期内不做直道加速**（防止出弯立即全速）。
- ✅ **有来源：硬件层面的过流/保护**：第十八届 HIT 方案采用“**电流环**”（“电流内环与速度外环相结合”）；IR2104 栅极驱动“有**硬件死区**，防止同桥臂导通”；DRV8701E 方案“**带过流保护功能**”；另有方案提出“加入电压监测回路……据此对控制器的控制参数进行动态修正”（**未给补偿系数**）。
- ⚠️ **❌ 未确认**：**电流环的具体限幅值 / 采样电阻 / 放大倍数**（HIT 报告只给原理图与“电流内环+速度外环”描述）；电磁组是否**普遍**采用电流环（仅一届报告明确提到）。
- ⚠️ **❌ 未确认**：来源中一份“按车速自动调整 PWM 限幅（v<1m/s→800、1≤v<2m/s→600、v≥2m/s→400）”的说法来自 **CSDN 文库 AIGC 生成答案页，权威性低，不建议作为设计依据**。

### 11.5 “入弯减速、出弯加速”与元素识别的联动

**✅ 有完整代码的联动（21 届 STC32G144K246 开源工程，元素识别 → 目标速度表）**：

```c
if (ImageStatus.straight_acc == 1)                       /* 直道判定标志 */
{
    if (accel_cooldown > 0) { accel_cooldown--; base_cmd = BASE_SPEED_DEFAULT; }
    else {
        if (gyro_z_abs <= 35) base_cmd = BASE_SPEED_DEFAULT + TURBO_BOOST_VALUE;      /* 陀螺仪确认很直才全速 */
        else                  base_cmd = BASE_SPEED_DEFAULT + TURBO_BOOST_VALUE / 2;  /* 角速度偏大只加一半 */
    }
}
else if (Road_type == LeftCirque || Road_type == RightCirque)
{ accel_cooldown = COOL_DOWN_FRAMES; base_cmd = BASE_SPEED_DEFAULT - 30; }            /* 环岛减速 */
else if (Road_type == Cross_ture || Road_type == Cross_mid)
{ accel_cooldown = COOL_DOWN_FRAMES; base_cmd = BASE_SPEED_DEFAULT - 30; }            /* 十字减速 */
else { accel_cooldown = COOL_DOWN_FRAMES; base_cmd = BASE_SPEED_DEFAULT - 40; }       /* 普通弯道减速 */
if(Ramp_Flag) base_cmd += 20;                                                          /* 坡道提速 */
base_cmd = base_cmd - mul_div_i16(steer_abs, TURN_SLOW_GAIN_NUM, TURN_SLOW_GAIN_DEN);  /* 按当前打角连续微调 */
```
★ 注意这里的“出弯加速”是通过 **`straight_acc` 直道标志 + `accel_cooldown` 冷却计数**实现的：**出弯后并不立刻全速，而是等冷却结束且陀螺仪确认车身稳定（`|gyro_z| ≤ 35`）才逐步加到全速**。这是一个很值得照抄的设计。
> ⚠️ `TURBO_BOOST_VALUE`、`TURN_SLOW_GAIN_NUM/DEN` 的具体数值在该源码快照中未出现 → **❌ 未确认**。

**✅ 加速/减速滞回曲线（哈工大报告原文）**：“车在从弯道入直道时加速和从直道入弯道时减速达不到最好的控制效果，直道入弯道减速不够快速，弯道出直道加速的时机不够及时。因此我们做了进一步的改进，**加速的曲线与减速的曲线分开形成滞回的效果**，结果表明，控制效果更好。”

**✅ 赛道记忆（长直道冲刺）**：
- 哈工大方案：“赛前在程序中人为设定直线速度不够灵活不够合理，所以我们在程序中**根据图像识别长直线提高了直线速度 `HighestSpeed`**。”
- 山东大学威海 ACE 队（完整赛道记忆算法）：“首先让车模在赛道上**慢速运行一周，通过记录编码器脉冲数，记录下各个元素的所在位置**。当正式发车时，如果当前赛道类型为长直道，则提高车速。”
- 该队同时说明了局限：“采用这种方案团队调试的极限速度在 **2.7 m/s 就到达了速度的天花板**”，随后转向“AI 只做元素提前判断、方向控制仍用传统 PID 的分段方案”。

**✅ 元素期间的控制切换**：识别到十字/环岛时**把斜电感从偏差计算中移除**；入环期间**切换为固定打角 + 陀螺仪积分**，出环再切回电感控制并**还原 A/B/C/P 参数**。

**✅ 丢线/出赛道保护**：19 届开源代码有“出赛道停车防止撞坏电感和损坏电机”等状态分支，并对直角/坡道分别给固定左右目标速度（`STRAIGHT_ANGLE_LEFT_L/R`、`SLOPE_SPEED`，**具体数值未在抓取内容中给出 → ❌ 未确认**）。

### 11.6 来源

1. **第十八届智能汽车竞赛技术报告·负压电磁·哈尔滨工业大学（紫丁香三队）**（“转向舵机 PD、驱动电机 PI”；**`SetSpeed = (124 - 3*error) * bas_speed`**；加速/减速滞回曲线；`HighestSpeed` 长直道；速度分段函数“直道/小半径弯/大半径弯/丢线”；增量式 PI；**电流内环+速度外环**；IR2104 硬件死区；1024 线增量式编码器）：
   <https://blog.csdn.net/zhuoqingjoking97298/article/details/132534760?spm=1001.2014.3001.5501>
2. **龙邱科技·第二十一届·疯狂电路组技术手册**（增量式速度环 `{9, 3.5, 0}`、“KP 可加到 KI 的 2~3 倍”、增量式与位置式参数不可互换、编码器极性核对流程、差速由转向环输出计算、电机 PWM 10k~30kHz、DRV8701 过流保护、限幅保护函数）：
   <https://blog.csdn.net/longqiu_LQ/article/details/160896921?spm=1001.2014.3001.5501>
3. **21st-smart-car-yan-guo-liu-hen-8of10-2.6mps（GitHub 开源，主控即 STC32G144K246）** —— 5 ms 速度环 / 4 ms 舵机环、增量式 PID `Kp=3.2 / Ki≈2.78 / Kd=0.1`、输出限幅 ±9500、`Kff1=1.0` 前馈、双重限幅、`STEP_ACC=20 / STEP_DEC=40`、`COOL_DOWN_FRAMES=200`、元素分段速度表、陀螺仪确认直道 `|gyro_z|≤35`、阿克曼差速与 `fast_tan` 查表、编码器正交计数清零测速、PWM 17 kHz / 满度 10000：
   <https://github.com/ZORE-dlfd/21st-smart-car-yan-guo-liu-hen-8of10-2.6mps>
4. 电磁组-19 届智能车电磁组电感处理与循迹代码带元素处理+讲解（开源）—— 四轮→三轮车模与差速转向、编码器方向判读代码、`bias_speed = BIAS_KP*bias + BIAS_KD*bias_differential`、直道计数 75 后切 `STRAIGHT_HIGH_SPEED`、`|bias|<=5` 直道判据、直角/坡道固定目标速度、位置式 PID（含积分限幅）：
   <https://blog.csdn.net/yqy142134/article/details/141270512>
5. 智能车电磁组——基本控制篇（四轮电磁参考方案）—— 转向 PD 代码、增量式电机 PID 代码、`poserror` 阈值 10 切换、差速系数 **0.48**：
   <https://blog.csdn.net/zxy19872029175/article/details/126766942?spm=1001.2014.3001.5501>
6. 关于智能车_电磁三轮 pid 控制的学习过程（三轮电磁第一手记录）—— 三轮差速闭环流程、速度环位置式 `{150, 0, 50}`、转向环 `{0.7, 0, 0.5}`、编码器与 PWM 量纲不匹配的处理：
   <https://blog.csdn.net/xcy88888888/article/details/130792635?spm=1001.2014.3001.5501>
7. 初调三轮电磁组感受（51hei 论坛第一手调试记录）—— 三轮差速最小半径受“单路 PWM 不能为负”限制、`operate_motor()` 正反转转弯、“速度环闭环配合方向环、当串级 PID 调参、先调速度 pid 再调方向、效果显著”、7.85 V 约 0.9 m/s：
   <http://www.51hei.com/bbs/dpj-152036-1.html>
8. 山东大学（威海）ACE 队技术报告（差速算法 + 前轮转角线性拟合、赛道记忆算法、方向环/速度环各 5 ms 定时器、2.7 m/s 天花板）：
   <https://www.eet-china.com/mp/a27552.html>
9. 直立车（平衡组）方案页（直立环 500 Hz~1 kHz / 速度环 50 Hz~100 Hz / 方向环 PD、先整定直立环再叠速度环、BTN8982 电流反馈与过温保护、电压监测动态修正；**方案性描述，参数未给**）：
   <https://www.iczoom.com/t-solution/85497-scheme.html>
10. CSDN《四轮电磁——速度环增量式 PID》（**本次抓取失败，正文未取到**）：
    <https://new.guyuehome.com/43318>
11. CSDN《智能车速度环 PID 参数整定与优化策略》与《基于 STC32G128K 缩微电磁四轮差速循迹》（**CSDN 文库 AIGC 生成答案页，权威性低，仅作思路参考**）：
    <https://wenku.csdn.net/answer/2wfjcbcqys> ，<https://wenku.csdn.net/answer/4tc3kh2wwd>

### 11.7 ⚠️ 使用本节数字前必须知道的三件事

1. **不同工程的 Kp/Ki/Kd 不可互抄**：因为**位置式与增量式的参数含义不同**（龙邱手册明文提醒），且**偏差量纲不同**（像素域 / 0~100 差比和域 / 舵机误差域）。本报告已逐项标注量纲。
2. **“归一化偏差 ±100 下速度环 Kp/Ki 的典型值”仍然 ❌ 未确认**：现有数字（3.2/2.78、9/3.5、150/50）都带各自量纲与控制器形式。
3. **可直接照抄的“结构”比“数值”更值钱**，本节可确证的结构是：
   - **编码器测速**（正交计数差值 + 立即清零，可不折算 m/s）；
   - **差速**（转向环输出 → 左右目标速度差，或阿克曼 `W/M·tanδ` 查表）；
   - **速度环 5 ms（舵机环 4 ms）、增量式 PID + 目标速度前馈 `Kff1` + 双重限幅**；
   - **加速度限幅（`STEP_ACC=20` / `STEP_DEC=40`，减速优先）+ 加速冷却帧 + 起步半速保护**；
   - **元素分段速度表**（直道提速、环岛/十字 −30、普通弯 −40、坡道 +20，基准 200）+ **陀螺仪确认车身稳定才给全速**；
   - **加/减速曲线分离形成滞回**。

---

## 12. 元素识别：十字 / 圆环（环岛）/ 三岔路口

### 12.0 前提：元素识别能力由**电感布局**决定

来源明确区分「单排电感」与「双排电感（前后两排）」：

> “最左和最右的横电感用于正常循迹，中间三个电感（两个竖电感一个横电感）用来判断元素”；纯电磁竞速组高手“一般都是架着**双排电感**，前面一排后面一排，循迹可以用双排电感加权，**识别元素可以用前排电感提前识别方便处理**，这样车速的上限更高”。

下文代码中的电感编号必须对照该文自己的布局理解：`diangan[0]`=最左横、`diangan[4]`=最右横、`diangan[1]`/`diangan[2]`=两个竖电感、`diangan[3]`=中间横电感（“一共五个电感”）。

### 12.1 圆环 / 环岛（roundabout）

**物理特征**：赛道电磁线在环岛处会绕一个圆圈再出去，因此进出口附近的电感值与正常路段有明显区别——表现为**左右横电感同时明显升高**，且**总电感量（中间竖电感之和）升高**。

**实测判据代码（5 电感单排布局，阈值取自原文）：**

```c
/* 满足条件提前识别到环岛 */
if( ((Detection==0) && (leftV>68) && (rightV>68)
     && (diangan[4]>2100) && ((diangan[1]+diangan[2])>3200))
     && Round==0 && Fork==0 )
{
    Detection = 1;          /* 已识别到环岛，尚未入环 */
    LED_Ctrl(LED2, ON);
}
else if((bmq_juli > 4700) && Detection==1)   /* 到达打脚入环的点 */
{
    Round = 1;              /* 打脚入环 */
}
```

- `leftV`/`rightV` 是归一化后的左右偏差量（原文只给阈值 68，**未给出定义式 → ❌ 未确认**）。
- `bmq_juli` 是**编码器积分位移**（见下）。

**核心手法：提前识别 → 用编码器积分延迟入环**

```c
if(Detection==1)
{
    bmq_juli += (ECPULSE1 - ECPULSE2);   /* 编码器积分 = 位移 */
}
```
> 原文：“这个时间我们可以用来**编码器积分**……对速度进行积分得到的不就是位移吗，我们用编码器积分算出小车从识别点到打角点的距离（这个距离可以把它显示到屏幕上**用手推一下得出一个粗略的值再微调**即可），然后找出打角点。”

**入环动作 = 取消循迹、强制固定打角一段时间**（`tly_jifen` 是陀螺仪积分；不会用陀螺仪时原文建议“先定义一个 time，让 time++ 来替代”）：

```c
if(Round==1 && (tly_jifen>>10) > -200)
{
    TempAngle = 30;                 /* 左环岛给正值，右环岛给负值 */
}
if((tly_jifen>>10) < -200)
{
    TempAngle = (leftV - rightV);   /* 切换回正常循迹 */
}
```
> ⚠️ 陀螺仪积分量 `tly_jifen>>10` 与 ±200 阈值对应的物理单位**未确认**。
> ⚠️ **必须清零标志位**：“在圆环中编码器积分可以一直开着，出了圆环以后把**圆环标志位和编码器积分都清零**，以便下一次识别环岛，**不清零可能就只能进一次环岛**。”

### 12.2 环岛的五状态状态机（负压电磁组实测）+ 左右环方向判断

来源给出 `RingFlag` 从 1 到 5 的完整时序。

**判别左环 / 右环（对称性法）**：“斜电感 3 和 5 之间数值相差较大，且 5 会大于 3，根据这个也可以判断圆环是左环还是右环”——即比较两个**斜置（八字）电感**的大小，大的一侧就是圆环所在侧。

**位置 1（识别点）的两个特征**：
1. “水平电感 1 和 4 的电感值是直道上的电感值约 **2 倍**，尤其是 4 电感”；
2. “斜电感 3 和 5 之间数值相差较大”。
→ 置 `RingFlag=1`。

**位置 1→2 的坑与两种对策（很有价值的工程细节）**：
- 坑：“由于靠近圆环一侧电感值会比远离圆环一侧电感值大，通过差比和计算，会使得车向靠近圆环方向运动，但是当车接近 2 位置之后，两侧电感值又相差不多，通过差比和又会使得车向外侧运动”→ 入环时**车身姿态不正**，且“很容易就冲出赛道”。
- **方案一**：识别到圆环后“**通过人为的控制舵机，让其以固定角度向前运行**”，运行到 2 时判断是入环点，`RingFlag=2`。
- **方案二**：在位置 1 处“**调整差比和公式中的 A,B,C,P 四个参数，使得误差 Error 变化平缓**……这里可以采用串口将小车从位置 1 到 2 这一段路程 **7 个电感的 ADC 值传输到电脑，然后通过 MATLAB 将其 Error 计算出来**，然后再调整四个参数”。

**位置 2→3（入环）**：“通过舵机固定打角，**陀螺仪积分**来计算当前角度，当陀螺仪角度为 **30°**（可根据需要更改）时到达位置 3，`RingFlag=3`”，随后**切回电感控制**，并**必须把 A,B,C,P 还原**“不然小车无法正常循迹”。

**位置 3→出环**：“当陀螺仪积分角度接近 **360°**（绕一圈）之后，表明小车已经到达出环点，即位置 2，`RingFlag=4`”；再把 A,B,C,P 设为入环那套参数，让车近似直线从 2 走到 4，`RingFlag=5`，到达 4 后恢复参数。

> ⚠️ A/B/C/P 的具体数值原文未给 → **❌ 未确认**。

### 12.3 十字路口（crossroad）—— **电磁组通常不处理，直接冲过去**

**这是本节最重要的结论，与摄像头组做法完全不同。**

> “当然元素也不止这三个，还有坡道，十字路口，断路和车库，**对于电磁车来说，十字路口和断路都不需要进行处理**，坡道慢速情况下也可以不进行处理。”
> 另一篇工程分享：“至于其他元素我觉得没必要判断，**直接冲就完了**”。

→ **“十字补线”“打死转向”是摄像头组的做法，不是电磁组的常见做法**。本次**未能在任何实际取到的电磁车来源中找到“十字打死转向/补线”的代码** → **❌ 未确认**。

**机理**：十字路口是另一条电磁线**垂直**穿过，两条导线在交叉点附近磁场叠加，使近处传感器读数异常；但车体一旦通过交叉点，差比和误差回到接近 0。

**十字与岔路的判据天然互斥（可直接用于防误判）**：岔路判据是“中间电感**掉下去**”（`diangan[4]<1000`），环岛是“中间电感**升上去**”（`diangan[4]>2100`）。十字处中间横电感被垂直线二次磁场抬高/畸变，因此**用“中间横电感 < 阈值”判岔路时不会在十字触发**。

### 12.4 三岔路口 / 岔路（bifurcation）

**物理特征（Y 字形）**：“岔路这里的电磁分布呈现一个 **Y 字形**（因此此处电感值相比其它的地方也会有明显区别，找到这个特殊点即可识别岔路）”。

**实测判据代码（含**双阈值窗口**）：**

```c
/* 识别岔路 */
if( diangan[4] < 1000
    && (diangan[1] + diangan[2]) < 500
    && (diangan[1] + diangan[2]) > 200
    && Round==0 && Fork==0 && Forkchu==0 )
{
    Fork = 1;                 /* 进入岔路处理函数 */
    LED_Ctrl(LED2, ON);
}
```

★ 注意这里同时用了**上界和下界**（`<500 && >200`）：既排除正常直道（总电感量大），也排除**完全无信号**（脱线/断路，总电感量≈0）。**这是防误判的关键写法。**

**处理动作与环岛同构**：“当小车的电感提前识别到岔路以后，再像圆环处理那样给一个固定差值打脚一段时间，进入岔路后切换回正常循迹，同样的速度不快时，**出岔路也不用处理**。”

**左右岔路的区分 = 次数标志位（不是几何判断）** —— 这是三岔最难也最实用的一点：

```c
if(Fork==1 && count==0 && (tly_jifen>>10) > -150)   /* count==0 右岔路 */
if(Fork==1 && count==1 && (tly_jifen>>10) <  150)   /* count==1 左岔路 */
```
> “出岔路以后不仅要把各个岔路标志位清零，还要把一个**次数标志位置 1**（即第一次识别到岔路时，该次数标志位为 0 向左打脚入左岔路，第二次识别到岔路时，该次数标志位为 1 向右打脚入右岔路），这样即可**区分开左右岔路**。”

**判断难点（可从上面代码直接推出三点）**：
1. **Y 字总电感量下降幅度与“丢线”接近**，必须用双阈值窗口而不是单阈值；
2. **难以从电感几何上区分左岔/右岔**（Y 字两侧对称性随车姿态变化），所以实际做法退化为**赛道地图知识 + 次数计数**；
3. **同一元素在一圈内会经过两次**（先左岔后右岔），所以 `count` 必须由出元素事件翻转，且必须与 `Fork`/`Forkchu` 一起清零。

### 12.5 元素识别常用的“具体量”

**（a）归一化总电感量（总 ADC 和）**
- 落地形式就是**中间两竖电感之和** `diangan[1]+diangan[2]`：环岛判据取 `>3200`、岔路判据取 `200 ~ 500`。
- “总电感量突变”是环岛/岔路识别的核心量。

**（b）差比和（归一化偏差）**
$$\text{err}=\frac{A(L-R)}{A(L+R)}=A\cdot\frac{L-R}{L+R}$$
原文：“`L-R` 代表了它左右电感的差值，`err` 表示差值占两端电感传感值的比例，**实现了归一化**”。

**（c）差比和差（卓晴提出，加入八字/斜电感加权）**
$$\text{err}=\frac{A(L-R)+B(LM-RM)}{A(L+R)+C\,|LM-RM|}$$
`LM`/`RM` 是**八字电感**——“由于其有一定的角度，这两个电感对弯道更加敏感，在直道上和两侧普通电感一样”。分母加 `C|LM-RM|` 的目的：“在差比和中添加它的值……**这样会导致归一化失效**，当 `B(LM-RM)` 较大时，分子会大于分母。为了解决这个问题，**在分母等大地加上 `C|LM-RM|`** 即可”。
> 本次取到的来源明确说该式子的分母含 `C|LM-RM|`；而 §9.2 中卓晴另一处原文 C4 的分母是 `LIMIT + C·(LM−RM)`。**两版本并存，引用时注明。**

**调参规律（可直接照做）：**
1. 先调 **A**，`B`/`C` 置零，若存在后级 PID 将 `i`、`d` 置零，`p` 调整到“在静止状态下，左右摆动车体时舵机能够灵活响应”；再放到直道上试跑——“A 参数决定其对直道的响应，故此时**在弯道处丢弯正常**”。
2. 再增大 **B**，同时增大 **C**，“**保持比例 B/C ≤ 1**”。
3. “增大 `A/B` 将增强 LR 两个电感的作用，增大 `B/C` 将提高车‘**切弯道倾向**’，该值越大，经过弯道时车将更贴近凹陷侧”。

**（d）左右电感对称性**
- 用作**环岛左右方向判据**：比较斜电感 3 与 5 的大小，大的一侧即圆环所在侧。
- 也可用编码器/陀螺仪积分作为替代/补充：环岛打角 `TempAngle = +30`（左环）/ `-30`（右环）。

**（e）连续几个周期偏差很小**
> ⚠️ **❌ 未确认**：**未能在取到的任一来源中找到以“连续 N 个周期 |err| < ε”作为元素判据的真实代码。** 取到的来源中，入环/出环的时序判据一律使用**编码器积分位移**、**陀螺仪积分角度**或**简单定时计数**。**不要按“连续几周期无偏差”去设计元素判据。**

### 12.6 状态机的常见组织方式

- **实测最简形式：用独立整型标志位而不是 enum**。`Detection`（已识别未入环）、`Round`（入环）、`Fork`（岔路）、`Forkchu`（岔路出）、`count`（岔路次数）、`PodaoFlag`、`BarrierFlag`、`RingFlag`——多个并列标志位配合 `else if` 链。互斥靠条件里显式写 `&& Round==0 && Fork==0`。
- **`RingFlag` 序号式状态机**：`1=识别`、`2=入环点`、`3=环内（陀螺仪 30°）`、`4=绕满 360° 出环`、`5=出环段`。同一变量承载“当前处于元素的哪一段”，配合不同的控制源（固定打角 / 电感 / 陀螺仪）切换。
- **enum + 状态转移的表述形式**（来源文章用图描述，付费墙截断，未取到完整枚举代码）：
```text
[*] --> 直道循迹
直道循迹 --> 弯道预判: 单侧电感值>阈值
弯道预判 --> 入弯控制: 差值持续增加
入弯控制 --> 直道循迹: 差值回归正常
直道循迹 --> 环岛探测: V电感突变+横电感对称
环岛探测 --> 环岛进入: 持续检测到特征
```
  该文给出的状态机三条“核心优势”可确认：**行为可预测性**、**逻辑可视化**、**异常隔离**（原文举例“如将坡道误判为十字路”）。
- **进入/退出条件成对出现**：进入用“电感特征 + 元素标志互斥”，退出用“**积分量越过阈值**”或“**回到正常循迹判据**”。
  - 出环岛：`(tly_jifen>>10) < -200` → `TempAngle = leftV - rightV`（切回差比和）；
  - 出岔路：**不处理**，靠“进入岔路后切换回正常循迹”自动回到主线；
  - 通用要求：“处理完记得**清空标志位**（这很重要！！！）”。

### 12.7 防误判（debounce）——来源中实际出现的四种

1. **双阈值窗口**（岔路 `200 < 和 < 500`）——排除正常直道与脱线两种情形。
2. **多标志位互斥**：`&& Round==0 && Fork==0 && Forkchu==0`——同一时刻只允许一个元素状态，防止环岛与岔路互相抢占。
3. **位移/角度积分门限代替时间门限**：`bmq_juli > 4700` 才允许入环打角；`(tly_jifen>>10) > -200` 才允许切回循迹。
4. **蜂鸣器/LED 在车验证法（强烈推荐）**：
   > “为了验证你写的识别环岛会不会误判，可以写一个**识别到环岛蜂鸣器就响**，让车子在赛道上跑一圈，看它会不会在除了环岛的其它地方也响……举一反三，**所有元素的识别处理都可以用蜂鸣器来验证**。”

### 12.8 顺带确认的其它元素（供交叉验证）

- **坡道**：“如果只依靠电感是不太可行的”——“由于电磁线是铺设在坡道上面的，当小车靠近坡道时，**电感值会明显增大**，但是当检测到电感值增大时，车前瞻离坡道已经很近了”→ **必须用 TOF**。
- **双 TOF 区分坡道/障碍物**：“**两个 TOF 模块按照上下来安装**，如果是坡道，那么上下两个 TOF 模块测得的距离会有一定的差值，如果是障碍物的话，差值会很小”。
- **障碍物绕行 = 陀螺仪目标角 + 编码器积分**：检测到后设 `Target_angle`（假设 45°）传给舵机方向环，同时编码器计数，到阈值（位置 2）再设回 0°；随后通过**电感 ADC 值**判断是否回到赛道（位置 3），检测到赛道就切回电磁循迹。
- **车库**：“车库前的斑马线地下会粘贴三个磁铁，电磁组需要自制**干簧管或者霍尔检测**来检测地上的磁铁（**推荐使用霍尔检测**）”。

### 12.9 来源

| # | URL | 取用内容 | 状态 |
|---|---|---|---|
| 1 | <https://blog.csdn.net/m0_69153234/article/details/132631989> | 5 电感布局、环岛阈值代码、编码器积分、岔路阈值代码、次数标志、路障、十字不处理、蜂鸣器验证法 | 正文完整取到 |
| 2 | <https://blog.csdn.net/weixin_51303932/article/details/134793651> | RingFlag 5 状态、斜电感 3/5 判左右环、A/B/C/P 调参、双 TOF 判坡道、障碍物绕行 | 正文完整取到 |
| 3 | <https://blog.csdn.net/baidu_38682444/article/details/125547317> | 差比和 / 差比和差公式、A/B/C 调参规律 | 正文完整取到 |
| 4 | <https://blog.csdn.net/weixin_29171087/article/details/159567406> | 状态机优势、状态转移图片段、“V 电感突变 + 横电感对称”判环岛 | 付费墙截断，仅取到已展示部分 |
| 5 | <https://blog.csdn.net/zhuoqingjoking97298/article/details/104135586> | 卓晴《电磁车环岛算法》 | **抓取失败（CSDN 521）→ 未确认** |
| 6 | <https://blog.csdn.net/zxy19872029175/article/details/126686224> | 《智能车电磁组——岔路》 | **抓取失败 → 未确认** |
| 7 | <https://blog.csdn.net/zxy19872029175/article/details/126482761> | 《智能车电磁组元素——圆环》 | **抓取失败 → 未确认** |
| 8 | <https://blog.csdn.net/weixin_34150830/article/details/94072187> | 一字/八字/斜置电感布局对比 | **抓取失败 → 未确认** |

### 12.10 本节“未确认”事项汇总

1. 卓晴《电磁车环岛算法》的全部内容（CSDN 反爬，三个镜像均失败）。
2. 《智能车电磁组——岔路》《元素——圆环》正文。
3. **电磁组十字路口“补线 / 打死转向”的具体代码形式**（两个电磁工程都明确说十字**不处理**；“补线”是摄像头组语境）。
4. **以“连续 N 个周期偏差很小”作为元素进入判据的真实代码**。
5. 环岛判据里 `leftV`/`rightV` 的定义式。
6. 陀螺仪积分量 `tly_jifen>>10` 与 ±150/±200 阈值对应的物理单位。
7. 环岛 A/B/C/P 的具体数值。
8. `Forkchu`/`Detection` 的完整状态转移。

---

## 13. 调试手段：虚拟示波器串口协议与 OLED 显示参数

> 本节小节的协议部分**不是从博客转述的**，而是直接读取了**逐飞科技针对 STC32G144K246 本芯片的官方开源库源码**
> （`libraries/zf_components/seekfree_assistant/seekfree_assistant.c` / `.h`，库版本 **V3.2.5 / 2026-06-27**），
> 因此帧格式可以视为**权威**。二手描述（网上常见的“0xAA 0x55 帧头 + 校验”）在 §13.4 做了辨析。

### 13.0 ⚠️ 关键前提：逐飞 `seekfree_assistant` 有**多代互不兼容**的帧格式，必须与上位机版本配套

本次核查确认**至少三代**协议，**不能混用**：

| 代 | 所在发行包 | 帧头 | 通道上限 | float 字节序 | 配哪个上位机 |
|---|---|---|---|---|---|
| **V1**（**本芯片工程实际使用**） | `libraries/zf_components/seekfree_assistant/`（**无 `_v2` 后缀**） | 4 字节：`0xAA,ch\|0x10,sum,len` | **8** | **大端**（需逐字节反转） | 逐飞助手 **V1 版** |
| **V2** | `libraries/zf_components/seekfree_assistant_v2/`（另一套发行包） | 8 字节：`0xAA,sum,0x10,ch,0,0,0,0` | **16** | **小端**（直接发） | 逐飞助手 **V2** |
| **更早（其它平台）** | CH32V307/QD4C 的 `zf_device_virtual_oscilloscope.c` | **无 magic 字头**，10 字节定长 | 4 | int16 小端 | —（附 CRC16/MODBUS） |

> **操作建议**：**以你工程里实际存在的目录名决定**——如果是 `zf_components/seekfree_assistant/`（无 `_v2`）就用 §13.2（下文，**V1，权威**）；如果是 `zf_components/seekfree_assistant_v2/` 就用 V2 格式并配逐飞助手 V2。**先用串口助手抓一帧看字节再决定上位机，不要凭记忆。**
>
> ⚠️ 本次核实的是**STC32G144K246 芯片官方库**（`Seekfree_STC32G144K_100Pin_Opensource_Library`，库版本 V3.2.5 / 2026-06-27），其中 `zf_components/` 下**只有 `seekfree_assistant/`，没有 `seekfree_assistant_v2/`**。V2 格式来自**另一个发行包**（`gitee.com/seekfree/seekfree_assistant` 的 V2 例程），**不代表本芯片官方库**。

### 13.1 逐飞助手（Seekfree Assistant）上位机

逐飞科技的上位机软件 **逐飞助手** 集成了三类功能：**串口助手 + 虚拟示波器 + 摄像头图像显示**；STM32/CH32/STC32 各平台的逐飞库都是同一套上位机协议（同一代协议内互相兼容）。

在 STC32G144K246 官方库里，协议实现在：
- `libraries/zf_components/seekfree_assistant/seekfree_assistant.c` / `.h`（协议与帧封包/解包）
- `seekfree_assistant_interface.c` / `.h`（传输接口：USB-CDC / 串口 / 无线串口 / SPI-WIFI 等绑定）

对外常用 API（原文原型，✅ 已核实）：

```c
extern seekfree_assistant_oscilloscope_struct seekfree_assistant_oscilloscope_data;   /* 发送缓冲 */
extern float seekfree_assistant_parameter[SEEKFREE_ASSISTANT_SET_PARAMETR_COUNT];     /* 接收到的参数 */
extern vuint8 seekfree_assistant_parameter_update_flag[SEEKFREE_ASSISTANT_SET_PARAMETR_COUNT];

void seekfree_assistant_oscilloscope_send  (seekfree_assistant_oscilloscope_struct *seekfree_assistant_oscilloscope);
void seekfree_assistant_data_analysis      (void);          /* 解析上位机下发的参数（放周期任务里） */
void seekfree_assistant_init               (void);
```

关键宏（原文）：

| 宏 | 值 | 含义 |
|---|---|---|
| `SEEKFREE_ASSISTANT_SEND_HEAD` | `0xAA` | **单片机 → 上位机** 帧头 |
| `SEEKFREE_ASSISTANT_RECEIVE_HEAD` | `0x55` | **上位机 → 单片机** 帧头 |
| `SEEKFREE_ASSISTANT_CAMERA_OSCILLOSCOPE` | `0x10` | 示波器功能码（放 `channel_num` 高 4 位） |
| `SEEKFREE_ASSISTANT_RECEIVE_SET_PARAMETER` | `0x20` | 上位机下发参数功能码 |
| `SEEKFREE_ASSISTANT_SET_OSCILLOSCOPE_COUNT` | `0x08` | **虚拟示波器最多 8 通道** |
| `SEEKFREE_ASSISTANT_SET_PARAMETR_COUNT` | `0x08` | 在线调参最多 8 个参数 |
| `SEEKFREE_ASSISTANT_CAMERA_FUNCTION` / `..._DOT_FUNCTION` | `0x02` / `0x03` | 摄像头图像 / 边线点功能码 |
| `SEEKFREE_ASSISTANT_BUFFER_SIZE` | `0x80` | 接收 FIFO 大小（128 字节） |

### 13.2 虚拟示波器数据帧（单片机 → 上位机）✅ 权威

结构体定义（原文顺序）：

```c
typedef struct
{
    uint8 head;         /* 帧头 = 0xAA */
    uint8 channel_num;  /* 低4位 = 通道数；高4位 = 功能位(或上 0x10) */
    uint8 check_sum;    /* 8 位累加校验 */
    uint8 length;       /* 整包长度 = 4 + 4*n */
    float dat[8];       /* 通道数据，最多 8 个 */
} seekfree_assistant_oscilloscope_struct;
```

**字节布局（n = 通道数）**：

| 偏移 | 长度 | 内容 |
|---|---|---|
| 0 | 1 | `0xAA` |
| 1 | 1 | `0x10 \| n`（n = 1~8） |
| 2 | 1 | `check_sum` |
| 3 | 1 | `length` = `4 + 4*n` |
| 4 | 4*n | n 个 IEEE754 单精度浮点，**大端（MSB 先发）** |

- **包长公式**（源码原文）：`packet_size = sizeof(struct) - (8 - channel_num) * 4`，即 **`4 + 4×n` 字节**。
  - 4 通道 → 20 字节；8 通道 → 36 字节。
- **校验和**：`check_sum = 8位累加和(整包字节)`，**累加时先把 `check_sum` 字段置 0**。
  → 等价说法：整包所有字节（含 `check_sum` 自身）累加后低 8 位 `== 0`。
- **⚠️ 字节序是大端，这是最容易踩的坑**：C251 是小端，所以逐飞源码里**显式做了逐字节反转**后再发送：

```c
/* 源码原文（seekfree_assistant_oscilloscope_send 内） */
temp_oscilloscope.channel_num = seekfree_assistant_oscilloscope->channel_num & 0x0f;
/* 由于大小端不匹配，所以需要进行调换 */
for(i = 0; i < temp_oscilloscope.channel_num; i++)
{
    ((uint8 *)&temp_oscilloscope.dat[i])[0] = ((uint8 *)&seekfree_assistant_oscilloscope->dat[i])[3];
    ((uint8 *)&temp_oscilloscope.dat[i])[1] = ((uint8 *)&seekfree_assistant_oscilloscope->dat[i])[2];
    ((uint8 *)&temp_oscilloscope.dat[i])[2] = ((uint8 *)&seekfree_assistant_oscilloscope->dat[i])[1];
    ((uint8 *)&temp_oscilloscope.dat[i])[3] = ((uint8 *)&seekfree_assistant_oscilloscope->dat[i])[0];
}
temp_oscilloscope.head         = SEEKFREE_ASSISTANT_SEND_HEAD;
packet_size                    = sizeof(temp_oscilloscope) - (SEEKFREE_ASSISTANT_SET_OSCILLOSCOPE_COUNT - temp_oscilloscope.channel_num) * 4;
temp_oscilloscope.length       = packet_size;
temp_oscilloscope.channel_num |= SEEKFREE_ASSISTANT_CAMERA_OSCILLOSCOPE;
temp_oscilloscope.check_sum    = 0;
temp_oscilloscope.check_sum    = seekfree_assistant_sum((uint8 *)&temp_oscilloscope, packet_size);
seekfree_assistant_transfer_callback((const uint8 *)&temp_oscilloscope, packet_size);
```

> **自己写上位机/解析器时的要点**：帧头 0xAA；第 2 字节低 4 位是通道数；第 4 字节（偏移 3）就是总长度，可以直接用它切包，不必靠固定长度；浮点按大端解析。

**发送用法**（官方示例，来自逐飞 + 卓晴《STC32G144K 逐飞开源库发布》一文的例程）：

```c
/* 写入需要发送的数据，有几个通道就写几个 */
seekfree_assistant_oscilloscope_data.dat[0] += 0.1f;
seekfree_assistant_oscilloscope_data.dat[1] += 0.5f;
seekfree_assistant_oscilloscope_data.dat[2] += 1;
seekfree_assistant_oscilloscope_data.dat[3] += 2;
seekfree_assistant_oscilloscope_data.channel_num = 4;      /* 本次发几个通道，最大 8 */

seekfree_assistant_oscilloscope_send(&seekfree_assistant_oscilloscope_data);
seekfree_assistant_data_analysis();                        /* 解析上位机下发的参数 */
system_delay_ms(20);
```

官方注释明确要求：**`seekfree_assistant_data_analysis()` 的实际使用中推荐放到周期中断等位置，需要确保函数能够及时被调用，调用周期不超过 20ms。**

### 13.3 在线调参帧（上位机 → 单片机）✅ 权威

结构体定义（**注意字段顺序**，原文）：

```c
typedef struct
{
    uint8 head;       /* 帧头 = 0x55 (SEEKFREE_ASSISTANT_RECEIVE_HEAD) */
    uint8 function;   /* 功能码 = 0x20 (SEEKFREE_ASSISTANT_RECEIVE_SET_PARAMETER) */
    uint8 channel;    /* 参数通道号，1 起始 */
    uint8 check_sum;  /* 8 位累加校验 */
    float dat;        /* 参数值，大端 */
} seekfree_assistant_parameter_struct;
```

- 总长固定 `sizeof = 8` 字节（`uint16 struct_len = sizeof(seekfree_assistant_parameter_struct);`）。
- 校验和算法同上（`check_sum` 置 0 后累加，与原 `check_sum` 比较）。
- **接收端同样要做字节序反转**：源码把 `dat` 的 `[0]↔[3]`、`[1]↔[2]` 搬进 `seekfree_assistant_parameter[channel-1]`，并置 `seekfree_assistant_parameter_update_flag[channel-1] = 1`。
- 解析器在**找不到帧头 `0x55` 时只丢弃 1 字节再重新找**（滑动搜索），所以数据流里混入杂字节不会卡死；协议用 `fifo` 缓冲，解析时关中断保护（`EA` 保存/恢复）。
- 参数下发通道数上限 `SEEKFREE_ASSISTANT_SET_PARAMETR_COUNT = 8`。

**典型在线调参用法**：

```c
if (seekfree_assistant_parameter_update_flag[0])
{
    seekfree_assistant_parameter_update_flag[0] = 0;
    g_param.kp = seekfree_assistant_parameter[0];   /* 上位机滑条直接改 Kp */
}
if (seekfree_assistant_parameter_update_flag[1])
{
    seekfree_assistant_parameter_update_flag[1] = 0;
    g_param.kd = seekfree_assistant_parameter[1];   /* 上位机滑条直接改 Kd */
}
```

### 13.4 关于“0xAA 0x55 帧头 + 校验”这类网上说法的辨析

网上大量博客把自定义上位机协议描述成“**0xAA 0x55 帧头 + 数据 + 校验和**”。就逐飞这套协议而言，这种说法是**把两个方向的帧头混在一起**了：

| 字节 | 实际角色 |
|---|---|
| `0xAA` | **单片机 → 上位机** 的帧头（`SEEKFREE_ASSISTANT_SEND_HEAD`） |
| `0x55` | **上位机 → 单片机** 的帧头（`SEEKFREE_ASSISTANT_RECEIVE_HEAD`） |

**两者不会出现在同一帧里**。如果你自己写上位机，按 §13.2 / §13.3 的布局解析即可。

如果你想**自己定一套更简单的协议**（只要波形够看），下面的 8 字节/通道方案在任何串口助手里都能用，也便于 STC32G 上用 `union` 拆字节：

```c
/* 自定义简易上位机协议：0xAA 0x55 + 通道数 + 数据 + 校验和 */
typedef union
{
    float    f;
    uint8    b[4];
} FloatByte_t;

void Vofa_Send(float *ch, uint8 n)
{
    uint8  i, k;
    uint8  sum = 0;
    FloatByte_t t;

    UART_SendByte(0xAA);
    UART_SendByte(0x55);
    UART_SendByte(n);
    sum = (uint8)(0xAA + 0x55 + n);

    for (i = 0; i < n; i++)
    {
        t.f = ch[i];
        for (k = 0; k < 4; k++)          /* 小端发送，PC 端按小端解析 */
        {
            UART_SendByte(t.b[k]);
            sum += t.b[k];
        }
    }
    UART_SendByte(sum);                   /* 累加校验 */
}
```

> 浮点拆字节用 `union`（不要用指针强制类型转换，C251 下容易踩对齐/优化坑）；`float` 在 C251 上是 4 字节 IEEE754。

### 13.5 OLED 显示哪些参数最有用

> 下面这份清单是**工程建议**（依据电磁循迹调试的实际需要归纳），不是某一份来源里的原文；标注了推荐程度。

| 优先级 | 显示项 | 为什么有用 |
|---|---|---|
| ★★★ | **各电感归一化后的值**（逐通道，0~100） | 一眼看出电感有没有装歪/增益不一致/某个通道没信号；是最直接的硬件体检 |
| ★★★ | **偏差 `error`（归一化后，如 ±100）** | 判断符号是否搞反（车往反方向打角）、量程是否够、死区是否合理 |
| ★★★ | **舵机输出（PWM 占空比或打角）** | 与 `error` 对照，验证 PD 参数与限幅是否合理 |
| ★★☆ | **目标速度 / 实际速度** | 验证速度环跟随、入弯减速是否生效 |
| ★★☆ | **状态机状态**（`NORMAL / CROSS / ROUND_L / ROUND_R / FORK`） | 元素误判排查的必备信息，比看波形快得多 |
| ★★☆ | **电池电压（可用 ADC 内部 1.19V 基准反推）** | 排除“跑一会儿就不行”的电源问题 |
| ★☆☆ | **Kp / Kd / 目标速度当前值** | 配合在线调参，确认参数确实下发成功 |
| ★☆☆ | **总电感量 / 竖直电感值** | 元素识别阈值的现场标定 |
| ★☆☆ | **编码器计数 / 环岛已行进距离** | 环岛“踩点”进出判断（部分队伍用编码器积分辅助） |

OLED 刷新建议：**只刷新变化的部分**或每 100~200ms 整屏刷新一次，**不要在控制周期（5~10ms）里整屏刷 I2C/SPI OLED**，否则会明显挤占控制时间。调试期用“多页菜单 + 按键翻页”，比一屏塞满更实用。

### 13.6 STC32G144K246 上调 ADS/串口调试的注意事项（来自官方库）

- **USB-CDC 可直接当调试串口**：逐飞 STC32G144K246 库提供 `usb_cdc_write_buffer()`，官方 flash 例程就是用它把数据打出来的（波特率概念不存在，比 UART 方便）。
- 若用普通串口：`zf_common_debug.h` 的 `DEBUG_UART_BAUDRATE` 默认 **115200**，串口助手必须一致，否则乱码。
- STC32G144K246 有 **8 组 USART**，且每个串口可以独立绑定定时器做波特率发生器（老 STC32G12K 是固定用定时器 2，多串口必须同波特率）——这让你可以「一路串口给上位机波形 / 一路串口给无线模块」互不干扰。
- ADC 采样建议用**多次均值滤波**：官方库提供 `adc_mean_filter_convert(channel, 10)`（10 次均值），电磁信号上很实用。
  ⚠️ 官方例程还提示：**同一个 ADC 模块的各通道共用最后一次设置的精度**（例程里最后一个 8bit 初始化会让同一模块全部变 8bit），所以**所有电感通道必须用同一个精度初始化**（建议统一 `ADC_12BIT`）。

### 13.7 来源

- **逐飞官方协议源码（本节 13.1~13.3 全部帧格式、校验、字节序、字段顺序、上限 8 通道、20ms 调用周期的唯一依据）**：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_components/seekfree_assistant/seekfree_assistant.c>
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_components/seekfree_assistant/seekfree_assistant.h>
- 逐飞官方 flash/ADC/USB-CDC 例程（`usb_cdc_write_buffer`、`adc_mean_filter_convert`、同模块精度共用提示、115200 波特率）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Example/Coreboard_Demo/E08_flash_demo/user/main.c>
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Example/Coreboard_Demo/E04_adc_demo/user/main.c>
- 库版本（V3.2.5 / 2026-06-27）：
  <https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/doc/version.txt>
- 逐飞开源库发布说明（8 组 USART 各自独立波特率、USB 下载、上位机虚拟示波器最大 8 通道、SPI-WIFI 图传）：
  <https://blog.csdn.net/zhuoqingjoking97298/article/details/156055422>
  仓库：<https://gitee.com/seekfree/STC32G144K246_100Pin_Library>
- 逐飞助手（串口助手 + 虚拟示波器 + 摄像头图像显示）：
  <https://download.csdn.net/download/qq_41701956/90347323>

---

# 全文「未确认」事项汇总

> 本报告的主要价值之一是**把“查不到的”和“查到的”分开**。下表汇总所有标注为未确认的事项，供后续补查或自行实测。

## 第一部分（IAP / EEPROM）

| # | 未确认事项 | 说明 |
|---|---|---|
| 1 | `IAP_CONTR` bit0 的确切定义 | 逐飞驱动用 `(IAP_CONTR & 0x01)` 当“命令状态”读，但数据手册中该位定义本次未确认。**不建议依赖它**，用固定延时更稳妥。 |
| 2 | `IAP_DATA1/IAP_DATA2/IAP_DATA3` 在 STC32G144K246 上的确切名称与地址 | 英文手册出现 `IAP_DATA3`（32 位数据通路），但字节读写只用 `IAP_DATA` 即可，本报告代码不使用它们。 |
| 3 | STC32G144K246 是否支持 `IAP_CMD = 0x07`（块擦除） | STC32G12K128 手册出现“CMD7：擦除 EEPROM 块”，但另有文章称“仅支持 CMD0~CMD3”。**矛盾，未确认**。本报告只用 0x03 页擦除。 |
| 4 | 论坛帖《STC32G144K246_def.h 中的 IAP_CMD 宏定义与手册不符》的正文 | 论坛对抓取返回 403，**未能读到**。处置方式：代码里直接写 1/2/3，不依赖 `_def.h` 的宏名。 |
| 5 | **IAP 访问 EEPROM 用的是 0 基址还是 24 位绝对地址** | 逐飞官方例程用 0 基址（可运行），STC32F12K54 手册也说“目标地址仍从 0000h 开始”；但 CSDN 笔记称“IAP 操作 EEPROM 要使用存储器绝对地址”，另有文章称起始固定 `0xFE0000`。**互相矛盾，未确认**。本报告把基址做成宏，并给出“先读验证、勿先擦 0 地址”的安全流程。 |
| 6 | 官方数据手册中 IAP 地址章节的原文 | PDF 无法直接抓取（`application/pdf` 不被支持），只能靠搜索引擎片段与二手来源推断。 |
| 7 | 逐飞《必看 STC-ISP 设置.png》截图内的具体数值 | 图片二进制受沙箱网络限制无法完整下载（443KB，抓取被截断），**截图数值未确认**。已用文字来源给出等价的三条约束。 |
| 8 | STC32G144K246 程序 >64K 时的 Keil ROM/far 具体配置步骤 | 只确认了“`Code Rom Size` 默认 Huge”和“xdata 超 64K 要用 far 关键字”，**具体配置步骤未确认**。 |

## 第二部分（算法）

| # | 未确认事项 | 说明 |
|---|---|---|
| 9 | **“水平 4 + 竖直 2”“水平 5”“水平 3 + 竖直 2”这三个术语** | 未在任何抓取来源中原样出现。已确认的等价形态是“3 横 + 2 竖（五电感）”“2 横 + 2 竖（四电感）”“3 水平 + 2 竖直 + 2 内八（七电感）”。 |
| 10 | **20 cm 赛道宽度下的水平电感间距经验公式/对照表** | 未找到。只确认了具体位置数值（±5 / ±10 cm）与“相邻 ≥2 cm”。 |
| 11 | **前瞻距离的具体 cm 数值** | 未找到作为推荐值出现的数字，只有定性结论（“前瞻高度稍微降低”“调整前瞻长度”）。 |
| 12 | **6.8 mH 作为电磁组电感量** | 未找到任何来源。6.8 在来源中一律是**谐振电容 6.8 nF**，怀疑是混淆。 |
| 13 | **“按键校准”作为归一化惯例** | 所有来源只谈“跑前/上电标定”和“运行中动态更新”，**无来源讨论按键校准**。 |
| 14 | **加权求和法的权重 {−2,−1,0,1,2}** | 未找到该确切权重集。已确证的是 {1,50,99}（3 路）与 {0.2,0.8,1,1,1,1,0.8,0.2}（8 路）。 |
| 15 | **“±50”作为归一化差比和的典型范围** | 未找到该说法。±50 出现在加权求和（权重 1/50/99）的语境。可确证的是**差比和 ×100 → ±100**。 |
| 16 | **用 `atan`/`atan2` 做偏差非线性修正** | 未在任何可信电磁组来源中出现。可信路线是开方比值法 / 差比和差 / 分段拟合+分段 Kp。 |
| 17 | **“圆形变换”的具体公式** | 只读到摘要，公式未确认。 |
| 18 | **“电感值 → 横向偏移”Flash 查表（每 mm 一点）** | 只出现在疑似 AI 生成且数据自相矛盾的文档中，**不建议采信**。 |
| 19 | **偏差归一化到 ±100 时 Kp/Kd 的“通用典型值”** | ⚠️ 所有抓到的数字都带各自偏差量纲（像素域 / 0~100 差比和域 / 舵机误差域），**不可直接互抄**。**最接近可直接用的起点是龙邱 21 届手册的 PD `{0.65, 1.88}`（输出也是 ±100 量纲）**；“存在通用典型值”这一说法本身不成立。 |
| 20 | **电磁组方向环积分饱和的专门文献论述** | 未找到专门论述。已确认的间接证据：厂商手册明文“位置式 PID 一般用不到积分项”；队伍实测“试过 PID 最后用单 PD”；HIT 报告承认**无 I 会导致入直道不准/超调，但选择动态 PD**。 |
| 20b | **不完全微分、Kd 项独立低通滤波** | ❌ 未确认：在已抓取的 5 份电磁组源码/报告中**均未发现**。只有原则性建议“电感车通常需要更强的低通滤波”，以及可操作的 **D 项限幅法**（`D_MAX = 225*Kd` 反推、目标值一阶低通 α=0.3~0.7、斜率限制）。 |
| 20c | **误差死区作为电磁组惯例** | ❌ 未确认（仅一篇博客出现 `if(abs(error)<20) error_angle=0;`，逐字读过的多份源码/报告中均无）。优先用**斜率限制与输出限幅**。 |
| 21 | ~~`target = base*(1 − k·\|error\|)` 公式与系数 k~~ → **现已确认（HIT 18 届）** | ✅ **`SetSpeed = (124 − 3*error) * bas_speed`**（error 为像素域中线偏差），等效 `base*(1 − 3*error/124)`，**k ≈ 3/124 ≈ 0.0242/像素；按满量程归一化则 k ≈ 3**。另有 `base − k*\|打角\|` 形式（21 届 STC32G 工程，但 `TURN_SLOW_GAIN` 宏体未给出）。⚠️ **“归一化 ±100 下的 k 值”仍未确认**。 |
| 22 | ~~速度环 Kp/Ki 典型数值~~ → **现已取得真实数字，但不可互抄** | ✅ 已确认：21 届 **STC32G144K246** 工程增量式 `Kp=3.2 / Ki≈2.78 / Kd=0.1`（限幅 ±9500、5 ms、PWM 17 kHz/满度 10000、`Kff1=1.0` 前馈）；龙邱手册增量式 `{9, 3.5, 0}`（“KP 可加到 KI 的 2~3 倍”）；三轮工程位置式 `{150, 0, 50}`。⚠️ **位置式与增量式参数不可互换**；“±100 归一化下的典型值”仍未确认。 |
| 23 | ~~直立车 / 三轮车电磁组的差速方案~~ → **现已确认方案，参数仍缺** | ✅ 三轮车（19 届起）用**差速转向**（`motor_L = base − bias_speed; motor_R = base + bias_speed`），机械局限是“单路 PWM 只能到 0 不能为负”→ 最小转弯半径受限。✅ 直立车是**直立环 + 速度环 + 方向环**，采样率 **直立环 500 Hz~1 kHz、速度环 50 Hz~100 Hz**，**必须先整定直立环**。⚠️ **直立车的具体速度环 PI 与直立环 PD 数值 ❌ 未确认**。 |
| 24 | ~~电流限幅、加速度限幅~~ → **加速度限幅已确认，电流限幅值仍缺** | ✅ 加速度限幅（“步长平滑器”）：**`STEP_ACC=20` / `STEP_DEC=40` 每 5 ms**（减速步长为加速 2 倍，“减速优先”），另有 `COOL_DOWN_FRAMES=200` 加速冷却与起步半速保护。✅ 占空比限幅：PID 输出 **±9500**、PWM 满度 **10000**。⚠️ **电流环的具体限幅值/采样电阻/放大倍数 ❌ 未确认**（仅 HIT 一届提到“电流内环+速度外环”）。 |
| 25 | **电磁组十字路口“补线 / 打死转向”的代码形式** | 未找到（两个电磁工程都明确说十字**不处理**；“补线”是摄像头组语境）。 |
| 26 | **以“连续 N 个周期偏差很小”作为元素进入判据的真实代码** | 未找到。来源中的时序判据一律是**编码器积分位移 / 陀螺仪积分角度 / 定时计数**。 |
| 27 | 环岛判据里 `leftV`/`rightV` 的定义式；陀螺仪积分阈值 ±150/±200 的物理单位；A/B/C/P 的具体数值 | 均未确认。 |
| 28 | 卓晴《电磁车环岛算法》《智能车电磁组——岔路 / 圆环》等文的正文 | CSDN 反爬（521），镜像亦失败。 |
| 29 | **逐飞助手 V2 协议是否为本芯片官方库所用** | 已确认：本芯片官方库 `zf_components/` 下**只有 `seekfree_assistant/`（V1，大端）**，没有 `_v2`。V2（小端、16 通道）来自**另一个发行包**（逐飞助手上位机仓库的 V2 例程）。**用时必须与上位机版本配套。** |
| 30 | `0xAA 0xBB` 帧头的自定义协议实例 | 未找到。建议直接用逐飞 `0xAA` + 8 位累加和，与逐飞助手兼容。 |
| 31 | OLED 显示**电池电压**的实现代码 | 未找到（逐飞包里有 ADC 驱动，但没有现成的电压显示代码）。 |
| 32 | 逐飞 STC32G144K246 包内是否存在名为 OLED 的显示驱动 | 该包只见 `ips114/ips200/ips200pro/tft180`，**OLED 驱动未确认**（可能是自备驱动或另一版库）。 |

---

## 给本项目的三条落地建议（基于以上全部核查）

1. **EEPROM 参数存储照抄 §5 的代码即可开跑**，但上电第一件事是**先读、不要先擦**，用来确认“IAP 基址是 0 还是绝对地址”这个唯一的存疑点；同时**关闭 STC-ISP 的“下载时擦除用户 EEPROM”**，否则参数每次烧录都会丢。
2. **传感器与算法按可确证的路线走**：电感用 **10 mH 工字 + 6.8 nF**（谐振 19.3~20 kHz）、高度 **10~15 cm**、左右对称、相邻 **≥2 cm**；偏差用**先归一化 0~100 再差比和 ×100（±100）**；非线性修正用**开方比值法或差比和差**，**不要用 atan**；元素识别用**双阈值窗口 + 多标志位互斥 + 编码器/陀螺仪积分门限**，并用**蜂鸣器在车验证**。
3. **参数从“可确证的起点”开始，然后实车标定**：
   - **舵机方向环**：位置式 PD，±100 口径起点 **Kp ≈ 0.65 / Kd ≈ 1.88**（龙邱 21 届手册）；对 D 项单独限幅（`D_MAX = 225*Kd` 反推法）；有陀螺仪时叠加 `−Gyro_Z*0.005`（并级，龙邱）或按 **1.3~1.4 倍**做前馈（21 届 STC32G144K246 工程）。
   - **速度环**：**增量式 PID，5 ms 周期**，起点 **Kp ≈ 3.2 / Ki ≈ 2.8 / Kd ≈ 0.1**（21 届 STC32G144K246 工程，PWM 17 kHz、满度 10000、输出限幅 ±9500），并加**目标速度前馈 `Kff1=1.0`**；或龙邱手册的 `{9, 3.5, 0}`。
   - **目标速度**：按元素分段（基准 200 时：环岛/十字 −30、普通弯 −40、坡道 +20、直道提速），加速度限幅 `STEP_ACC=20 / STEP_DEC=40`（每 5 ms），加**加速冷却帧**与**陀螺仪确认车身稳定才给全速**。
   - ⚠️ **以上数字来自不同队伍/不同车模，只能当起点**；尤其注意**位置式与增量式的参数含义不同、不可互换**（龙邱手册明文提醒）。

