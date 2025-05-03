import os
import unittest
import hashlib
import numpy as np
from sysconfig import get_platform

from G722 import G722

IS_S390X = get_platform() == 'linux-s390x'

@unittest.skipIf(IS_S390X, "G722 encode/decode test is broken on BE; skipping until fixed")
class TestEncodeDecode(unittest.TestCase):
    DATA_DIR = os.path.join(os.path.dirname(__file__), '../test_data')
    PCM_FILE = os.path.join(DATA_DIR, 'pcminb.dat')
    MD5_FILE = os.path.join(DATA_DIR, 'pcminb.checksum')

    def setUp(self):
        # load raw PCM as int16 array
        with open(self.PCM_FILE, 'rb') as f:
            pcm_bytes = f.read()
        self.pcm = np.frombuffer(pcm_bytes, dtype='<i2')
        total = int(self.pcm.sum(dtype=np.int64))
        self.assertEqual(total, 7206958, "Input checksum fail")

        # load expected MD5s; expect lines like "48000_8000 2334 cfa9e73241616fa366293aafdf2fde32 11a9661b59beeae7d520c38e1a3524e4"
        self.expected = {}
        with open(self.MD5_FILE, 'r') as f:
            for line in (l for l in f if not l.startswith('#')):
                key, size, denc, ddec = line.strip().split()
                self.expected[key] = (denc, ddec, int(size))

    def test_all_bitrates_and_sample_rates(self):
        bitrates    = (48000, 56000, 64000)
        sample_rates = (8000, 16000)

        for br in bitrates:
            for sr in sample_rates:
                key = f"{br}_{sr}"
                with self.subTest(bitrate=br, sample_rate=sr):
                    codec = G722(sr, br)
                    encoded = codec.encode(self.pcm)
                    md5 = hashlib.md5(encoded).hexdigest()

                    # sanity check that we have an expected hash
                    self.assertIn(key, self.expected, f"no MD5 entry for {key}")
                    denc, ddec, csize = self.expected[key]
                    self.assertEqual(csize, len(encoded),
                                     f"Encoded size mismatch for {key}")
                    self.assertEqual(denc, md5,
                                     f"MD5 mismatch for {key}")

                    decoded = codec.decode(encoded)
                    decoded_le = decoded.astype('<i2')
                    decoded_bytes = decoded_le.tobytes()
                    self.assertEqual(len(decoded), len(self.pcm),
                                     f"Decoded size mismatch for {key}")
                    md5_dec  = hashlib.md5(decoded_bytes).hexdigest()
                    self.assertEqual(ddec, md5_dec,
                                     f"Decoded MD5 mismatch for {key}")

if __name__ == '__main__':
    unittest.main()
