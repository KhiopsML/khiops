# !/bin/bash
cmake --preset linux-gcc-release -DFULL=OFF -DBUILD_JARS=OFF -DTESTING=OFF
cmake --build --preset build-linux-gcc-release --target MODL --target MODL_Coclustering
