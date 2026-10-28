"""Replay recorded Simulink detections through the existing UDP input adapter.

Run the C++ sender in --live mode, then run this instead of Simulink for matched
measurement inputs. Host scheduling and UART delivery are still not synchronized
to the control tasks. CSV timestamps preserve missing-detection gaps.
"""
import argparse
import csv
import math
import socket
import struct
import time


def read_detections(path):
    fields = ["time_sec", "target_x", "target_y", "target_vx", "target_vy"]
    with open(path, newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != fields:
            raise ValueError("Unexpected detection CSV header")
        rows = [tuple(float(row[key]) for key in fields) for row in reader]
    if not rows or any(not all(math.isfinite(v) for v in row) for row in rows):
        raise ValueError("CSV must contain finite detections")
    if any(b[0] < a[0] for a, b in zip(rows, rows[1:])):
        raise ValueError("Detection timestamps must be nondecreasing")
    return rows


def replay(path, port=51001):
    rows = read_detections(path)
    start = time.monotonic()
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
        for row in rows:
            remaining = start + row[0] - rows[0][0] - time.monotonic()
            if remaining > 0:
                time.sleep(remaining)
            udp.sendto(struct.pack("<5d", *row), ("127.0.0.1", port))
    print(f"Sent {len(rows)} recorded detections to localhost:{port}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv")
    parser.add_argument("--port", type=int, default=51001)
    args = parser.parse_args()
    replay(args.csv, args.port)
