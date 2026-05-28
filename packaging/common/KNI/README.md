# packaging/common/KNI

This directory contains scripts and templates for building, running, and documenting
the KNI (Khiops Native Interface) examples.

## Scripts

| Script | Purpose |
|--------|---------|
| `build-c-linux.sh` | Compile C examples on Linux |
| `build-c-windows.cmd` | Compile C examples on Windows |
| `build-java.sh` | Compile Java examples |
| `run-java-linux.sh` | Run Java example on Linux |
| `run-java-windows.cmd` | Run Java example on Windows |
| `run-python.sh` | Run Python mono-table example |
| `run-multitable-python.sh` | Run Python multi-table example |

## Documentation Templates

| File | Purpose |
|------|---------|
| `README.txt.in` | Template for native KNI package README (DEB, RPM, NSIS, tarballs). Installed with the package. **Not used for pip wheels** (pip uses `packaging/pip/README-kni.md`). |
| `template-README.md` | Template for KNI-tutorial repository README. Contains `@PLACEHOLDER@` variables replaced by CMake during configure. Used by `update-kni-tutorial.yml` workflow. |

## Reference Results

The `results.ref/` directory contains reference output files used by CI to validate
KNI example outputs:

- `R_Iris.txt` - Expected output for Iris dataset (mono-table)
- `R_SpliceJunction.txt` - Expected output for Splice Junction dataset (multi-table)

Test scripts compare generated outputs against these reference files to ensure
correctness across all platforms and package types.

## CI Testing

The scripts are used by two CI workflows:

- **Native packages (DEB, RPM)**: the `test-kni` composite action
  (`.github/actions/test-kni/action.yml`) runs the C, Java, and Python
  scripts to validate the installed native package. It is called by
  `pack-debian.yml` and `pack-rpm.yml`.

- **pip package**: the `test-kni` job in `.github/workflows/pack-pip.yml`
  runs the Python scripts (`run-python.sh`,
  `run-multitable-python.sh`) via bash on all pip platforms to
  validate the installed `khiops-kni` wheel.

## KNI Tutorial Repository

The workflow `.github/workflows/update-kni-tutorial.yml` automatically updates
the [KNI-tutorial](https://github.com/KhiopsML/KNI-tutorial) repository with
each Khiops release. When triggered manually, it performs:

1. **CMake configure**: Generates C example files and README from templates
   - `KNIRecodeFile.c` (from `KNIRecodeFile.cpp` + main function)
   - `KNIRecodeMTFiles.c` (from `KNIRecodeMTFiles.cpp` + main function)
   - `kni.README.md` (from `template-README.md` with script content embedded)

2. **Copy files to tutorial repo**:
   - Generated C examples → `tutorial/cpp/`
   - C header files → `tutorial/include/`
   - Generated README → `tutorial/README.md`

3. **Generate Python API documentation**:
   - Uses `pdoc` to generate HTML docs from `packaging/pip/kni/` source
   - Outputs to `tutorial/python/docs/`
   - Documentation is linked from the README

4. **Commit and tag**:
   - Commits all changes to KNI-tutorial main branch
   - Creates version tag matching Khiops version
   - Creates version branch

The `template-README.md` file uses CMake `@PLACEHOLDER@` substitutions (e.g.
`@RUN_PYTHON_LINUX@`) to inline script content directly into the tutorial
README. This ensures the example commands shown in the tutorial are exactly
the commands tested by CI.
