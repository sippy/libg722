import unittest

from G722 import G722

class TestEncoder(unittest.TestCase):
    def test_encode_len(self):
        bitrates = [48000, 56000, 64000]
        sample_rates = [8000, 16000]

        for bitrate in bitrates:
            for sample_rate in sample_rates:
                with self.subTest(bitrate=bitrate, sample_rate=sample_rate):
                    g722 = G722(sample_rate, bitrate)
                    audio_data = b"\x00\x01\x02\x03"  # Example raw audio data
                    encoded_data = g722.encode(audio_data)
                    got = len(encoded_data)
                    want = 4 if sample_rate == 8000 else 2
                    self.assertEqual(want, got)

if __name__ == '__main__':
    unittest.main()
