# !/bin/bash
cmake --preset linux-gcc-release -DBUILD_JARS=OFF -DTESTING=OFF
cmake --build --preset linux-gcc-release --parallel --target MODL --target MODL_Coclustering
