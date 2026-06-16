#!/usr/bin/env python3
"""Program TouchGFX assets onto the external Winbond W25Q64 over USART2.

Typical usage:

    python Tools/flash_assets.py --port COM7 assets.bin

Recovery mode:
    Start this tool, hold the Nucleo USER button, press RESET, then release USER.

Extra checks:
    python Tools/flash_assets.py --port COM7 --uart-test
    python Tools/flash_assets.py --port COM7 --diagnose

Protocol summary:
    ADHTPROG -> H + JEDEC[3]
    ADHTHEAD + len + crc + package_id -> A
    erase progress: S + current:u16 + total:u16, then R
    chunks: host sends exactly 256 bytes except the final partial chunk;
            device replies K + next_offset:u32 after each chunk
    verification: V, then D

The script is intentionally stop-and-wait: it never sends the next chunk until
the MCU acknowledges the previous one.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import time
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional

try:
    import serial
except ImportError:
    serial = None


KNOCK = b"ADHTPROG"
DIAGNOSTIC_KNOCK = b"ADHTDIAG"
UART_TEST_KNOCK = b"ADHTUART"

# Legacy firmware replies with b"U" and immediately starts the counter stream.
# A future safer firmware may reply with b"UOK0" and then start the stream.
UART_TEST_REPLY_LEGACY = b"U"
UART_TEST_REPLY_STRONG_TAIL = b"OK0"
UART_TEST_MODE_TX = b"T"
UART_TEST_MODE_RX = b"R"
UART_TEST_CONFIG_MAGIC = b"UCFG"
UART_TEST_RESULT_MAGIC = b"URES"

UART_TEST_BYTES = 65536
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

DEFAULT_BAUD = 38400

HANDSHAKE_TIMEOUT = 0.5
HEADER_TIMEOUT = 5.0
ERASE_TIMEOUT = 120.0
CHUNK_TIMEOUT = 5.0
VERIFY_TIMEOUT = 60.0
RECOVERY_SETTLE_SECONDS = 6.0

ERROR_MESSAGES = {
    ERROR_FLASH_ID: "external flash not detected or unexpected JEDEC ID",
    ERROR_HEADER: "invalid asset header or image length; regenerate assets.bin and asset_manifest together, rebuild firmware, then retry",
    ERROR_UART_TIMEOUT: "UART timeout",
    ERROR_ERASE: "flash sector erase failed",
    ERROR_PROGRAM: "flash page programming failed",
    ERROR_CRC: "CRC mismatch in the received asset stream",
    ERROR_MANIFEST: "asset manifest write failed",
    ERROR_VERIFY: "flash read-back verification failed",
}


class FlashProtocolError(RuntimeError):
    """Base protocol error."""


class DeviceError(FlashProtocolError):
    """Error explicitly reported by the device as E + reason."""

    def __init__(self, where: str, reason: bytes):
        self.where = where
        self.reason = reason
        message = ERROR_MESSAGES.get(reason, f"unknown device error {reason!r}")
        super().__init__(f"{where}: {message}")


@dataclass(frozen=True)
class CounterReport:
    length: int
    errors: int
    first_bad: Optional[int]
    expected: Optional[int]
    got: Optional[int]


@dataclass(frozen=True)
class UartConfigReport:
    baud: int
    system_core_clock: int
    pclk1: int
    brr: int
    cr1: int
    cr2: int
    cr3: int


def _format_hex(data: bytes | bytearray) -> str:
    return " ".join(f"{byte:02X}" for byte in data)



def _uart_test_timeout_for_baud(baud: int, override_s: Optional[float] = None) -> float:
    """Return a safe absolute timeout for the UART counter stream.

    UART is 8N1, so every byte costs 10 line bits. Add margin for USB/VCP,
    Python scheduling and the handshake bytes.
    """
    if override_s is not None:
        return max(1.0, override_s)

    if baud <= 0:
        return UART_TEST_TIMEOUT

    ideal_s = (UART_TEST_BYTES * 10.0) / float(baud)
    return max(UART_TEST_TIMEOUT, ideal_s * 1.8 + 5.0)


def check_counter_stream(data: bytes) -> CounterReport:
    """Verify byte[i] == i & 0xFF."""
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

    return CounterReport(
        length=len(data),
        errors=errors,
        first_bad=first_bad,
        expected=expected_byte,
        got=got_byte,
    )


def _read_exact(ser, size: int, timeout: float, where: str) -> bytes:
    """Read exactly size bytes within one absolute timeout.

    pyserial's timeout applies per read call. This wrapper uses a real deadline
    so a stream of tiny partial reads cannot extend the wait indefinitely.
    """
    previous_timeout = getattr(ser, "timeout", None)
    deadline = time.monotonic() + timeout
    data = bytearray()

    try:
        while len(data) < size:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise FlashProtocolError(
                    f"timeout while waiting for {where}: "
                    f"got {len(data)}/{size} bytes"
                )

            ser.timeout = min(0.25, remaining)
            part = ser.read(size - len(data))
            if part:
                data.extend(part)

        return bytes(data)
    finally:
        ser.timeout = previous_timeout


def _read_code(ser, timeout: float, where: str) -> bytes:
    return _read_exact(ser, 1, timeout, where)


def _read_device_error_reason(ser, where: str) -> bytes:
    return _read_exact(ser, 1, HEADER_TIMEOUT, f"{where} error code")


def _raise_device_error(ser, where: str) -> None:
    reason = _read_device_error_reason(ser, where)
    raise DeviceError(where, reason)


def _is_stale_recovery_error(reason: bytes) -> bool:
    """Errors that may be emitted by a previous programming session."""
    return reason in {
        ERROR_HEADER,
        ERROR_UART_TIMEOUT,
        ERROR_ERASE,
        ERROR_PROGRAM,
        ERROR_CRC,
        ERROR_MANIFEST,
        ERROR_VERIFY,
    }


def _warn_stale_error(output: Callable[[str], None], where: str, reason: bytes) -> None:
    message = ERROR_MESSAGES.get(reason, f"unknown device error {reason!r}")
    output(f"warning: ignored stale {where} error from previous session: {message}")


def _settle_recovery_after_stale_error(
    ser,
    output: Callable[[str], None],
    sleep_fn: Callable[[float], None],
) -> None:
    output("waiting for any old asset-stream session to time out before retrying...")
    sleep_fn(RECOVERY_SETTLE_SECONDS)
    ser.reset_input_buffer()
    if hasattr(ser, "reset_output_buffer"):
        ser.reset_output_buffer()


def _write_all(ser, data: bytes, where: str) -> None:
    written = ser.write(data)
    ser.flush()
    if written != len(data):
        raise FlashProtocolError(
            f"{where}: wrote {written}/{len(data)} bytes to serial port"
        )


def _read_one_with_timeout(ser, timeout: float) -> bytes:
    previous_timeout = getattr(ser, "timeout", None)
    ser.timeout = timeout
    try:
        return ser.read(1)
    finally:
        ser.timeout = previous_timeout


def _wait_for_hello(
    ser,
    knock_retries: int,
    output: Callable[[str], None],
    sleep_fn: Callable[[float], None] = time.sleep,
) -> None:
    ser.reset_input_buffer()
    output(
        "knocking... board should be in recovery (black screen); "
        "tap RESET once if it doesn't connect"
    )

    unexpected = bytearray()

    for _attempt in range(knock_retries):
        _write_all(ser, KNOCK, "program knock")
        reply = _read_one_with_timeout(ser, HANDSHAKE_TIMEOUT)

        if not reply:
            continue

        if reply == ERROR:
            reason = _read_device_error_reason(ser, "handshake")
            if _is_stale_recovery_error(reason):
                _warn_stale_error(output, "handshake", reason)
                _settle_recovery_after_stale_error(ser, output, sleep_fn)
                continue
            raise DeviceError("handshake", reason)

        if reply != HELLO:
            unexpected.extend(reply)
            if len(unexpected) <= 16:
                output(f"warning: ignored unexpected handshake byte: {_format_hex(reply)}")
            continue

        jedec_id = _read_exact(ser, 3, HEADER_TIMEOUT, "JEDEC ID")
        output("connected: JEDEC ID " + _format_hex(jedec_id))

        if jedec_id != EXPECTED_JEDEC_ID:
            error_reply = _read_exact(ser, 2, HEADER_TIMEOUT, "JEDEC error response")
            if error_reply != ERROR + ERROR_FLASH_ID:
                raise FlashProtocolError(
                    "unexpected response after invalid JEDEC ID: "
                    + _format_hex(error_reply)
                )

            raise FlashProtocolError(
                "unexpected JEDEC ID "
                + _format_hex(jedec_id)
                + ", expected EF 40 17"
            )

        return

    if unexpected:
        output("last unexpected handshake bytes: " + _format_hex(unexpected[-16:]))

    raise FlashProtocolError(
        "no HELLO from device; hold USER while pressing RESET, "
        "verify the internal-only firmware image was downloaded, and verify baudrate"
    )


def _wait_for_diagnostic_reply(
    ser,
    knock_retries: int,
    output: Callable[[str], None],
    sleep_fn: Callable[[float], None] = time.sleep,
) -> None:
    ser.reset_input_buffer()
    output("requesting read-only SPI diagnostic; tap RESET once if it doesn't connect")

    for _attempt in range(knock_retries):
        _write_all(ser, DIAGNOSTIC_KNOCK, "diagnostic knock")
        reply = _read_one_with_timeout(ser, HANDSHAKE_TIMEOUT)

        if not reply:
            continue

        if reply == ERROR:
            reason = _read_device_error_reason(ser, "diagnostic handshake")
            if _is_stale_recovery_error(reason):
                _warn_stale_error(output, "diagnostic handshake", reason)
                _settle_recovery_after_stale_error(ser, output, sleep_fn)
                continue
            raise DeviceError("diagnostic handshake", reason)

        if reply == DIAGNOSTIC_REPLY:
            return

    raise FlashProtocolError(
        "no diagnostic reply from device; verify the updated internal firmware "
        "is running in recovery"
    )


def _read_byte(ser, timeout: float, where: str) -> int:
    return _read_exact(ser, 1, timeout, where)[0]


def _read_diagnostic_header(ser) -> tuple[int, int]:
    scanned = bytearray()

    while len(scanned) < DIAGNOSTIC_SCAN_LIMIT:
        scanned.append(_read_byte(ser, HEADER_TIMEOUT, "diagnostic header"))

        if scanned.endswith(DIAGNOSTIC_HEADER_MAGIC):
            version_and_count = _read_exact(
                ser, 2, HEADER_TIMEOUT, "diagnostic report header"
            )
            return version_and_count[0], version_and_count[1]

        if scanned.endswith(DIAGNOSTIC_LEGACY_HEADER_PREFIX):
            tail = _read_exact(ser, 2, HEADER_TIMEOUT, "diagnostic legacy header tail")
            if tail == b"OK":
                version_and_count = _read_exact(
                    ser, 2, HEADER_TIMEOUT, "diagnostic report header"
                )
                return version_and_count[0], version_and_count[1]

            if tail[0] == DIAGNOSTIC_VERSION and 0 < tail[1] <= 4:
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


def _read_diagnostic_record_magic(ser) -> None:
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


def _read_diagnostic_record(ser) -> dict:
    _read_diagnostic_record_magic(ser)

    header = _read_exact(ser, 2, HEADER_TIMEOUT, "diagnostic mode header")
    mode = header[0]
    stable_count = header[1]

    if mode not in (0, 3):
        raise FlashProtocolError(f"diagnostic report contains invalid SPI mode {mode}")

    if stable_count > DIAGNOSTIC_SAMPLE_COUNT:
        raise FlashProtocolError(
            f"diagnostic report has invalid stable count "
            f"{stable_count}/{DIAGNOSTIC_SAMPLE_COUNT}"
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


def _read_uart_config_report(ser) -> UartConfigReport:
    magic = _read_exact(ser, 4, HEADER_TIMEOUT, "UART-test config magic")
    if magic != UART_TEST_CONFIG_MAGIC:
        raise FlashProtocolError(
            "UART-test config framing mismatch: got "
            + _format_hex(magic)
            + ", expected "
            + _format_hex(UART_TEST_CONFIG_MAGIC)
        )

    fields = struct.unpack(
        "<IIIIIII",
        _read_exact(ser, 7 * 4, HEADER_TIMEOUT, "UART-test config payload"),
    )
    return UartConfigReport(*fields)


def _print_uart_config(config: UartConfigReport, output: Callable[[str], None]) -> None:
    output(
        "UART config: "
        f"baud={config.baud}, "
        f"SystemCoreClock={config.system_core_clock}, "
        f"PCLK1={config.pclk1}, "
        f"BRR=0x{config.brr:08X}, "
        f"CR1=0x{config.cr1:08X}, "
        f"CR2=0x{config.cr2:08X}, "
        f"CR3=0x{config.cr3:08X}"
    )


def diagnose_flash(
    ser,
    knock_retries: int = 50,
    output: Callable[[str], None] = print,
    sleep_fn: Callable[[float], None] = time.sleep,
) -> dict:
    _wait_for_diagnostic_reply(ser, knock_retries, output, sleep_fn)

    version, mode_count = _read_diagnostic_header(ser)
    if version != DIAGNOSTIC_VERSION:
        raise FlashProtocolError(
            f"unsupported diagnostic version {version}, expected {DIAGNOSTIC_VERSION}"
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


def _send_header(ser, data: bytes) -> int:
    crc = zlib.crc32(data) & 0xFFFFFFFF
    package_id = hashlib.sha256(data).digest()[:PACKAGE_ID_SIZE]
    header = HEADER_MAGIC + struct.pack("<II", len(data), crc) + package_id

    _write_all(ser, header, "asset header")

    reply = _read_code(ser, HEADER_TIMEOUT, "header acknowledgement")
    if reply == ERROR:
        _raise_device_error(ser, "header")

    if reply != HEADER_ACCEPTED:
        raise FlashProtocolError(
            f"header: expected {HEADER_ACCEPTED!r}, got {reply!r}"
        )

    return crc


def _wait_for_erase(ser, output: Callable[[str], None]) -> None:
    # Silent erase: the device no longer streams per-sector progress (each
    # message was a device->host packet an imperfect link could corrupt, which
    # is where the transfer kept failing). It sends a single READY ('R') once
    # the whole erase completes.
    output("erasing external flash (silent, up to ~1 min)...")
    reply = _read_code(ser, ERASE_TIMEOUT, "erase ready")

    if reply == ERROR:
        _raise_device_error(ser, "erase")

    if reply != READY:
        raise FlashProtocolError(f"erase: expected {READY!r}, got {reply!r}")

    output("erase complete")


def _stream_data(
    ser,
    data: bytes,
    output: Callable[[str], None],
    delay_after_chunk_s: float = 0.0,
) -> None:
    sent = 0

    while sent < len(data):
        chunk = data[sent:sent + CHUNK]
        _write_all(ser, chunk, f"chunk at offset {sent}")

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
                f"device acknowledged offset {next_offset}, expected {expected_offset}"
            )

        sent = next_offset
        output(f"programming: {sent}/{len(data)} bytes")

        if delay_after_chunk_s > 0.0:
            time.sleep(delay_after_chunk_s)


def _wait_for_verification(ser, output: Callable[[str], None]) -> None:
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


def program_assets(
    ser,
    data: bytes,
    knock_retries: int = 50,
    output: Callable[[str], None] = print,
    sleep_fn: Callable[[float], None] = time.sleep,
    delay_after_chunk_s: float = 0.0,
) -> None:
    if not data:
        raise FlashProtocolError("asset image is empty")

    crc = zlib.crc32(data) & 0xFFFFFFFF
    package_id = hashlib.sha256(data).digest()[:PACKAGE_ID_SIZE]

    output(f"image: {len(data)} bytes, crc32=0x{crc:08X}, package_id={package_id.hex()}")

    _wait_for_hello(ser, knock_retries, output, sleep_fn)

    sent_crc = _send_header(ser, data)
    if sent_crc != crc:
        raise FlashProtocolError("internal host CRC error")

    output("header accepted; erasing external flash...")
    _wait_for_erase(ser, output)

    _stream_data(ser, data, output, delay_after_chunk_s)

    _wait_for_verification(ser, output)


def _wait_for_uart_test(
    ser,
    knock_retries: int,
    output: Callable[[str], None],
    mode: bytes,
    chunk_delay_ms: int,
) -> tuple[Optional[UartConfigReport], bytes]:
    """Wait for UART test and return config plus any stream prefix consumed.

    Supports:
    - legacy firmware: U + 00 01 02 03 ...
    - stronger firmware: UOK0 + UCFG + config, then stream/result

    Important: with legacy firmware, if the first stream bytes are already
    corrupted or dropped, that is exactly what the UART test is supposed to
    measure. Therefore we must NOT reject the test just because the prefix is
    not 00 01 02 03. We accept the stream and let check_counter_stream() report
    the real first mismatch.
    """
    ser.reset_input_buffer()
    output("requesting UART link test; tap RESET once if it doesn't connect")

    unexpected = bytearray()
    delay = max(0, min(65535, int(chunk_delay_ms)))
    request = UART_TEST_KNOCK + mode + struct.pack("<H", delay)

    for _attempt in range(knock_retries):
        _write_all(ser, request, "UART-test knock")
        reply = _read_one_with_timeout(ser, HANDSHAKE_TIMEOUT)

        if not reply:
            continue

        if reply == ERROR:
            _raise_device_error(ser, "uart test handshake")

        if reply != UART_TEST_REPLY_LEGACY:
            unexpected.extend(reply)
            if len(unexpected) <= 16:
                output(f"warning: ignored unexpected UART-test byte: {_format_hex(reply)}")
            continue

        # We got the legacy 'U' reply. Read the next three bytes. They might be:
        #   OK0          -> strong handshake; config follows
        #   00 01 02     -> legacy stream prefix, clean so far
        #   anything else -> legacy stream already corrupted; still accept it
        #                    and let the counter checker report the error.
        next3 = _read_exact(ser, 3, HEADER_TIMEOUT, "UART-test preamble")

        if next3 == UART_TEST_REPLY_STRONG_TAIL:
            config = _read_uart_config_report(ser)
            _print_uart_config(config, output)
            return config, b""

        stream_prefix = next3 + _read_exact(
            ser, 1, HEADER_TIMEOUT, "UART-test legacy stream prefix"
        )

        if stream_prefix != b"\x00\x01\x02\x03":
            output(
                "warning: UART-test stream starts corrupted or with dropped bytes; "
                f"prefix was {_format_hex(stream_prefix)}, expected 00 01 02 03"
            )

        return None, stream_prefix

    if unexpected:
        output("last unexpected UART-test bytes: " + _format_hex(unexpected[-16:]))

    raise FlashProtocolError(
        "no UART-test reply from device; verify ADHTUART is implemented "
        "in the firmware you actually flashed, and verify baudrate"
    )


def uart_test_tx(
    ser,
    knock_retries: int = 240,
    output: Callable[[str], None] = print,
    baud: int = DEFAULT_BAUD,
    timeout_override_s: Optional[float] = None,
    chunk_delay_ms: int = 0,
) -> CounterReport:
    """Isolate the device->host UART link. No SPI, no flash."""
    _config, stream_prefix = _wait_for_uart_test(
        ser,
        knock_retries,
        output,
        UART_TEST_MODE_TX,
        chunk_delay_ms,
    )
    if not stream_prefix:
        stream_prefix = _read_exact(
            ser, 4, HEADER_TIMEOUT, "UART-test TX stream prefix"
        )

    timeout_s = _uart_test_timeout_for_baud(baud, timeout_override_s)
    ideal_s = (UART_TEST_BYTES * 10.0) / float(baud) if baud > 0 else 0.0
    output(
        f"receiving {UART_TEST_BYTES} bytes of counter pattern "
        f"(ideal line time at {baud} baud: {ideal_s:.1f}s, timeout: {timeout_s:.1f}s)..."
    )

    started_at = time.monotonic()
    data = bytearray(stream_prefix)
    deadline = time.monotonic() + timeout_s

    previous_timeout = getattr(ser, "timeout", None)
    try:
        while len(data) < UART_TEST_BYTES and time.monotonic() < deadline:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break

            ser.timeout = min(0.25, remaining)
            part = ser.read(UART_TEST_BYTES - len(data))

            if part:
                data.extend(part)
            # If no bytes arrive, keep waiting until the absolute deadline.
    finally:
        ser.timeout = previous_timeout

    report = check_counter_stream(bytes(data))
    elapsed_s = max(0.001, time.monotonic() - started_at)
    throughput = report.length / elapsed_s
    missing = UART_TEST_BYTES - report.length

    output(
        f"UART test: received {report.length}/{UART_TEST_BYTES} bytes, "
        f"missing {missing}, mismatches {report.errors}, "
        f"elapsed {elapsed_s:.1f}s, throughput {throughput:.1f} B/s"
    )

    if report.first_bad is not None:
        output(
            f"  first mismatch at byte {report.first_bad}: "
            f"expected 0x{report.expected:02X}, got 0x{report.got:02X}"
        )

    if report.errors == 0 and missing == 0:
        output("OK: device->host UART link is clean")
    else:
        output("FAIL: UART link drops/corrupts bytes (device->host)")

    return report


def uart_test_rx(
    ser,
    knock_retries: int = 240,
    output: Callable[[str], None] = print,
    baud: int = DEFAULT_BAUD,
    timeout_override_s: Optional[float] = None,
    chunk_delay_ms: int = 0,
) -> CounterReport:
    """Isolate the host->device UART link. No SPI, no flash."""
    config, legacy_prefix = _wait_for_uart_test(
        ser,
        knock_retries,
        output,
        UART_TEST_MODE_RX,
        chunk_delay_ms,
    )
    if config is None or legacy_prefix:
        raise FlashProtocolError(
            "UART RX test requires firmware with UOK0 + UCFG support"
        )

    timeout_s = _uart_test_timeout_for_baud(baud, timeout_override_s)
    ideal_s = (UART_TEST_BYTES * 10.0) / float(baud) if baud > 0 else 0.0
    output(
        f"sending {UART_TEST_BYTES} bytes of counter pattern "
        f"(ideal line time at {baud} baud: {ideal_s:.1f}s, timeout: {timeout_s:.1f}s)..."
    )

    started_at = time.monotonic()
    sent = 0
    while sent < UART_TEST_BYTES:
        chunk_size = min(CHUNK, UART_TEST_BYTES - sent)
        chunk = bytes(((sent + index) & 0xFF) for index in range(chunk_size))
        _write_all(ser, chunk, f"UART RX-test chunk at offset {sent}")
        sent += chunk_size
        if chunk_delay_ms > 0:
            time.sleep(chunk_delay_ms / 1000.0)

    magic = _read_exact(ser, 4, timeout_s, "UART RX-test result magic")
    if magic != UART_TEST_RESULT_MAGIC:
        raise FlashProtocolError(
            "UART RX-test result framing mismatch: got "
            + _format_hex(magic)
            + ", expected "
            + _format_hex(UART_TEST_RESULT_MAGIC)
        )

    payload = _read_exact(ser, 14, HEADER_TIMEOUT, "UART RX-test result payload")
    received, errors, first_bad = struct.unpack("<III", payload[:12])
    expected = payload[12]
    got = payload[13]
    elapsed_s = max(0.001, time.monotonic() - started_at)
    throughput = received / elapsed_s
    missing = UART_TEST_BYTES - received
    first_bad_value = None if first_bad == 0xFFFFFFFF else first_bad
    expected_value = None if first_bad_value is None else expected
    got_value = None if first_bad_value is None else got
    report = CounterReport(
        length=received,
        errors=errors,
        first_bad=first_bad_value,
        expected=expected_value,
        got=got_value,
    )

    output(
        f"UART RX test: device received {received}/{UART_TEST_BYTES} bytes, "
        f"missing {missing}, mismatches {errors}, "
        f"elapsed {elapsed_s:.1f}s, throughput {throughput:.1f} B/s"
    )
    if report.first_bad is not None:
        output(
            f"  first mismatch at byte {report.first_bad}: "
            f"expected 0x{report.expected:02X}, got 0x{report.got:02X}"
        )

    if report.errors == 0 and missing == 0:
        output("OK: host->device UART link is clean")
    else:
        output("FAIL: UART link drops/corrupts bytes (host->device)")

    return report


def uart_test_repeat(
    ser,
    direction: str,
    repeat: int,
    knock_retries: int,
    output: Callable[[str], None],
    baud: int,
    timeout_override_s: Optional[float],
    chunk_delay_ms: int,
) -> None:
    clean = 0
    total = max(1, repeat)

    for attempt in range(1, total + 1):
        output(f"UART {direction} run {attempt}/{total}")
        if direction == "rx":
            report = uart_test_rx(
                ser,
                knock_retries,
                output,
                baud,
                timeout_override_s,
                chunk_delay_ms,
            )
        else:
            report = uart_test_tx(
                ser,
                knock_retries,
                output,
                baud,
                timeout_override_s,
                chunk_delay_ms,
            )

        if report.errors == 0 and report.length == UART_TEST_BYTES:
            clean += 1

        time.sleep(0.2)
        ser.reset_input_buffer()
        ser.reset_output_buffer()

    output(f"UART {direction} summary: {clean}/{total} clean runs")
    if clean != total:
        raise FlashProtocolError(
            f"UART {direction} test failed: {clean}/{total} clean runs"
        )


# Backward-compatible name for older tests/imports.
def uart_test(*args, **kwargs) -> CounterReport:
    return uart_test_tx(*args, **kwargs)


def _is_retryable_error(error: FlashProtocolError) -> bool:
    if isinstance(error, DeviceError):
        # Header mismatch usually means assets.bin does not match the manifest
        # compiled into the firmware. Retrying the same file will not help.
        if error.reason in {ERROR_HEADER, ERROR_FLASH_ID}:
            return False

        # Program/CRC/timeout can be transient link/state problems.
        return error.reason in {
            ERROR_UART_TIMEOUT,
            ERROR_PROGRAM,
            ERROR_CRC,
            ERROR_ERASE,
            ERROR_MANIFEST,
            ERROR_VERIFY,
        }

    message = str(error).lower()

    non_retryable_fragments = [
        "asset image is empty",
        "unexpected jedec id",
        "invalid asset header",
        "no hello from device",
        "no diagnostic reply",
        "no valid uart-test reply",
    ]

    return not any(fragment in message for fragment in non_retryable_fragments)


def _program_with_retries(
    ser,
    data: bytes,
    knock_retries: int,
    attempts: int,
    output: Callable[[str], None] = print,
    delay_after_chunk_s: float = 0.0,
) -> None:
    last_error = None
    max_attempts = max(1, attempts)

    for attempt in range(1, max_attempts + 1):
        try:
            program_assets(
                ser,
                data,
                knock_retries,
                output,
                delay_after_chunk_s=delay_after_chunk_s,
            )
            return

        except FlashProtocolError as error:
            last_error = error

            if not _is_retryable_error(error):
                raise

            if attempt >= max_attempts:
                break

            output(f"attempt {attempt}/{max_attempts} failed: {error}")
            output(
                "transient link/device-state glitch; restarting the whole session "
                "(re-knock, re-erase, re-stream)..."
            )

            time.sleep(RECOVERY_SETTLE_SECONDS)
            ser.reset_input_buffer()
            ser.reset_output_buffer()

    raise FlashProtocolError(
        f"gave up after {max_attempts} attempts; last error: {last_error}"
    )


def _open_serial(port: str, baud: int):
    if serial is None:
        raise SystemExit("pyserial is required: pip install pyserial")

    ser = serial.Serial(
        port=port,
        baudrate=baud,
        timeout=HANDSHAKE_TIMEOUT,
        write_timeout=5.0,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    )

    # Keep modem-control lines inactive. This avoids accidental resets on boards
    # or adapters that wire DTR/RTS to reset/boot pins.
    try:
        ser.dtr = False
        ser.rts = False
    except (AttributeError, OSError, serial.SerialException):
        pass

    time.sleep(0.2)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    return ser


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument(
        "binary",
        nargs="?",
        help="asset image, for example assets.bin; omit with --diagnose or --uart-test",
    )

    parser.add_argument("--port", required=True, help="serial port, for example COM7")

    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help=f"must match firmware USART2 baud; default: {DEFAULT_BAUD}",
    )

    parser.add_argument("--knock-retries", type=int, default=240)

    parser.add_argument(
        "--uart-test",
        action="store_true",
        help="alias for --uart-test-tx; no SPI/flash",
    )

    parser.add_argument(
        "--uart-test-tx",
        action="store_true",
        help="test STM32->PC UART stream; no SPI/flash",
    )

    parser.add_argument(
        "--uart-test-rx",
        action="store_true",
        help="test PC->STM32 UART stream; no SPI/flash",
    )

    parser.add_argument(
        "--uart-test-repeat",
        type=int,
        default=1,
        help="number of consecutive UART test runs required to be clean",
    )

    parser.add_argument(
        "--uart-test-chunk-delay-ms",
        type=int,
        default=0,
        help="wait this many ms between UART-test chunks",
    )

    parser.add_argument(
        "--uart-test-timeout-s",
        type=float,
        default=None,
        help="override UART-test stream timeout; by default it is computed from baudrate",
    )

    parser.add_argument(
        "--diagnose",
        action="store_true",
        help="run read-only SPI flash diagnostics; never erase or program",
    )

    parser.add_argument(
        "--attempts",
        type=int,
        default=6,
        help="auto-retry the whole programming session on transient link glitches",
    )

    parser.add_argument(
        "--delay-after-chunk-ms",
        type=float,
        default=0.0,
        help="debug option: wait this many ms after every K ACK before next chunk",
    )

    args = parser.parse_args()

    uart_test_requested = args.uart_test or args.uart_test_tx or args.uart_test_rx

    if not args.diagnose and not uart_test_requested and not args.binary:
        parser.error("binary is required unless --diagnose or a UART test is used")

    if args.diagnose and uart_test_requested:
        parser.error("--diagnose and UART tests are mutually exclusive")

    if args.uart_test_rx and (args.uart_test or args.uart_test_tx):
        parser.error("choose only one UART direction test at a time")

    delay_after_chunk_s = max(0.0, args.delay_after_chunk_ms) / 1000.0

    try:
        with _open_serial(args.port, args.baud) as ser:
            if uart_test_requested:
                direction = "rx" if args.uart_test_rx else "tx"
                uart_test_repeat(
                    ser,
                    direction,
                    args.uart_test_repeat,
                    args.knock_retries,
                    print,
                    baud=args.baud,
                    timeout_override_s=args.uart_test_timeout_s,
                    chunk_delay_ms=max(0, args.uart_test_chunk_delay_ms),
                )
                return

            if args.diagnose:
                diagnose_flash(ser, args.knock_retries)
                return

            binary_path = Path(args.binary)
            data = binary_path.read_bytes()

            _program_with_retries(
                ser,
                data,
                args.knock_retries,
                args.attempts,
                delay_after_chunk_s=delay_after_chunk_s,
            )

    except FlashProtocolError as error:
        raise SystemExit(f"FAILED: {error}") from error


if __name__ == "__main__":
    main()
