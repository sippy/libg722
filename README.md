# libg722

[![Build & Test](https://github.com/sippy/libg722/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/sippy/libg722/actions/workflows/build_and_test.yml)

## Introduction

The G.722 module is a bit-exact implementation of the ITU G.722 specification
for all three specified bit rates: 64,000 bps, 56,000 bps and 48,000 bps. It
passes the ITU tests.

To allow fast and flexible interworking with narrowband telephony, the
encoder and decoder support an option for the linear audio to be an 8 kHz
stream. In this mode the codec is considerably faster, and
still fully compatible with wideband terminals using G.722.

## History

The code was originally written by Milton Anderson <milton@thumper.bellcore.com>
from BELLCORE, and was modified by Chengxiang Lu and Alex Hauptmann from the
Speech Group, School of Computer Science, Carnegie Mellon University, to be
fairly fast and efficient while retaining high fidelity.

Steve Underwood <steveu@coppice.org> improved the code a lot later on and
contributed it into several popular open source projects.

Himanshu Soni <sonihimanshu@gmail.com> has adjusted some coefficients
to avoid overflows and underflows in the decoder.

Phil Schatzmann <phil.schatzmann@gmail.com> has added CMake glue and Arduino
support.

Packaged as libg722 by Sippy Software, Inc.

## Build and Install Library

### macOS and Linux

```sh
git clone https://github.com/sippy/libg722.git
cmake -B libg722/build -S libg722
make -C libg722/build clean all test install
```

**Note for macOS users:** The library will be installed to `~/Library/libg722`
by default. If you prefer a different location, you can specify it with:
```sh
cmake -B libg722/build -S libg722 -DCMAKE_INSTALL_PREFIX=/your/preferred/path
```

After installation, you may need to add the library path to your environment:
```sh
export DYLD_LIBRARY_PATH="$HOME/Library/libg722/lib:$DYLD_LIBRARY_PATH"
```

### iOS

```sh
git clone https://github.com/sippy/libg722.git
cmake -B libg722/build-ios-device -S libg722 \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
make -C libg722/build-ios-device
```

## Install Python Module With pip

The core package has no required NumPy dependency:

```sh
pip install G722
```

Install the optional NumPy decode backend with the `numpy` extra:

```sh
pip install "G722[numpy]"
```

This installs `G722-numpy` at the same version as `G722`. You can also install
the addon package directly:

```sh
pip install G722-numpy
```

## Build and Install Python Module From Source

The default source build installs the core package:

```sh
git clone https://github.com/sippy/libg722.git
pip install ./libg722
```

After installing the core package, build the optional NumPy backend from the
same checkout:

```sh
LIBG722_PACKAGE_VARIANT=numpy-addon pip install ./libg722
```

## Python Build Options

`LIBG722_BUILD_MODE` controls build profile for the main `G722` extension:
- `production`: build with optimization (`-O2`, or `/O2` on Windows).
- `debug`: build with `-g3 -O0`.
- `auto` (default): if `.` is a git repository and `git` is available, run `git diff v{version} -- .`; build in `debug` mode when it differs, otherwise `production`. If `.` is not a git repository or `git` is unavailable, use `production`.

`LIBG722_PACKAGE_VARIANT` controls which Python distribution is built from this repository:
- `core` (default): builds/publishes `G722`.
- `numpy-addon`: builds/publishes `G722-numpy` from `python/G722_numpy_mod.c`.

## Python API

`G722(sample_rate, bit_rate, use_numpy=None)` accepts an optional `use_numpy` flag:
- `True`: return NumPy arrays from `decode()` (raises if `G722-numpy` is not installed).
- `False`: return Python `array('h')` from `decode()`.
- omitted or `None`: use the `G722-numpy` backend when installed, otherwise return `array('h')`.

## Pull Library Into Your Docker Container

Published Docker images contain the installed library and public headers under
`/usr/local`. The default-branch images are published with these tags:

- `sippylabs/libg722:latest-debian_sid-slim`
- `sippylabs/libg722:latest-debian_12-slim`
- `sippylabs/libg722:latest-debian_13-slim`
- `sippylabs/libg722:latest-ubuntu_latest`

Branch, release, pull request and SHA tags use the same base-image suffixes,
for example `<ref>-debian_12-slim`.

```dockerfile
ARG BASE_IMAGE=debian:sid-slim
ARG LIBG722_IMAGE=sippylabs/libg722:latest-debian_sid-slim
FROM ${LIBG722_IMAGE} AS libg722
FROM ${BASE_IMAGE} AS build
COPY --from=libg722 /usr/local/lib/libg722.* /usr/local/lib/
COPY --from=libg722 /usr/local/include/g722* /usr/local/include/
```

## License

This code is mostly Public Domain. Library test code is under BSD 2-clause
license.

See LICENSE for copyright details and each individual file for specifics.
