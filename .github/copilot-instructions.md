# Khiops Workspace Instructions

## Project Overview

**Khiops** is an AutoML suite for supervised and unsupervised learning with data preparation, feature engineering, and scoring capabilities. It's a multi-platform C++ core with Python distribution (wheels, pip, conda) and native installers (Windows, Linux, macOS). Key features include:
- **MODL**: Minimum Description Length-based model selection for decision trees and Naive Bayes classifiers
- **Feature Selection**: Automatic selection of relevant features for modeling
- **Discretization**: Optimal binning of continuous variables

- **License**: BSD-3-Clause-Clear
- **Current Version**: 11.0.1-a.2 (pre-release)
- **Website**: https://khiops.org
- **Source**: https://github.com/KhiopsML/khiops

## Repository Structure

```
src/
  ├─ Learning/        # ML algorithms, MODL, SNBPredictor, DTForest, data prep
  ├─ Norm/            # Core numerical algorithms, base functionality
  └─ Parallel/        # MPI-based parallelization
test/
  ├─ UnitTests/       # CTest unit tests (norm_test, parallel_test, etc.)
  ├─ LearningTest/    # Integration/standard tests
  └─ MemoryStatsVisualizer/
scripts/              # Python and CMake helpers, copyright/encoding checks
packaging/            # Conda, Linux RPM/Debian, Windows NSIS configs
```

## Core Development Conventions

### C++ Code Guidelines

**Detailed C++ coding guidelines** (style, naming, class structure, assertions) are in [`.github/instructions/cpp-changes.instructions.md`](.github/instructions/cpp-changes.instructions.md) — they auto-load when editing `*.cpp` or `*.h` files.

### Pre-commit Hooks

All commits must pass `.pre-commit-config.yaml`:

- **C++/Java**: `clang-format` (strict)
- **Python**: `black` (3.9+)
- **CMake**: `cmake-format`
- **JSON**: Auto-format (except test data)
- **Custom Checks**: Copyright year verification, UTF-8 encoding

### Naming Conventions

- **Learning module**: `KW*` prefix (e.g., `KWData`, `KWUtils`, `KWKhiopsVersion`)
- **Parallel module**: `PL*` prefix (e.g., `PLParallelTask`)
- **Norm module**: Core/base prefixes
- **Tests**: Separate implementation per module (`UnitTests/`, `StandardTests/`)

### Version Management

Version is defined in `src/Learning/KWUtils/KWKhiopsVersion.h`:

```c++
#define KHIOPS_VERSION KHIOPS_STR(11.0.1-a.2)
```

Python wheel version is automatically extracted from this via scikit-build-core metadata provider (in `packaging/pip/pyproject-khiops.toml` and `pyproject-kni.toml`).

## Common Development Workflows

### Adding a Feature

1. **Branch from** `main` (or active feature branch)
2. **Code** in appropriate module (`src/Learning`, `src/Norm`, or `src/Parallel`)
3. **Run tests**: Configure with `-DTESTING=ON`, then `ctest`
4. **Format code**: Pre-commit hooks will format; manually run if needed:
   ```bash
   clang-format -i src/path/to/file.cpp
   cmake-format -i CMakeLists.txt
   black scripts/
   ```
5. **Push & PR**: Ensure pre-commit checks pass before pushing

### Debugging C++ Code

- Use CMake preset with `Debug` build type: `cmake --preset macos-clang-debug`
- Executables in `build/<preset>/bin/`
- Use native debuggers (lldb on macOS, gdb on Linux)
- Set breakpoints in `GlobalExit()` (`src/Norm/base/Standard.cpp`) to catch assertion failures and memory leaks — it is the only call to `exit` in the codebase

### Cross-Platform Considerations

- **Windows**: Uses MSVC, MS-MPI (standalone installer) or Intel MPI `impi-rt` (pip wheels), NSIS installer
- **macOS**: Both Intel and Apple Silicon (ARM64) supported
- **Linux**: x86_64 and ARM64 support, RPM/Debian packages
- Always test platform-specific paths in `packaging/` directory

## Important Patterns & Pitfalls

### ✅ DO

- Use `**/*.cpp` and `**/*.h` glob patterns for C++ code changes
- Enable `TESTING=ON` when modifying core algorithms
- Run full test suite before submitting PR
- Check GitHub Actions workflows in `.github/workflows/`
- Use CMake presets for consistent configuration across platforms
- Respect pre-commit formatting — don't override clang-format

### ❌ DON'T

- Modify version in `CMakeLists.txt` (use `KWKhiopsVersion.h` only)
- Assume MPI availability (some distributions exclude it; mark as optional)
- Add Python dependencies to `pyproject.toml` without team review (wheel is C++-only)
- Commit unformatted code (pre-commit will block it)
- Mix platform-specific code without guards (`#ifdef _WIN32`, etc.)

## GitHub Actions CI/CD

CI workflow details are maintained in [`.github/instructions/ci-workflows.instructions.md`](.github/instructions/ci-workflows.instructions.md).

Quick trigger conventions:
- **Automatic on PR**: run test/packaging checks when relevant files change
- **Automatic on tag push**: build release packaging artifacts
- **Manual (`workflow_dispatch`)**: heavy/specialized jobs (e.g., macOS packaging, conda tests, container builds)

## Module Reference

| Module | Language | Purpose |
|--------|----------|---------|
| **src/Learning** | C++ | MODL, decision trees, Naive Bayes, feature selection, discretization |
| **src/Norm** | C++ | Numerical algorithms, base data structures, memory allocator, assertions (`require`/`ensure`/`assert`), error management, GUI launcher |
| **src/Parallel** | C++ | MPI task distribution, resource management |
| **test/UnitTests** | C++ | Isolated component tests (norm_test, parallel_test) |
| **test/LearningTest** | C++ | 800+ non-regression scenarios (Khiops, Coclustering, KNITransfer). Managed by `test/LearningTestTool/` |
| **packaging/** | CMake, YAML, Python | Distribution configs, Conda, Docker, installers |


## Building Khiops

See the [development environment setup page](https://github.com/KhiopsML/khiops/wiki/Setting-Up-the-Development-Environment) before executing any of these steps.

### Prerequisites

- **CMake** ≥ 3.22
- **Ninja** (default generator, required)
- **MPI**: MPICH (Linux/macOS) or MS-MPI (Windows standalone) / Intel MPI `impi-rt` (Windows pip)
- **Python** ≥ 3.9 (for wheel builds)

### Build System

Built with **CMake** and **Ninja**. Python wheels use **scikit-build-core** (configs in `packaging/pip/`). Native packages (RPM, Debian) are generated with **CPack**.

### CMake Presets

Khiops uses CMake presets for standard build configurations on each platform:

| Platform | Debug Preset | Release Preset |
|----------|-------------|----------------|
| **Windows** | `windows-msvc-debug` | `windows-msvc-release` |
| **Linux** | `linux-gcc-debug` | `linux-gcc-release` |
| **macOS** | `macos-clang-debug` | `macos-clang-release` |

```bash
# List available presets
cmake --list-presets

# Configure + build (example: macOS release)
cmake --preset macos-clang-release
cmake --build --preset macos-clang-release
```

### Quick Build from Scratch

```bash
cd khiops
cmake --fresh -S . -B build -D TESTING=OFF -D BUILD_JARS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build build/ --parallel
```

### Custom CMake Options

| Option | Default | Purpose |
|--------|---------|---------|
| `MPI` | ON | MPI support |
| `TESTING` | OFF | Build and run unit tests with googletest |
| `BUILD_LEX_YACC` | OFF | Rebuild JSON and Khiops Dictionary parsers (requires Flex/Bison) |
| `BUILD_JARS` | ON | Build JARs for the Khiops Desktop GUI |
| `C11` | ON | Use C++11 standard |
| `GENERATE_VIEWS` | OFF | Regenerate GUI C++ files from `.dd` files |

Presets can be modified via:
- `CMakePresets.json`: shared file committed to the repo (for common/temporary settings)
- `CMakeUserPresets.json`: user-specific file, not committed

### Packaging (CPack and Python Wheels)

Detailed packaging guidance is maintained in dedicated instruction files:
- Python wheels (`khiops-core`, `khiops-kni`), `pyproject.toml`, and pip/conda distribution rules: [`.github/instructions/python-wheel.instructions.md`](.github/instructions/python-wheel.instructions.md)
- CPack packaging (DEB, RPM, NSIS, archives), `packaging/install.cmake`, and `packaging/packaging.cmake`: [`.github/instructions/cmake-changes.instructions.md`](.github/instructions/cmake-changes.instructions.md)

Use these instruction files as the source of truth for packaging changes.

### Running Tests

```bash
# Unit tests with CTest (requires -DTESTING=ON)
ctest --preset <preset-name>
ctest --preset macos-clang-debug -j<N>   # Parallel execution

# Test executables
./build/<preset>/bin/norm_test
./build/<preset>/bin/parallel_test
./build/<preset>/bin/learning_test
```

## Debugging and Testing

Detailed debugging/testing guidelines (memory allocator, assertions, error management, trace patterns, unit/integration tests) are in [`.github/instructions/cpp-changes.instructions.md`](.github/instructions/cpp-changes.instructions.md) and apply when editing C++ files.

## User Interface Development

Detailed UI guidelines (MVC pattern, `.dd` files, `genere`, and CMake integration) are in [`.github/instructions/ui-changes.instructions.md`](.github/instructions/ui-changes.instructions.md) and apply when editing UI-related files.

## Parallel Library

Detailed parallel-programming guidelines (`PLParallelTask`, shared variables, registration, simulated mode) are in [`.github/instructions/cpp-changes.instructions.md`](.github/instructions/cpp-changes.instructions.md) and apply when editing C++ files.

## Contact & Resources

- **Team Email**: khiops.team@orange.com
- **Developer Wiki**: https://github.com/KhiopsML/khiops/wiki
- **Issue Tracker**: https://github.com/KhiopsML/khiops/issues