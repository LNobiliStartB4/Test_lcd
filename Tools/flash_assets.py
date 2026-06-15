#!/usr/bin/env python3
"""Program the TouchGFX asset blob onto the external Winbond W25Q64 over UART.

Host side of the in-firmware asset flasher (Core/Src/asset_flasher.c). Streams a
binary image of the ExtFlashSection to the board, which erases the flash, programs
it page-by-page, writes the manifest and CRC-validates the result.

Get the .bin out of the built firmware (the ExtFlashSection is linked at the
virtual base 0x90000000):

    arm-none-eabi-objcopy -O binary --only-section=ExtFlashSection \\
        Debug/Display_test_prova.elf assets.bin

Then, with the board powered and connected (ST-Link VCP = USART2), reset it and
within the knock window run:

    python tools/flash_assets.py --port COM5 assets.bin

The CRC here is the standard CRC-32 (poly 0xEDB88320) computed by zlib.crc32 —
the same value the firmware's asset_crc32 produces, so the on-device validation
must match.
"""

import argparse
import struct
import sys
import zlib

try:
    import serial  # pyserial
except ImportError:
    sys.exit("pyserial is required: pip install pyserial")

KNOCK = b"ADHTPROG"
CHUNK = 256

READY = b"R"
CHUNK_OK = b"K"
ERROR = b"E"
DONE_OK = b"D"
DONE_FAIL = b"F"


def expect(ser, what, where):
    got = ser.read(1)
    if got != what:
        raise SystemExit(
            f"FAILED at {where}: expected {what!r}, got {got!r} "
            "(timeout or device rejected the transfer)"
        )


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("binary", help="asset image (e.g. assets.bin)")
    ap.add_argument("--port", required=True, help="serial port, e.g. COM5 or /dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200, help="baud rate (default 115200)")
    ap.add_argument("--knock-retries", type=int, default=50,
                    help="how many times to send the knock while waiting for the board")
    args = ap.parse_args()

    with open(args.binary, "rb") as f:
        data = f.read()
    if not data:
        raise SystemExit("asset image is empty")

    crc = zlib.crc32(data) & 0xFFFFFFFF
    header = KNOCK + struct.pack("<II", len(data), crc)
    print(f"image: {len(data)} bytes, crc32=0x{crc:08X}")

    with serial.Serial(args.port, args.baud, timeout=2) as ser:
        # Send the knock until the board (which only listens for a short window
        # after reset) answers READY. Resend the full header each attempt.
        print("knocking... (reset the board now if nothing happens)")
        for attempt in range(args.knock_retries):
            ser.reset_input_buffer()
            ser.write(header)
            ser.flush()
            reply = ser.read(1)
            if reply == READY:
                break
            if reply == ERROR:
                raise SystemExit("device rejected the header (bad length / not ready)")
        else:
            raise SystemExit("no READY from device — is it in the knock window?")

        print("erase done, streaming...")
        sent = 0
        while sent < len(data):
            ser.write(data[sent:sent + CHUNK])
            ser.flush()
            expect(ser, CHUNK_OK, f"chunk at offset {sent}")
            sent += CHUNK
            done = min(sent, len(data))
            print(f"\r  {done}/{len(data)} bytes", end="", flush=True)
        print()

        result = ser.read(1)
        if result == DONE_OK:
            print("OK — flash programmed and validated.")
        elif result == DONE_FAIL:
            raise SystemExit("device reported FAIL (CRC mismatch or validation failed)")
        else:
            raise SystemExit(f"unexpected final reply: {result!r}")


if __name__ == "__main__":
    main()
