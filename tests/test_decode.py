import unittest
from array import array

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

    def test_constructor_use_numpy(self):
        g722 = G722(8000, 64000)
        encoded_data = b"0123"
        default_decoded = g722.decode(encoded_data)
        has_numpy = hasattr(default_decoded, "dtype")

        force_array = G722(8000, 64000, False)
        forced_array_decoded = force_array.decode(encoded_data)
        self.assertIsInstance(forced_array_decoded, array)

        if has_numpy:
            force_numpy = G722(8000, 64000, True)
            forced_numpy_decoded = force_numpy.decode(encoded_data)
            self.assertTrue(hasattr(forced_numpy_decoded, "dtype"))
        else:
            with self.assertRaises(RuntimeError):
                G722(8000, 64000, True)

if __name__ == '__main__':
    unittest.main()
