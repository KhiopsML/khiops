# Pip Wheel Packaging

This directory contains the `pyproject.toml` configurations for the three Khiops pip packages:

| File                         | Package              | Description                         |
| ---------------------------- | -------------------- | ----------------------------------- |
| `pyproject-khiops.toml`      | `khiops-core`        | Khiops executables with MPI support |
| `pyproject-kni.toml`         | `khiops-kni`         | KNI shared library and C header     |
| `pyproject-knitransfer.toml` | `khiops-knitransfer` | Test package for khiops-kni         |

## How It Works

The repository root `pyproject.toml` is replaced at build time with one of the configs
from this directory. All use the same build pipeline:
**cibuildwheel** → **scikit-build-core** → **CMake + Ninja**.

In CI (`.github/workflows/pack-pip.yml`), the workflow:

1. Copies `pyproject-khiops.toml` to the root, builds `khiops-core` wheels
2. Copies `pyproject-kni.toml` to the root, builds `khiops-kni` wheels
3. Copies `pyproject-knitransfer.toml` to the root, builds `khiops-knitransfer` wheels

## Building Locally

### Prerequisites

- CMake ≥ 3.22
- Ninja
- Python ≥ 3.9
- MPI (for `khiops-core` only): openmpi on Linux/macOS, impi-devel on Windows

### khiops-core

```bash
cp packaging/pip/pyproject-khiops.toml pyproject.toml
pip wheel . -w wheelhouse
```

### khiops-kni

```bash
cp packaging/pip/pyproject-kni.toml pyproject.toml
pip wheel . -w wheelhouse
```

### khiops-knitransfer

```bash
cp packaging/pip/pyproject-knitransfer.toml pyproject.toml
pip wheel . -w wheelhouse
```

## Package Differences

|                   | **khiops-core**                                     | **khiops-kni**            | **khiops-knitransfer**           |
| ----------------- | --------------------------------------------------- | ------------------------- | -------------------------------- |
| Build targets     | `MODL`, `MODL_Coclustering`, `_khiopsgetprocnumber` | `KhiopsNativeInterface`   | `KNITransfer`                    |
| Install component | `KHIOPS_CORE`                                       | `KNI`                     | `KNI_TRANSFER`                   |
| MPI               | Yes                                                 | No                        | No                               |
| Runtime deps      | openmpi (Linux/macOS), impi-rt (Windows)            | None                      | None                             |
| Contents          | Executables                                         | Shared library + C header | Command-line tool (test for KNI) |

All packages:

- Extract their version from `src/Learning/KWUtils/KWKhiopsVersion.h`
- Build for CPython 3.9 only (stable ABI, one wheel works for all Python 3.x)
- Skip musllinux and win32