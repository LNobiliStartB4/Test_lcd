#!/usr/bin/env python3
"""Program TouchGFX assets onto the external Winbond W25Q64 over USART2.

Start this tool, hold the Nucleo USER button, press RESET, then release USER:

    python Tools/flash_assets.py --port COM7 assets.bin

The protocol deliberately separates the repeated boot knock from the asset
header. This prevents delayed sector erases or UART replies from injecting a
second header into the binary asset stream.
"""

import argparse
import hashlib
import struct
import sys
import time
import zlib

try:
    import serial
except ImportError:
    serial = None


KNOCK = b"ADHTPROG"
DIAGNOSTIC_KNOCK = b"ADHTDIAG"
UART_TEST_KNOCK = b"ADHTUART"
UART_TEST_REPLY = b"U"
UART_TEST_BYTES = 65536          # device->host stress stream length
UART_TEST_TIMEOUT = 30.0
HEADER_MAGIC = b"ADHTHEAD"
CHUNK = 256
PACKAGE_ID_SIZE = 16
DIAGNOSTIC_SAMPLE_COUNT = 32
DIAGNOSTIC_VERSION = 1
DIAGNOSTIC_HEADER_MAGIC = b"DGOK"
DIAGNOSTIC_LEGACY_HEADER_PREFIX = b"DG"
DIAGNOSTIC_RECORD_MAGIC = b"DREC"
DIAGNOSTIC_SCAN_LIMIT = 256

HELLO = b"H"
DIAGNOSTIC_REPLY = b"Q"
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
RECOVERY_SETTLE_SECONDS = 6.0

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


def check_counter_stream(data):
    """Verify a device->host UART stress stream where byte[i] == (i & 0xFF).

    A clean link yields 0 errors. A flipped byte is one error at its index; a
    dropped/inserted byte shifts everything after it (cascade of mismatches),
    with first_bad marking where the link slipped. The caller also compares
    len(data) against the expected length to catch missing bytes."""
    errors = 0
    first_bad = None
    expected_byte = None
    got_byte = None
    for index, byte in enumerate(data):
        expected = index & 0xFF
        if byte != expected:
            errors += 1
            if first_bad is None:
                first_bad = index
                expected_byte = expected
                got_byte = byte
    return {
        "length": len(data),
        "errors": errors,
        "first_bad": first_bad,
        "expected": expected_byte,
        "got": got_byte,
    }


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
    reason = _read_device_error_reason(ser, where)
    message = ERROR_MESSAGES.get(reason, f"unknown device error {reason!r}")
    raise FlashProtocolError(f"{where}: {message}")


def _read_device_error_reason(ser, where):
    return _read_exact(ser, 1, HEADER_TIMEOUT, f"{where} error code")


def _is_stale_recovery_error(reason):
    """Errors that can be emitted by a previous programming session.

    Seeing one before the fresh HELLO/Q handshake means the board was still
    unwinding an old transfer when the new tool instance started. Consume it
    and keep knocking; the firmware loops back to recovery after the error.
    """
    return reason in {
        ERROR_HEADER,
        ERROR_UART_TIMEOUT,
        ERROR_ERASE,
        ERROR_PROGRAM,
        ERROR_CRC,
        ERROR_MANIFEST,
        ERROR_VERIFY,
    }


def _warn_stale_error(output, where, reason):
    message = ERROR_MESSAGES.get(reason, f"unknown device error {reason!r}")
    output(
        f"warning: ignored stale {where} error from previous session: {message}"
    )


def _settle_recovery_after_stale_error(ser, output, sleep_fn):
    output(
        "waiting for any old asset-stream session to time out before retrying..."
    )
    sleep_fn(RECOVERY_SETTLE_SECONDS)


def _wait_for_hello(ser, knock_retries, output, sleep_fn=time.sleep):
    ser.reset_input_buffer()
    output("knocking... board should be in recovery (black screen); "
           "tap RESET once if it doesn't connect")

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
            reason = _read_device_error_reason(ser, "handshake")
            if _is_stale_recovery_error(reason):
                _warn_stale_error(output, "handshake", reason)
                _settle_recovery_after_stale_error(ser, output, sleep_fn)
                continue
            message = ERROR_MESSAGES.get(reason, f"unknown device error {reason!r}")
            raise FlashProtocolError(f"handshake: {message}")
        if reply != HELLO:
            continue

        jedec_id = _read_exact(ser, 3, HEADER_TIMEOUT, "JEDEC ID")
        output("connected: JEDEC ID " + " ".join(f"{byte:02X}" for byte in jedec_id))
        if jedec_id != EXPECTED_JEDEC_ID:
            error_reply = _read_exact(
                ser, 2, HEADER_TIMEOUT, "JEDEC error response"
            )
            if error_reply != ERROR + ERROR_FLASH_ID:
                raise FlashProtocolError(
                    "unexpected response after invalid JEDEC ID: "
                    + " ".join(f"{byte:02X}" for byte in error_reply)
                )
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


def _wait_for_diagnostic_reply(ser, knock_retries, output, sleep_fn=time.sleep):
    ser.reset_input_buffer()
    output("requesting read-only SPI diagnostic; tap RESET once if it doesn't connect")

    for _attempt in range(knock_retries):
        ser.write(DIAGNOSTIC_KNOCK)
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
            reason = _read_device_error_reason(ser, "diagnostic handshake")
            if _is_stale_recovery_error(reason):
                _warn_stale_error(output, "diagnostic handshake", reason)
                _settle_recovery_after_stale_error(ser, output, sleep_fn)
                continue
            message = ERROR_MESSAGES.get(reason, f"unknown device error {reason!r}")
            raise FlashProtocolError(f"diagnostic handshake: {message}")
        if reply == DIAGNOSTIC_REPLY:
            return

    raise FlashProtocolError(
        "no diagnostic reply from device; verify the updated internal firmware "
        "is running in recovery"
    )


def _format_hex(data):
    return " ".join(f"{byte:02X}" for byte in data)


def _read_byte(ser, timeout, where):
    return _read_exact(ser, 1, timeout, where)[0]


def _read_diagnostic_header(ser):
    scanned = bytearray()

    while len(scanned) < DIAGNOSTIC_SCAN_LIMIT:
        scanned.append(_read_byte(ser, HEADER_TIMEOUT, "diagnostic header"))

        if scanned.endswith(DIAGNOSTIC_HEADER_MAGIC):
            version_and_count = _read_exact(
                ser, 2, HEADER_TIMEOUT, "diagnostic report header"
            )
            return version_and_count[0], version_and_count[1]

        if scanned.endswith(DIAGNOSTIC_LEGACY_HEADER_PREFIX):
            tail = _read_exact(
                ser, 2, HEADER_TIMEOUT, "diagnostic legacy header tail"
            )
            if tail == b"OK":
                version_and_count = _read_exact(
                    ser, 2, HEADER_TIMEOUT, "diagnostic report header"
                )
                return version_and_count[0], version_and_count[1]
            if (
                tail[0] == DIAGNOSTIC_VERSION
                and 0 < tail[1] <= 4
            ):
                return tail[0], tail[1]
            scanned.extend(tail)

    raise FlashProtocolError(
        "diagnostic header framing mismatch: scanned "
        + _format_hex(scanned[-16:])
        + ", expected "
        + _format_hex(DIAGNOSTIC_HEADER_MAGIC)
        + " or legacy "
        + _format_hex(DIAGNOSTIC_LEGACY_HEADER_PREFIX)
        + " <version> <count>"
    )


def _read_diagnostic_record_magic(ser):
    scanned = bytearray()

    while len(scanned) < DIAGNOSTIC_SCAN_LIMIT:
        scanned.append(_read_byte(ser, HEADER_TIMEOUT, "diagnostic record magic"))
        if scanned.endswith(DIAGNOSTIC_RECORD_MAGIC):
            return

    raise FlashProtocolError(
        "diagnostic record framing mismatch: scanned "
        + _format_hex(scanned[-16:])
        + ", expected "
        + _format_hex(DIAGNOSTIC_RECORD_MAGIC)
    )


def _read_diagnostic_record(ser):
    _read_diagnostic_record_magic(ser)
    header = _read_exact(ser, 2, HEADER_TIMEOUT, "diagnostic mode header")
    mode = header[0]
    stable_count = header[1]
    if mode not in (0, 3):
        raise FlashProtocolError(f"diagnostic report contains invalid SPI mode {mode}")
    if stable_count > DIAGNOSTIC_SAMPLE_COUNT:
        raise FlashProtocolError(
            f"diagnostic report has invalid stable count {stable_count}/"
            f"{DIAGNOSTIC_SAMPLE_COUNT}"
        )
    sample_bytes = _read_exact(
        ser,
        DIAGNOSTIC_SAMPLE_COUNT * 3,
        HEADER_TIMEOUT,
        f"mode {mode} JEDEC samples",
    )
    samples = [
        sample_bytes[index:index + 3]
        for index in range(0, len(sample_bytes), 3)
    ]
    id90 = _read_exact(ser, 2, HEADER_TIMEOUT, f"mode {mode} ID 90h")
    release_id = _read_exact(ser, 1, HEADER_TIMEOUT, f"mode {mode} ID ABh")[0]
    status = _read_exact(ser, 3, HEADER_TIMEOUT, f"mode {mode} status registers")
    sfdp = _read_exact(ser, 4, HEADER_TIMEOUT, f"mode {mode} SFDP signature")
    return {
        "mode": mode,
        "stable_count": stable_count,
        "samples": samples,
        "id90": id90,
        "release_id": release_id,
        "status": status,
        "sfdp": sfdp,
    }


def diagnose_flash(ser, knock_retries=50, output=print, sleep_fn=time.sleep):
    _wait_for_diagnostic_reply(ser, knock_retries, output, sleep_fn)
    version, mode_count = _read_diagnostic_header(ser)
    if version != DIAGNOSTIC_VERSION:
        raise FlashProtocolError(
            f"unsupported diagnostic version {version}, "
            f"expected {DIAGNOSTIC_VERSION}"
        )

    report = {}
    for _index in range(mode_count):
        record = _read_diagnostic_record(ser)
        mode = record["mode"]
        report[mode] = record
        unique_samples = []
        for sample in record["samples"]:
            if sample not in unique_samples:
                unique_samples.append(sample)
        output(
            f"mode {mode}: JEDEC stable {record['stable_count']}/"
            f"{DIAGNOSTIC_SAMPLE_COUNT}; observed "
            + ", ".join(_format_hex(sample) for sample in unique_samples)
        )
        output(
            f"mode {mode}: ID90 {_format_hex(record['id90'])}; "
            f"AB {record['release_id']:02X}; "
            f"status {_format_hex(record['status'])}; "
            f"SFDP {_format_hex(record['sfdp'])}"
        )

    mode0 = report.get(0)
    if mode0 is None:
        raise FlashProtocolError("diagnostic report is missing SPI mode 0")
    if (
        mode0["stable_count"] != DIAGNOSTIC_SAMPLE_COUNT
        or any(sample != EXPECTED_JEDEC_ID for sample in mode0["samples"])
    ):
        raise FlashProtocolError(
            "mode 0 JEDEC is not stable; do not erase or program the flash"
        )
    if mode0["id90"] != b"\xEF\x16":
        raise FlashProtocolError(
            f"mode 0 ID 90h is {_format_hex(mode0['id90'])}, expected EF 16"
        )
    if mode0["release_id"] != 0x16:
        raise FlashProtocolError(
            f"mode 0 release ID is {mode0['release_id']:02X}, expected 16"
        )
    if mode0["sfdp"] != b"SFDP":
        raise FlashProtocolError(
            f"mode 0 SFDP is {_format_hex(mode0['sfdp'])}, "
            "expected 53 46 44 50"
        )

    output("OK: SPI mode 0 is stable and the W25Q64JV identity is consistent")
    if 3 in report and report[3]["stable_count"] != DIAGNOSTIC_SAMPLE_COUNT:
        output("warning: SPI mode 3 was not stable; runtime remains on mode 0")
    return report


def _send_header(ser, data):
    crc = zlib.crc32(data) & 0xFFFFFFFF
    package_id = hashlib.sha256(data).digest()[:PACKAGE_ID_SIZE]
    header = HEADER_MAGIC + struct.pack("<II", len(data), crc) + package_id
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


def program_assets(ser, data, knock_retries=50, output=print, sleep_fn=time.sleep):
    if not data:
        raise FlashProtocolError("asset image is empty")

    crc = zlib.crc32(data) & 0xFFFFFFFF
    output(f"image: {len(data)} bytes, crc32=0x{crc:08X}")
    _wait_for_hello(ser, knock_retries, output, sleep_fn)
    sent_crc = _send_header(ser, data)
    if sent_crc != crc:
        raise FlashProtocolError("internal host CRC error")
    output("header accepted; erasing external flash...")
    _wait_for_erase(ser, output)
    _stream_data(ser, data, output)
    _wait_for_verification(ser, output)


def _wait_for_uart_test(ser, knock_retries, output):
    ser.reset_input_buffer()
    output("requesting UART link test; tap RESET once if it doesn't connect")
    for _attempt in range(knock_retries):
        ser.write(UART_TEST_KNOCK)
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
            _raise_device_error(ser, "uart test handshake")
        if reply == UART_TEST_REPLY:
            return
    raise FlashProtocolError("no UART-test reply from device")


def uart_test(ser, knock_retries=240, output=print):
    """Isolate the device->host UART link: the device streams UART_TEST_BYTES of
    a counter pattern (byte[i] == i & 0xFF); we verify it. No SPI, no flash."""
    _wait_for_uart_test(ser, knock_retries, output)
    output(f"receiving {UART_TEST_BYTES} bytes of counter pattern...")
    previous_timeout = getattr(ser, "timeout", None)
    ser.timeout = UART_TEST_TIMEOUT
    try:
        data = bytearray()
        deadline = time.monotonic() + UART_TEST_TIMEOUT
        while len(data) < UART_TEST_BYTES and time.monotonic() < deadline:
            part = ser.read(UART_TEST_BYTES - len(data))
            if not part:
                break
            data.extend(part)
    finally:
        ser.timeout = previous_timeout

    report = check_counter_stream(bytes(data))
    missing = UART_TEST_BYTES - report["length"]
    output(f"UART test: received {report['length']}/{UART_TEST_BYTES} bytes, "
           f"missing {missing}, mismatches {report['errors']}")
    if report["first_bad"] is not None:
        output(f"  first mismatch at byte {report['first_bad']}: "
               f"expected 0x{report['expected']:02X}, got 0x{report['got']:02X}")
    if report["errors"] == 0 and missing == 0:
        output("OK: device->host UART link is clean")
    else:
        output("FAIL: UART link drops/corrupts bytes (device->host)")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "binary",
        nargs="?",
        help="asset image, for example assets.bin (omit with --diagnose)",
    )
    parser.add_argument("--port", required=True, help="serial port, for example COM7")
    parser.add_argument("--baud", type=int, default=38400,
                        help="must match the firmware USART2 baud (lowered to "
                             "38400 for link reliability)")
    parser.add_argument("--knock-retries", type=int, default=240)
    parser.add_argument(
        "--uart-test",
        action="store_true",
        help="isolate the device->host UART link (no SPI/flash, read-only)",
    )
    parser.add_argument(
        "--attempts",
        type=int,
        default=6,
        help="auto-retry the whole programming session on a transient link "
             "glitch (dropped/garbled byte) instead of giving up",
    )
    parser.add_argument(
        "--diagnose",
        action="store_true",
        help="run read-only SPI flash diagnostics; never erase or program",
    )
    args = parser.parse_args()

    if serial is None:
        raise SystemExit("pyserial is required: pip install pyserial")

    if not args.diagnose and not args.uart_test and not args.binary:
        parser.error("binary is required unless --diagnose or --uart-test is used")

    try:
        with serial.Serial(args.port, args.baud, timeout=HANDSHAKE_TIMEOUT) as ser:
            if args.uart_test:
                uart_test(ser, args.knock_retries)
            elif args.diagnose:
                diagnose_flash(ser, args.knock_retries)
            else:
                with open(args.binary, "rb") as asset_file:
                    data = asset_file.read()
                _program_with_retries(ser, data, args.knock_retries,
                                      args.attempts)
    except FlashProtocolError as error:
        raise SystemExit(f"FAILED: {error}") from error


def _program_with_retries(ser, data, knock_retries, attempts, output=print):
    """Run the full programming session, auto-retrying on a transient link
    glitch. A single dropped/garbled byte aborts one session, but the device
    returns to recovery and waits for a fresh knock, so we simply restart the
    whole sequence (re-knock, re-erase, re-stream). Re-erasing is harmless."""
    last_error = None
    for attempt in range(1, max(1, attempts) + 1):
        try:
            program_assets(ser, data, knock_retries, output)
            return
        except FlashProtocolError as error:
            last_error = error
            if attempt >= attempts:
                break
            output(f"attempt {attempt}/{attempts} failed: {error}")
            output("transient link glitch; restarting the whole session "
                   "(re-knock, re-erase, re-stream)...")
            # Let any in-progress device session time out and fall back to the
            # recovery knock-wait before we knock again.
            time.sleep(6.0)
            ser.reset_input_buffer()
    raise FlashProtocolError(
        f"gave up after {attempts} attempts; last error: {last_error}"
    )


if __name__ == "__main__":
    main()
