import unittest

from Tools import flash_assets


def counter(n, start=0):
    return bytes((start + i) & 0xFF for i in range(n))


class CheckCounterStreamTests(unittest.TestCase):
    def test_clean_stream_has_no_errors(self):
        data = counter(1000)  # wraps past 255 cleanly
        report = flash_assets.check_counter_stream(data)
        self.assertEqual(report["errors"], 0)
        self.assertEqual(report["length"], 1000)
        self.assertIsNone(report["first_bad"])

    def test_single_bit_flip_is_one_error(self):
        data = bytearray(counter(500))
        data[100] ^= 0x20  # flip one byte
        report = flash_assets.check_counter_stream(bytes(data))
        self.assertEqual(report["errors"], 1)
        self.assertEqual(report["first_bad"], 100)
        self.assertEqual(report["expected"], 100 & 0xFF)
        self.assertEqual(report["got"], (100 & 0xFF) ^ 0x20)

    def test_dropped_byte_shows_first_bad_at_drop(self):
        # Drop index 50: everything from 50 on is shifted -> mismatches.
        data = counter(300)
        dropped = data[:50] + data[51:]
        report = flash_assets.check_counter_stream(dropped)
        self.assertEqual(report["first_bad"], 50)
        self.assertGreater(report["errors"], 100)  # cascade after the drop
        self.assertEqual(report["length"], 299)

    def test_empty_stream(self):
        report = flash_assets.check_counter_stream(b"")
        self.assertEqual(report["errors"], 0)
        self.assertEqual(report["length"], 0)
        self.assertIsNone(report["first_bad"])

    def test_short_stream_flags_missing_bytes(self):
        # Received fewer than expected -> caller compares length vs expected.
        report = flash_assets.check_counter_stream(counter(100))
        self.assertEqual(report["length"], 100)
        self.assertEqual(report["errors"], 0)


if __name__ == "__main__":
    unittest.main()
