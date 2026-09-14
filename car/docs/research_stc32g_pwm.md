# STC32G144K246 增强型 PWM（PWMA/PWMB）寄存器级核查报告

> 目标：电磁循迹小车 —— **2 路电机 PWM（BTN7971/DRV8701，各 1 路 PWM + 1 路方向 GPIO）+ 1 路 50Hz 舵机 PWM**
> 核查方式：STC 官方/半官方资料 + STC32G144K246 专用开源库头文件与驱动源码交叉比对。
> 凡是本轮**没有拿到一手证据**的条目，均显式标注 **【未确认】**，不做推测性断言。

---

## 0. 先说一个关键前提（会影响你所有代码）

`STC32G.H` 与 `stc32g144k246.h` **不是同一个外设集合**。

| 头文件 | 适用芯片 | 增强型 PWM 组数 | 说明 |
|---|---|---|---|
| `STC32G.H`（STC-ISP 生成） | STC32G12K128 / 8K64 等 | **2 组：PWMA、PWMB**（各 4 通道，共 8 路） | 老手册只有 PWMA/PWMB |
| `stc32g144k246.h`（STC-ISP / 官方库配套） | **STC32G144K246** | **6 组：PWMA、PWMB、PWMC、PWMD、PWME、PWMF**（45 路 PWM） | 组名与地址已从头部文件实测确认 |

本报告同时给出两者：**寄存器“拼写”完全一致（`PWMA_ARRH` 这种带下划线的写法）**，
STC32G144K246 多出 C/D/E/F 四组，以及每组的 5/6 通道扩展寄存器（`PWMx_PS2` / `PWMx_ENO2` / `PWMx_CCER3`）。

> **结论（回答你的疑问“是 `PWMA_ARRH` 还是 `PWMARRH`”）**：
> - **`PWMA_ARRH` 是对的**，带下划线。已在两份头文件里实测到该拼写：
>   - STC32G144K246：`#define PWMA_ARRH (*(unsigned char volatile far *)0x7efed2)`
>   - STC8H：`#define PWMA_ARRH (*(unsigned char volatile xdata *)0xfed2)`（另有 `PWMA_ARR` 整字访问宏）
> - **`PWMARRH` 是错误的拼写**：那是 **STC15/STC8A 老系列的 `PWM`（PCA/增强型 PWM）模块**命名风格
>   （老系列还有 `CMOD / CCAPM0 / CL / CH / CCAP0H` 这类），**不是 STC32G 的增强型 PWM**。
> - **STC8H 与 STC32G 的 `PWMA_` 寄存器名是一致的**（同偏移：`+0x10 PSCRH / +0x12 ARRH / +0x15 CCR1H`），
>   只是 **STC8H 额外提供 `PWM1_xxx` 数字后缀别名**（`PWM1_CR1` = `PWMA_CR1`，同一地址 `0xfec0`）。
>   **STC32G 没有 `PWM1_CR1` 这种别名**，统一用 `PWMA_` / `PWMB_` / … / `PWMF_`。
> - 地址空间不同：STC8H 的 PWM 寄存器在 **xdata 0xFEC0 段**，STC32G 的在 **XFR 0x7EFEC0 段（far 指针）**，
>   且 **STC32G 访问前必须 `EAXFR = 1`**。

---

## 1. 寄存器清单与位域

### 1.1 实测 SFR / XFR 地址表（STC32G144K246，来源：seekfree 库 `stc32g144k246.h`）

```c
/* ---- PWMA（组 A）: 0x7EFEC0 ~ 0x7EFEDF ---- */
#define PWMA_CR1     (*(unsigned char volatile far *)0x7efec0)
#define PWMA_CR2     (*(unsigned char volatile far *)0x7efec1)
#define PWMA_SMCR    (*(unsigned char volatile far *)0x7efec2)
#define PWMA_ETR     (*(unsigned char volatile far *)0x7efec3)
#define PWMA_IER     (*(unsigned char volatile far *)0x7efec4)
#define PWMA_SR1     (*(unsigned char volatile far *)0x7efec5)
#define PWMA_SR2     (*(unsigned char volatile far *)0x7efec6)
#define PWMA_EGR     (*(unsigned char volatile far *)0x7efec7)
#define PWMA_CCMR1   (*(unsigned char volatile far *)0x7efec8)
#define PWMA_CCMR2   (*(unsigned char volatile far *)0x7efec9)
#define PWMA_CCMR3   (*(unsigned char volatile far *)0x7efeca)
#define PWMA_CCMR4   (*(unsigned char volatile far *)0x7efecb)
#define PWMA_CCER1   (*(unsigned char volatile far *)0x7efecc)
#define PWMA_CCER2   (*(unsigned char volatile far *)0x7efecd)
#define PWMA_CNTRH   (*(unsigned char volatile far *)0x7efece)
#define PWMA_CNTRL   (*(unsigned char volatile far *)0x7efecf)
#define PWMA_PSCRH   (*(unsigned char volatile far *)0x7efed0)
#define PWMA_PSCRL   (*(unsigned char volatile far *)0x7efed1)
#define PWMA_ARRH    (*(unsigned char volatile far *)0x7efed2)
#define PWMA_ARRL    (*(unsigned char volatile far *)0x7efed3)
#define PWMA_RCR     (*(unsigned char volatile far *)0x7efed4)
#define PWMA_CCR1H   (*(unsigned char volatile far *)0x7efed5)
#define PWMA_CCR1L   (*(unsigned char volatile far *)0x7efed6)
#define PWMA_CCR2H   (*(unsigned char volatile far *)0x7efed7)
#define PWMA_CCR2L   (*(unsigned char volatile far *)0x7efed8)
#define PWMA_CCR3H   (*(unsigned char volatile far *)0x7efed9)
#define PWMA_CCR3L   (*(unsigned char volatile far *)0x7efeda)
#define PWMA_CCR4H   (*(unsigned char volatile far *)0x7efedb)
#define PWMA_CCR4L   (*(unsigned char volatile far *)0x7efedc)
#define PWMA_BKR     (*(unsigned char volatile far *)0x7efedd)
#define PWMA_DTR     (*(unsigned char volatile far *)0x7efede)
#define PWMA_OISR    (*(unsigned char volatile far *)0x7efedf)

/* ---- PWMB（组 B）: 0x7EFEE0 ~ 0x7EFEFF，结构与 PWMA 相同 ---- */
#define PWMB_CR1     (*(unsigned char volatile far *)0x7efee0)
/* ... CR2/SMCR/ETR/IER/SR1/SR2/EGR/CCMR1-4/CCER1-2/CNTRH/L
       PSCRH/L/ARRH/L/RCR 与 PWMA 完全同构 ... */
#define PWMB_CCR5H   (*(unsigned char volatile far *)0x7efef5)
#define PWMB_CCR5L   (*(unsigned char volatile far *)0x7efef6)
#define PWMB_CCR6H   (*(unsigned char volatile far *)0x7efef7)
#define PWMB_CCR6L   (*(unsigned char volatile far *)0x7efef8)
#define PWMB_CCR7H   (*(unsigned char volatile far *)0x7efef9)
#define PWMB_CCR7L   (*(unsigned char volatile far *)0x7efefa)
#define PWMB_CCR8H   (*(unsigned char volatile far *)0x7efefb)
#define PWMB_CCR8L   (*(unsigned char volatile far *)0x7efefc)
#define PWMB_BKR     (*(unsigned char volatile far *)0x7efefd)
#define PWMB_DTR     (*(unsigned char volatile far *)0x7efefe)
#define PWMB_OISR    (*(unsigned char volatile far *)0x7efeff)

/* ---- 输出选择 / 使能（XFR 区，SYS 段）---- */
#define PWMA_ETRPS   (*(unsigned char volatile far *)0x7efeb0)
#define PWMA_ENO     (*(unsigned char volatile far *)0x7efeb1)   /* PWM1P..PWM4N 输出使能 */
#define PWMA_PS      (*(unsigned char volatile far *)0x7efeb2)   /* 通道1~4 引脚组选择 */
#define PWMA_IOAUX   (*(unsigned char volatile far *)0x7efeb3)
#define PWMB_ETRPS   (*(unsigned char volatile far *)0x7efeb4)
#define PWMB_ENO     (*(unsigned char volatile far *)0x7efeb5)
#define PWMB_PS      (*(unsigned char volatile far *)0x7efeb6)
#define PWMB_IOAUX   (*(unsigned char volatile far *)0x7efeb7)
#define PWMA_PS2     (*(unsigned char volatile far *)0x7efeb8)   /* 通道5/6 引脚组选择 */
#define PWMA_RCRH    (*(unsigned char volatile far *)0x7efeb9)
#define PWMB_RCRH    (*(unsigned char volatile far *)0x7efeba)

/* ---- 时钟分频（XFR 区）---- */
#define PWMA_CLKDIV  (*(unsigned char volatile far *)0x7efe91)
#define PWMB_CLKDIV  (*(unsigned char volatile far *)0x7efe92)
#define PWMC_CLKDIV  (*(unsigned char volatile far *)0x7efe96)
#define PWMD_CLKDIV  (*(unsigned char volatile far *)0x7efe97)
#define PWME_CLKDIV  (*(unsigned char volatile far *)0x7ef68b)
#define PWMF_CLKDIV  (*(unsigned char volatile far *)0x7ef68f)

/* ---- PWMA 的 5/6 通道扩展（0x7EF93x，STC32G12K128 没有这一段）---- */
#define PWMA_ENO2    (*(unsigned char volatile far *)0x7ef930)
#define PWMA_CR3     (*(unsigned char volatile far *)0x7ef932)
#define PWMA_SR3     (*(unsigned char volatile far *)0x7ef933)
#define PWMA_CCER3   (*(unsigned char volatile far *)0x7ef934)
#define PWMA_CCMR5   (*(unsigned char volatile far *)0x7ef93c)
#define PWMA_CCR5H   (*(unsigned char volatile far *)0x7ef940)
#define PWMA_CCR5L   (*(unsigned char volatile far *)0x7ef941)
#define PWMA_CCR6H   (*(unsigned char volatile far *)0x7ef943)
#define PWMA_CCR6L   (*(unsigned char volatile far *)0x7ef944)
#define PWMA_DER     (*(unsigned char volatile far *)0x7ef948)
#define PWMA_DBA     (*(unsigned char volatile far *)0x7ef949)
#define PWMA_DBL     (*(unsigned char volatile far *)0x7ef94a)
#define PWMA_DMACR   (*(unsigned char volatile far *)0x7ef94b)

/* ---- PWMC / PWMD（0x7EF8B0 ~ 0x7EF8FF）：与 PWMA/PWMB 完全同构 ---- */
#define PWMC_ENO     (*(unsigned char volatile far *)0x7ef8b1)
#define PWMC_PS      (*(unsigned char volatile far *)0x7ef8b2)
#define PWMD_ENO     (*(unsigned char volatile far *)0x7ef8b5)
#define PWMD_PS      (*(unsigned char volatile far *)0x7ef8b6)
#define PWMC_PS2     (*(unsigned char volatile far *)0x7ef8b8)
#define PWMC_CR1     (*(unsigned char volatile far *)0x7ef8c0)
#define PWMC_CR2     (*(unsigned char volatile far *)0x7ef8c1)
#define PWMC_SMCR    (*(unsigned char volatile far *)0x7ef8c2)
#define PWMC_EGR     (*(unsigned char volatile far *)0x7ef8c7)
#define PWMC_CCMR1   (*(unsigned char volatile far *)0x7ef8c8)   /* ~CCMR4: 0x7ef8c9/ca/cb */
#define PWMC_CCER1   (*(unsigned char volatile far *)0x7ef8cc)   /* CCER2: 0x7ef8cd */
#define PWMC_PSCRH   (*(unsigned char volatile far *)0x7ef8d0)   /* PSCRL: 0x7ef8d1 */
#define PWMC_ARRH    (*(unsigned char volatile far *)0x7ef8d2)   /* ARRL : 0x7ef8d3 */
#define PWMC_CCR1H   (*(unsigned char volatile far *)0x7ef8d5)   /* CCR1L~CCR4L: d6~dc */
#define PWMC_BKR     (*(unsigned char volatile far *)0x7ef8dd)
#define PWMC_DTR     (*(unsigned char volatile far *)0x7ef8de)
#define PWMD_CR1     (*(unsigned char volatile far *)0x7ef8e0)
#define PWMD_CR2     (*(unsigned char volatile far *)0x7ef8e1)
#define PWMD_SMCR    (*(unsigned char volatile far *)0x7ef8e2)
#define PWMD_EGR     (*(unsigned char volatile far *)0x7ef8e7)
#define PWMD_CCMR1   (*(unsigned char volatile far *)0x7ef8e8)
#define PWMD_CCER1   (*(unsigned char volatile far *)0x7ef8ec)
#define PWMD_PSCRH   (*(unsigned char volatile far *)0x7ef8f0)
#define PWMD_ARRH    (*(unsigned char volatile far *)0x7ef8f2)
#define PWMD_CCR5H   (*(unsigned char volatile far *)0x7ef8f5)   /* PWMD 只有 5~8 通道 */
#define PWMD_CCR6H   (*(unsigned char volatile far *)0x7ef8f7)
#define PWMD_CCR7H   (*(unsigned char volatile far *)0x7ef8f9)
#define PWMD_CCR8H   (*(unsigned char volatile far *)0x7ef8fb)
#define PWMD_BKR     (*(unsigned char volatile far *)0x7ef8fd)
#define PWMD_DTR     (*(unsigned char volatile far *)0x7ef8fe)

/* ---- PWME / PWMF（0x7EF6B0 ~ 0x7EF6FF）：同样与 PWMA/PWMB 同构 ---- */
#define PWME_ENO     (*(unsigned char volatile far *)0x7ef6b1)
#define PWME_PS      (*(unsigned char volatile far *)0x7ef6b2)
#define PWMF_ENO     (*(unsigned char volatile far *)0x7ef6b5)
#define PWMF_PS      (*(unsigned char volatile far *)0x7ef6b6)
#define PWME_PS2     (*(unsigned char volatile far *)0x7ef6b8)
#define PWME_CR1     (*(unsigned char volatile far *)0x7ef6c0)
#define PWME_CR2     (*(unsigned char volatile far *)0x7ef6c1)
#define PWME_SMCR    (*(unsigned char volatile far *)0x7ef6c2)
#define PWME_EGR     (*(unsigned char volatile far *)0x7ef6c7)
#define PWME_CCMR1   (*(unsigned char volatile far *)0x7ef6c8)   /* ~CCMR4: c9/ca/cb */
#define PWME_CCER1   (*(unsigned char volatile far *)0x7ef6cc)   /* CCER2: 0x7ef6cd */
#define PWME_PSCRH   (*(unsigned char volatile far *)0x7ef6d0)   /* PSCRL: d1 */
#define PWME_ARRH    (*(unsigned char volatile far *)0x7ef6d2)   /* ARRL : d3 */
#define PWME_CCR1H   (*(unsigned char volatile far *)0x7ef6d5)   /* CCR1L~CCR4L: d6~dc */
#define PWME_BKR     (*(unsigned char volatile far *)0x7ef6dd)
#define PWME_DTR     (*(unsigned char volatile far *)0x7ef6de)
#define PWMF_CR1     (*(unsigned char volatile far *)0x7ef6e0)
#define PWMF_CR2     (*(unsigned char volatile far *)0x7ef6e1)
#define PWMF_SMCR    (*(unsigned char volatile far *)0x7ef6e2)
#define PWMF_EGR     (*(unsigned char volatile far *)0x7ef6e7)
#define PWMF_CCMR1   (*(unsigned char volatile far *)0x7ef6e8)
#define PWMF_CCER1   (*(unsigned char volatile far *)0x7ef6ec)
#define PWMF_PSCRH   (*(unsigned char volatile far *)0x7ef6f0)
#define PWMF_ARRH    (*(unsigned char volatile far *)0x7ef6f2)
#define PWMF_CCR5H   (*(unsigned char volatile far *)0x7ef6f5)   /* PWMF 只有 5~8 通道 */
#define PWMF_CCR6H   (*(unsigned char volatile far *)0x7ef6f7)
#define PWMF_CCR7H   (*(unsigned char volatile far *)0x7ef6f9)
#define PWMF_CCR8H   (*(unsigned char volatile far *)0x7ef6fb)
#define PWMF_BKR     (*(unsigned char volatile far *)0x7ef6fd)
#define PWMF_DTR     (*(unsigned char volatile far *)0x7ef6fe)
```

**六组 PWM 的地址规律（已全部实测确认）：**

| 组 | ENO/PS 段 | 时基段 | 通道结构 | 备注 |
|---|---|---|---|---|
| PWMA | `0x7EFEB0~B8` | `0x7EFEC0~DF` | 1~4 带 P/N 互补 + 5/6 扩展（`0x7EF93x`） | 有 `PWMA_PS2` |
| PWMB | `0x7EFEB4~BA` | `0x7EFEE0~FF` | 只有 5~8 单端 | |
| PWMC | `0x7EF8B0~B9` | `0x7EF8C0~DF` | 1~4 带 P/N + 5/6 扩展（`0x7EF95x`） | 有 `PWMC_PS2`，含 `HSPWMC_*` |
| PWMD | `0x7EF8B4~BA` | `0x7EF8E0~FF` | 只有 5~8 单端 | 含 `HSPWMD_*` |
| PWME | `0x7EF6B0~B9` | `0x7EF6C0~DF` | 1~4 带 P/N + 5/6（`PWME_PS2`/`PWME_ENO2`） | |
| PWMF | `0x7EF6B4~BA` | `0x7EF6E0~FF` | 只有 5~8 单端 | |

> **规律**：每组基础段 **32 字节（0xC0~0xDF 或 0xE0~0xFF）**，寄存器偏移完全一致：
> `+0 CR1, +1 CR2, +2 SMCR, +3 ETR, +4 IER, +5 SR1, +6 SR2, +7 EGR,
>  +8~+B CCMR1~4, +C CCER1, +D CCER2, +E CNTRH, +F CNTRL,
>  +10 PSCRH, +11 PSCRL, +12 ARRH, +13 ARRL, +14 RCR,
>  +15 CCR1H, +16 CCR1L, +17 CCR2H, +18 CCR2L, +19 CCR3H, +1A CCR3L, +1B CCR4H, +1C CCR4L,
>  +1D BKR, +1E DTR, +1F OISR`
> 同理 A 组（`CR1=0x7EFEC0` → 偏移与上表一致）；B/D/F 组的 `+15~+1C` 对应 CCR5H~CCR8L。

> 证据 URL：
> - seekfree 官方库头文件：https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_common/stc32g144k246.h
> - 科宇科技 STC32G144K246 开源库（寄存器名 + 驱动交叉验证）：https://gitee.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary

---

### 1.2 `PWMA_CCMR1/2/3/4`（捕获/比较模式寄存器）—— 已确认

**写 CCMRx 之前必须先清零对应的 CCxE**（通道关闭时才可写 CCMR / CCxS）。

输出（PWM）模式下各位域：

| 位域 | 名称 | 说明 |
|---|---|---|
| OCxCE | 输出比较 x 清零使能 | 0：OCxREF 不受 ETRF 影响；1：检测到 ETRF 高电平则 OCxREF=0 |
| OCxM[2:0] | 输出比较 x 模式 | **110 = PWM 模式 1**（CNT < CCRx 时 OCxREF 输出高，否则低）；011 = 输出比较翻转 |
| OCxPE | 输出比较 x 预装载使能 | 0：CCRx 立即生效；1：CCRx 在更新事件（UEV）时载入影子寄存器 |
| OCxFE | 输出比较 x 快速使能 | 一般填 0 |
| CCxS[1:0] | 捕获/比较 x 选择 | **00 = 输出（PWM/比较）方向**；其它值 = 输入捕获，并选择 TI 输入脚 |

- `CCxS` **仅在通道关闭时（`PWMA_CCER1` 的 CCxE=0）可写**。
- STC 例程里常见值：`PWMA_CCMR1 = 0x60;` → OC1CE=0、OC1M=110(PWM 模式1)、OC1PE=0、OC1FE=0、CC1S=00。
- 科宇库用的是 **`0x68`**（PWM 模式 1 **+ OCxPE=1 预装载使能**）。
  → `0x68` 更稳妥：CCR 改动在更新事件才生效，不会在周期中间产生毛刺。

**推荐值：`PWMA_CCMRn = 0x68`（PWM 模式 1 + 预装载）**

> 证据：EEPW 转载 STC32G 手册《捕获/比较模式寄存器 PWMxCCMRn》
> https://forum.eepw.com.cn/forum/thread/threadid/388733/
> 该文明确写出：`PWMB_CCMR1 = 0x60` → `OC5M=110 : PWM模式1……CC5S=00 : 输出方向`。

---

### 1.3 `PWMA_CCER1 / CCER2`（捕获/比较使能寄存器）—— 已确认

`PWMA_CCER1` 管通道 1、2；`PWMA_CCER2` 管通道 3、4（PWM5/6 在 `PWMA_CCER3`）。

| 位 | 名称 | 说明 |
|---|---|---|
| CCxE | OCx 输入捕获/比较输出使能 | 0=关闭；1=开启 |
| CCxP | OCx 极性 | **输出模式下：0=高电平有效，1=低电平有效** |
| CCxNE | OCxN 比较输出使能 | 互补通道 N 端输出使能 |
| CCxNP | OCxN 极性 | 0=高有效，1=低有效 |

位分配（`CCER1`）：
`B0=CC1E, B1=CC1P, B2=CC1NE, B3=CC1NP, B4=CC2E, B5=CC2P, B6=CC2NE, B7=CC2NP`，
`CCER2` 同理对应 CC3/CC4。

- **只输出 P 端（单端）**：通道 1 → `CCER1 |= 0x01`；通道 2 → `CCER1 |= 0x10`；通道 3 → `CCER2 |= 0x01`；通道 4 → `CCER2 |= 0x10`。
- **极性取反（低有效）**：再或上 `0x02` / `0x20`。
- 注意：**写 CCMRx 前必须先 `PWMA_CCER1 = 0x00; PWMA_CCER2 = 0x00;` 关通道。**
- 对于有互补输出的通道，`CCxE/CCxP/CCxNE/CCxNP` 是**预装载位**；若 `PWMA_CR2` 的 `CCPC=1`，只有 COM 事件发生才从预装载位取新值。
- `LOCK` 级别（`PWMA_BKR` 的 LOCK 位）≥2 后，`CCxP` 不可改写。

> 证据：EEPW 转载手册《捕获/比较使能寄存器 PWMxCCERn》
> https://forum.eepw.com.cn/forum/thread/threadid/388731/
> 该文含实例：`PWMB_CCER1 = 0x33;` → 开启通道输出 + 低电平有效。

---

### 1.4 `PWMA_CR1 / CR2 / SMCR / EGR / IER / SR1 / SR2 / ISR` —— 部分确认

| 寄存器 | 已确认的关键位 |
|---|---|
| `PWMA_CR1` | `CEN`(B0) 计数器使能；`ARPE`(B7) 自动重装载预装载使能；`UDIS`(B1) 禁止更新事件 UEV；`CMS[1:0]` 对齐模式（00=边沿对齐；01/10/11=中央对齐）；`OPM` 单脉冲；`DIR` 计数方向 |
| `PWMA_CR2` | `CCPC`(B0) 预装载控制位：=1 时 CCxE/CCxP/OCxM 等需 COM 事件才更新 |
| `PWMA_EGR` | `UG`(B0) 软件产生更新事件：置 1 后 ARR/PSCR 预装载值立即载入影子寄存器并复位计数器 |
| `PWMA_SMCR` | 从模式选择 `SMS[2:0]`，如 `SMS=110` 为触发模式 |
| `PWMA_IER/SR1/SR2` | 中断使能/状态：更新、触发、输入捕获、输出比较、刹车 |
| `PWMA_OISR` | 空闲/关闭状态时各通道输出电平设定（**不是** `PWMA_ISR`） |

> **`PWMA_ISR` 不存在**：已实测确认 STC32G144K246 头文件里**没有 `PWMA_ISR`**，
> 只有 `PWMA_OISR`（`0x7EFEDF`，各组都有对应的 `PWMx_OISR`）。
> PWM 中断请读 **`PWMA_SR1 / PWMA_SR2 / PWMA_SR3`**，中断向量号库实现取 **26(PWMA) / 27(PWMB)**。

> 证据：
> - `ARPE/UDIS/UG`：EEPW《PWMA 的时基单元》 https://forum.eepw.com.cn/forum/thread/threadid/388537/
>   （原文：“自动预装载已使能（PWMA_CR1 寄存器的 ARPE 位为 1）……软件置位了 PWMA_EGR 寄存器的 UG 位”）
> - `CMS` 取值：STC32G 手册片段（搜索命中）：“只有在计数器向下计数时（CMS=01）……向上和向下计数时（CMS=11）”
> - `SMS=110`：STC32G 英文版数据手册（mikrocontroller.net 镜像）
> - `CCPC`：同 §1.3 来源

---

### 1.5 `PWMA_ENO` / `PWMA_ENO2`（输出使能）—— 已确认

`PWMA_ENO` 按 **P/N 交替** 排布，每通道 2 位：

| 位 | 位名 | 位 | 位名 |
|---|---|---|---|
| B0 | ENO1P（选择 PWM1P 输出） | B1 | ENO1N |
| B2 | ENO2P | B3 | ENO2N |
| B4 | ENO3P | B5 | ENO3N |
| B6 | ENO4P | B7 | ENO4N |

`PWMA_ENO2` 管通道 5、6：`B0=ENO5P, B1=ENO5N, B2=ENO6P, B3=ENO6N`。

> 所以：
> - 只输出 PWM1P → `PWMA_ENO = 0x01`
> - PWM1P + PWM2P → `PWMA_ENO = 0x05`
> - PWM1P + PWM1N（互补） → `PWMA_ENO = 0x03`
> - PWM3P → `PWMA_ENO = 0x10`
> - PWM5P（STC32G144K246 组 E 的通道 5）→ `PWME_ENO2 |= 0x01`（注意：是 **组 E** 的独立通道，不是 PWMA 的通道 5）

> 证据：STC 官方手册 PWMA 寄存器表片段（搜索命中原文）
> `| PWMA | ENO1P | 选择PWM1P输出 | ENO1N ... |`
> https://www.stcaimcu.com/data/download/Datasheet/STC32G.pdf （#291#198）
> 交叉验证（位运算实现）：科宇库 `ky_pwm.c` 的 `_pwm_set_eno()`：
> ```c
> if (group == PWM_MODE_A || group == PWM_MODE_C || group == PWM_MODE_E) {
>     idx = channel_id - 1;
>     if (is_n) mask = (1 << (idx * 2 + 1));
>     else      mask = (1 << (idx * 2));
> }
> ```

---

### 1.6 `PWMA_BKR`（刹车）—— 部分确认

| 位 | 名称 | 说明 |
|---|---|---|
| MOE | 主输出使能 | **必须置 1，PWM 才会真正输出到引脚**；例程一律 `PWMA_BKR = 0x80;` |
| AOE | 自动输出使能 | 刹车事件后自动恢复输出 |
| BKP | 刹车输入使能 | |
| OSSR | 运行模式下“关闭状态”选择 | |
| OSSI | 空闲模式下“关闭状态”选择 | |
| LOCK[1:0] | 写保护级别 | LOCK≥2 后 CCxP 不可改；LOCK=3 后 OCxM 不可改 |

**AOE/BKP/OSSR/OSSI/LOCK 的具体位序：【未确认】**（本轮未取到 `PWMA_BKR` 的逐位表）。
只确认 **`0x80` 是 MOE 位，且是所有 STC 例程的标准写法**。

> 证据：51hei 实测例程《用STC32G144K246驱动WS2812（PWM方式）》：
> `PWMA_BKR = 0x80;` → 注释/上下文即“使能主输出”
> http://www.51hei.com/bbs/dpj-243275-1.html
> 以及 CSDN《STC32系列单片机PWM 自定义函数》：“PWMA_BKR =0x80; 为使能主输出”
> https://blog.csdn.net/qq_62503791/article/details/141068892

**主输出关闭后引脚输出什么电平 → 【未确认】**（论坛有《PWM的主输出使能关闭后PWM引脚输出什么电平》一帖，但正文 403 无法读取）。
工程上建议：**任何时候先保证 MOE=1，需要“停机”时改 CCR=0 或把 GPIO 拉低，不要靠关 MOE 停机。**

---

### 1.7 `PWMA_DTR`（死区）—— 部分确认

- `PWMA_DTR` 为死区时间发生器寄存器，**只在互补输出时才有意义**。
- STC 例程里常见 `PWMA_DTR = 0x00;`（死区为 0，即互补但不插死区）。
- **DTG 位域的分档计算公式【未确认】**（本轮未取到 DTR 位表原文）。

> 证据：CSDN《STC32系列单片机PWM 自定义函数》：
> “当设置为输出两路互补PWM时可设置 PWMA_DTR 寄存器来配置死区时间，本文是设置为0”
> https://blog.csdn.net/qq_62503791/article/details/141068892

---

## 2. 引脚映射

### 2.1 PS 字段编码规则（已实测确认）

`PWMA_PS` 是**每通道 2 位**的字段，`B1:B0` = 通道 1，`B3:B2` = 通道 2，`B5:B4` = 通道 3，`B7:B6` = 通道 4。
`PWMA_PS2` 管通道 5、6（`B1:B0`=通道 5，`B3:B2`=通道 6）。

字段值含义：
- **0 → 第 1 组引脚（P1 口，默认组）**
- **1 → 第 2 组引脚（P2 口）**
- **2 → 第 3 组引脚（P6 口）**
- **3 → 第 4 组引脚（P3/P4 口，仅通道 4 有）**

验证（`ky_pwm.h` 的枚举编码 = `group<<13 | channel_pol<<9 | ps<<7 | pin`）：

```c
// PWMA 通道1
PWMA_CH1P_P10 = (0<<13 | 0<<9 | 0<<7 | GPIO_P10),   // PS=0 → P1.0
PWMA_CH1N_P11 = (0<<13 | 1<<9 | 0<<7 | GPIO_P11),   // PS=0 → P1.1 (N)
PWMA_CH1P_P20 = (0<<13 | 0<<9 | 1<<7 | GPIO_P20),   // PS=1 → P2.0
PWMA_CH1N_P21 = (0<<13 | 1<<9 | 1<<7 | GPIO_P21),   // PS=1 → P2.1
PWMA_CH1P_P60 = (0<<13 | 0<<9 | 2<<7 | GPIO_P60),   // PS=2 → P6.0
PWMA_CH1N_P61 = (0<<13 | 1<<9 | 2<<7 | GPIO_P61),   // PS=2 → P6.1
// PWMA 通道2
PWMA_CH2P_P12 / PWMA_CH2N_P13 /   // PS=0 → P1.2 / P1.3
PWMA_CH2P_P22 / PWMA_CH2N_P23 /   // PS=1 → P2.2 / P2.3
PWMA_CH2P_P62 / PWMA_CH2N_P63,    // PS=2 → P6.2 / P6.3
// PWMA 通道3
PWMA_CH3P_P14 / PWMA_CH3N_P15 /   // PS=0 → P1.4 / P1.5
PWMA_CH3P_P24 / PWMA_CH3N_P25 /   // PS=1 → P2.4 / P2.5
PWMA_CH3P_P64 / PWMA_CH3N_P65,    // PS=2 → P6.4 / P6.5
// PWMA 通道4
PWMA_CH4P_P16 / PWMA_CH4N_P17 /   // PS=0 → P1.6 / P1.7
PWMA_CH4P_P26 / PWMA_CH4N_P27 /   // PS=1 → P2.6 / P2.7
PWMA_CH4P_P66 / PWMA_CH4N_P67 /   // PS=2 → P6.6 / P6.7
PWMA_CH4P_P34 / PWMA_CH4N_P33,    // PS=3 → P3.4 / P3.3  ← 唯一有第4组脚的通道
```

**PWMB（组 B）不是 P/N 互补结构，只有单端通道 5~8：**

```c
// 通道5
PWMB_CH5_P20  (PS=0), PWMB_CH5_P17  (PS=1), PWMB_CH5_P00 (PS=2), PWMB_CH5_P74 (PS=3)
// 通道6
PWMB_CH6_P21  (PS=0), PWMB_CH6_P54  (PS=1), PWMB_CH6_P01 (PS=2), PWMB_CH6_P75 (PS=3)
// 通道7
PWMB_CH7_P22  (PS=0), PWMB_CH7_P33  (PS=1), PWMB_CH7_P02 (PS=2), PWMB_CH7_P76 (PS=3)
// 通道8
PWMB_CH8_P23  (PS=0), PWMB_CH8_P34  (PS=1), PWMB_CH8_P03 (PS=2), PWMB_CH8_P77 (PS=3)
```

> 证据：科宇科技 STC32G144K246 开源库 `ky_pwm.h`（`PWM_Channel_t` 枚举，2025.12.22 V2.0）
> https://gitee.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary/blob/master/stc32g144k246_100pin_ky_library/library/drivers/ky_pwm.h

### 2.2 哪几路是“互补带死区”的成对通道

**已确认（STC32G12K128 官方文本）**：

- **PWMA 组有 4 对互补对称带死区通道：PWM1P/PWM1N、PWM2P/PWM2N、PWM3P/PWM3N、PWM4P/PWM4N**
  （即通道 1~4，每对 P/N 可独立输出，也可组成互补对称输出并插入可编程死区）
- **PWMB 组只能输出单端 PWM：PWM5、PWM6、PWM7、PWM8，无互补、无死区**
- 官方原文补充：“若单独使能了 PWM1P 输出，则 PWM1N 就不能再独立输出，除非 PWM1P 和 PWM1N 组成一组互补对称输出”
- 捕获功能只能从 **P 端**（PWMxP）输入

> 证据：EEPW《高级PWM的基本介绍》 https://forum.eepw.com.cn/forum/thread/threadid/388532/
> EEPW《高级PWM的功能简介》 https://forum.eepw.com.cn/forum/thread/threadid/388533/

**对 STC32G144K246 的延伸（重要，但需注意证据等级）**：
STC32G144K246 有 6 组 PWM。科宇库源码里 `_pwm_enable_output()` 对 **组 A/C/E 使用 CCER1/CCER2/CCER3 的 P/N 位对**，
对 **组 B/D/F 只有单端通道 5~8**（用 `channel_id - 5` 索引）。这与“A/B 两组”的旧描述一致地推广为
**A、C、E 带互补死区；B、D、F 是单端**。
→ 这一推广**来自驱动库实现，属强旁证但非手册原文，标记为“高度可信，未逐字确认”**。

### 2.3 磁循迹小车的引脚分配建议（基于已确认映射）

| 用途 | 建议通道 | 引脚 | PS 设置 |
|---|---|---|---|
| 电机 1 PWM | PWMA 通道 1（PWM1P） | **P1.0** | `PWMA_PS` 的 B1:B0 = 0 |
| 电机 2 PWM | PWMA 通道 2（PWM2P） | **P1.2** | `PWMA_PS` 的 B3:B2 = 0 |
| 舵机 PWM（50Hz） | **PWMB 通道 5（PWM5）** | **P2.0** | `PWMB_PS` 的 B1:B0 = 0 |
| 电机方向 GPIO ×2 | 任意普通 IO | 如 P0.0 / P0.1 | 推挽输出 |
| （备选）舵机用 PWMA 通道 3 | PWM3P | P1.4 | B5:B4 = 0 |

**为什么舵机不走 PWMA**：见 §4.4 —— 同一组 PWM 共用 ARR，频率必须相同。
20kHz 与 50Hz 差 400 倍，**必须分成两组（PWMA 给电机，PWMB 给舵机）**。

---

## 3. 时钟与频率计算

### 3.1 时钟源

**已确认**：STC32G 的高级 PWM 计数器**时钟源 = 系统时钟（SYSclk）经 `PWMA_PSCRH/PSCRL` 预分频**，
预分频系数为 **1~65535 的任意整数**；两组（PWMA / PWMB）时钟可**分别独立设置**。

官方文本：“第一组 PWM/PWMA 的时钟频率可以是系统时钟经过寄存器 PWMA_PSCRH 和 PWMA_PSCRL 进行分频后的时钟，
分频值可以是 1~65535 之间的任意值。”
> 证据：EEPW《高级PWM的基本介绍》 https://forum.eepw.com.cn/forum/thread/threadid/388532/

**关于“是不是 SYSclk/2”**：
- STC32G 的 PWMA/PWMB 在**系统时钟（SYSclk）**域；`PSCR` 分频值范围 **1~65535**（不是 STM32 的 1~65536），
  这与“PSC 寄存器写 N 表示 N+1 分频”一致。
- **【未确认】**：`PWMA_CLKDIV`（地址 `0x7EFE91`）这个 STC32G144K246 新增寄存器的**分频含义与默认值**，
  本轮未取到手册原文。它可能是“PWM 时钟相对 SYSclk 的额外分频”。
  **建议：上电后先不要动它（保持复位默认值），用示波器实测 PWM 频率，再用 §3.4 的 `PWM_CLK_COMP` 校准。**

### 3.2 频率与占空比公式（已确认）

```
计数时钟  f_CNT = f_SYSclk / (PSCR + 1)          // PSCR = PWMA_PSCRH:PWMA_PSCRL
PWM 频率  f_PWM = f_CNT / (ARR + 1)              // ARR   = PWMA_ARRH:PWMA_ARRL
占空比    D     = CCRx / (ARR + 1)               // CCRx  = PWMA_CCRxH:PWMA_CCRxL
```

- **PWM 模式 1**（`OCxM=110`）：`CNT < CCRx` 时 **OCxREF 输出高**，否则低。
  → **是高电平计数**：CCR 越大，高电平越宽。
- **极性位**：`CCxP=0` 高电平有效（引脚输出 = OCxREF）；`CCxP=1` 低电平有效（引脚输出 = OCxREF 取反）。
  → **对“高电平有效”的驱动（BTN7971/DRV8701 的 PWM 脚）用 `CCxP=0`**，占空比即 `CCR/(ARR+1)`。
- **`ARR=0` 是特例**：此时计数器周期只有 1 个计数，PWM 退化成“每个计数时钟输出一个占空比由 CCR 决定”的
  单脉冲串；正常 PWM 请保证 `ARR ≥ 1`。

> 证据：
> - 公式来源（库函数实现）：科宇库 `ky_pwm.c`
>   ```c
>   psc_arr_val = system_clock_freq / freq;
>   if (psc_arr_val > PWM_MAX_DUTY) { psc = psc_arr_val / PWM_MAX_DUTY - 1; arr = PWM_MAX_DUTY - 1; }
>   else                            { psc = 0;                          arr = psc_arr_val - 1; }
>   /* 占空比：*/ ccr_val = duty * (arr + 1) / PWM_MAX_DUTY;   // duty 范围 0~10000
>   ```
> - PWM 模式 1 定义：EEPW《捕获/比较模式寄存器 PWMxCCMRn》
>   ```c
>   OC5M=110 : PWM模式1。当PWMn_CNT < PWMn_CCR1 时，OCnREF输出高，否则OCnREF输出低
>   ```
>   https://forum.eepw.com.cn/forum/thread/threadid/388733/
> - 极性：EEPW《PWMxCCERn》“CC1P……0：高电平有效；1：低电平有效” https://forum.eepw.com.cn/forum/thread/threadid/388731/

### 3.3 具体寄存器值表

**电机 PWM：目标 20 kHz**

| 主频 | PSCR(值/寄存器) | ARR(值/寄存器) | 实际频率 | 误差 |
|---|---|---|---|---|
| **24 MHz** | 0 → `PSCRH=0x00, PSCRL=0x00` | 1199 → `ARRH=0x04, ARRL=0xAF` | 20016.7 Hz | +0.08% |
| **40 MHz** | 0 → `PSCRH=0x00, PSCRL=0x00` | 1999 → `ARRH=0x07, ARRL=0xCF` | 20000 Hz | 0 |

> 20 kHz 下 24 MHz 只需 1200 个计数、40 MHz 需 2000 个计数，都远小于 65536，
> 所以 **PSC 一律写 0**（分频系数 1，分辨率最高）。

**舵机 PWM：目标 50 Hz**

> 下表 **"PSCR 写值"** 是**真正写进 `PSCRL` 的数值**；分频系数 = PSCR 写值 + 1。

| 主频 | PSCR 写值 | 分频系数 | ARR 写值 | 实际频率 | 1 计数 = |
|---|---|---|---|---|---|
| **24 MHz** | `PSCRH=0x00, PSCRL=0x07` | 8 | `ARRH=0xEA, ARRL=0x5F` (=59999) | 50.000 Hz | 1/3 µs ≈ 0.333 µs |
| **40 MHz**（方案 A，推荐） | `PSCRH=0x00, PSCRL=0x0E` | 15 | `ARRH=0xD0, ARRL=0x55` (=53333) | 50.000 Hz | 0.375 µs |
| **40 MHz**（方案 B，§4 代码 `pwm_calc()` 自动算出） | `PSCRH=0x00, PSCRL=0x0C` | 13 | `ARRH=0xF0, ARRL=0x61` (=61537) | 49.999 Hz | 0.325 µs |

> **24 MHz 方案已与 §4 的 `pwm_calc()` 输出完全一致；40 MHz 用方案 B 时 `pwm_calc()` 会得到 PSC=12/ARR=61537，
> 若要精确 50.000 Hz 请手工写方案 A（PSC=14 / ARR=53333）。**

**舵机脉宽 → CCR 换算（0.5 ms ~ 2.5 ms ↔ 0° ~ 180°）**

| 主频 | 0.5 ms | 1.0 ms | 1.5 ms(中位) | 2.0 ms | 2.5 ms |
|---|---|---|---|---|---|
| 24 MHz (ARR=59999) | 1500 | 3000 | 4500 | 6000 | 7500 |
| 40 MHz 方案A (ARR=53333) | 1333 | 2667 | 4000 | 5333 | 6667 |
| 40 MHz 方案B (ARR=61537) | 1538 | 3077 | 4615 | 6154 | 7692 |

> 数值由 `f_CNT = f_SYSclk/(PSCR+1)`、`f_PWM = f_CNT/(ARR+1)` 直接算出，可自行复核。
> 若实测频率只有期望值的一半，说明本芯片该组的实际 PWM 时钟是 `SYSclk/2`
> （或 `PWMA_CLKDIV` 默认带分频）——此时应把 **`PSCRL` 写值按“分频系数翻倍”调整**
> （例如 24 MHz 舵机从 PSCR=7/分频 8 改成 PSCR=15/分频 16），或直接用 §3.4 的 `PWM_CLK_COMP` 补偿宏。

### 3.4 建议：把频率做成“可一步校准”的写法

因为你最终以**实测**为准（示波器/逻辑分析仪），下面这个宏可以一次性把整机频率对齐：

```c
/* ===== PWM 时钟补偿系数 =====
 * 上电后用示波器测 P1.0 的 PWM 频率：
 *   测得 20kHz 附近 → PWM_CLK_COMP 保持 1
 *   测得 10kHz 附近 → 说明实际 PWM 时钟是 SYSclk/2，改成 2
 *   （或直接改 PSCR/ARR 成比例）                                  */
#define PWM_CLK_COMP   1u
```

---

## 4. 可直接编译的初始化代码

> 以下代码针对 **STC32G144K246 + Keil C251 + STC-ISP 生成的 `STC32G.H` / `stc32g144k246.h`**。
> 两组方案给全：**(a) 2 路 20 kHz 电机 PWM 用 PWMA**，**(b) 1 路 50 Hz 舵机 PWM 用 PWMB**。
> 代码里所有寄存器名都取自 §1.1 实测表。

### 4.0 公共头部 + 主频定义

```c
#include "STC32G.H"          /* 或 stc32g144k246.h，两者 PWM 寄存器拼写一致 */

typedef unsigned char  u8;
typedef unsigned int   u16;
typedef unsigned long  u32;

/* ===== 主频：与 STC-ISP 里下载时选择的 IRC 频率必须一致 ===== */
#define F_CPU_HZ        24000000UL      /* 24MHz；40MHz 改成 40000000UL */

/* ===== PWM 时钟补偿（见 §3.4；默认 1，实测校准后调整） ===== */
#define PWM_CLK_COMP    1u
#define PWM_CLK_HZ      (F_CPU_HZ / PWM_CLK_COMP)

/* ===== 目标频率 ===== */
#define MOT_PWM_FREQ    20000u          /* 电机 20kHz */
#define SRV_PWM_FREQ    50u             /* 舵机 50Hz  */

/* ===== 舵机脉宽（单位 0.1us），按 §3.3 表；改主频时同步改这三个 ===== */
/*  24MHz, PSC=7,  ARR=59999  */
#define SRV_MIN_US      5000u           /* 0.5ms  ->   0 deg */
#define SRV_MID_US      15000u          /* 1.5ms  ->  90 deg */
#define SRV_MAX_US      25000u          /* 2.5ms  -> 180 deg */
/*  40MHz, PSC=14, ARR=53333 时改成 5000 / 15000 / 25000（脉宽不变，CCR 由公式算） */

/* 舵机中位 CCR：由 (PSC, ARR) 与 SRV_MID_US 推得，供初始化时用 */
#define SRV_MID_CCR     ((u16)(((u32)SRV_MID_US * (PWM_CLK_HZ / 10000UL)) \
                               / (1000UL * ((PWM_CLK_HZ / SRV_PWM_FREQ) / 65536UL + 1UL))))

/* ===== 编译期自检：ARR 必须落在 16 位内 ===== */
#if ((PWM_CLK_HZ / MOT_PWM_FREQ) > 65536UL)
#error "电机 PWM 周期超出 16 位，需要 PSC 分频（改用 pwm_calc）"
#endif
```

**编译期展开校验（24 MHz，`PWM_CLK_HZ = 24000000`）：**

| 项 | 展开结果 |
|---|---|
| 电机 `total = PWM_CLK_HZ / 20000` | 1200 ≤ 65536 → `pwm_calc()` 给 **PSC=0, ARR=1199** ✔ |
| 舵机 `total = PWM_CLK_HZ / 50` | 480000 > 65536 → `pwm_calc()` 给 **PSC=7, ARR=59999** ✔ |
| `SRV_MID_CCR` | `15000*2400 / (1000*8) = 4500` ✔（与 §3.3 表一致） |

`pwm_calc()` 统一负责 PSC/ARR 拆分：

```c
/* 把 clk_hz 分频到 freq：优先 PSC=0 拿最高分辨率，装不下再粗分频 */
static void pwm_calc(u32 clk_hz, u16 freq, u16 *psc, u16 *arr)
{
    u32 total = clk_hz / freq;          /* 一个 PWM 周期需要的计数个数 */
    if (total <= 65536UL) {             /* 16 位 ARR 装得下 */
        *psc = 0;
        *arr = (u16)(total - 1);
    } else {
        *psc = (u16)(total / 65536UL);                      /* 先粗分频 */
        *arr = (u16)((clk_hz / ((u32)(*psc) + 1UL) / freq) - 1UL);
    }
}
```

- 24 MHz / 20 kHz → **PSC=0, ARR=1199** ✔（与 §3.3 表一致）
- 40 MHz / 20 kHz → **PSC=0, ARR=1999** ✔
- 24 MHz / 50 Hz  → **PSC=7, ARR=59999** ✔（`PSCRL = 0x07`）
- 40 MHz / 50 Hz  → `total = 800000` → **PSC=12, ARR=61537**（=§3.3 方案 B，49.999 Hz）。
  若要精确 50.000 Hz，请手工写 §3.3 方案 A：`PSCRL=0x0E`（分频 15）、`ARRH:ARRL = 0xD0 0x55`（=53333）。

### 4.1 (a) PWMA 输出 2 路 20 kHz 电机 PWM（PWM1P=P1.0，PWM2P=P1.2）

```c
/* =========================================================================
 *  电机 PWM 初始化：PWMA 通道1(PWM1P -> P1.0) + 通道2(PWM2P -> P1.2)
 *  频率 20kHz，初始占空比 0
 * ========================================================================= */
void MotorPWM_Init(void)
{
    u16 psc, arr;

    EAXFR = 1;              /* 使能访问 XFR(扩展 SFR) —— 访问 PWMA_* 前必须置位! */
    WTST  = 0x00;           /* CPU 执行速度最快 */

    /* ---------- 1. 引脚配置：必须先设成推挽输出 ---------- */
    /* P1.0 / P1.2 推挽输出：M1=0, M0=1 */
    P1M1 &= ~0x05;          /* P1.0、P1.2 的 M1 清 0 */
    P1M0 |=  0x05;          /* P1.0、P1.2 的 M0 置 1  -> 推挽 */

    /* ---------- 2. 配置计数时钟与周期（频率对所有通道共用） ---------- */
    pwm_calc(PWM_CLK_HZ, MOT_PWM_FREQ, &psc, &arr);
    PWMA_PSCRH = (u8)(psc >> 8);
    PWMA_PSCRL = (u8)(psc);
    PWMA_ARRH  = (u8)(arr >> 8);
    PWMA_ARRL  = (u8)(arr);

    /* ---------- 3. 先关通道，再写 CCMRx（手册硬性要求） ---------- */
    PWMA_CCER1 = 0x00;      /* 关通道 1、2 */
    PWMA_CCER2 = 0x00;      /* 关通道 3、4 */

    /* 通道模式：PWM 模式 1 + 预装载使能  →  0x68
     *   0x68 = 0110 1000b
     *     OC1CE=0, OC1M=110(PWM模式1), OC1PE=1(预装载), OC1FE=0, CC1S=00(输出) */
    PWMA_CCMR1 = 0x68;      /* 通道1 */
    PWMA_CCMR2 = 0x68;      /* 通道2 */

    /* 初始比较值 = 0 -> 上电占空比 0%，电机不转（安全） */
    PWMA_CCR1H = 0x00;  PWMA_CCR1L = 0x00;
    PWMA_CCR2H = 0x00;  PWMA_CCR2L = 0x00;

    /* ---------- 4. 使能通道输出（极性：CCxP=0 高电平有效） ---------- */
    /* CCER1: B0=CC1E, B4=CC2E  -> 0x11 */
    PWMA_CCER1 = 0x11;

    /* ---------- 5. 引脚组选择：通道1/2 都用第 1 组（P1 口） ---------- */
    /* PWMA_PS: B1:B0=通道1, B3:B2=通道2；0 = P1 口 */
    PWMA_PS &= ~0x0F;       /* 只改通道1~4 的字段，保留其它组设置 */

    /* ---------- 6. 使能 PWM 输出脚：ENO1P(B0) + ENO2P(B2) ---------- */
    PWMA_ENO = 0x05;

    /* ---------- 7. 主输出使能 MOE=1，然后启动计数器 ---------- */
    PWMA_BKR |= 0x80;       /* MOE = 1，不置位则引脚无输出 */
    PWMA_EGR  = 0x01;       /* UG=1：立即把 PSC/ARR 载入影子寄存器并复位计数器 */
    PWMA_CR1  = 0x09;       /* ARPE=1(自动重装载预装载) + CEN=1(启动计数) */
}

/* =========================================================================
 *  设置电机占空比
 *    ch    : 1 或 2
 *    duty  : 0 ~ 1000  (0 = 0%，1000 = 100%)
 *  说明：单路电机驱动(BTN7971/DRV8701 的 PWM 脚 + 1 路方向 GPIO)，
 *        占空比 0% 就是停车，不需要负占空比；方向交给 GPIO。
 * ========================================================================= */
void MotorPWM_SetDuty(u8 ch, u16 duty)
{
    u16 arr, ccr;

    if (duty > 1000) duty = 1000;

    arr = ((u16)PWMA_ARRH << 8) | PWMA_ARRL;
    ccr = (u16)(((u32)duty * (arr + 1u)) / 1000u);   /* 高电平计数 */

    if (ch == 1) {
        PWMA_CCR1H = (u8)(ccr >> 8);
        PWMA_CCR1L = (u8)ccr;
    } else {
        PWMA_CCR2H = (u8)(ccr >> 8);
        PWMA_CCR2L = (u8)ccr;
    }
}

/* =========================================================================
 *  方向 GPIO（举例：P0.0 / P0.1）
 * ========================================================================= */
void MotorDir_Init(void)
{
    P0M1 &= ~0x03;
    P0M0 |=  0x03;          /* P0.0 / P0.1 推挽输出 */
}

void Motor_SetDir(u8 motor, u8 forward)
{
    if (motor == 1) { if (forward) P00 = 1; else P00 = 0; }
    else            { if (forward) P01 = 1; else P01 = 0; }
}
```

### 4.2 (b) PWMB 输出 1 路 50 Hz 舵机 PWM（PWM5 = P2.0）

```c
/* =========================================================================
 *  舵机 PWM 初始化：PWMB 通道5(PWM5 -> P2.0)，50Hz
 *  0.5ms ~ 2.5ms  ->  0° ~ 180°
 *  注意：PWMB 只有单端输出(PWM5~PWM8)，无互补、无死区，正好适合舵机
 * ========================================================================= */
void ServoPWM_Init(void)
{
    u16 psc, arr;

    EAXFR = 1;

    /* ---------- 1. 引脚：P2.0 推挽输出 ---------- */
    P2M1 &= ~0x01;
    P2M0 |=  0x01;

    /* ---------- 2. 周期：50Hz ---------- */
    pwm_calc(PWM_CLK_HZ, SRV_PWM_FREQ, &psc, &arr);
    PWMB_PSCRH = (u8)(psc >> 8);
    PWMB_PSCRL = (u8)(psc);
    PWMB_ARRH  = (u8)(arr >> 8);
    PWMB_ARRL  = (u8)(arr);

    /* ---------- 3. 关通道 -> 写模式 ---------- */
    PWMB_CCER1 = 0x00;
    PWMB_CCER2 = 0x00;
    PWMB_CCMR1 = 0x68;      /* 通道5（PWMB 的 CCMR1 对应 PWM5）：PWM 模式1 + 预装载 */

    /* 初始 1.5ms = 中位 */
    PWMB_CCR5H = (u8)(SRV_MID_CCR >> 8);
    PWMB_CCR5L = (u8)(SRV_MID_CCR);

    /* ---------- 4. 使能通道5输出 ---------- */
    /* PWMB_CCER1: B0 = CC5E  */
    PWMB_CCER1 = 0x01;

    /* ---------- 5. 引脚组：通道5 用第 1 组 = P2.0 ---------- */
    /* PWMB_PS: B1:B0 = 通道5 的 PS 字段；0 = P2.0 */
    PWMB_PS &= ~0x03;

    /* ---------- 6. 输出使能：PWM5 单端输出 -> ENO5P = B0 ---------- */
    PWMB_ENO = 0x01;

    /* ---------- 7. 主输出 + 启动 ---------- */
    PWMB_BKR |= 0x80;
    PWMB_EGR  = 0x01;       /* UG=1 */
    PWMB_CR1  = 0x09;       /* ARPE=1 + CEN=1 */
}

/* =========================================================================
 *  设置舵机角度
 *    angle : 0 ~ 180 (度)
 *  内部：脉宽 = 0.5ms + angle/180 * 2.0ms
 * ========================================================================= */
void Servo_SetAngle(u16 angle)
{
    u32 arr;
    u32 us;             /* 脉宽，单位 0.1us（避免浮点） */
    u32 ccr;
    u32 psc;

    if (angle > 180) angle = 180;

    arr = ((u32)PWMB_ARRH << 8) | PWMB_ARRL;
    psc = ((u32)PWMB_PSCRH << 8) | PWMB_PSCRL;

    /* 脉宽 = 5000 + angle/180 * 20000，单位 0.1us  ->  500.0us ~ 2500.0us */
    us = SRV_MIN_US + ((u32)angle * (SRV_MAX_US - SRV_MIN_US)) / 180UL;

    /* CCR = 脉宽(0.1us) * f_PWMclk / 1e7 / (PSC+1)
     *     = us * (PWM_CLK_HZ / 10000) / (1000 * (PSC+1))          整数等价式
     * 24MHz/PSC=7 : 15000*2400/(1000*8) = 4500  ✔  (1.5ms 中位)
     * 40MHz/PSC=14: 15000*4000/(1000*15) = 4000 ✔  (1.5ms 中位)          */
    ccr = (us * (PWM_CLK_HZ / 10000UL)) / (1000UL * (psc + 1UL));

    if (ccr > arr) ccr = arr;

    PWMB_CCR5H = (u8)(ccr >> 8);
    PWMB_CCR5L = (u8)ccr;
}
```

**校验 `Servo_SetAngle` 的整数算式（24 MHz，PSC=7，ARR=59999）：**
- 中位：angle=90 → us = 5000 + 90*20000/180 = 5000+10000 = 15000（即 1.5 ms）
  - `ccr = 15000 * (24000000/10000) / (1000 * 8) = 15000*2400/8000 = 4500` ✔（与 §3.3 表一致）
- 0.5 ms：angle=0 → us=5000 → `ccr = 5000*2400/8000 = 1500` ✔
- 2.5 ms：angle=180 → us=25000 → `ccr = 25000*2400/8000 = 7500` ✔
- 与 `SRV_MID_CCR` 宏（编译期常量 4500）结果一致 ✔

> **注意：`Servo_SetAngle` 只对 50 Hz 那一组（PWMB）有效**，
> 因为这个公式里的 `(PSC+1)` 与 `ARR` 已经隐含了 50 Hz 的周期关系。
> 直接写死表值也可以（最简单、零误差）：
> ```c
> /* 24MHz, PWMB: PSC=7, ARR=59999 */
> #define SRV_MIN_CCR   1500u   /* 0.5ms ->   0 deg */
> #define SRV_MID_CCR   4500u   /* 1.5ms ->  90 deg */
> #define SRV_MAX_CCR   7500u   /* 2.5ms -> 180 deg */
> /* 角度 -> CCR 线性插值： */
> ccr = SRV_MIN_CCR + ((u32)angle * (SRV_MAX_CCR - SRV_MIN_CCR)) / 180UL;
> ```
> 40 MHz 方案 A（PSC=14, ARR=53333）时改为 1333 / 4000 / 6667。

### 4.3 “设置占空比”函数的标准写法（回答你问的 8 位高低字节问题）

STC 的 `CCR` / `ARR` / `PSC` 都是 **16 位值拆成两个 8 位 SFR**（`H` 高字节 + `L` 低字节）。

**先写高字节还是先写低字节？**
- **【未确认】**：本轮**没有**在 STC 手册 / 官方例程里找到“必须高字节先行”的原文要求。
  STC 官方例程与社区例程**普遍采用“先写 H，再写 L”**，但这是**惯例写法，不是已确认的硬件约束**。
- **实践结论：请按“先 H 后 L”写**（与所有 STC 例程一致，风险最低）；同时**避免在 PWM 运行中把 H/L 分开写**
  导致中间态——因为 `OCxPE=1` 时 CCR 是双缓冲的，中间态不会输出到波形，但仍建议按固定顺序写。

```c
/* 16 位值写入 高/低字节对，宏写法最不易错（先 H 后 L） */
#define PWM_SET_CCR(H, L, v)   do { (H) = (u8)((v) >> 8); (L) = (u8)(v); } while (0)
#define PWM_GET_CCR(H, L)      ( ((u16)(H) << 8) | (u16)(L) )

/* 例： */
PWMA_CCR1H = (u8)(ccr >> 8);     /* 先高字节 */
PWMA_CCR1L = (u8)ccr;            /* 后低字节 */
/* 读回： */
u16 cur = ((u16)PWMA_CCR1H << 8) | PWMA_CCR1L;
```

**实时改占空比是否安全**：`CCMR` 里 `OCxPE=1`（我们用的 `0x68`）时，CCR 是**双缓冲**的：
写入进预装载寄存器，在**下一个更新事件（UEV，即计数器溢出）**时整体载入影子寄存器。
所以**不会出现半个周期的畸形波**。若用 `0x60`（OCxPE=0），CCR 立即生效，同一周期内可能出现一次异常宽/窄的脉冲——
对电机影响不大，对舵机建议用 `0x68`。
> `OCxPE` 的逐字定义来源：EEPW《PWMxCCMRn》
> “OC5PE：输出比较 5 预装载使能 0：禁止 PWMn_CCR1 寄存器的预装载功能，可随时写入……1：开启预装载功能……
>  PWMn_CCR1 的预装载值在更新事件到来时被加载至当前寄存器中”
> https://forum.eepw.com.cn/forum/thread/threadid/388733/

---

### 4.4 同一路 PWMA 里不同通道能否用不同频率？

**不能。**
- 一组 PWM 只有**一个计数器、一个 `PSCR`、一个 `ARR`**（时基单元共用），
  所有通道的 `CCR` 都跟同一个计数器比较。
  → **`ARR` 相同 ⇒ 同组所有通道频率必然相同**，只能各自不同的占空比/相位。
- 官方时基单元描述：“16 位向上/向下计数器、16 位自动重载寄存器、重复计数器、预分频器”——**每组件只有一套**。
  > 证据：EEPW《PWMA 的时基单元》 https://forum.eepw.com.cn/forum/thread/threadid/388537/

**所以你的分配应该是：**

| 需求 | 方案 |
|---|---|
| 2 路电机 PWM @20 kHz | **PWMA** 通道 1、2（P1.0 / P1.2）—— 同组同频，正好 |
| 1 路舵机 PWM @50 Hz | **PWMB** 通道 5（P2.0）—— 独立的 PSCR/ARR，可设成 50 Hz |
| 若还要第 3 路电机 / 更好分辨率 | STC32G144K246 还有 **PWMC / PWMD / PWME / PWMF** 可用（组 E 的通道 5/6 走 `PWME_PS2`/`PWME_ENO2`） |

**另一条路（不推荐但可行）**：舵机用 **PWMA 的一个通道 + 每 20 ms 软件改一次 `ARR`**——
因为 ARR 是共用的，改它会把电机频率一起改掉，**绝对不能用**。

**也可以**：舵机不用增强 PWM，改用 **PCA/CCP 模块（STC32G 有 3 路 CCP）** 或 **普通定时器中断软件 PWM**。
这两条路 STC32G144K246 都有资源，但本轮**未逐条核实其寄存器细节**，此处只作为备选提示。

---

## 5. 死区 / 刹车

### 5.1 单路驱动（BTN7971 / DRV8701 的 PWM + DIR 接法）需要互补输出吗？

**不需要。**

- BTN7971B / DRV8701 这类**单路 H 桥**，常见的“PWM + DIR”接法：
  - `PWM` 脚接单片机一路 PWM → 控制**有效/快衰减**，
  - `DIR`（或 `IN1/IN2`）脚接**普通 GPIO** → 控制方向。
  - 内部 H 桥的上下管切换由芯片自己处理，**不需要 MCU 提供互补带死区波形**。
- 因此：**只使能 P 端**（`PWMA_ENO = 0x05`，`PWMA_CCER1 = 0x11`，`CCxP=0`），
  **不要使能 N 端，`PWMA_DTR` 保持 0x00 即可**（不配互补时死区寄存器无效）。

### 5.2 什么时候才需要互补 + 死区？

- 你用 **6 个分立 MOSFET 搭三相/独立半桥**，MCU 直接驱动上下管栅极 → 必须互补 + 死区。
- STC32G144K246 上一轮**未确认**是否有“7 组不同周期的 PWM / 45 路 PWM / 144MHz 硬件移相”这些宣传点对应的具体寄存器，
  但 **PWMA/PWMC/PWME 支持 4 对互补对称带死区通道**（组 A 已由官方文本确认；组 C/E 由库实现旁证）。
- 死区配置入口：`PWMA_DTR`（时间值，具体分档公式 **【未确认】**）。

### 5.3 刹车（BKR）

- **`PWMA_BKR` 的 MOE 位必须为 1** 才会有引脚输出。所有 STC 例程都是 `PWMA_BKR = 0x80;`。
- 刹车输入信号是 `PWMFLT`（可来自引脚或比较器输出；`BRKBPS` 位选择），
  触发后把输出置于复位/确定状态。**本小车项目不需要刹车输入**，`BKP` 保持 0 即可。
- **不要用“关 MOE”来做停机**：关掉后引脚电平状态【未确认】；停机请用 `CCR=0`（占空比 0）或直接拉低方向/使能脚。

---

## 6. 常见坑（初始化顺序 / 端口 / EGR）

按下面顺序写，基本不会翻车：

1. **先 `EAXFR = 1;`（或 `P_SW2 |= 0x80;`）**
   `PWMA_*` / `PWMB_*` / `PWMA_ENO` / `PWMA_PS` 等都在 **XFR（扩展 SFR）区**（如 `0x7EFEC0`）。
   **不置 `EAXFR` 就写这些寄存器会写到别处或无效**，这是新手最常踩的坑。
   > 证据：51hei 实测例程 `EAXFR = 1; //扩展寄存器(XFR)访问使能`
   > http://www.51hei.com/bbs/dpj-243275-1.html

2. **端口模式必须先设成推挽**
   `PxM1 = 0; PxM0 = 1;`（对应位）= 推挽输出。
   `PxM1=0, PxM0=0` 是**准双向口**，做 PWM 输出会因上拉能力弱导致波形边沿差/电平不到轨。
   （注意：CSDN 那篇例程写的是“配置IO口 设置为准双向口”——**那是错的做法**，请用推挽。）

3. **`PWMA_ENO` 和 `PWMA_PS` 都要配**，只配一个不出波形：
   - `PS` 决定“信号从哪个引脚出来”；
   - `ENO` 决定“这个通道的输出要不要接到引脚”。
   - 顺序无所谓，但**两个都必须在 `CR1.CEN=1` 之前或之后完成均可**（推荐都在启动前配好）。

4. **必须先关通道，再写 `CCMRx`**
   手册硬性要求：`CCxS` 仅在 `CCxE=0` 时可写。
   标准套路：
   ```c
   PWMA_CCER1 = 0x00;      /* 关通道1、2 */
   PWMA_CCER2 = 0x00;      /* 关通道3、4 */
   PWMA_CCMR1 = 0x68;
   PWMA_CCMR2 = 0x68;
   PWMA_CCER1 = 0x11;      /* 再开通道 */
   ```
   > 证据：EEPW《PWMxCCMRn》“注：CC1S 仅在通道关闭时（PWMA_CCER1 寄存器的 CC1E=0）才是可写的”
   > https://forum.eepw.com.cn/forum/thread/threadid/388733/

5. **`PWMA_EGR` 的 `UG` 位作用**
   写 `PWMA_EGR = 0x01;` 会**软件产生一次更新事件**：
   - 把 `PWMA_PSCR` 的预装载值载入预分频器
   - 把 `PWMA_ARR` 的预装载值载入影子寄存器
   - 复位计数器 `CNTR`
   → **作用：让你刚写的 PSC/ARR 立刻生效，不用等第一个溢出周期。**
   如果 `CR1.ARPE=1`（我们用的 `0x09`）而你没写 `UG`，PSC/ARR 要等到第一次溢出才真正生效——
   表现为“上电第一拍频率不对”。**所以 `UG=1` 建议保留。**
   > 证据：EEPW《PWMA 的时基单元》“更新事件的产生条件：……软件置位了 PWMA_EGR 寄存器的 UG 位”
   > https://forum.eepw.com.cn/forum/thread/threadid/388537/

6. **`PWMA_BKR` 的 MOE 忘记了**
   现象：寄存器全对、`ENO` 也开了，示波器就是没波形。**90% 是漏了 `PWMA_BKR |= 0x80;`**。

7. **`OC1FE` 的写入时机**
   论坛有专门讨论帖《PWM寄存器配置的疑问（OC1FE必须在通道打开时才能写）》，
   说明 `OCxFE` 可能**只能在通道打开时写**。我们的 `0x68` 里 `OCxFE=0`，不涉及，但若你要用快速使能，
   请先去确认这一点（本条 **【未确认】** 细节，仅提示存在这一坑）。
   https://www.stcaimcu.com/thread-9282-1-17.html
8. **`CCR` 与 `ARR` 的边界**
   - `CCR = 0` → 恒低（0%）；`CCR ≥ ARR+1` → 恒高（100%）。
   - 写 `CCR > ARR` 时，PWM 模式 1 下会输出恒高，**不是溢出回绕**，是安全的。
   - 舵机代码里我们做了 `if (ccr > arr) ccr = arr;` 的钳位。

9. **`ARR` 与同组通道的耦合**
   改 `ARR` 会同时改变**同组所有通道**的频率。运行中不要随意改 `ARR`（舵机/电机分属不同组就没问题）。

10. **主频一致性**
    `F_CPU_HZ` 必须与 **STC-ISP 下载时选择的 IRC 频率**一致，否则频率算错。
    更稳的做法：用 STC-ISP 生成的 `STC32G.H` 里的频率宏，或干脆用示波器实测后反推 `PWM_CLK_COMP`。

---

## 7. 未确认清单（明确列出，避免误用）

| 条目 | 状态 |
|---|---|
| PWMC / PWMD / PWME / PWMF 的 `CR1/ARR/CCR1/PS/ENO` 基础寄存器地址 | **✔ 已补齐**（见 §1.1，六组地址全部实测确认） |
| `PWMA_CLKDIV`（0x7EFE91）的具体分频含义与复位默认值 | **【未确认】**（寄存器存在已确认，另有 `PWMB/PWMC/PWMD/PWME/PWMF_CLKDIV`） |
| PWM 时钟到底是 `SYSclk` 还是 `SYSclk/2` | 官方文本（EEPW 转载手册）说“**系统时钟经 PSCR 分频**”，未提 /2；**但未见一手的逐字手册原文，标记为待示波器实测确认**。代码里已提供 `PWM_CLK_COMP` 一键补偿 |
| `PWMA_BKR` 的 AOE/BKP/OSSR/OSSI/LOCK 逐位位置 | **【未确认】**（只确认 B7=MOE=0x80） |
| `PWMA_DTR` 死区分档公式 | **【未确认】** |
| `PWMA_ISR` 是否存在 | **✔ 已确认不存在** —— 只有 `PWMA_OISR`（0x7EFEDF）。中断状态用 `PWMA_SR1/SR2/SR3` |
| 组 C/E 是否与组 A 一样是“4 对互补带死区” | **高度可信（库实现旁证），未逐字确认** |
| STC32G144K246 各封装（LQFP64/100PIN）的具体引脚号表 | **【未确认】**（引脚图是图片，无法取文本） |
| 主输出 MOE 关闭后引脚的实际电平 | **【未确认】** |
| `CCR`/`ARR`/`PSC` 的 H/L 字节**是否必须“先高后低”** | **【未确认】** —— 未找到手册原文要求；所有 STC 例程都用“先 H 后 L”，本文代码也照此写。因 `OCxPE=1` 时 CCR 双缓冲，即使中间态也不会输出到波形 |
| `PWMA_BKR` 的 MOE 在“纯 PWM 输出（非互补/非刹车）”场景下是否也必需 | **✔ 已由成品例程反证必需** —— 51hei 的 STC32G144K246 WS2812 例程（单端 PWM5 输出）同样写 `PWMA_BKR = 0x80`，不置则无输出 |
| PWMA 中断向量号 | 科宇库用 **26(PWMA) / 27(PWMB)**，**未与手册逐字核对** |

---

## 8. 参考来源汇总

1. **seekfree（逐飞）STC32G144K246 官方风格头文件**（寄存器名与地址一手证据）
   https://raw.giteeusercontent.com/seekfree/STC32G144K246_100Pin_Library/raw/master/Seekfree_STC32G144K_100Pin_Opensource_Library/libraries/zf_common/stc32g144k246.h
   - 本地副本（本次核查实际使用，170,674 字节）：
     `C:\Users\y\Desktop\car\docs\_stc_tmp\seekfree_stc32g144k246.h`
1b. **STC8H 头文件**（用于比对 `PWMA_ARRH` 拼写与 `PWM1_xxx` 别名；非 STC32G 使用）
   `C:\Users\y\Desktop\car\docs\_stc_tmp\STC8H_utf8.H`
2. **科宇科技 STC32G144K246 开源库**（6 组 PWM 驱动实现 + 引脚枚举）
   - 仓库：https://gitee.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary
   - `ky_pwm.c`：https://gitee.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary/blob/master/stc32g144k246_100pin_ky_library/library/drivers/ky_pwm.c
   - `ky_pwm.h`（引脚枚举）：https://gitee.com/beijing-keyu---jiangxi/KEYU_AI8052U_OpenLibrary/blob/master/stc32g144k246_100pin_ky_library/library/drivers/ky_pwm.h
3. **EEPW 转载 STC32G 手册 — 高级PWM的基本介绍**（PWMA/PWMB 结构、时钟、互补）
   https://forum.eepw.com.cn/forum/thread/threadid/388532/
4. **EEPW — 高级PWM的功能简介**（P/N 独立使能规则、捕获只能从 P 端）
   https://forum.eepw.com.cn/forum/thread/threadid/388533/
5. **EEPW — PWMA 的主要特性**（16 位上下计数、4 通道、4 路互补死区、刹车 PWMFLT）
   https://forum.eepw.com.cn/forum/thread/threadid/388535/
6. **EEPW — PWMA 的时基单元**（ARPE/UDIS/UG/更新事件、PSCR 载入时机）
   https://forum.eepw.com.cn/forum/thread/threadid/388537/
7. **EEPW — 捕获/比较使能寄存器 PWMxCCERn**（CCxE/CCxP/CCxNE/CCxNP 逐位）
   https://forum.eepw.com.cn/forum/thread/threadid/388731/
8. **EEPW — 捕获/比较模式寄存器 PWMxCCMRn**（OCxM=110 PWM模式1、OCxPE、CCxS）
   https://forum.eepw.com.cn/forum/thread/threadid/388733/
9. **STC 官方数据手册 STC32G144K246.pdf**（本轮 PDF 无法直接解析，仅通过搜索引擎片段引用）
   https://www.stcaimcu.com/data/download/Datasheet/STC32G144K246.pdf
   - 片段一（PWMA_PS 引脚表）：http://www.stcmcudata.com/STC8F-DATASHEET/STC32G.pdf#312#53
   - 片段二（PWMA ENO1P/ENO1N 输出选择表）：https://www.stcaimcu.com/data/download/Datasheet/STC32G.pdf#291#198
10. **51hei — 用STC32G144K246驱动WS2812（PWM方式）**（该芯片实测可运行的最小 PWM 初始化序列）
    http://www.51hei.com/bbs/dpj-243275-1.html
11. **CSDN — STC32系列单片机PWM 自定义函数**（PWMA_PS/PWMA_ENO/PWMA_DTR/PWMA_BKR 用法 + 频率公式）
    https://blog.csdn.net/qq_62503791/article/details/141068892
12. **STC 官方手册片段（CMS 对齐模式）**
    https://www.stcaimcu.com/data/download/Datasheet/STC32G.pdf#291#139
13. **STC 英文版数据手册（mikrocontroller.net 镜像）** — `PWMA_SMCR` SMS=110
    https://www.mikrocontroller.net/attachment/613402/stc32g-en.pdf
14. **STC32G144K246 数据手册（20260618）讨论帖**
    https://www.stcaimcu.com/forum.php?mod=viewthread&tid=24785

---

*报告生成：基于 STC32G144K246 专用头文件 / 官方示例库 / STC 官方及半官方手册转载的交叉核对。凡未取得一手证据的条目均已在 §7 显式标注。*
