---
name: python-wheel
description: "Use when modifying Python wheel packaging, pyproject.toml, build configuration, or wheel distribution (pip, conda). Applies to packaging/pip/, .github/workflows/pack-pip.yml, packaging/conda/."
applyTo: "packaging/pip/**"
---

# Python Wheel Packaging Instructions

## Overview

Khiops distributes **three C++ wheels** (no Python code) via [scikit-build-core](https://scikit-build-core.readthedocs.io/), built with **cibuildwheel**:

| Package | Description | MPI |
|---------|-------------|-----|
| `khiops-core` | Khiops executables (`MODL`, `MODL_Coclustering`, `_khiopsgetprocnumber`) | Yes |
| `khiops-kni` | KNI shared library + C header (`KhiopsNativeInterface`) | No |
| `khiops-knitransfer` | KNITransfer command-line tool (test of `khiops-kni`) | No |

Both packages:
- Use `scikit_build_core.build` as build backend
- Extract version from `src/Learning/KWUtils/KWKhiopsVersion.h`
- Build for CPython 3.9 only (stable ABI — one wheel works for all Python 3.x)
- Skip musllinux and win32
- Disable `repair-wheel-command` on all platforms (MPI libs come from pip packages)

## Key Files

| File | Purpose | Edit When |
|------|---------|-----------|
| `packaging/pip/pyproject-khiops.toml` | `khiops-core` wheel config | Changing khiops-core build flags, dependencies, targets |
| `packaging/pip/pyproject-kni.toml` | `khiops-kni` wheel config | Changing khiops-kni build flags, targets |
| `packaging/pip/pyproject-knitransfer.toml` | `khiops-knitransfer` wheel config | Changing knitransfer build flags, targets |
| `packaging/pip/README.md` | Packaging docs (package differences, local build instructions) | Updating packaging documentation |
| `src/Learning/KWUtils/KWKhiopsVersion.h` | Version definition (11.0.1-a.2) | Bumping version (all wheels auto-read this) |
| `.github/workflows/pack-pip.yml` | CI workflow for building & publishing all wheels | Changing CI matrix, publish targets |

**Build-time mechanism**: the chosen config is copied to root `pyproject.toml` before each build. In CI, `khiops-core` is built first, then `khiops-kni`, then `khiops-knitransfer` in the same job.

## Build Configuration Details

### khiops-core (`pyproject-khiops.toml`)

```toml
[tool.scikit-build]
cmake.args = ["-G", "Ninja"]
cmake.define.CMAKE_BUILD_TYPE = "Release"
cmake.define.MPI = true
cmake.define.TESTING = false
cmake.define.BUILD_LEX_YACC = false
cmake.define.BUILD_JARS = false
cmake.define.GENERATE_VIEWS = false
cmake.define.C11 = true
build.targets = ["MODL", "MODL_Coclustering", "_khiopsgetprocnumber"]
install.components = ["KHIOPS_CORE"]
```

**Dependencies** (platform-specific):
```toml
# Build
requires = ["ninja", "scikit-build-core>=0.11.6",
    "mpich ; platform_system in '[Linux, Darwin]'",
    "impi-devel ; platform_system == 'Windows'"]
# Runtime
dependencies = [
    "mpich ; platform_system in '[Linux, Darwin]'",
    "impi-rt ; platform_system == 'Windows'"]
```

### khiops-kni (`pyproject-kni.toml`)

```toml
[tool.scikit-build]
cmake.args = ["-G", "Ninja"]
cmake.define.CMAKE_BUILD_TYPE = "Release"
cmake.define.MPI = false       # No MPI needed for KNI
cmake.define.TESTING = false
cmake.define.BUILD_LEX_YACC = false
cmake.define.BUILD_JARS = false
cmake.define.GENERATE_VIEWS = false
cmake.define.C11 = true
build.targets = ["KhiopsNativeInterface"]
install.components = ["KNI"]
```

**Dependencies**: none (no MPI, no runtime deps).

### khiops-knitransfer (`pyproject-knitransfer.toml`)

```toml
[tool.scikit-build]
cmake.args = ["-G", "Ninja"]
cmake.define.CMAKE_BUILD_TYPE = "Release"
cmake.define.MPI = false       # No MPI needed for KNITransfer
cmake.define.TESTING = false
cmake.define.BUILD_LEX_YACC = false
cmake.define.BUILD_JARS = false
cmake.define.GENERATE_VIEWS = false
cmake.define.C11 = true
build.targets = ["KNITransfer"]
install.components = ["KNI_TRANSFER"]
```

**Dependencies**: none (no MPI, no runtime deps).

### Version Extraction (all packages)

Both configs use `scikit-build-core.metadata.regex` to extract the version:

```toml
[tool.scikit-build.metadata]
version.provider = "scikit_build_core.metadata.regex"
version.input = "src/Learning/KWUtils/KWKhiopsVersion.h"
```

The regex converts `11.0.1-a.2` → `11.0.1a2` (PEP 440).

**When modifying version**:
1. Edit `src/Learning/KWUtils/KWKhiopsVersion.h` → Set `#define KHIOPS_VERSION KHIOPS_STR(X.Y.Z)`
2. Do NOT edit `pyproject-*.toml` version fields
3. Do NOT edit `CMakeLists.txt` version (it reads from `KWKhiopsVersion.h`)

## Common Tasks

### Building Wheels Locally

```bash
# khiops-core
cp packaging/pip/pyproject-khiops.toml pyproject.toml
pip wheel . -w wheelhouse

# khiops-kni
cp packaging/pip/pyproject-kni.toml pyproject.toml
pip wheel . -w wheelhouse

# khiops-knitransfer
cp packaging/pip/pyproject-knitransfer.toml pyproject.toml
pip wheel . -w wheelhouse
```

Prerequisites: CMake ≥ 3.22, Ninja, Python ≥ 3.9. For `khiops-core` only: mpich (Linux/macOS) or impi-devel (Windows).

### Modifying CMake Arguments

Edit the relevant `pyproject-*.toml` in `packaging/pip/`:

```toml
[tool.scikit-build]
cmake.define.CMAKE_BUILD_TYPE = "Release"  # Change to "Debug"
cmake.define.MY_NEW_FLAG = true            # Add new variable
```

### Adding Python Dependencies

⚠️ **Both wheels are C++-only**. `khiops-kni` has zero dependencies. `khiops-core` only depends on MPI packages.

If you must add a dependency:
1. **Justify it**: Why can't this be done in C++?
2. **Get team approval**: Discuss in PR/issue before committing
3. **Document it**: Add comment in the relevant `pyproject-*.toml`

### Testing Wheel Installation

```bash
# Build khiops-core
cp packaging/pip/pyproject-khiops.toml pyproject.toml
pip wheel . -w wheelhouse

# Install in fresh env
python -m venv test_env && source test_env/bin/activate
pip install wheelhouse/khiops_core-*.whl

# Verify
khiops -s
```

## CI/CD (`pack-pip.yml`)

The workflow builds both packages sequentially in the same job per platform:
1. Copies `pyproject-khiops.toml` → root, builds `khiops-core` wheels with cibuildwheel
2. Runs standard tests with the installed `khiops-core` wheel
3. Copies `pyproject-kni.toml` → root, builds `khiops-kni` wheels
4. Uploads all wheels as artifacts

**Matrix**: Ubuntu x64 + ARM64, Windows (x64 only — impi-rt unavailable on ARM), macOS Intel + ARM.

**Publishing**: On tag push + manual `workflow_dispatch`, publishes to TestPyPI or PyPI via trusted publishing.

**Triggers**: PR changes to `**CMakeLists.txt` (excluding tests), `**.cmake`, `pack-pip.yml`, or `pyproject.toml`.

## Common Pitfalls

### ❌ Editing the Wrong pyproject.toml

The root `pyproject.toml` is **overwritten at build time**. Always edit configs in `packaging/pip/`:
- `packaging/pip/pyproject-khiops.toml` for `khiops-core`
- `packaging/pip/pyproject-kni.toml` for `khiops-kni`
- `packaging/pip/pyproject-knitransfer.toml` for `khiops-knitransfer`

### ❌ Modifying Version in Wrong Place

```cpp
// ❌ DON'T: Edit CMakeLists.txt
project(khiops VERSION 11.0.1)

// ❌ DON'T: Edit pyproject-*.toml [project] section directly
version = "11.0.1"

// ✅ DO: Edit only this file
// src/Learning/KWUtils/KWKhiopsVersion.h
#define KHIOPS_VERSION KHIOPS_STR(11.0.1-a.2)
```

### ❌ Adding MPI to khiops-kni or khiops-knitransfer

`khiops-kni` and `khiops-knitransfer` must remain MPI-free (`cmake.define.MPI = false`, no MPI in dependencies).

### ❌ Changing MPI Dependencies Without Platform Guards

```toml
# ❌ Bad: Assumes MPI everywhere
dependencies = ["mpich"]

# ✅ Good: Platform-specific (khiops-core only)
dependencies = [
    "mpich ; platform_system in '[Linux, Darwin]'",
    "impi-rt ; platform_system == 'Windows'"
]
```

## Resources

- **scikit-build-core docs**: https://scikit-build-core.readthedocs.io/
- **pyproject.toml schema**: https://packaging.python.org/specifications/pyproject-toml/
- **Wheel spec**: https://packaging.python.org/specifications/wheels/
- **GitHub Actions `pack-pip.yml`**: CI/CD for automated wheel building
