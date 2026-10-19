#!/usr/bin/env python3
"""Keep the newest locally built wheel version per package in pip's cache.

The cache must belong to a single platform/Python build. All wheels at the
newest version are retained; downloaded HTTP cache entries are left alone.
"""

import argparse
import subprocess
import sys
from pathlib import Path

from packaging.utils import InvalidWheelFilename, parse_wheel_filename


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true",
                        help="list obsolete wheels without deleting them")
    args = parser.parse_args()

    # Ask pip for paths instead of depending on its internal cache layout.
    output = subprocess.check_output(
        [sys.executable, "-m", "pip", "cache", "list", "--format=abspath"],
        universal_newlines=True,
    )
    wheels = []
    newest = {}
    for line in output.splitlines():
        path = Path(line)
        if path.suffix != ".whl":
            continue
        try:
            name, version, _, _ = parse_wheel_filename(path.name)
        except (InvalidWheelFilename, ValueError):
            print("Skipping unrecognized wheel: {}".format(path), file=sys.stderr)
            continue
        wheels.append((path, name, version))
        if name not in newest or version > newest[name]:
            newest[name] = version

    removed = 0
    kept = 0
    size = 0
    for path, name, version in wheels:
        if version >= newest[name]:
            print("Keeping {} (newest {} version: {})".format(path, name, version))
            kept += 1
            continue
        size += path.stat().st_size
        print("{} {} (keeping {})".format(
            "Would remove" if args.dry_run else "Removing", path, newest[name]))
        if not args.dry_run:
            path.unlink()
        removed += 1

    print("{} {} obsolete wheels ({:.1f} MiB); kept {} wheels".format(
        "Would remove" if args.dry_run else "Removed", removed,
        size / 1024 ** 2, kept))


if __name__ == "__main__":
    main()
