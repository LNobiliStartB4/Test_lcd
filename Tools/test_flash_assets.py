import struct
import unittest

from Tools import flash_assets


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


def progress(current, total):
    return flash_assets.ERASE_PROGRESS + struct.pack("<HH", current, total)


def ack(next_offset):
    return flash_assets.CHUNK_OK + struct.pack("<I", next_offset)


class FlashAssetsProtocolTests(unittest.TestCase):
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
            progress(1, 2),
            progress(2, 2),
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
        self.assertEqual(serial_port.reset_count, 1)

    def test_rejects_wrong_chunk_ack_offset(self):
        data = b"x" * 300
        serial_port = FakeSerial([
            flash_assets.HELLO,
            flash_assets.EXPECTED_JEDEC_ID,
            flash_assets.HEADER_ACCEPTED,
            progress(1, 1),
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
            progress(1, 1),
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


if __name__ == "__main__":
    unittest.main()
