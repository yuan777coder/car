# 电磁感应循迹小车（STC32G144K246）

基于 **STC32G144K246** 核心板及其配套主板，采用 C 语言开发的电磁感应循迹小车。

- 电感采集赛道电磁信号 → **RS824** 运放调理 → 主控 **12 位 ADC** 采样
- 归一化 + 加权偏差 → **舵机 PD 转向** + **电机速度环调速**
- 外接 **SSD1306 OLED**（I2C）实时显示 ADC、偏差、舵机 PWM、车速等
- 调试串口支持**文本回传**与**上位机虚拟示波器**，可在线改参并掉电保存

> 编译器为 **Keil uVision + C251**（STC32G 是 251 内核，不是 C51），
> 需使用 STC-ISP 生成的官方头文件 `STC32G.H`。搭建步骤见
> [docs/keil_setup.md](docs/keil_setup.md)。

---

## 目录结构

```
car/
├── README.md                本文件
├── docs/
│   ├── architecture.md      架构与模块设计
│   ├── pinmap.md            引脚分配表（改硬件先看这里）
│   ├── tuning.md            调参指南（归一化/舵机PD/速度环）
│   └── keil_setup.md        Keil C251 工程搭建步骤
├── src/
│   ├── main.c               程序入口
│   ├── common.h             公共类型与宏
│   ├── config.h             算法/功能配置（电感数量、默认参数、功能开关）
│   ├── board_config.h       引脚与外设分配（改引脚只改这里）
│   ├── bsp/                 板级驱动（GPIO/延时/定时器/ADC/PWM/串口/EEPROM）
│   ├── drivers/             器件驱动（软件I2C + SSD1306 + 字库）
│   ├── algo/                算法（滤波/归一化/偏差/舵机PD/电机速度环/元素）
│   └── app/                 应用层（主流程/显示/按键/参数/调试）
└── tools/
    └── scope.py             上位机虚拟示波器
```

**分层原则**

| 层 | 职责 | 依赖 |
|----|------|------|
| `bsp/` | 操作 STC32G 寄存器，向上提供硬件无关接口 | `STC32G.H`、`board_config.h` |
| `drivers/` | 操作具体器件（OLED、字库） | `bsp` 提供的 GPIO/延时 |
| `algo/` | 纯算法，不含任何寄存器访问 | 只有 `common.h`/`config.h` |
| `app/` | 把上面几层串成整车控制流程 | 全部 |

`algo/` 层完全不碰寄存器，因此换芯片时基本不动；换引脚只改 `board_config.h`；
调参数优先用串口在线改（见下文），确认后再 `save` 固化。

---

## 快速上手

1. **搭建工程**：按 [docs/keil_setup.md](docs/keil_setup.md) 新建 Keil 工程，
   加入 `src/` 下全部 `.c`，头文件搜索路径加上 `src`、`src/bsp`、`src/drivers`、
   `src/algo`、`src/app`。
2. **对引脚**：按 [docs/pinmap.md](docs/pinmap.md) 核对 `board_config.h` 与你的
   主板接线是否一致（电感 ADC 通道、电机/舵机 PWM、OLED I2C、按键）。
3. **烧录**：用 STC-ISP 下载，IRC 频率务必与 `board_config.h` 的 `MAIN_FREQ_MHZ`
   一致（默认 24MHz）。
4. **上电**：OLED 显示 `PARAM DEFAULT`，串口打印 `== EM CAR READY ==`。
5. **标定**：把车放赛道上，K3 短按（或串口发 `cal`），沿赛道推车走一圈
   （6 秒），完成自动归一化标定并保存。
6. **起跑**：K1 短按（或串口发 `run`）；K1 再按停止。

---

## 按键

| 按键 | 短按 | 长按 |
|------|------|------|
| K1 | 启动 / 停止 | 恢复出厂参数 |
| K2 | 切换显示页 | — |
| K3 | 开始归一化标定 | — |
| K4 | 保存参数到 EEPROM | — |

## 串口命令（115200 8N1，`\r\n` 结束）

```
kp=700  kd=60  trim=0     舵机 PD / 中值微调
base=400 min=220 max=900  速度规划
pikp=30 piki=4            速度环 PI（仅编码器模式）
cal / run / stop          标定 / 启动 / 停止
page=N                    切换显示页
scope / text              示波器二进制帧 / 文本回传
save / load / default     保存 / 重载 / 恢复默认
?                         打印当前参数
```

改参数后记得 `save`，否则断电丢失。示波器图形界面见 `tools/scope.py`。

---

## 关键设计说明

- **归一化**：每个电感通道记录 `min`（远离导线）与 `max`（正对导线），
  把绝对 ADC 映射到 `0~1000`，消除各通道增益/零点差异。
- **偏差**：默认用归一化加权平均 `Σ(v·w)·GAIN/Σv`（`w = -2,-1,0,1,2`），
  满偏约 ±1000；可选左右差比和 `(R-L)/(R+L)`（`DEV_MODE=1`）。
- **舵机 PD**：位置伺服机构只做 P+D，不做积分（避免直线漂移），
  输出带变化率限制保护齿轮。
- **速度环**：默认开环占空比（`ENCODER_ENABLE=0`），弯道按偏差自动减速；
  接编码器后置 `ENCODER_ENABLE=1` 走增量式 PI 闭环。
- **元素识别**：十字/圆环/三岔默认只识别并显示，不干预控制
  （`ELEMENT_ACTION_ENABLE=0`），判据需现场标定后再打开。

详见 [docs/architecture.md](docs/architecture.md) 与 [docs/tuning.md](docs/tuning.md)。

---

## 已知约定 / 注意事项

- 显示为英文/数字缩写（字库仅 ASCII，中文需 16×16 点阵，未内置）。
- 符号约定：`dev > 0` 表示导线在车右侧、需向右打舵；方向反了改
  `config.h` 的 `SERVO_DIR` 或 `DEV_DIR`。
- OLED 上下/左右镜像由 `board_config.h` 的 `OLED_FLIP_X/Y` 控制。
- 电机接线反了改 `config.h` 的 `MOTOR_REVERSE_L/R`，别去改电机线。
