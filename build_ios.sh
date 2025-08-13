#!/bin/bash

rm -rf build-macos build-ios-device build-ios-simulator g722.xcframework

# Build for macOS
cmake -B build-macos -S . \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
make -C build-macos

# Build for iOS device
cmake -B build-ios-device -S . \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
make -C build-ios-device

# Build for iOS simulator
cmake -B build-ios-simulator -S . \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
make -C build-ios-simulator

# Create include directory and copy headers
mkdir -p build-macos/include
cp g722*.h build-macos/include/

mkdir -p build-ios-device/include
cp g722*.h build-ios-device/include/

mkdir -p build-ios-simulator/include
cp g722*.h build-ios-simulator/include/

# Create the XCFramework
xcodebuild -create-xcframework \
  -library build-macos/libg722.a \
  -headers build-macos/include \
  -library build-ios-device/libg722.a \
  -headers build-ios-device/include \
  -library build-ios-simulator/libg722.a \
  -headers build-ios-simulator/include \
  -output g722.xcframework