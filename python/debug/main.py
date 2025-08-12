#!/usr/bin/env python3
"""
serial_to_png_4vars_mac.py

Read 4 numeric values (x y z w) from a serial console and periodically save a PNG time-series graph.
No max points — will store all data until stopped.
Default port: /dev/cu.usbmodem1101 (common on macOS for Arduino-like boards)
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
    """Extract up to 4 numbers from a string like 'x y z w'."""
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

def plot_and_save(points, outpath, title=None, dpi=150):
    if len(points) == 0:
        return
    times, xs, ys, zs, ws = zip(*points)
    t0 = times[0]
    t_seconds = np.array([(t - t0).total_seconds() for t in times])

    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t_seconds, xs, label="Error", linewidth=1)
    ax.plot(t_seconds, ys, label="Proportional", linewidth=1)
    ax.plot(t_seconds, zs, label="Derivative", linewidth=1)
    ax.plot(t_seconds, ws, label="Distance", linewidth=1)

    ax.set_xlabel("seconds")
    ax.set_ylabel("value")
    if title:
        ax.set_title(title)
    ax.grid(True, linestyle=':', linewidth=0.5)

    all_vals = np.array([xs, ys, zs, ws])
    ymin, ymax = all_vals.min(), all_vals.max()
    yrange = ymax - ymin or 1.0
    ax.set_ylim(ymin - 0.1 * yrange, ymax + 0.1 * yrange)
    ax.set_xlim(t_seconds.min(), t_seconds.max())
    ax.legend(loc="upper right")

    last_time_str = times[-1].strftime("%Y-%m-%d %H:%M:%S")
    ax.annotate(
        f"last: X={xs[-1]:.3g} Y={ys[-1]:.3g} Z={zs[-1]:.3g} W={ws[-1]:.3g}\n{last_time_str}",
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
    p = argparse.ArgumentParser(description="Read 4 numeric values from serial and save PNG timeseries (unlimited points).")
    p.add_argument("--port", "-p", type=str, default="/dev/cu.usbmodem1101", help="Serial port (default: /dev/cu.usbmodem1101)")
    p.add_argument("--baud", "-b", type=int, default=115200, help="Baud rate (default 9600)")
    p.add_argument("--out", "-o", type=str, default="graph.png", help="Output PNG file path")
    p.add_argument("--update", "-u", type=float, default=0.5, help="Seconds between PNG updates")
    p.add_argument("--title", "-t", type=str, default=None, help="Title for the plot")
    p.add_argument("--timeout", type=float, default=1.0, help="Serial read timeout (seconds)")
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

    print(f"Reading from {args.port} at {args.baud} baud. Writing PNG every {args.update}s to {args.out}")
    try:
        while not stop_event.is_set():
            snapshot = list(queue)
            if snapshot:
                plot_and_save(snapshot, args.out, title=args.title)
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
