import argparse
import struct
from G722 import G722

def main():
    parser = argparse.ArgumentParser(description='G.722 codec utility')
    parser.add_argument('input_file', type=str, help='Input file path')
    parser.add_argument('output_file', type=str, help='Output file path')
    parser.add_argument('--sln16k', action='store_true', help='Use 16000 Hz sample rate')
    parser.add_argument('--encode', action='store_true', help='Encode mode')
    parser.add_argument('--bend', action='store_true', help='Use big-endian byte order')
    args = parser.parse_args()

    sample_rate = 16000 if args.sln16k else 8000
    bit_rate = 64000  # Example bit rate

    try:
        with open(args.input_file, 'rb') as fi, open(args.output_file, 'wb') as fo:
            codec = G722(sample_rate, bit_rate)
            if args.encode:
                while True:
                    pcm_data = fi.read(2 * 1024)  # Read blocks of 2048 bytes
                    if not pcm_data:
                        break
                    # Convert samples to little or big endian based on flag
                    format_string = '>' if args.bend else '<'
                    pcm_samples = struct.unpack(format_string + 'h' * (len(pcm_data) // 2), pcm_data)
                    encoded_data = codec.encode(pcm_samples)
                    fo.write(encoded_data)
            else:
                while True:
                    encoded_data = fi.read(1024)  # Read blocks of 1024 bytes
                    if not encoded_data:
                        break
                    decoded_samples = codec.decode(encoded_data)
                    # Convert samples to little or big endian based on flag
                    format_string = '>' if args.bend else '<'
                    output_data = struct.pack(format_string + 'h' * len(decoded_samples), *decoded_samples)
                    fo.write(output_data)

    except IOError as e:
        print(f"File error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
