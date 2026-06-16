import struct
import unittest
import hashlib

from Tools import flash_assets
from Tools import build_asset_metadata


class FakeSerial:
    def __init__(self, read_chunks):
        self.read_chunks = list(read_chunks)
        self.writes = []
        self.timeout = 2
        self.reset_count = 0

    def read(self, size):
        if not self.read_chunks:
            return b""

        chunk = self.read_chunks.pop(0)
        if len(chunk) <= size:
            return chunk

        self.read_chunks.insert(0, chunk[size:])
        return chunk[:size]

    def write(self, data):
        self.writes.append(bytes(data))
        return len(data)

    def flush(self):
        pass

    def reset_input_buffer(self):
        self.reset_count += 1

    def reset_output_buffer(self):
        pass


def progress(current, total):
    return flash_assets.ERASE_PROGRESS + struct.pack("<HH", current, total)


def ack(next_offset):
    return flash_assets.CHUNK_OK + struct.pack("<I", next_offset)


def diagnostic_record(mode, samples, id90=b"\xEF\x16", release_id=0x16,
                      status=b"\x00\x02\x60", sfdp=b"SFDP"):
    stable_count = sum(
        sample == flash_assets.EXPECTED_JEDEC_ID for sample in samples
    )
    return (
        flash_assets.DIAGNOSTIC_RECORD_MAGIC
        +
        bytes((mode, stable_count))
        + b"".join(samples)
        + id90
        + bytes((release_id,))
        + status
        + sfdp
    )


def uart_config_report():
    return (
        flash_assets.UART_TEST_CONFIG_MAGIC
        + struct.pack(
            "<IIIIIII",
            9600,
            84000000,
            42000000,
            0x1117,
            0x200C,
            0x0000,
            0x0000,
        )
    )


class FlashAssetsProtocolTests(unittest.TestCase):
    def test_manifest_v2_contains_crc_and_package_identity(self):
        data = b"current-touchgfx-assets"
        manifest, metadata = build_asset_metadata.build_manifest(data)

        self.assertEqual(len(manifest), build_asset_metadata.MANIFEST_SIZE)
        magic, version, length, crc = struct.unpack("<IIII", manifest[:16])
        self.assertEqual(magic, build_asset_metadata.MAGIC)
        self.assertEqual(version, build_asset_metadata.VERSION)
        self.assertEqual(length, len(data))
        self.assertEqual(crc, flash_assets.zlib.crc32(data) & 0xFFFFFFFF)
        self.assertEqual(
            manifest[16:],
            hashlib.sha256(data).digest()[:flash_assets.PACKAGE_ID_SIZE],
        )
        self.assertEqual(metadata["data_length"], len(data))

    def test_times_out_without_hello_and_sends_only_knocks(self):
        serial_port = FakeSerial([b"", b""])

        with self.assertRaisesRegex(
            flash_assets.FlashProtocolError,
            "no HELLO",
        ):
            flash_assets.program_assets(
                serial_port,
                b"asset",
                knock_retries=2,
                output=lambda _message: None,
            )

        self.assertEqual(serial_port.writes, [flash_assets.KNOCK] * 2)

    def test_retries_only_knock_before_sending_header_once(self):
        data = bytes(range(256)) + b"tail"
        serial_port = FakeSerial([
            b"",
            b"",
            flash_assets.HELLO,
            flash_assets.EXPECTED_JEDEC_ID,
            flash_assets.HEADER_ACCEPTED,
            flash_assets.READY,
            ack(256),
            ack(len(data)),
            flash_assets.VERIFYING,
            flash_assets.DONE_OK,
        ])

        flash_assets.program_assets(
            serial_port,
            data,
            knock_retries=5,
            output=lambda _message: None,
        )

        self.assertEqual(serial_port.writes[:3], [flash_assets.KNOCK] * 3)
        header_writes = [
            write for write in serial_port.writes
            if write.startswith(flash_assets.HEADER_MAGIC)
        ]
        self.assertEqual(len(header_writes), 1)
        expected_id = hashlib.sha256(data).digest()[:flash_assets.PACKAGE_ID_SIZE]
        self.assertEqual(
            header_writes[0],
            flash_assets.HEADER_MAGIC
            + struct.pack("<II", len(data), flash_assets.zlib.crc32(data) & 0xFFFFFFFF)
            + expected_id,
        )
        self.assertEqual(serial_port.reset_count, 1)

    def test_rejects_wrong_chunk_ack_offset(self):
        data = b"x" * 300
        serial_port = FakeSerial([
            flash_assets.HELLO,
            flash_assets.EXPECTED_JEDEC_ID,
            flash_assets.HEADER_ACCEPTED,
            flash_assets.READY,
            ack(255),
        ])

        with self.assertRaisesRegex(
            flash_assets.FlashProtocolError,
            "offset 255.*expected 256",
        ):
            flash_assets.program_assets(
                serial_port,
                data,
                output=lambda _message: None,
            )

    def test_reports_crc_error_distinctly(self):
        data = b"asset-data"
        serial_port = FakeSerial([
            flash_assets.HELLO,
            flash_assets.EXPECTED_JEDEC_ID,
            flash_assets.HEADER_ACCEPTED,
            flash_assets.READY,
            ack(len(data)),
            flash_assets.ERROR,
            flash_assets.ERROR_CRC,
        ])

        with self.assertRaisesRegex(
            flash_assets.FlashProtocolError,
            "CRC mismatch",
        ):
            flash_assets.program_assets(
                serial_port,
                data,
                output=lambda _message: None,
            )

    def test_wrong_jedec_consumes_complete_device_error(self):
        serial_port = FakeSerial([
            flash_assets.HELLO,
            b"\xEF\x45\x54",
            flash_assets.ERROR,
            flash_assets.ERROR_FLASH_ID,
        ])

        with self.assertRaisesRegex(
            flash_assets.FlashProtocolError,
            "unexpected JEDEC ID EF 45 54",
        ):
            flash_assets.program_assets(
                serial_port,
                b"asset",
                knock_retries=1,
                output=lambda _message: None,
            )

        self.assertEqual(serial_port.read_chunks, [])

    def test_programming_ignores_stale_error_before_hello(self):
        data = b"asset-data"
        serial_port = FakeSerial([
            flash_assets.ERROR,
            flash_assets.ERROR_CRC,
            flash_assets.HELLO,
            flash_assets.EXPECTED_JEDEC_ID,
            flash_assets.HEADER_ACCEPTED,
            flash_assets.READY,
            ack(len(data)),
            flash_assets.VERIFYING,
            flash_assets.DONE_OK,
        ])
        messages = []
        sleeps = []

        flash_assets.program_assets(
            serial_port,
            data,
            knock_retries=2,
            output=messages.append,
            sleep_fn=sleeps.append,
        )

        self.assertEqual(serial_port.writes[:2], [flash_assets.KNOCK] * 2)
        self.assertTrue(any("ignored stale handshake error" in msg for msg in messages))
        self.assertEqual(sleeps, [flash_assets.RECOVERY_SETTLE_SECONDS])

    def test_diagnostic_reports_mode0_and_mode3(self):
        good_samples = [flash_assets.EXPECTED_JEDEC_ID] * 32
        serial_port = FakeSerial([
            flash_assets.DIAGNOSTIC_REPLY,
            flash_assets.DIAGNOSTIC_HEADER_MAGIC,
            bytes((flash_assets.DIAGNOSTIC_VERSION, 2)),
            diagnostic_record(0, good_samples),
            diagnostic_record(3, good_samples),
        ])
        messages = []

        report = flash_assets.diagnose_flash(
            serial_port,
            knock_retries=1,
            output=messages.append,
        )

        self.assertEqual(serial_port.writes, [flash_assets.DIAGNOSTIC_KNOCK])
        self.assertEqual(report[0]["stable_count"], 32)
        self.assertEqual(report[3]["stable_count"], 32)
        self.assertTrue(any("mode 0: JEDEC stable 32/32" in msg for msg in messages))
        self.assertTrue(any("SFDP 53 46 44 50" in msg for msg in messages))

    def test_diagnostic_accepts_legacy_header_without_ok_suffix(self):
        good_samples = [flash_assets.EXPECTED_JEDEC_ID] * 32
        serial_port = FakeSerial([
            flash_assets.DIAGNOSTIC_REPLY,
            flash_assets.DIAGNOSTIC_LEGACY_HEADER_PREFIX
            + bytes((flash_assets.DIAGNOSTIC_VERSION, 2)),
            diagnostic_record(0, good_samples),
            diagnostic_record(3, good_samples),
        ])

        report = flash_assets.diagnose_flash(
            serial_port,
            knock_retries=1,
            output=lambda _message: None,
        )

        self.assertEqual(report[0]["stable_count"], 32)
        self.assertEqual(report[3]["stable_count"], 32)

    def test_diagnostic_scans_past_noise_before_header_and_record(self):
        good_samples = [flash_assets.EXPECTED_JEDEC_ID] * 32
        serial_port = FakeSerial([
            flash_assets.DIAGNOSTIC_REPLY,
            b"\x00\xFF",
            flash_assets.DIAGNOSTIC_HEADER_MAGIC,
            bytes((flash_assets.DIAGNOSTIC_VERSION, 1)),
            b"\x40\x17\xEF\x40",
            diagnostic_record(0, good_samples),
        ])

        report = flash_assets.diagnose_flash(
            serial_port,
            knock_retries=1,
            output=lambda _message: None,
        )

        self.assertEqual(report[0]["stable_count"], 32)

    def test_diagnostic_ignores_stale_programming_error_before_reply(self):
        good_samples = [flash_assets.EXPECTED_JEDEC_ID] * 32
        serial_port = FakeSerial([
            flash_assets.ERROR,
            flash_assets.ERROR_CRC,
            flash_assets.DIAGNOSTIC_REPLY,
            flash_assets.DIAGNOSTIC_HEADER_MAGIC,
            bytes((flash_assets.DIAGNOSTIC_VERSION, 2)),
            diagnostic_record(0, good_samples),
            diagnostic_record(3, good_samples),
        ])
        messages = []
        sleeps = []

        report = flash_assets.diagnose_flash(
            serial_port,
            knock_retries=2,
            output=messages.append,
            sleep_fn=sleeps.append,
        )

        self.assertEqual(
            serial_port.writes[:2],
            [flash_assets.DIAGNOSTIC_KNOCK] * 2,
        )
        self.assertEqual(report[0]["stable_count"], 32)
        self.assertTrue(
            any("ignored stale diagnostic handshake error" in msg for msg in messages)
        )
        self.assertEqual(sleeps, [flash_assets.RECOVERY_SETTLE_SECONDS])

    def test_diagnostic_rejects_unstable_mode0(self):
        unstable_samples = [flash_assets.EXPECTED_JEDEC_ID] * 31
        unstable_samples.append(b"\xEF\x45\x54")
        good_samples = [flash_assets.EXPECTED_JEDEC_ID] * 32
        serial_port = FakeSerial([
            flash_assets.DIAGNOSTIC_REPLY,
            flash_assets.DIAGNOSTIC_HEADER_MAGIC,
            bytes((flash_assets.DIAGNOSTIC_VERSION, 2)),
            diagnostic_record(0, unstable_samples),
            diagnostic_record(3, good_samples),
        ])

        with self.assertRaisesRegex(
            flash_assets.FlashProtocolError,
            "mode 0 JEDEC is not stable",
        ):
            flash_assets.diagnose_flash(
                serial_port,
                knock_retries=1,
                output=lambda _message: None,
            )

    def test_diagnostic_rejects_corrupt_header(self):
        serial_port = FakeSerial([
            flash_assets.DIAGNOSTIC_REPLY,
            b"DG\xFF\x02",
        ])

        with self.assertRaises(flash_assets.FlashProtocolError):
            flash_assets.diagnose_flash(
                serial_port,
                knock_retries=1,
                output=lambda _message: None,
            )

    def test_diagnostic_rejects_misaligned_record(self):
        serial_port = FakeSerial([
            flash_assets.DIAGNOSTIC_REPLY,
            flash_assets.DIAGNOSTIC_HEADER_MAGIC,
            bytes((flash_assets.DIAGNOSTIC_VERSION, 1)),
            b"\x40\xEF\x17\x00",
        ])

        with self.assertRaises(flash_assets.FlashProtocolError):
            flash_assets.diagnose_flash(
                serial_port,
                knock_retries=1,
                output=lambda _message: None,
            )

    def test_uart_tx_strong_handshake_reads_config_and_stream(self):
        serial_port = FakeSerial([
            flash_assets.UART_TEST_REPLY_LEGACY,
            flash_assets.UART_TEST_REPLY_STRONG_TAIL,
            uart_config_report(),
            bytes(range(256)) * 256,
        ])
        messages = []

        report = flash_assets.uart_test_tx(
            serial_port,
            knock_retries=1,
            output=messages.append,
            baud=9600,
        )

        self.assertEqual(report.errors, 0)
        self.assertEqual(report.length, flash_assets.UART_TEST_BYTES)
        self.assertEqual(
            serial_port.writes[0],
            flash_assets.UART_TEST_KNOCK
            + flash_assets.UART_TEST_MODE_TX
            + struct.pack("<H", 0),
        )
        self.assertTrue(any("UART config:" in msg for msg in messages))

    def test_uart_rx_reports_device_result(self):
        serial_port = FakeSerial([
            flash_assets.UART_TEST_REPLY_LEGACY,
            flash_assets.UART_TEST_REPLY_STRONG_TAIL,
            uart_config_report(),
            flash_assets.UART_TEST_RESULT_MAGIC
            + struct.pack("<III", flash_assets.UART_TEST_BYTES, 0, 0xFFFFFFFF)
            + b"\x00\x00",
        ])

        report = flash_assets.uart_test_rx(
            serial_port,
            knock_retries=1,
            output=lambda _message: None,
            baud=9600,
        )

        self.assertEqual(report.errors, 0)
        self.assertEqual(report.length, flash_assets.UART_TEST_BYTES)
        self.assertEqual(
            serial_port.writes[0],
            flash_assets.UART_TEST_KNOCK
            + flash_assets.UART_TEST_MODE_RX
            + struct.pack("<H", 0),
        )
        self.assertEqual(
            b"".join(serial_port.writes[1:]),
            bytes(range(256)) * 256,
        )


if __name__ == "__main__":
    unittest.main()
