# Keil C251 工程搭建步骤

STC32G144K246 是 **251 内核**，用 **Keil uVision + C251 编译器**（不是 C51）。

## 1. 准备

1. 装好 Keil C251（含 STC 的 Device Pack 或直接手动配）。
2. 用 STC-ISP 的「头文件」功能生成芯片头文件（名字可能是 `STC32G.H`、
   `STC32G144K246.H` 或 `stc32g144k246.h`），放到工程目录。
   - 若名字不是 `STC32G.H`，改 `src/main.c` 里的 `#include "STC32G.H"` 那一行。

## 2. 新建工程

1. Keil → Project → New µVision Project → 选芯片 **STC32G144K246**。
2. 弹出是否复制启动文件（STARTUP.A51 / .S）：**不复制**，我们用 STC-ISP 生成
   的启动文件。若编译报 `STARTUP` 缺符号，从 STC-ISP 生成并加入即可。

## 3. 加入源文件与头文件路径

1. 把 `src/` 下所有 `.c` 加入工程（含 `bsp/ drivers/ algo/ app/` 子目录下的）。
2. Options for Target → C251 → Include Paths，加入：
   ```
   src
   src/bsp
   src/drivers
   src/algo
   src/app
   ```

## 4. 关键编译选项

| 选项 | 值 |
|------|-----|
| Memory Model | **Large: variables in XDATA**（或 Huge） |
| Code Rom Size | **Huge: 64K or more**（144K246 程序可能超 64K） |
| 其它 | 保持默认；`EAXFR`/`WTST` 相关已在源码里处理 |

## 5. 中断与启动文件

- 本项目中断服务函数用 `interrupt N`，N 见 `board_config.h`，无需手写向量表。
- 若 STC-ISP 生成的头文件里中断向量号不同，只改 `board_config.h` 那一组宏。

## 6. 下载

1. STC-ISP 选择 STC32G144K246，IRC 频率设为 **24MHz**（与 `MAIN_FREQ_MHZ` 一致）。
2. **关闭「下载时擦除用户 EEPROM」**。
3. 在「EEPROM 设置」里分配一段 EEPROM（例如 4KB），代码只写偏移 0 起的一页。
4. **WTST（Flash 等待周期）由 STC-ISP 的「WTST 初值」选项按所选频率自动设置**，
   代码里不手动写（24MHz 口径 WTST=1）。若改主频，让 STC-ISP 重新生成初值。

## 7. 验证

上电后：
- 串口（115200 8N1）应打印 `== EM CAR READY ==` 与 `param: DEFAULT ...`。
- OLED 显示 STATUS/MAIN 页面。
- 若串口无输出，先查 UART 引脚（P3.0/P3.1）和波特率，再查 `MAIN_FREQ_MHZ`。
