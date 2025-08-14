#!/usr/bin/env python3
"""
serial_to_png_4vars_mac_split.py

Read 4 numeric values (x y z w) from a serial console and periodically save two PNG time-series graphs:
- xyz.png → x, y, z
- w.png   → w
"""

import argparse
import re
import signal
import sys
from collections import deque
from datetime import datetime
from threading import Event, Thread
import time

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import serial

NUMBER_RE = re.compile(r'[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?')

def parse_numbers(s: str):
    matches = NUMBER_RE.findall(s)
    if len(matches) < 4:
        return None
    try:
        return tuple(float(matches[i]) for i in range(4))
    except ValueError:
        return None

def reader_thread(ser, queue: deque, stop_event: Event):
    while not stop_event.is_set():
        try:
            line = ser.readline()
            if not line:
                continue
            try:
                s = line.decode(errors='replace').strip()
            except Exception:
                s = str(line)
            vals = parse_numbers(s)
            if vals is not None:
                queue.append((datetime.now(), *vals))
        except serial.SerialException as e:
            print("Serial error:", e, file=sys.stderr)
            stop_event.set()
        except Exception as e:
            print("Read warning:", e, file=sys.stderr)
            time.sleep(0.1)

def plot_and_save_xyz(points, outpath, title=None, dpi=150):
    if len(points) == 0:
        return
    times, xs, ys, zs, _ = zip(*points)
    t0 = times[0]
    t_seconds = np.array([(t - t0).total_seconds() for t in times])

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t_seconds, xs, label="P", linewidth=1)
    ax.plot(t_seconds, ys, label="D", linewidth=1)
    ax.plot(t_seconds, zs, label="Angle", linewidth=1)

    ax.set_xlabel("seconds")
    ax.set_ylabel("value")
    if title:
        ax.set_title(title + " (X,Y,Z)")
    ax.grid(True, linestyle=':', linewidth=0.5)

    all_vals = np.array([xs, ys, zs])
    ymin, ymax = all_vals.min(), all_vals.max()
    yrange = ymax - ymin or 1.0
    ax.set_ylim(ymin - 0.1 * yrange, ymax + 0.1 * yrange)
    ax.set_xlim(t_seconds.min(), t_seconds.max())
    ax.legend(loc="upper right")

    last_time_str = times[-1].strftime("%Y-%m-%d %H:%M:%S")
    ax.annotate(
        f"last: X={xs[-1]:.3g} Y={ys[-1]:.3g} Z={zs[-1]:.3g}\n{last_time_str}",
        xy=(t_seconds[-1], zs[-1]),
        xytext=(-10, 10),
        textcoords='offset points',
        ha='right', va='bottom',
        fontsize=8,
        bbox=dict(boxstyle="round,pad=0.3", alpha=0.2)
    )
    fig.tight_layout()
    fig.savefig(outpath, dpi=dpi)
    plt.close(fig)

def plot_and_save_w(points, outpath, title=None, dpi=150):
    if len(points) == 0:
        return
    times, _, _, _, ws = zip(*points)
    t0 = times[0]
    t_seconds = np.array([(t - t0).total_seconds() for t in times])

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t_seconds, ws, label="wall_angle", linewidth=1, color="orange")

    ax.set_xlabel("seconds")
    ax.set_ylabel("value")
    if title:
        ax.set_title(title + " (W)")
    ax.grid(True, linestyle=':', linewidth=0.5)

    ymin, ymax = min(ws), max(ws)
    yrange = ymax - ymin or 1.0
    ax.set_ylim(ymin - 0.1 * yrange, ymax + 0.1 * yrange)
    ax.set_xlim(t_seconds.min(), t_seconds.max())
    ax.legend(loc="upper right")

    last_time_str = times[-1].strftime("%Y-%m-%d %H:%M:%S")
    ax.annotate(
        f"last: W={ws[-1]:.3g}\n{last_time_str}",
        xy=(t_seconds[-1], ws[-1]),
        xytext=(-10, 10),
        textcoords='offset points',
        ha='right', va='bottom',
        fontsize=8,
        bbox=dict(boxstyle="round,pad=0.3", alpha=0.2)
    )
    fig.tight_layout()
    fig.savefig(outpath, dpi=dpi)
    plt.close(fig)

def main():
    p = argparse.ArgumentParser(description="Read 4 numeric values from serial and save 2 PNGs: XYZ + W")
    p.add_argument("--port", "-p", type=str, default="/dev/cu.usbmodem1101")
    p.add_argument("--baud", "-b", type=int, default=115200)
    p.add_argument("--out_xyz", type=str, default="xyz.png")
    p.add_argument("--out_w", type=str, default="w.png")
    p.add_argument("--update", "-u", type=float, default=0.3)
    p.add_argument("--title", "-t", type=str, default=None)
    p.add_argument("--timeout", type=float, default=1.0)
    args = p.parse_args()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=args.timeout, dsrdtr=False)
    except serial.SerialException as e:
        print("Failed to open serial port:", e, file=sys.stderr)
        sys.exit(2)

    queue = deque()
    stop_event = Event()

    def handle_sigint(signum, frame):
        print("\nStopping...")
        stop_event.set()

    signal.signal(signal.SIGINT, handle_sigint)
    signal.signal(signal.SIGTERM, handle_sigint)

    t = Thread(target=reader_thread, args=(ser, queue, stop_event), daemon=True)
    t.start()

    print(f"Reading from {args.port} at {args.baud} baud. Writing PNGs every {args.update}s.")
    try:
        while not stop_event.is_set():
            snapshot = list(queue)
            if snapshot:
                plot_and_save_xyz(snapshot, args.out_xyz, title=args.title)
                plot_and_save_w(snapshot, args.out_w, title=args.title)
            stop_event.wait(args.update)
    finally:
        stop_event.set()
        t.join(timeout=1.0)
        try:
            ser.close()
        except Exception:
            pass
        print("Exited.")

if __name__ == "__main__":
    main()
