import unittest

import numpy as np

from G722 import G722

class TestEncoder(unittest.TestCase):
    def test_encode_len(self):
        bitrates = (48000, 56000, 64000)
        sample_rates = (8000, 16000)
        sample_inputs = (b"\x00\x01\x02\x03", np.array((0, 1, 2, 3), dtype=np.int16), (0, 1, 2, 3), ('aa', 'bb', 'cc'), 42)
        def do_test(br, sr, id):
            with self.subTest(bitrate=br, sample_rate=sr, input_data=id):
                g722 = G722(sr, br)
                try:
                    encoded_data = g722.encode(id)
                except TypeError:
                    got = isinstance(id, int) or isinstance(id[0], str)
                    want = True
                    self.assertEqual(want, got)
                    return
                else:
                    got = isinstance(id, int)
                    want = False
                    self.assertEqual(want, got)
                got = len(encoded_data)
                want = 4 if sample_rate == 8000 else 2
                self.assertEqual(want, got)
        for bitrate in bitrates:
            for sample_rate in sample_rates:
                for sample_input in sample_inputs:
                    do_test(bitrate, sample_rate, sample_input)

if __name__ == '__main__':
    unittest.main()
