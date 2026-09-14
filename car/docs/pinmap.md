# 引脚分配表（pinmap）

> 全部引脚集中在 `src/board_config.h`，改接线只改那一处。下表是默认分配。

| 功能 | 引脚 | 说明 |
|------|------|------|
| 电感 ADC ×7 | P0.0~P0.6 | ADC2 的 CH0~CH6；下标 0..4=水平(左→右)，5..6=竖直(左、右) |
| 电池 ADC（可选） | P0.7 | ADC2 CH7，默认 `BAT_ADC_ENABLE=0` |
| 电机L PWM | P1.0 | PWMA 通道1（PWM1P），20kHz |
| 电机L 方向 | P1.1 | 推挽输出 |
| 电机R PWM | P1.2 | PWMA 通道2（PWM2P），20kHz |
| 电机R 方向 | P1.3 | 推挽输出 |
| 舵机 PWM | P2.0 | PWMB 通道5（PWM5），50Hz |
| OLED SCL | P2.4 | 软件 I2C，4.7~10k 上拉 |
| OLED SDA | P2.5 | 同上 |
| UART1 RX / TX | P3.0 / P3.1 | 调试串口 115200 |
| K1 / K2 | P3.2 / P3.3 | 低电平有效，内部上拉 |
| 编码器L/R A相 | P3.4 / P3.5 | T0/T1 计数脚（可选，默认关） |
| K3 / K4 | P3.6 / P3.7 | |
| 编码器L/R B相 | P4.0 / P4.1 | 判方向（可选） |

## 定时器/外设资源分配

| 资源 | 用途 |
|------|------|
| Timer0 | 系统节拍（1ms 中断 → CTRL_PERIOD_MS 控制周期） |
| Timer1 | 编码器左轮计数（仅 `ENCODER_ENABLE=1`） |
| Timer2 | UART1 波特率发生器（1T 模式） |
| Timer3 | 编码器右轮计数（仅 `ENCODER_ENABLE=1`，配置需按手册核对） |
| PWMA | 电机 2 路 PWM（20kHz） |
| PWMB | 舵机 1 路 PWM（50Hz） |
| ADC | 电感 7 通道（查询式） + 可选电池 |

## 主频

> ⚠ **硬件必接**：STC32G 的 ADC 有独立参考电压脚 **ADC_VREF+**，不能悬空
> （否则 ADC 值会漂移/不变）。参考电压接 3.3V（或你的基准源），并在 STC-ISP 里
> 确认参考源选择。

- 默认 `MAIN_FREQ_MHZ = 24`，必须与 STC-ISP 下载时设置的 IRC 频率一致。
- 24MHz 下：电机 PWM `PSC=0, ARR=1199`（≈20kHz）；舵机 `PSC=7, ARR=59999`（50Hz），
  0.5/1.5/2.5ms → CCR `1500/4500/7500`。这些由 `pwm.c` 按主频自动计算，无需手改。
- 上电后用示波器测一次 P1.0 频率：若只有预期一半，说明该芯片 PWM 时钟是 SYSclk/2，
  在 `pwm.c` 的 `pwm_calc` 调用处把 `clk` 除以 2 即可。

## 中断向量号（STC32G）

T0=1、T1=3、T2=12、T3=19、T4=20、UART1=4（经 STC32G144K246.H 实测）。
已集中定义在 `board_config.h`。
