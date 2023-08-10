# !/bin/bash

# On macOS, we have to build with the compiler outside conda. With conda's clang the following error occurs:
# ld: unsupported tapi file type '!tapi-tbd' in YAML file '/Applications/Xcode_14.2.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libSystem.tbd' for architecture x86_64
if [ "$(uname)" == "Darwin" ]
then
    cmake --preset macos-clang-release -DBUILD_JARS=OFF -DTESTING=OFF -DCMAKE_CXX_COMPILER=/usr/bin/clang++
    cmake --build --preset macos-clang-release --parallel --target MODL MODL_Coclustering
else
    cmake --preset linux-gcc-release -DBUILD_JARS=OFF -DTESTING=OFF
    cmake --build --preset linux-gcc-release --parallel --target MODL MODL_Coclustering
fi
