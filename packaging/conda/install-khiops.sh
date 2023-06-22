#!/bin/bash

# Echo all output
set -x

# Copy the MODL binaries to the anaconda PREFIX path
mkdir -p $PREFIX/bin
cp build/linux-gcc-release/bin/MODL $PREFIX/bin
cp build/linux-gcc-release/bin/MODL_Coclustering $PREFIX/bin
