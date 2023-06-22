cmake --preset windows-msvc-release -DFULL=OFF -DBUILD_JARS=OFF -DTESTING=OFF
cmake --build --preset build-windows-msvc-release --target MODL --target MODL_Coclustering
