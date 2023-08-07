cmake --preset windows-msvc-release -DBUILD_JARS=OFF -DTESTING=OFF
cmake --build --preset windows-msvc-release --parallel --target MODL MODL_Coclustering
