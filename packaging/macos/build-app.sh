#!/bin/bash
set -euo pipefail

# Build a self-contained Khiops.app + Khiops.dmg for macOS (bypass CPack).
#
# Two full CMake configurations are required because MPI is a compile-time choice
# (MODL/MODL_Coclustering are linked against OpenMPI when MPI=ON):
#   - "mpi" build:    MPI=ON  -> produces MODL_openmpi, MODL_Coclustering_openmpi
#                      (linked against Homebrew's open-mpi, NOT vendored/signed by us)
#   - "serial" build: MPI=OFF -> produces MODL, MODL_Coclustering (no external dependency)
#
# Khiops.app ships both binaries; khiops_env picks the MPI variant at runtime only if
# Homebrew's open-mpi is installed, otherwise it falls back to the serial one and warns
# the user via a macOS notification.
#
# The launcher scripts themselves are NOT macOS-specific forks: install.cmake configures
# the shared templates in packaging/linux/common/ (khiops.in, khiops_env/khiops_env.in)
# with the bundle-specific values before this script assembles the applications.
#
# Usage: build-app.sh <path-to-vendored-jre> <minimum-macos-version> [preset]
# The vendored JRE is expected to be a minimal jlink-produced runtime
# (bin/java, lib/server/libjvm.dylib) - e.g. an Eclipse JustJ minimal JRE.

REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." &>/dev/null && pwd)"
JRE_SRC_DIR="${1:?Usage: build-app.sh <path-to-vendored-jre> <minimum-macos-version> [preset]}"
MACOS_MINIMUM_SYSTEM_VERSION="${2:?Usage: build-app.sh <path-to-vendored-jre> <minimum-macos-version> [preset]}"
PRESET="${3:-macos-clang-release}"

# Minimum macOS system version for the app bundle, supplied by the caller.

BUILD_SERIAL_DIR="$REPO_ROOT/build/macos-app-serial"
BUILD_MPI_DIR="$REPO_ROOT/build/macos-app-mpi"
STAGE_DIR="$REPO_ROOT/build/macos-app-stage"
APP_DIR="$STAGE_DIR/Khiops.app"
COCLUSTERING_APP_DIR="$STAGE_DIR/Khiops Coclustering.app"

KHIOPS_VERSION=$(sed -n 's/.*KHIOPS_VERSION KHIOPS_STR(\([^)]*\)).*/\1/p' \
    "$REPO_ROOT/src/Learning/KWUtils/KWKhiopsVersion.h" | head -n1)

echo "==> Building serial variant (MPI=OFF)"
# --release 11 targets the vendored JRE's bytecode level regardless of which (newer) JDK
# Homebrew provides at build time; without it, javac defaults to the host JDK's own
# version and the vendored JRE fails with UnsupportedClassVersionError at runtime.
cmake --preset "$PRESET" -B "$BUILD_SERIAL_DIR" -D MPI=OFF -D TESTING=OFF -D BUILD_JARS=ON \
    -D CMAKE_JAVA_COMPILE_FLAGS="--release;11" -D KHIOPS_MACOS_APP=ON \
    -D CMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_MINIMUM_SYSTEM_VERSION" \
    -D KHIOPS_MACOS_JRE_DIR="$JRE_SRC_DIR"
cmake --build "$BUILD_SERIAL_DIR" --target MODL MODL_Coclustering khiops_jar norm_jar --parallel

echo "==> Building MPI variant (MPI=ON, linked against Homebrew's open-mpi)"
# Khiops JAR files have already been built in the sequential build
cmake --preset "$PRESET" -B "$BUILD_MPI_DIR" -D MPI=ON -D TESTING=OFF -D BUILD_JARS=OFF -D KHIOPS_MACOS_APP=ON \
    -D CMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_MINIMUM_SYSTEM_VERSION" \
    -D KHIOPS_MACOS_JRE_DIR="$JRE_SRC_DIR"
cmake --build "$BUILD_MPI_DIR" --target MODL MODL_Coclustering --parallel

echo "==> Assembling Khiops.app in $APP_DIR"
# Retried: if a Finder window is open on $STAGE_DIR, Finder can recreate .DS_Store right
# between the content wipe and the rmdir, making a single rm -rf fail with "Directory not
# empty". Close any Finder window on that path if this keeps failing after the retry.
rm -rf "$STAGE_DIR" 2>/dev/null || rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"
cmake --install "$BUILD_SERIAL_DIR" --prefix "$STAGE_DIR" --component KHIOPS_MACOS_APP
cmake --install "$BUILD_MPI_DIR" --prefix "$STAGE_DIR" --component KHIOPS_MACOS_APP

echo "==> Khiops.app staged at $APP_DIR"
echo "==> Khiops Coclustering.app staged at $COCLUSTERING_APP_DIR"

# Ad-hoc signing (no Apple Developer account needed) satisfies the kernel's requirement
# that Mach-O binaries carry a signature to run on Apple Silicon. It does NOT satisfy
# Gatekeeper for a downloaded (quarantined) file - that requires a real Developer ID
# signature plus notarization, which this script deliberately does not attempt.
# Set KHIOPS_CODESIGN_IDENTITY to a Developer ID Application identity in CI. Keep
# ad-hoc signing as the default for local builds and development.
CODESIGN_IDENTITY="${KHIOPS_CODESIGN_IDENTITY:--}"
EMBEDDED_BINARIES=(
    MODL
    MODL_Coclustering
    MODL_openmpi
    MODL_Coclustering_openmpi
)

sign_embedded_binaries() {
    local bundle_dir="$1"
    for binary_name in "${EMBEDDED_BINARIES[@]}"; do
        codesign "${CODESIGN_FLAGS[@]}" \
            "$bundle_dir/Contents/Resources/bin/$binary_name"
    done
}

if [[ "$CODESIGN_IDENTITY" == "-" ]]; then
    echo "==> Ad-hoc signing (local/testing only, does not satisfy Gatekeeper for downloads)"
    CODESIGN_FLAGS=(--force --sign -)
else
    echo "==> Signing with Developer ID identity: $CODESIGN_IDENTITY"
    CODESIGN_FLAGS=(--force --options runtime --timestamp --sign "$CODESIGN_IDENTITY")
fi

sign_embedded_binaries "$APP_DIR"
sign_embedded_binaries "$COCLUSTERING_APP_DIR"
codesign --deep "${CODESIGN_FLAGS[@]}" "$APP_DIR"
codesign --deep "${CODESIGN_FLAGS[@]}" "$COCLUSTERING_APP_DIR"

DMG_PATH="$REPO_ROOT/build/Khiops-$KHIOPS_VERSION.dmg"
echo "==> Creating DMG at $DMG_PATH"
rm -f "$DMG_PATH"
hdiutil create -volname Khiops -srcfolder "$STAGE_DIR" -format UDZO "$DMG_PATH"

cat <<EOF

DMG ready: $DMG_PATH

- Local builds use ad-hoc signing and are fine for local testing on MacOS machines.
- A Developer ID-signed DMG still needs notarization before public distribution.
- If an unnotarized DMG is downloaded from anywhere else, macOS will mark it quarantined and
  Gatekeeper will refuse to open it with a normal double-click. Workarounds for
  testing: right-click > Open, or "System Settings > Privacy & Security > Open Anyway",
  or 'xattr -d com.apple.quarantine "$APP_DIR"'.
- Public distribution still requires: codesign with a Developer ID Application
  certificate, 'xcrun notarytool submit', then 'xcrun stapler staple' on the .dmg
  (or on Khiops.app before re-creating the dmg) - none of that is run by this script.
EOF
