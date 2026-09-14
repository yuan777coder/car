#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
scope.py —— 电磁循迹小车虚拟示波器（上位机）

配合 src/app/debug.c 的"示波器模式"使用：
在小车串口里输入命令 `scope` 切到二进制帧模式，`text` 切回文本模式。

帧格式
    0xAA 0x55 LEN D0 D1 ... D(2N-1) CS
    LEN = 通道数 * 2
    每个通道 2 字节，高字节在前（有符号补码）
    CS  = 数据区所有字节之和的低 8 位

用法
    列出串口:
        python scope.py --list
    实时画波形（需要 pyserial + matplotlib）:
        python scope.py -p COM3
    存成 CSV，不画图（只需要 pyserial）:
        python scope.py -p COM3 --csv log.csv
    在终端里打印数值（不需要 matplotlib）:
        python scope.py -p COM3 --text

依赖
    pip install pyserial
    pip install matplotlib     # 只有 --csv / --text 模式不需要

通道含义（前 5 个固定，后面是各水平电感的归一化值）
    0 偏差 DEV        1 舵机 PWM SRV     2 左轮速度 SPD_L
    3 右轮速度 SPD_R  4 水平总强度 SUM   5.. 各水平电感归一化值
"""

import argparse
import sys
import time

FRAME_HEAD_0 = 0xAA
FRAME_HEAD_1 = 0x55

FIXED_CH_NAMES = ["DEV", "SRV", "SPD_L", "SPD_R", "SUM"]
FIXED_CH_COLORS = ["#e6194b", "#3cb44b", "#4363d8", "#f58231", "#911eb4"]
EXTRA_COLORS = ["#42d4f4", "#f032e6", "#bfef45", "#fabed4", "#469990"]


def channel_names(ch_count):
    names = list(FIXED_CH_NAMES)
    for i in range(max(0, ch_count - len(FIXED_CH_NAMES))):
        names.append("N%d" % i)
    return names[:ch_count]


def channel_colors(ch_count):
    colors = list(FIXED_CH_COLORS)
    for i in range(max(0, ch_count - len(FIXED_CH_COLORS))):
        colors.append(EXTRA_COLORS[i % len(EXTRA_COLORS)])
    return colors[:ch_count]


class FrameParser:
    """逐字节喂入，吐出 (通道号->值 的列表, 原始帧字节数)"""

    def __init__(self, ch_count):
        self.ch_count = ch_count
        self.expected_len = ch_count * 2
        self.buf = bytearray()
        self.bad_frames = 0

    def feed(self, data):
        """返回本次新解析出的帧列表，每帧是一个 int 列表"""
        self.buf.extend(data)
        frames = []
        while True:
            # 找帧头
            idx = -1
            for i in range(len(self.buf) - 1):
                if self.buf[i] == FRAME_HEAD_0 and self.buf[i + 1] == FRAME_HEAD_1:
                    idx = i
                    break
            if idx < 0:
                # 保留最后 1 字节，可能是半个帧头
                if len(self.buf) > 1:
                    del self.buf[:-1]
                return frames
            if idx > 0:
                del self.buf[:idx]

            # 一帧总长 = 2(帧头) + 1(长度) + expected_len(数据) + 1(校验)
            need = 4 + self.expected_len
            if len(self.buf) < need:
                return frames

            length = self.buf[2]
            if length != self.expected_len:
                # 长度不对：丢掉这一个帧头继续找
                self.bad_frames += 1
                del self.buf[:2]
                continue

            payload = self.buf[3:3 + self.expected_len]
            cs = self.buf[3 + self.expected_len]
            calc = sum(payload) & 0xFF
            if cs != calc:
                self.bad_frames += 1
                del self.buf[:2]
                continue

            values = []
            for i in range(self.ch_count):
                hi = payload[2 * i]
                lo = payload[2 * i + 1]
                v = (hi << 8) | lo
                if v >= 0x8000:
                    v -= 0x10000
                values.append(v)
            frames.append(values)
            del self.buf[:need]


def open_serial(port, baud):
    try:
        import serial  # pyserial
    except ImportError:
        sys.exit("需要 pyserial：pip install pyserial")
    return serial.Serial(port, baud, timeout=0.02)


def list_ports():
    try:
        from serial.tools import list_ports as lp
    except ImportError:
        sys.exit("需要 pyserial：pip install pyserial")
    ports = list(lp.comports())
    if not ports:
        print("没有发现串口")
        return
    for p in ports:
        print("%-10s %s" % (p.device, p.description))


def run_text(ser, parser):
    names = channel_names(parser.ch_count)
    print("  ".join("%8s" % n for n in names))
    try:
        while True:
            frames = parser.feed(ser.read(256))
            for f in frames:
                print("  ".join("%8d" % v for v in f))
    except KeyboardInterrupt:
        print("\n已停止，坏帧 %d" % parser.bad_frames)


def run_csv(ser, parser, path, max_rows):
    names = channel_names(parser.ch_count)
    rows = 0
    with open(path, "w", encoding="utf-8") as fp:
        fp.write("t_ms," + ",".join(names) + "\n")
        t0 = time.time()
        try:
            while True:
                frames = parser.feed(ser.read(256))
                for f in frames:
                    t = (time.time() - t0) * 1000.0
                    fp.write("%.1f," % t + ",".join(str(v) for v in f) + "\n")
                    rows += 1
                    if max_rows and rows >= max_rows:
                        print("已采集 %d 行，写入 %s" % (rows, path))
                        return
                if frames:
                    fp.flush()
        except KeyboardInterrupt:
            print("\n已停止，共 %d 行，坏帧 %d" % (rows, parser.bad_frames))


def run_plot(ser, parser):
    try:
        import matplotlib.pyplot as plt
        import matplotlib.animation as animation
    except ImportError:
        sys.exit("需要 matplotlib：pip install matplotlib（或改用 --csv / --text）")

    ch = parser.ch_count
    names = channel_names(ch)
    colors = channel_colors(ch)

    # 每个通道一个子图，方便看不同量纲的信号
    fig, axes = plt.subplots(ch, 1, sharex=True, figsize=(10, 1.6 * ch))
    if ch == 1:
        axes = [axes]
    fig.suptitle("EM CAR SCOPE  (Ctrl+C to quit)")
    try:
        fig.canvas.manager.set_window_title("EM CAR SCOPE")
    except Exception:
        pass

    WINDOW = 500
    data = [[] for _ in range(ch)]
    lines = []
    for i, ax in enumerate(axes):
        (ln,) = ax.plot([], [], color=colors[i], linewidth=1.1)
        ax.set_ylabel(names[i], fontsize=8, rotation=0, ha="right", va="center")
        ax.grid(True, alpha=0.3)
        ax.tick_params(labelsize=7)
        lines.append(ln)
    axes[-1].set_xlabel("samples", fontsize=8)

    state = {"n": 0}

    def update(_frame):
        frames = parser.feed(ser.read(4096))
        for f in frames:
            for i in range(ch):
                data[i].append(f[i])
                if len(data[i]) > WINDOW:
                    del data[i][0]
        if frames:
            state["n"] += len(frames)
            for i in range(ch):
                lines[i].set_data(range(len(data[i])), data[i])
                axes[i].set_xlim(0, max(WINDOW, len(data[i])))
                axes[i].relim()
                axes[i].autoscale_view(scalex=False, scaley=True)
            fig.canvas.draw_idle()
        return lines

    ani = animation.FuncAnimation(fig, update, interval=50, blit=False,
                                  cache_frame_data=False)
    try:
        plt.tight_layout()
        plt.show()
    except KeyboardInterrupt:
        pass
    print("坏帧 %d" % parser.bad_frames)
    del ani


def main():
    ap = argparse.ArgumentParser(
        description="电磁循迹小车虚拟示波器",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__)
    ap.add_argument("-p", "--port", help="串口号，如 COM3 或 /dev/ttyUSB0")
    ap.add_argument("-b", "--baud", type=int, default=115200, help="波特率，默认 115200")
    ap.add_argument("-n", "--channels", type=int, default=10,
                    help="通道数，默认 10（= 5 + IND_H_NUM；请与 config.h 的 SCOPE_CH_NUM 一致）")
    ap.add_argument("--csv", metavar="FILE", help="把数据写入 CSV（不画图）")
    ap.add_argument("--text", action="store_true", help="在终端打印数值（不画图）")
    ap.add_argument("--max-rows", type=int, default=0, help="CSV 模式最多采集多少行，0=不限")
    ap.add_argument("--list", action="store_true", help="列出可用串口后退出")
    args = ap.parse_args()

    if args.list:
        list_ports()
        return

    if not args.port:
        ap.error("请用 -p 指定串口，或先用 --list 查看可用串口")

    parser = FrameParser(args.channels)
    ser = open_serial(args.port, args.baud)
    print("已打开 %s @ %d，通道数 %d。Ctrl+C 退出。" % (args.port, args.baud, args.channels))

    try:
        if args.csv:
            run_csv(ser, parser, args.csv, args.max_rows)
        elif args.text:
            run_text(ser, parser)
        else:
            run_plot(ser, parser)
    finally:
        ser.close()
        print("坏帧 %d" % parser.bad_frames)


if __name__ == "__main__":
    main()
