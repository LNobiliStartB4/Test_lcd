#!/usr/bin/env python3
"""Program TouchGFX assets onto the external Winbond W25Q64 over USART2.

Start this tool, hold the Nucleo USER button, press RESET, then release USER:

    python Tools/flash_assets.py --port COM7 assets.bin

The protocol deliberately separates the repeated boot knock from the asset
header. This prevents delayed sector erases or UART replies from injecting a
second header into the binary asset stream.
"""

import argparse
import struct
import sys
import time
import zlib

try:
    import serial
except ImportError:
    serial = None


KNOCK = b"ADHTPROG"
HEADER_MAGIC = b"ADHTHEAD"
CHUNK = 256

HELLO = b"H"
HEADER_ACCEPTED = b"A"
ERASE_PROGRESS = b"S"
READY = b"R"
CHUNK_OK = b"K"
VERIFYING = b"V"
DONE_OK = b"D"
ERROR = b"E"

ERROR_FLASH_ID = b"I"
ERROR_HEADER = b"H"
ERROR_UART_TIMEOUT = b"T"
ERROR_ERASE = b"E"
ERROR_PROGRAM = b"P"
ERROR_CRC = b"C"
ERROR_MANIFEST = b"M"
ERROR_VERIFY = b"V"

EXPECTED_JEDEC_ID = b"\xEF\x40\x17"

HANDSHAKE_TIMEOUT = 0.5
HEADER_TIMEOUT = 5.0
ERASE_TIMEOUT = 120.0
CHUNK_TIMEOUT = 5.0
VERIFY_TIMEOUT = 60.0

ERROR_MESSAGES = {
    ERROR_FLASH_ID: "external flash not detected or unexpected JEDEC ID",
    ERROR_HEADER: "invalid asset header or image length",
    ERROR_UART_TIMEOUT: "UART timeout",
    ERROR_ERASE: "flash sector erase failed",
    ERROR_PROGRAM: "flash page programming failed",
    ERROR_CRC: "CRC mismatch in the received asset stream",
    ERROR_MANIFEST: "asset manifest write failed",
    ERROR_VERIFY: "flash read-back verification failed",
}


class FlashProtocolError(RuntimeError):
    pass


def _read_exact(ser, size, timeout, where):
    previous_timeout = getattr(ser, "timeout", None)
    ser.timeout = timeout
    try:
        data = bytearray()
        while len(data) < size:
            part = ser.read(size - len(data))
            if not part:
                raise FlashProtocolError(f"timeout while waiting for {where}")
            data.extend(part)
        return bytes(data)
    finally:
        ser.timeout = previous_timeout


def _read_code(ser, timeout, where):
    return _read_exact(ser, 1, timeout, where)


def _raise_device_error(ser, where):
    reason = _read_exact(ser, 1, HEADER_TIMEOUT, f"{where} error code")
    message = ERROR_MESSAGES.get(reason, f"unknown device error {reason!r}")
    raise FlashProtocolError(f"{where}: {message}")


def _wait_for_hello(ser, knock_retries, output):
    ser.reset_input_buffer()
    output("knocking... hold USER, press RESET, then release USER")

    for _attempt in range(knock_retries):
        ser.write(KNOCK)
        ser.flush()

        previous_timeout = getattr(ser, "timeout", None)
        ser.timeout = HANDSHAKE_TIMEOUT
        try:
            reply = ser.read(1)
        finally:
            ser.timeout = previous_timeout

        if not reply:
            continue
        if reply == ERROR:
            _raise_device_error(ser, "handshake")
        if reply != HELLO:
            continue

        jedec_id = _read_exact(ser, 3, HEADER_TIMEOUT, "JEDEC ID")
        output("connected: JEDEC ID " + " ".join(f"{byte:02X}" for byte in jedec_id))
        if jedec_id != EXPECTED_JEDEC_ID:
            raise FlashProtocolError(
                "unexpected JEDEC ID "
                + " ".join(f"{byte:02X}" for byte in jedec_id)
                + ", expected EF 40 17"
            )
        return

    raise FlashProtocolError(
        "no HELLO from device; hold USER while pressing RESET "
        "and verify that the internal-only firmware image was downloaded"
    )


def _send_header(ser, data):
    crc = zlib.crc32(data) & 0xFFFFFFFF
    header = HEADER_MAGIC + struct.pack("<II", len(data), crc)
    ser.write(header)
    ser.flush()

    reply = _read_code(ser, HEADER_TIMEOUT, "header acknowledgement")
    if reply == ERROR:
        _raise_device_error(ser, "header")
    if reply != HEADER_ACCEPTED:
        raise FlashProtocolError(
            f"header: expected {HEADER_ACCEPTED!r}, got {reply!r}"
        )
    return crc


def _wait_for_erase(ser, output):
    deadline = time.monotonic() + ERASE_TIMEOUT
    last_progress = (0, 0)

    while time.monotonic() < deadline:
        remaining = max(0.1, deadline - time.monotonic())
        reply = _read_code(ser, remaining, "erase progress")
        if reply == ERROR:
            _raise_device_error(ser, "erase")
        if reply == READY:
            output("erase complete")
            return
        if reply != ERASE_PROGRESS:
            raise FlashProtocolError(f"erase: unexpected reply {reply!r}")

        current, total = struct.unpack(
            "<HH", _read_exact(ser, 4, CHUNK_TIMEOUT, "erase progress payload")
        )
        if (current < last_progress[0]) or (total == 0) or (current > total):
            raise FlashProtocolError(
                f"erase: invalid progress {current}/{total}"
            )
        last_progress = (current, total)
        output(f"erasing sectors: {current}/{total}")

    raise FlashProtocolError("erase timed out after 120 seconds")


def _stream_data(ser, data, output):
    sent = 0
    while sent < len(data):
        chunk = data[sent:sent + CHUNK]
        ser.write(chunk)
        ser.flush()

        reply = _read_code(ser, CHUNK_TIMEOUT, f"chunk at offset {sent}")
        if reply == ERROR:
            _raise_device_error(ser, f"chunk at offset {sent}")
        if reply != CHUNK_OK:
            raise FlashProtocolError(
                f"chunk at offset {sent}: expected {CHUNK_OK!r}, got {reply!r}"
            )

        next_offset = struct.unpack(
            "<I", _read_exact(ser, 4, CHUNK_TIMEOUT, "chunk ACK offset")
        )[0]
        expected_offset = sent + len(chunk)
        if next_offset != expected_offset:
            raise FlashProtocolError(
                f"device acknowledged offset {next_offset}, "
                f"expected {expected_offset}"
            )

        sent = next_offset
        output(f"programming: {sent}/{len(data)} bytes")


def _wait_for_verification(ser, output):
    reply = _read_code(ser, VERIFY_TIMEOUT, "verification start")
    if reply == ERROR:
        _raise_device_error(ser, "transfer")
    if reply != VERIFYING:
        raise FlashProtocolError(
            f"verification: expected {VERIFYING!r}, got {reply!r}"
        )

    output("verifying flash CRC...")
    reply = _read_code(ser, VERIFY_TIMEOUT, "verification result")
    if reply == ERROR:
        _raise_device_error(ser, "verification")
    if reply != DONE_OK:
        raise FlashProtocolError(
            f"verification: expected {DONE_OK!r}, got {reply!r}"
        )
    output("OK: assets programmed and validated; board is restarting")


def program_assets(ser, data, knock_retries=50, output=print):
    if not data:
        raise FlashProtocolError("asset image is empty")

    crc = zlib.crc32(data) & 0xFFFFFFFF
    output(f"image: {len(data)} bytes, crc32=0x{crc:08X}")
    _wait_for_hello(ser, knock_retries, output)
    sent_crc = _send_header(ser, data)
    if sent_crc != crc:
        raise FlashProtocolError("internal host CRC error")
    output("header accepted; erasing external flash...")
    _wait_for_erase(ser, output)
    _stream_data(ser, data, output)
    _wait_for_verification(ser, output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", help="asset image, for example assets.bin")
    parser.add_argument("--port", required=True, help="serial port, for example COM7")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--knock-retries", type=int, default=50)
    args = parser.parse_args()

    if serial is None:
        raise SystemExit("pyserial is required: pip install pyserial")

    with open(args.binary, "rb") as asset_file:
        data = asset_file.read()

    try:
        with serial.Serial(args.port, args.baud, timeout=HANDSHAKE_TIMEOUT) as ser:
            program_assets(ser, data, args.knock_retries)
    except FlashProtocolError as error:
        raise SystemExit(f"FAILED: {error}") from error


if __name__ == "__main__":
    main()
