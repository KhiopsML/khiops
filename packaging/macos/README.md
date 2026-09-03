# macOS Packaging

The macOS packaging script builds two self-contained application bundles and a DMG for
the host architecture:

- `Khiops.app`
- `Khiops Coclustering.app`
- `Khiops-<version>.dmg`

The script bypasses CPack because the application bundle contains both serial and MPI
executables and selects the variant at runtime.

## Requirements

The Khiops macOS package follows Apple's macOS support policy: its minimum supported
version is the oldest macOS version officially supported by Apple. As of 2026, this
is macOS 14.0 (Sonoma). The generated app bundle therefore targets macOS 14.0. The
target is passed through `CMAKE_OSX_DEPLOYMENT_TARGET` so that the compiled binaries
advertise the same minimum version, and it is also used for `LSMinimumSystemVersion`
in the generated app bundles.

The release workflow produces one DMG for each supported architecture (`arm64` and
`x86_64`), not one DMG for each macOS version. A DMG built for one architecture must
be used on a Mac with that architecture, or through the appropriate compatibility
layer provided by macOS.

The following tools are required:

- CMake 3.22 or later
- Ninja
- Xcode Command Line Tools, including `clang`, `javac`, `codesign`, and `hdiutil`
- Open MPI available to CMake at build time
- A Java runtime to compile the Khiops JARs

Open MPI can be installed with Homebrew:

```bash
brew install open-mpi
```

The vendored runtime passed to the script must contain at least:

- `legal/`
- `bin/java`
- `lib/server/libjvm.dylib`

A minimal `jlink` runtime or an equivalent JRE distribution is recommended. The JARs
are compiled with Java bytecode target 11, so the runtime must support Java 11.

## Building

Run the command from the repository root:

```bash
bash packaging/macos/build-app.sh <path-to-vendored-jre> <minimum-macos-version> [preset]
```

The preset defaults to `macos-clang-release`. For example:

```bash
bash packaging/macos/build-app.sh /path/to/jre 14.0
```

An alternate preset can be supplied as the third argument:

```bash
bash packaging/macos/build-app.sh /path/to/jre 14.0 macos-clang-debug
```

The script performs these steps:

1. Builds the serial variant with `MPI=OFF` and builds `khiops.jar` and `norm.jar`.
2. Builds the MPI variant with `MPI=ON`.
3. Configures the app-specific scripts and metadata through `packaging/install.cmake`.
4. Installs the serial and MPI artifacts into the common staging directory with the
  `KHIOPS_MACOS_APP` CMake component.
5. Signs the embedded binaries and bundles.
6. Creates a compressed DMG with `hdiutil`.

The script builds for the architecture of the host machine. The CI workflow runs the
script separately for the `arm64` and `x86_64` architectures and publishes the two
resulting DMGs independently.

## MPI Runtime Selection

The app contains these four executables:

- `MODL`
- `MODL_Coclustering`
- `MODL_openmpi`
- `MODL_Coclustering_openmpi`

`khiops_env` searches for Homebrew Open MPI in `/opt/homebrew` on Apple Silicon and
`/usr/local` on Intel.
When Open MPI is available, the MPI executables are selected. Otherwise, the app
falls back to the serial executables and reports that parallel computation is
unavailable.

Open MPI is therefore needed to compile the MPI variant, but it does not need to be
available on the target machine for the application to start in serial mode.

## Generated Files

The MPI build generates temporary app files under:

```text
build/macos-app-mpi/tmp/macos-app/
```

The staging directory contains the final bundles:

```text
build/macos-app-stage/
├── Khiops.app/
│   └── Contents/
│       ├── Info.plist
│       ├── MacOS/khiops
│       └── Resources/
│           ├── bin/
│           ├── doc/licenses/
│           ├── jars/
│           └── jre/
├── Khiops Coclustering.app/
└── LICENSES/
```

The DMG is written to `build/Khiops-<version>.dmg`. In CI, the DMG is published as an
architecture-specific artifact.

The install component is run once for each CMake configuration. The serial
configuration installs the shared resources (JARs, JRE, and licenses), while the
MPI configuration installs the MPI executables, generated launchers, and bundle
metadata. Both configurations target the same staging directory.

## Signing and Notarization

Local builds use ad-hoc signing by default:

```text
codesign --force --sign - ...
```

Ad-hoc signing is sufficient for local testing, but it does not satisfy Gatekeeper
for a downloaded DMG. To sign with a Developer ID certificate, set
`KHIOPS_CODESIGN_IDENTITY` before running the script:

```bash
export KHIOPS_CODESIGN_IDENTITY="Developer ID Application: Example (TEAMID)"
bash packaging/macos/build-app.sh /path/to/jre 14.0
```

The script does not notarize or staple the DMG. Public distribution still requires
submission with `xcrun notarytool` and stapling with `xcrun stapler`.

## Validation

Useful checks after a build are:

```bash
bash -n build/macos-app-stage/Khiops.app/Contents/Resources/bin/khiops_env
bash -n build/macos-app-stage/Khiops.app/Contents/Resources/bin/khiops
plutil -lint build/macos-app-stage/Khiops.app/Contents/Info.plist
otool -l build/macos-app-stage/Khiops.app/Contents/Resources/bin/MODL
codesign --verify --deep --strict build/macos-app-stage/Khiops.app
```

The `LC_BUILD_VERSION` load command reported by `otool` and the
`LSMinimumSystemVersion` value in `Info.plist` should both indicate macOS 14.0.

The generated environment can be inspected with:

```bash
build/macos-app-stage/Khiops.app/Contents/Resources/bin/khiops_env --env
```

## Template Maintenance

`packaging/install.cmake` is the single source for substitutions, generation, and
installation of the app files.

- `packaging/linux/common/khiops.in` and `khiops_env/khiops_env.in` are shared by
  Linux and macOS.
- Files containing `@TOKEN@` placeholders keep the `.in` suffix and are generated
  with `configure_file`.
- Static fragments such as `java_settings.sh` and `LICENSES-README.txt` are copied
  with `configure_file(... COPYONLY)`.
- The application icons are versioned as `khiops.icns` and
  `khiops_coclustering.icns` in `packaging/common/images/` and installed directly
  into each bundle.
- The `KHIOPS_MACOS_APP` install component owns bundle paths and executable
  permissions. `build-app.sh` invokes it for the serial and MPI configurations,
  then handles icons, signing, and DMG creation.