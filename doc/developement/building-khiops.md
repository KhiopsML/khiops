# Building Khiops (In Progress)

See the [development environment setup page](./setting-up-dev-environment.md) before following these steps.

## Default Build Settings
Khiops is built with CMake. For convenience and reproducibility, we provide standard configurations for each platform using CMake presets. The presets for each platform are:
- Windows
  - `windows-msvc-debug`
  - `windows-msvc-release`
- Linux
  - `linux-gcc-debug`
  - `linux-gcc-release`

- macOS
  - `macos-clang-debug`
  - `macos-clang-release`

To perform a full release build, run the following commands:
```bash
# Windows
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release

# Linux
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release

# macOS
cmake --preset macos-clang-release
cmake --build --preset macos-clang-release
```



## Custom Options
If you want to use custom settings beyond the standard ones (e.g., `CMAKE_CXX_COMPILER` or a different generator specified with `-G`), you can use CMake command-line options. The following options can be set with `-D` flags:
- `MPI` (default `ON`)
  - Whether to build with MPI support
- `TESTING` (default `OFF`)
  - Whether to build and run the unit tests with GoogleTest
- `BUILD_LEX_YACC` (default `OFF`)
  - Whether to rebuild the JSON and Khiops Dictionary parsers
  - Requires a Lex/Yacc installation (e.g., Flex/Bison)
- `BUILD_JARS` (default `ON`)
  - Whether to build the JARs implementing the Khiops Desktop GUI
- `C11` (default `ON`)
  - Whether to use the C++11 standard
- `GENERATE_VIEWS` (default `OFF`)
  - Whether to regenerate the GUI C++ files to reflect changes in the `.dd` files.

The presets mentioned above take the default values of these options. These presets can be modified using:
- `CMakePresets.json`: a shared file committed to the repository, for common settings
  - It can be used for temporary settings: set an option to `ON`, apply the setting, and reset the option to `OFF` before committing.
- `CMakeUserPresets.json`: a user-specific file that is not committed to the repository, for local settings.