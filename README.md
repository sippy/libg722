# libg722

[![Build & Test](https://github.com/sippy/libg722/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/sippy/libg722/actions/workflows/build_and_test.yml)

## Introduction

The G.722 module is a bit exact implementation of the ITU G.722 specification
for all three specified bit rates - 64000bps, 56000bps and 48000bps. It passes
the ITU tests.

To allow fast and flexible interworking with narrow band telephony, the
encoder and decoder support an option for the linear audio to be an 8k
samples/second stream. In this mode the codec is considerably faster, and
still fully compatible with wideband terminals using G.722.

## History

The code was originally written by Milton Anderson <milton@thumper.bellcore.com>
from BELLCORE, and has been modified by the Chengxiang Lu and Alex Hauptmann
from the Speech Group, School of Computer Science, Carnegie Mellon University,
to be fairly fast and efficient, while retaining high fidelity.

Steve Underwood <steveu@coppice.org> improved the code a lot later on and
contributed it into several popular open source projects.

Himanshu Soni <sonihimanshu@gmail.com> has adjusted some coefficients
to avoid over/under-flows in the decoder.

Phil Schatzmann <phil.schatzmann@gmail.com> has added cmake-glue and Arduino
support.

Librarized by Sippy Software, Inc.

## Build and Install library:

### MacOS & Linux

```
git clone https://github.com/sippy/libg722.git
cmake -B libg722/build -S libg722
make -C libg722/build clean all test install
```

**Note for macOS users:** The library will be installed to `~/Library/libg722` by default. If you prefer a different location, you can specify it with:
```
cmake -B libg722/build -S libg722 -DCMAKE_INSTALL_PREFIX=/your/preferred/path
```

After installation, you may need to add the library path to your environment:
```
export DYLD_LIBRARY_PATH="$HOME/Library/libg722/lib:$DYLD_LIBRARY_PATH"
```

### iOS

```
git clone https://github.com/sippy/libg722.git
cmake -B libg722/build-ios-device -S libg722 \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
make -C libg722/build-ios-device
```

## Install Python module from PyPy:

```
pip install G722
```

## Build and Install Python module from source code:

```
git clone https://github.com/sippy/libg722.git
pip install libg722/
```

## Pull library into your Docker container:
```
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
