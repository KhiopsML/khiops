#!/bin/bash

# Echo all output
set -x


# The binary location depends on the preset name used at the configure step (Cf. build.sh)
if [ "$(uname)" == "Darwin" ]
then
    BUILD_DIR="macos-clang-release"
else
    BUILD_DIR="linux-gcc-release"
fi

# Copy the MODL binaries to the anaconda PREFIX path
mkdir -p $PREFIX/bin
cp build/$BUILD_DIR/bin/MODL $PREFIX/bin
cp build/$BUILD_DIR/bin/MODL_Coclustering $PREFIX/bin
