---
name: cmake-changes
description: "Use when modifying CMake files, build configuration, presets, or CTest setup. Covers CMake conventions, preset management, component organization, and cross-platform builds."
applyTo: ["**/CMakeLists.txt", "CMakePresets.json", "**/CMakePresets.json", "CMakeUserPresets.json","packaging/install.cmake", "packaging/packaging.cmake", "scripts/generate_gui.cmake", "scripts/get_mpi_implementation.cmake"]
---

# CMake Build Configuration Instructions for Khiops

## CMake Overview

Khiops uses **CMake 3.22+** with **Ninja** as the default generator. Build configuration is managed through:
- **CMakePresets.json** — Shared, committed presets for team builds
- **CMakeUserPresets.json** — User-local presets (not committed)
- **Preset-specific CMakeLists.txt** — Per-module configuration files
- **`packaging/install.cmake`** — All `install()` rules (centralized)
- **`packaging/packaging.cmake`** — CPack configuration for DEB, RPM, NSIS, archives
- **`scripts/generate_gui.cmake`** — Helper functions for generating C++ GUI code from `.dd` files
- **`scripts/get_mpi_implementation.cmake`** — Helper function to detect the active MPI implementation

## Preset Management

### Available Presets

All presets are defined in `CMakePresets.json`:

| Platform | Debug Preset | Release Preset |
|----------|-------------|----------------|
| **macOS** | `macos-clang-debug` | `macos-clang-release` |
| **Linux** | `linux-gcc-debug` | `linux-gcc-release` |
| **Windows** | `windows-msvc-debug` | `windows-msvc-release` |

### Using Presets

```bash
# List all presets
cmake --list-presets

# Configure with preset
cmake --preset macos-clang-debug

# Build with preset
cmake --build --preset macos-clang-debug

# Run tests with preset
ctest --preset macos-clang-debug -j<N>
```

### Modifying Presets

**For shared changes** (affects team):
- Edit `CMakePresets.json` in repository root
- Commit changes to git
- Common changes: compiler paths, default flags, new platforms

**For personal changes** (local only):
- Create or edit `CMakeUserPresets.json` (git-ignored)
- Override preset values without affecting team builds
- Example: enable sanitizers, custom compiler, local paths

**Preset structure**:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "my-preset",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/my-preset",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "MPI": true,
        "TESTING": true
      }
    }
  ]
}
```

## CMake Variables & Build Options

### Core Variables

| Variable | Default | Purpose | Valid Values |
|----------|---------|---------|--------------|
| `CMAKE_BUILD_TYPE` | Release | Build configuration | Debug, Release, RelWithDebInfo, MinSizeRel |
| `MPI` | ON | Enable MPI parallelization | ON, OFF |
| `TESTING` | OFF | Build unit tests | ON, OFF |
| `BUILD_LEX_YACC` | OFF | Rebuild grammar parsers | ON, OFF |
| `BUILD_JARS` | ON | Build Java GUI components | ON, OFF |
| `GENERATE_VIEWS` | OFF | Regenerate UI from `.dd` files | ON, OFF |
| `C11` | ON | Use C++11 standard | ON, OFF |

### Setting Variables

**At configure time**:

```bash
cmake --preset macos-clang-debug -D TESTING=ON -D MPI=OFF
```

**In CMakePresets.json**:

```json
"cacheVariables": {
  "CMAKE_BUILD_TYPE": "Debug",
  "TESTING": true,
  "MPI": false
}
```

**In CMakeLists.txt**:

```cmake
set(MPI ON CACHE BOOL "Enable MPI" FORCE)
```

## CMakeLists.txt Structure

### File Organization

Each module has a `CMakeLists.txt`:
- **Top-level**: `CMakeLists.txt` — Coordinates all modules
- **Module-level**: `src/Learning/CMakeLists.txt`, `src/Norm/CMakeLists.txt`, etc.
- **Sub-module**: Each component has its own `CMakeLists.txt`

### Standard Layout

```cmake
cmake_minimum_required(VERSION 3.22)
project(khiops)

# Enable testing (if needed)
if(TESTING)
  enable_testing()
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define libraries and executables
add_library(KWLearning STATIC ...)
add_executable(khiops ...)

# Link dependencies
target_link_libraries(KWLearning PUBLIC KWNorm)
target_link_libraries(khiops PRIVATE KWLearning)

# Install rules
install(TARGETS khiops DESTINATION bin COMPONENT KHIOPS_CORE)
install(TARGETS KWLearning DESTINATION lib COMPONENT KHIOPS_CORE)

# Add subdirectories
add_subdirectory(src/Norm)
add_subdirectory(src/Learning)
add_subdirectory(src/Parallel)
```

### Naming Conventions

**Targets** (libraries, executables):
- Main libraries: `KW*` (e.g., `KWLearning`, `KWNorm`, `KWData`)
- Parallel: `PL*` (e.g., `PLParallelTask`)
- Test executables: `*_test` (e.g., `norm_test`, `learning_test`)

**Variables**:
- `MY_VAR` (uppercase with underscores)
- Prefix with module name: `LEARNING_SOURCES`, `NORM_HEADERS`

**Functions**:
- `my_function_name` (lowercase with underscores)
- Custom functions: `khiops_add_executable()`, `khiops_add_test()`

## Component & Installation Setup

### `packaging/install.cmake` — Single Source for All Install Rules

**All `install()` commands are centralized in `packaging/install.cmake`**, not scattered across individual `CMakeLists.txt` files. This file is included by the top-level `CMakeLists.txt`.

**When adding a new installed target**, edit `packaging/install.cmake`. Do NOT add `install()` calls inside module-level `CMakeLists.txt`.

Installation paths adapt to the build context via condition flags:

| Flag | Meaning |
|------|---------|
| `IS_PIP` | Building for pip wheel (uses `SKBUILD_*` variables for paths) |
| `IS_CONDA` | Building for conda (some installs disabled) |
| `IS_WINDOWS` | Windows-specific paths and layout |
| `IS_LINUX` | Linux paths (`usr/bin`, `usr/lib`, etc.) |
| `IS_MACOS` | macOS paths (no GUI support) |
| `IS_FEDORA_LIKE` | RPM-based distros (Fedora/Rocky-specific paths) |

Example structure inside `install.cmake`:

```cmake
# Destination changes depending on context
if(IS_PIP)
  set(LIB_DIR ${SKBUILD_DATA_DIR}/lib)
elseif(IS_WINDOWS)
  set(LIB_DIR lib)
else()
  set(LIB_DIR usr/lib)
endif()

# Install rule uses computed destination
install(
  TARGETS KhiopsNativeInterface
  LIBRARY DESTINATION ${LIB_DIR} COMPONENT KNI
  PUBLIC_HEADER DESTINATION ${INCLUDE_DIR} COMPONENT KNI
)
```

### Install Components

Each installed file belongs to a named component. Components map to distributed packages:

| Component | Contents | Distributed in |
|-----------|----------|-----------------|
| `KHIOPS_CORE` | Core Khiops binaries (no GUI, no docs) | pip wheel, khiops-core Native installers (DEB, RPM) |
| `KHIOPS` | Full GUI distribution | khiops Native installers (DEB, RPM) |
| `KNI` | KhiopsNativeInterface shared lib + header | `khiops-kni` pip wheel, KNI Native installers (DEB, RPM) |
| `KNI_TRANSFER` | KNITransfer test binary | Separate tech package |
| `KHISTO` | Khisto histogram tool | Separate package |

Test executables have **no component** and are therefore never installed:

```cmake
# In CMakeLists.txt: build only, no install
add_executable(norm_test ...)
# No install() call for test targets
```

## Platform-Specific Configuration

`set_khiops_options()` function in top-level `CMakeLists.txt` configures platform-specific options, it should be used on each target to ensure consistent configuration across platforms. For example:
```cmake
set_khiops_options(MODL)
```

### MPI Configuration

```cmake
if(MPI)
  find_package(MPI REQUIRED)
  target_link_libraries(PLParallelTask PUBLIC MPI::MPI_CXX)
else()
  message(STATUS "MPI disabled — sequential execution only")
endif()
```

The only library that depends on MPI is `PLMPI`, which is only built when `MPI` is enabled. All other libraries and executables should be independent of MPI to allow building without it.

### `scripts/get_mpi_implementation.cmake` — MPI Implementation Detection

This script provides the `get_mpi_implementation()` function, which detects which MPI implementation is installed (OpenMPI, MPICH, or Intel MPI) and sets the appropriate variables.

**When to use**: when build behavior must differ depending on the MPI implementation (e.g., different launcher commands or library flags).

```cmake
# In CMakeLists.txt
include(${PROJECT_SOURCE_DIR}/scripts/get_mpi_implementation.cmake)
get_mpi_implementation()

# After the call, these variables are set:
# MPI_IMPL     — "openmpi", "mpich", or "intel" (empty if not detected)
# IS_OPEN_MPI  — TRUE if OpenMPI
# IS_MPICH     — TRUE if MPICH
# IS_INTEL_MPI — TRUE if Intel MPI

if(IS_MPICH)
  # MPICH-specific configuration
endif()
```

**Detection strategy** (adapts to environment):
- **Standard**: searches `MPI_LIBRARIES` path (from `find_package(MPI)`)
- **pip**: queries `importlib.metadata` for installed package names
- **conda build**: reads `$ENV{mpi}` variable
- **conda runtime**: runs `conda list | grep mpi`

### `scripts/generate_gui.cmake` — GUI Code Generation from `.dd` Files

This script provides two functions for generating C++ view classes from `.dd` attribute specification files using the `genere` tool. Only active when `GENERATE_VIEWS=ON`.

**Functions**:
- `generate_gui_add_view(ClassName ClassLabel AttributeFileName LogFile [options])` — registers a code generation step triggered when the `.dd` file changes
- `generate_gui_add_view_add_dependency(TargetName)` — ensures `genere` is built before the target

```cmake
# In CMakeLists.txt
include(${PROJECT_SOURCE_DIR}/scripts/generate_gui.cmake)

generate_gui_add_view(KWDatabase "Database" KWDatabase.dd KWDatabase.dd.log -nomodel -noarrayview)

add_executable(myTarget ...)
if(GENERATE_VIEWS)
  target_sources(myTarget PRIVATE KWDatabase.dd.log)  # Required: triggers re-generation
endif()
generate_gui_add_view_add_dependency(myTarget)
```

**Options** (same as the `genere` tool):

| Option | Effect |
|--------|--------|
| `-nomodel` | Skip generating the model class |
| `-noview` | Skip generating view and array-view classes |
| `-noarrayview` | Skip generating the array-view class |
| `-super <ClassName>` | Set a custom parent class |
| `-nousersection` | Do not generate `// ## Custom` user sections |

**Important**: the `.dd.log` file must be added as a target source with `target_sources()`. Without it, `generate_gui_add_view` is never triggered. The log file acts as a build signal — it is the declared output of the custom command.

**Workflow**:
1. Set `GENERATE_VIEWS=ON` in preset or on command line
2. Modify the `.dd` file
3. Build — `genere` regenerates the C++ view files automatically
4. Code in `// ## Custom ... // ##` sections is preserved across regenerations

### Library Search

```cmake
# Find system library
find_package(MPI REQUIRED)

# Or use optional find
find_package(FLEX)
find_package(BISON)

if(NOT FLEX_FOUND OR NOT BISON_FOUND)
  message(WARNING "Lex/Yacc not available — using pre-generated files")
endif()
```

## CTest & Testing Setup

### Adding Tests

```cmake
# Enable testing
enable_testing()

# Add a test executable
add_executable(norm_test test/UnitTests/norm_test.cpp ...)

# Register it with CTest
add_test(NAME NormTest COMMAND norm_test)

# With custom working directory or environment
add_test(
  NAME LearningTest
  COMMAND learning_test --verbose
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
```

### Test Organization

```cmake
# Unit tests (isolated, fast)
if(TESTING)
  add_executable(norm_test ${NORM_TEST_SOURCES})
  target_link_libraries(norm_test PRIVATE KWNorm gtest)
  add_test(NAME NormUnitTests COMMAND norm_test)
endif()

```

### CTest Execution

```bash
# Run all tests
ctest --preset macos-clang-debug

# Run specific test
ctest --preset macos-clang-debug -R NormTest

# Parallel execution
ctest --preset macos-clang-debug -j4

# Verbose output
ctest --preset macos-clang-debug --verbose

# Stop on first failure
ctest --preset macos-clang-debug --stop-on-failure
```

## External Dependencies

### Finding Packages

```cmake
# Required dependency
find_package(MPI REQUIRED)

# Optional dependency
find_package(FLEX)
if(FLEX_FOUND)
  target_include_directories(MyTarget PRIVATE ${FLEX_INCLUDE_DIRS})
else()
  message(STATUS "FLEX not found — grammar generation disabled")
endif()
```

### Linking

```cmake
# Link public dependency (visible to consumers)
target_link_libraries(MyLib PUBLIC OtherLib)

# Link private dependency (internal only)
target_link_libraries(MyLib PRIVATE InternalLib)

# Link interface (header-only, no linking needed)
target_link_libraries(MyLib INTERFACE HeaderOnlyLib)
```

### Custom Find Modules

For third-party packages without official CMake support:

```cmake
# Place in cmake/FindMyPackage.cmake
# Then use:
find_package(MyPackage REQUIRED)
```

## Common CMake Operations

### Adding a New Library

```cmake
# Collect sources
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.h")

# Create library
add_library(MyLib STATIC ${SOURCES} ${HEADERS})

# Set options and properties
set_khiops_options(MyLib)

# Set include directories
target_include_directories(MyLib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(MyLib PUBLIC KWNorm)

# In packaging/install.cmake: install rule
install(
  TARGETS MyLib
  LIBRARY DESTINATION lib COMPONENT KHIOPS_CORE
  ARCHIVE DESTINATION lib COMPONENT KHIOPS_CORE
)
```

### Adding an Executable

```cmake
# In the module CMakeLists.txt: build only
add_executable(my_program src/main.cpp)
set_khiops_options(my_program)
target_link_libraries(my_program PRIVATE MyLib)

# In packaging/install.cmake: install rule
install(TARGETS my_program DESTINATION bin COMPONENT KHIOPS_CORE)
```

## CPack & Packaging (`packaging/packaging.cmake`)

### `packaging/packaging.cmake` — CPack Configuration

All CPack configuration for native packages (DEB, RPM, NSIS, Archive) is in **`packaging/packaging.cmake`**, included by the top-level `CMakeLists.txt`.

This file sets:
- Package metadata (vendor, contact, homepage, license)
- Per-component descriptions (shown in package managers)
- Generator-specific settings (DEB, RPM, ARCHIVE, NSIS)

### Key CPack Variables

```cmake
# Global metadata (packaging.cmake)
set(CPACK_PACKAGE_VENDOR "Orange")
set(CPACK_PACKAGE_CONTACT "Khiops Team <khiops.team@orange.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL https://khiops.org)
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

# Component-based packaging (one package per component)
set(CPACK_DEB_COMPONENT_INSTALL YES)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

# Package version taken from KHIOPS_VERSION
set(CPACK_DEBIAN_PACKAGE_VERSION ${KHIOPS_VERSION})
```

### Adding a Component Description

Each component distributed as a package needs a description in `packaging.cmake`:

```cmake
# In packaging/packaging.cmake
set(CPACK_COMPONENT_MY_COMPONENT_DESCRIPTION
    "Short title
Longer multi-line description of what this component provides.
Describe contents, purpose, and dependencies.")
```

> **Convention**: `CPACK_COMPONENT_<UPPERCASE_COMPONENT_NAME>_DESCRIPTION`

### Building Packages

```bash
# Configure
cmake --preset linux-gcc-release

# Build
cmake --build --preset linux-gcc-release

# Generate DEB packages
cpack --preset linux-gcc-release -G DEB

# Generate RPM packages
cpack --preset linux-gcc-release -G RPM

# Generate Archive (tar.gz)
cpack --preset linux-gcc-release -G TGZ

# Only one component
cpack --preset linux-gcc-release -G DEB -D CPACK_COMPONENTS_ALL=KHIOPS_CORE
```

Packages are output to `build/<preset>/packages/`.

### DEB-Specific Settings

```cmake
# In packaging/packaging.cmake — modify these when changing DEB packaging
set(CPACK_DEBIAN_PACKAGE_RELEASE 1)         # Package iteration (not software version)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)     # Use standard Debian naming convention
set(CPACK_DEBIAN_PACKAGE_SECTION "math")    # Debian section
set(CPACK_DEBIAN_PACKAGE_VERSION ${KHIOPS_VERSION})  # Full version including pre-release suffix
```

### Archive Naming

```cmake
# In packaging/packaging.cmake — user-friendly archive filenames
set(CPACK_ARCHIVE_KHIOPS_CORE_FILE_NAME khiops-core-${KHIOPS_VERSION})
set(CPACK_ARCHIVE_KNI_FILE_NAME kni-${KHIOPS_VERSION})
set(CPACK_ARCHIVE_KHISTO_FILE_NAME khisto-${KHIOPS_VERSION})
```

## Pre-commit & Validation

### Running Pre-commit Checks

```bash
# Check CMake formatting
cmake-format --check CMakeLists.txt

# Auto-format
cmake-format -i CMakeLists.txt

# Validate syntax
cmake --lint=style CMakeLists.txt
```

**Pre-commit hook checks**:
- `cmake-format` for style consistency
- Syntax validation (optional)
- Copyright year verification
- UTF-8 encoding

## Common Pitfalls

### ❌ Adding install() Outside `packaging/install.cmake`

```cmake
# ❌ Wrong: install() inside a module CMakeLists.txt
# src/Learning/CMakeLists.txt
add_executable(MODL ...)
install(TARGETS MODL DESTINATION bin COMPONENT KHIOPS_CORE)  # Don't do this here

# ✅ Right: build in CMakeLists.txt, install in packaging/install.cmake
# src/Learning/CMakeLists.txt
add_executable(MODL ...)

# packaging/install.cmake
install(TARGETS MODL DESTINATION ${BIN_DIR} COMPONENT KHIOPS_CORE)
```

### ❌ Forgetting COMPONENT in install()

```cmake
# ❌ Wrong: Installs to default (all) components
install(TARGETS myexe DESTINATION bin)

# ✅ Right: Explicitly specify component
install(TARGETS myexe DESTINATION bin COMPONENT KHIOPS_CORE)
```

### ❌ Using Glob with Git Ignore

```cmake
# ❌ Bad: Misses new files added after configure
file(GLOB SOURCES "src/*.cpp")

# ✅ Better: Explicit list or wildcard with explicit paths
set(SOURCES
  src/file1.cpp
  src/file2.cpp
)
# Or regenerate after adding files
```

### ❌ Not Setting Build Type in Preset

```json
{
  "cacheVariables": {
    // ❌ Missing CMAKE_BUILD_TYPE
    "TESTING": true
  }
}

// ✅ Add explicit build type
{
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "TESTING": true
  }
}
```

### ❌ Hardcoding Paths

```cmake
# ❌ Wrong: Not portable
set(MPI_PATH "/usr/lib/mpich")

# ✅ Right: Let find_package() locate it
find_package(MPI REQUIRED)
```

### ❌ Breaking Cross-Platform with Absolute Paths

```cmake
# ❌ Wrong: Windows-only path
target_include_directories(MyLib PRIVATE "C:\\dev\\include")

# ✅ Right: Relative or find_package()
target_include_directories(MyLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

### ❌ Installing Test Executables

```cmake
# ❌ Wrong: Tests included in distribution
install(TARGETS norm_test DESTINATION bin COMPONENT KHIOPS_CORE)

# ✅ Right: No component = not installed
add_executable(norm_test ...)
# No install() call for test targets
```

## Resources

- **CMake Documentation**: https://cmake.org/cmake/help/latest/
- **CMake Best Practices**: https://cliutils.gitlab.io/modern-cmake/
- **CMakePresets Reference**: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
- **scikit-build-core**: https://scikit-build-core.readthedocs.io/
- **Khiops CMake Structure**: Top-level `CMakeLists.txt` and per-module configs in `src/*/CMakeLists.txt`
