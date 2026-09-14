# 调参指南

> 顺序很重要：先硬件对中 → 归一化标定 → 舵机 PD → 速度 → 元素。
> 每一步都建议开着串口（`tools/scope.py` 或串口助手）看数据。

## 0. 上电前

1. 对照 `docs/pinmap.md` 核对接线。
2. STC-ISP 下载时 IRC 频率 = `board_config.h` 的 `MAIN_FREQ_MHZ`（默认 24MHz）。
3. 关掉 STC-ISP 的「下载时擦除用户 EEPROM」，否则参数每次都被清。

## 1. 电感机械对中（最重要）

- 左右电感高度、倾角严格对称；相邻电感间距 ≥2cm；横竖电感别贴太近（防谐振）。
- 10mH 工字电感配 6.8nF 谐振电容，谐振点 19.8~20.2kHz。
- 用「ADC 页面」（K2 翻页）或串口 `N0 N1 N2...` 看：把车放导线正上方，
  左右对称通道的归一化值应接近。偏差大就调机械，别急着调参数。

## 2. 归一化标定

1. 把车放赛道上，K3 短按（或串口 `cal`）。
2. 沿赛道推车走一圈，让每个电感都经历「远离导线」和「正对导线」。
3. 6 秒后自动结束并保存，OLED 显示 CALIB SAVED。
4. 之后 STATUS 页面 PARAM 应显示 EEPROM，CALIB 显示 LOADED。

## 3. 舵机 PD

先只给很小的速度，让车慢速跑直线/缓弯：

- 车在直线上左右摆动 → Kp 太大，减小（串口 `kp=500`）。
- 入弯转不够、冲出外道 → Kp 太小，增大。
- 过弯后在直线上来回晃（超调）→ Kd 太小，增大（`kd=...`）。
- 直线上抖舵 → Kd 太大或死区太小，减 Kd 或增大 `DEV_DEADZONE`。
- 起步：`kp=700 kd=60`（默认），按 100 步进微调。
- 机械中值不准（车总是往一边偏）→ `trim=±N` 微调，`save`。

方向反了：车往右偏却还往右打 → 改 `config.h` 的 `SERVO_DIR` 或 `DEV_DIR` 符号。

## 4. 速度

- 开环模式（默认）：`base=` 设基础速度（‰占空比），`min=` 设弯道最低速度。
  弯道减速由 `SPD_CURVE_START`/`SPD_CURVE_ENABLE` 控制。
- 接编码器后：`ENCODER_ENABLE=1`，`pikp= pikp= piki=` 调增量式 PI。
  - 速度追不上目标/加速慢 → 增大 Kp。
  - 稳态有静差或周期性波动 → 增大 Ki。
  - 起步太猛/抖动 → 减小 Kp、加 `MOTOR_PI_OUT_LIMIT` 限幅。
- 电机接线反了 → `MOTOR_REVERSE_L/R` 置 1，别改线。

## 5. 元素识别

默认只显示不干预。先开 `scope` 看竖直电感通道和总强度波形，再改 `config.h` 的
`EL_*` 阈值，确认判据可靠后把 `ELEMENT_ACTION_ENABLE=1`。

## 6. 常用串口命令

```
kp=700  kd=60  trim=0     舵机
base=400 min=220 max=900  速度规划
pikp=30 piki=4            速度环 PI
scope / text              波形 / 文本
save / load / default     保存/重载/恢复默认
cal / run / stop          标定/启动/停止
?                         打印参数
```

## 7. 常见现象排查

| 现象 | 可能原因 |
|------|----------|
| 电机/舵机无输出 | 漏 `PWMA_BKR`/`PWMB_BKR` 的 MOE；引脚没设推挽；`EAXFR` 没置 1 |
| PWM 频率差一半 | 该芯片 PWM 时钟是 SYSclk/2，`pwm.c` 里 `clk` 除 2 |
| 参数每次上电丢失 | STC-ISP 勾了「擦除用户 EEPROM」；或没 `save` |
| 某通道归一化恒 0 | 该电感没接/运放无输出/标定时没经历最大磁场 |
| 直线跑偏 | 机械不对称；中值没调；左右运放增益差太大 |
| OLED 花屏 | I2C 太快，`soft_i2c.c` 的 `I2C_DELAY_US` 调大到 4~5 |
