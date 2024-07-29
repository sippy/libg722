import unittest

from G722 import G722

class TestDecoder(unittest.TestCase):
    def test_decode_len(self):
        bitrates = [48000, 56000, 64000]
        sample_rates = [8000, 16000]

        for bitrate in bitrates:
            for sample_rate in sample_rates:
                with self.subTest(bitrate=bitrate, sample_rate=sample_rate):
                    g722 = G722(sample_rate, bitrate)
                    encoded_data = b"0123"  # Example encoded data
                    decoded_data = g722.decode(encoded_data)
                    got = len(decoded_data)
                    want = 4 if sample_rate == 8000 else 8
                    self.assertEqual(want, got)


if __name__ == '__main__':
    unittest.main()
