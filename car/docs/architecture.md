# 架构与模块设计

## 分层

```
app/      整车控制流程、显示、按键、参数、调试
algo/     纯算法（不碰寄存器）：滤波/归一化/偏差/舵机PD/电机速度环/元素
drivers/  器件驱动：软件I2C + SSD1306 OLED + 字库
bsp/      板级驱动：GPIO/延时/定时器/ADC/PWM/串口/EEPROM/编码器
board_config.h  引脚与外设分配（唯一改硬件的地方）
config.h        算法配置与默认参数
```

依赖方向：`app → algo/drivers/bsp → board_config.h → STC32G.H`。
`algo/` 层不依赖任何寄存器，换芯片基本不动。

## 主循环与控制周期

- Timer0 每 1ms 中断一次，累加到 `CTRL_PERIOD_MS`（默认 5ms）置一个标志。
- 主循环 `Car_Loop()` 轮询该标志，标志到就执行一个控制周期。
- 控制周期内顺序：
  1. ADC 扫描 7 路电感
  2. 滑动平均滤波
  3. （标定中）更新 min/max
  4. 归一化 `0~1000`
  5. 偏差计算（加权平均或差比和）
  6. 元素识别（十字/圆环/三岔）
  7. 舵机 PD → PWM
  8. 速度规划（弯道减速）+ 电机速度环 → PWM
- 非控制周期穿插：按键扫描（10ms）、串口收发、OLED 刷新（100ms 节流）。

## 关键算法

### 归一化
每通道离线标定 `min`（远离导线）与 `max`（正对导线），
`norm = (x - min) * 1000 / (max - min)`，夹在 0~1000。
标定由按键/串口触发，推车 6 秒完成，结果存 EEPROM。

### 偏差（deviation.c）
`DEV_MODE=0` 加权平均：`dev = Σ(v·w) · DEV_GAIN / Σv`，`w={-2,-1,0,1,2}`。
`DEV_MODE=1` 左右差比和：`dev = (R-L) · DEV_MAX / (R+L)`。
约定 `dev>0` = 导线在车右侧 → 右转。丢线时保持上一次偏差。

### 舵机 PD（servo.c）
位置式 `out = (Kp·dev + Kd·ddev)/PD_DIV`，输出 = 中值 + out，带变化率限幅。
不加积分（避免直线漂移）。

### 电机速度环（motor.c）
`ENCODER_ENABLE=0`：开环，目标速度即占空比（‰）。
`ENCODER_ENABLE=1`：增量式 PI（T0/T1 计数测速）。
速度规划：`|dev|` 超过 `SPD_CURVE_START` 后线性减速到 `min_spd`。

### 元素识别（element.c）
默认只识别并显示/回传，不干预控制（`ELEMENT_ACTION_ENABLE=0`）。
判据：十字=总强度明显高于基线；圆环/三岔=左右竖直电感差超阈值。
阈值需现场标定。

## 参数存储（param.c + eeprom.c）

参数结构体（舵机 KP/KD/trim、速度 base/min/max、PI、归一化标定值）+ magic +
校验和，整页存到 EEPROM。上电校验 magic+checksum+sanity 后才采用，否则用默认值。
写入流程：擦页 → 写结构体。只有用户主动 `save`/标定结束才写。
