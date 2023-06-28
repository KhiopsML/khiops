# Khiops project

# Compilation

```bash
mkdir build
cmake -B build -S . -DMPI=ON -DFULL=OFF -DBUILD_JARS=ON -DTESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/ --parallel
```

# Packaging

## CPack: deb, rpm and zip

The different packages are built with cpack. The used generators are:

- DEB for debian
- ZIP for KNI on windows
- RPM for redhat
  
Run the following command to launch the packaging. The resulting packages are located on `build/packages`.

```bash
cd build
cpack -G DEB
```

On Fedora-like distro, the environment module mpi must be loaded before configure, build and package processes:

```bash
source /etc/profile.d/modules.sh
module load mpi/mpich-x86_64
mkdir build && cd build
cmake -B . -S .. -DMPI=ON -DFULL=OFF -DBUILD_JARS=ON -DTESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
cpack -G RPM
```

### CPack TODOs

- test packaging on macOS with `cpack -G FreeBSD`. It should work (we just have to get kni and khiops-sample out from the install)
- test NSIS on windows (`cpack -G NSIS64`)
- rewiew the KNI doc (the same one for Linux and Windows)
- change the package name to include the disto version on DEB and RPM
- Questions:
  - remove packaging/common/khiops/doc/*.docx
  - remove packaging/common/khiops/doc/KhiopsTutorial.pptx
  - remove packaging/common/khiops/whatsnewV9.0.1.txt

## Conda

The package version is not imported from the source (TODO) then, we have to set the environment variable `KHIOPS_VERSION` before building the package.

```bash
# extract package version from sources
set KHIOPS_VERSION=$(grep "KHIOPS_VERSION" src/Learning/KWUtils/KWKhiopsVersion.h | cut -d"(" -f2 | cut -d")" -f1)
# build package
conda-build packaging/conda/
```
