"""UART receive validation traffic. Linux sender; dry-run works on any host.

Stop the normal Linux sender before using this tool on the same serial port.
Example: python3 tools/uart_rx_exercise.py --port /dev/ttyUSB1 --case recovery
"""
import argparse
import binascii
import json
import os
import struct
import time


def frame(sequence, x=515, y=-97):
    body = bytes((1, 1, 8)) + struct.pack("<HhhH", sequence & 0xFFFF, x, y, 0)
    return b"\xaa\x55" + body + struct.pack("<H", binascii.crc_hqx(body, 0xFFFF))


def scenario(name, count=10):
    """Return (delay BEFORE write in seconds, bytes) pairs."""
    if name == "normal":
        return [(0.1, frame(i + 1, y=-97 if i % 2 == 0 else 97)) for i in range(count)]
    if name == "fragmented":
        data = frame(1)
        return [(0.005, bytes((b,))) for b in data]
    if name == "recovery":
        corrupt = bytearray(frame(1))
        corrupt[8] ^= 1
        return [(0, b"\x00\xff\xaa"), (0.02, bytes(corrupt)),
                (0.02, frame(2)[:7]), (0.02, frame(3)), (0.1, frame(4))]
    if name == "invalid":
        return [(0, frame(1)), (0.2, frame(2, x=0)), (0.2, frame(3, x=-1))]
    if name == "burst":
        return [(0, b"".join(frame(i + 1) for i in range(count)))]
    raise ValueError(name)


def send(port, chunks):
    import termios  # Linux/POSIX only; no pyserial dependency.
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    original = termios.tcgetattr(fd)
    try:
        attr = termios.tcgetattr(fd)
        attr[0] = attr[1] = attr[3] = 0
        attr[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attr[4] = attr[5] = termios.B115200
        attr[6][termios.VMIN] = 0
        attr[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attr)
        for delay, data in chunks:
            time.sleep(delay)
            remaining = memoryview(data)
            while remaining:
                n = os.write(fd, remaining)
                if n <= 0:
                    raise OSError("UART write made no progress")
                remaining = remaining[n:]
            termios.tcdrain(fd)
    finally:
        termios.tcsetattr(fd, termios.TCSANOW, original)
        os.close(fd)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port")
    parser.add_argument("--case", choices=("normal", "fragmented", "recovery", "invalid", "burst"), default="normal")
    parser.add_argument("--count", type=int, default=10)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    if not 1 <= args.count <= 100000:
        parser.error("--count must be between 1 and 100000")
    if not args.dry_run and not args.port:
        parser.error("specify --port or --dry-run")
    chunks = scenario(args.case, args.count)
    if args.dry_run:
        print(json.dumps([{"delay_s": t, "hex": b.hex()} for t, b in chunks], indent=2))
    else:
        send(args.port, chunks)
        print(f"Sent {sum(len(b) for _, b in chunks)} bytes; check board counters and trace.")
