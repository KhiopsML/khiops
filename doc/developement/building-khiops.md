# Building Khiops (In Progress)

See the [development environment setup page](./setting-up-dev-environment.md) before executing any of these steps.

## Default Build Settings
Khiops is built with CMake. For convenience and reproducibility, we provide standard configurations to build in each platform by means of CMake presets. The presets for each platform are:
- Windows
  - `windows-msvc-debug`
  - `windows-msvc-release`
- Linux
  - `linux-gcc-debug`
  - `linux-gcc-release`

- macOS
  - `macos-clang-debug`
  - `macos-clang-release`

So to execute a full release build you run the following commands:
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
If want to use your own settings, other than standard ones (ex. `CMAKE_CXX_COMPILER`, `-G`), we provide the following custom options in form of flags `-D`:
- `MPI` (default `ON`)
  - Whether to build with MPI support
- `TESTING` (default `OFF`)
  - Whether to build and run the unit tests with googletest
- `BUILD_LEX_YACC` (default `OFF`)
  - Whether to rebuild the JSON and Khiops Dictionary parsers
  - Requires a Lex/Yacc installation (ex. Flex/Bison)
- `BUILD_JARS` (default `ON`)
  - Whether to build the JARs implementing the Khiops Desktop GUI
- `C11` (default `ON`)
  - Whether to use the C++11 standard.
- `GENERATE_VIEWS` (default `OFF`)
  - Whether to regenerate the GUI CPP files to reflect changes in the `.dd` files.

The presets mentioned above take the default values of these options.

These presets can be modified using :
- CMakePresets.json: common file saved with commits, for common parameters
  - can be used for temporary settings, by setting an option to `ON`, applying the option, and resetting the option to `OFF` before the commit
- CMakeUserPresets.json: user file not saved with commits, for user parameters.