#!/bin/bash
set -euo pipefail

# Build a self-contained Khiops.app + Khiops.dmg for macOS (Option B: bypass CPack).
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
# The launcher scripts themselves are NOT macOS-specific forks: this script reuses the
# shared templates in packaging/linux/common/ (khiops.in, khiops_env/khiops_env.in),
# just substituting different placeholder values (see @KHIOPS_MPI_RUNTIME_SELECT@ and
# @KHIOPS_JAVA_SETTINGS@ below).
#
# Usage: build-app.sh <path-to-vendored-jre> [preset]
# The vendored JRE is expected to be a minimal jlink-produced runtime
# (bin/java, lib/server/libjvm.dylib) - e.g. an Eclipse JustJ minimal JRE.

REPO_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." &>/dev/null && pwd)"
PRESET="${2:-macos-clang-release}"
JRE_SRC_DIR="${1:?Usage: build-app.sh <path-to-vendored-jre> [preset]}"

BUILD_SERIAL_DIR="$REPO_ROOT/build/macos-app-serial"
BUILD_MPI_DIR="$REPO_ROOT/build/macos-app-mpi"
STAGE_DIR="$REPO_ROOT/build/macos-app-stage"
APP_DIR="$STAGE_DIR/Khiops.app"

KHIOPS_VERSION=$(sed -n 's/.*KHIOPS_VERSION KHIOPS_STR(\([^)]*\)).*/\1/p' \
    "$REPO_ROOT/src/Learning/KWUtils/KWKhiopsVersion.h" | head -n1)

echo "==> Building serial variant (MPI=OFF)"
# --release 11 targets the vendored JRE's bytecode level regardless of which (newer) JDK
# Homebrew provides at build time; without it, javac defaults to the host JDK's own
# version and the vendored JRE fails with UnsupportedClassVersionError at runtime.
cmake --preset "$PRESET" -B "$BUILD_SERIAL_DIR" -D MPI=OFF -D TESTING=OFF -D BUILD_JARS=ON \
    -D CMAKE_JAVA_COMPILE_FLAGS="--release;11"
cmake --build "$BUILD_SERIAL_DIR" --target MODL MODL_Coclustering khiops_jar norm_jar --parallel

echo "==> Building MPI variant (MPI=ON, linked against Homebrew's open-mpi)"
cmake --preset "$PRESET" -B "$BUILD_MPI_DIR" -D MPI=ON -D TESTING=OFF -D BUILD_JARS=OFF
cmake --build "$BUILD_MPI_DIR" --target MODL MODL_Coclustering --parallel

echo "==> Assembling Khiops.app in $APP_DIR"
# Retried: if a Finder window is open on $STAGE_DIR, Finder can recreate .DS_Store right
# between the content wipe and the rmdir, making a single rm -rf fail with "Directory not
# empty". Close any Finder window on that path if this keeps failing after the retry.
rm -rf "$STAGE_DIR" 2>/dev/null || rm -rf "$STAGE_DIR"
mkdir -p "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Resources/bin" "$APP_DIR/Contents/Resources/jars"

cp "$BUILD_SERIAL_DIR/bin/MODL" "$BUILD_SERIAL_DIR/bin/MODL_Coclustering" "$APP_DIR/Contents/Resources/bin/"
cp "$BUILD_MPI_DIR/bin/MODL_openmpi" "$BUILD_MPI_DIR/bin/MODL_Coclustering_openmpi" "$APP_DIR/Contents/Resources/bin/"
cp "$BUILD_SERIAL_DIR/jars/khiops.jar" "$BUILD_SERIAL_DIR/jars/norm.jar" "$APP_DIR/Contents/Resources/jars/"

cp -R "$JRE_SRC_DIR" "$APP_DIR/Contents/Resources/jre"

# Reuse the shared launcher templates (packaging/linux/common) instead of maintaining
# macOS-only forks; only the placeholder values differ.
TEMPLATE_DIR="$REPO_ROOT/packaging/linux/common"
SUBST_DIR=$(mktemp -d)
trap 'rm -rf "$SUBST_DIR"' EXIT

# Multi-line placeholders are injected via sed's r/d trick (plain 's' can't hold newlines).
cat >"$SUBST_DIR/java_settings.sh" <<'EOF'
unset KHIOPS_JAVA_ERROR
unset KHIOPS_JAVA_PATH
unset KHIOPS_CLASSPATH
_KHIOPS_APP_BIN_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
_JRE_DIR="$_KHIOPS_APP_BIN_DIR/../jre"
# libjvm.dylib is loaded via dlopen, not exec'd: only readability is required, not +x
if [[ -f "$_JRE_DIR/lib/server/libjvm.dylib" ]]; then
    KHIOPS_JAVA_PATH="$_JRE_DIR/lib/server"
    KHIOPS_CLASSPATH="$_KHIOPS_APP_BIN_DIR/../jars/norm.jar:$_KHIOPS_APP_BIN_DIR/../jars/khiops.jar"
else
    KHIOPS_JAVA_ERROR="Bundled JRE not found ($_JRE_DIR)"
fi
unset _KHIOPS_APP_BIN_DIR
EOF

# OpenMPI is never vendored: it must come from `brew install open-mpi`. Homebrew's prefix
# differs by architecture (Apple Silicon vs Intel), so both are probed. MODL_openmpi is
# only used when a working mpiexec is found; otherwise the serial MODL is used instead
# and KHIOPS_MPI_ERROR carries the message shown to the user (see khiops.in's notify_user).
cat >"$SUBST_DIR/mpi_runtime_select.sh" <<'EOF'
_BREW_MPI_PREFIX=""
for _CANDIDATE_PREFIX in /opt/homebrew /usr/local; do
    if [[ -x "$_CANDIDATE_PREFIX/bin/mpiexec" ]]; then
        _BREW_MPI_PREFIX="$_CANDIDATE_PREFIX"
        break
    fi
done
if [[ -z $HOME ]]; then
    KHIOPS_MPI_HOME=${TMPDIR:-${TEMP:-${TMP:-/tmp}}}
fi
if [[ -n "$_BREW_MPI_PREFIX" && -x "$(get_script_dir)MODL_openmpi" ]]; then
    KHIOPS_PATH="$(get_script_dir)MODL_openmpi"
    KHIOPS_COCLUSTERING_PATH="$(get_script_dir)MODL_Coclustering_openmpi"
    KHIOPS_MPI_COMMAND="$_BREW_MPI_PREFIX/bin/mpiexec"
    if [[ -n $KHIOPS_PROC_NUMBER ]]; then
        KHIOPS_MPI_COMMAND="$KHIOPS_MPI_COMMAND -n $KHIOPS_PROC_NUMBER"
    fi
    if [[ -n $KHIOPS_PROC_NUMBER && $KHIOPS_PROC_NUMBER -le 2 ]]; then
        KHIOPS_PATH="$(get_script_dir)MODL"
        KHIOPS_COCLUSTERING_PATH="$(get_script_dir)MODL_Coclustering"
        KHIOPS_MPI_COMMAND=""
    fi
else
    KHIOPS_PATH="$(get_script_dir)MODL"
    KHIOPS_COCLUSTERING_PATH="$(get_script_dir)MODL_Coclustering"
    KHIOPS_MPI_COMMAND=""
    KHIOPS_MPI_ERROR="OpenMPI not found: Khiops is running in serial mode. Install it with 'brew install open-mpi' to enable parallel computation."
fi
unset _BREW_MPI_PREFIX
KHIOPS_MPI_RUNTIME_SELECTED=true
EOF

sed \
    -e "/@USE_ENVIRONMENT_MODULE@/d" \
    -e "/@KHIOPS_JAVA_SETTINGS@/{" \
    -e "r $SUBST_DIR/java_settings.sh" \
    -e "d" \
    -e "}" \
    -e "/@ADDITIONAL_ENV_VAR@/d" \
    -e "s/@GUI_SUPPORTED@/true/" \
    -e "s#@KHIOPS_DRIVERS_PATH@#\$(get_script_dir)#" \
    -e "/@KHIOPS_MPI_RUNTIME_SELECT@/{" \
    -e "r $SUBST_DIR/mpi_runtime_select.sh" \
    -e "d" \
    -e "}" \
    -e "s#@MODL_PATH@#\$(get_script_dir)#g" \
    -e "s/@MODL_NAME@/MODL/" \
    -e "s/@MODL_COCLUSTERING_NAME@/MODL_Coclustering/" \
    -e "s/@MPI_EXTRA_FLAG@//" \
    -e "s/@SET_PROC_NUMBER@//" \
    -e "s/@MPIEXEC_NUMPROC_FLAG@/-n/" \
    -e "s/@KHIOPS_MPI_QUIET@//" \
    -e "/@ADDITIONAL_ENV_VAR_DISPLAY@/d" \
    "$TEMPLATE_DIR/khiops_env/khiops_env.in" >"$APP_DIR/Contents/Resources/bin/khiops_env"

sed \
    -e "s/@LD_LIBRARY_PATH@/DYLD_LIBRARY_PATH/g" \
    -e "s#@KHIOPS_BINARY_PATH@#\$KHIOPS_PATH#" \
    -e "s/@TOOL_EXT@/_kh/" \
    "$TEMPLATE_DIR/khiops.in" >"$APP_DIR/Contents/Resources/bin/khiops"
sed \
    -e "s/@LD_LIBRARY_PATH@/DYLD_LIBRARY_PATH/g" \
    -e "s#@KHIOPS_BINARY_PATH@#\$KHIOPS_COCLUSTERING_PATH#" \
    -e "s/@TOOL_EXT@/_khc/" \
    "$TEMPLATE_DIR/khiops.in" >"$APP_DIR/Contents/Resources/bin/khiops_coclustering"

sed "s/@KHIOPS_VERSION@/$KHIOPS_VERSION/" "$REPO_ROOT/packaging/macos/Info.plist.in" >"$APP_DIR/Contents/Info.plist"

"$REPO_ROOT/packaging/macos/make-icns.sh" \
    "$REPO_ROOT/packaging/common/images/khiops.png" \
    "$APP_DIR/Contents/Resources/khiops.icns"

# CFBundleExecutable: Finder launches this directly, so it must live in Contents/MacOS
# and simply dispatch to the real launcher shipped in Resources/bin.
cat >"$APP_DIR/Contents/MacOS/khiops" <<'EOF'
#!/bin/bash
DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../Resources/bin" &>/dev/null && pwd)"
exec "$DIR/khiops"
EOF

chmod +x \
    "$APP_DIR/Contents/MacOS/khiops" \
    "$APP_DIR/Contents/Resources/bin/khiops" \
    "$APP_DIR/Contents/Resources/bin/khiops_coclustering" \
    "$APP_DIR/Contents/Resources/bin/khiops_env" \
    "$APP_DIR/Contents/Resources/bin/MODL" \
    "$APP_DIR/Contents/Resources/bin/MODL_Coclustering" \
    "$APP_DIR/Contents/Resources/bin/MODL_openmpi" \
    "$APP_DIR/Contents/Resources/bin/MODL_Coclustering_openmpi"

echo "==> Khiops.app staged at $APP_DIR"

# Ad-hoc signing (no Apple Developer account needed) satisfies the kernel's requirement
# that Mach-O binaries carry a signature to run on Apple Silicon. It does NOT satisfy
# Gatekeeper for a downloaded (quarantined) file - that requires a real Developer ID
# signature plus notarization, which this script deliberately does not attempt.
echo "==> Ad-hoc signing (local/testing only, does not satisfy Gatekeeper for downloads)"
codesign --force --deep --sign - "$APP_DIR"

DMG_PATH="$REPO_ROOT/build/Khiops-$KHIOPS_VERSION.dmg"
echo "==> Creating unsigned DMG at $DMG_PATH"
rm -f "$DMG_PATH"
hdiutil create -volname Khiops -srcfolder "$STAGE_DIR" -format UDZO "$DMG_PATH"

cat <<EOF

Unsigned/unnotarized DMG ready: $DMG_PATH

- Fine for local use and for opening it yourself on this Mac.
- If this DMG is downloaded from anywhere else, macOS will mark it quarantined and
  Gatekeeper will refuse to open it with a normal double-click. Workarounds for
  testing: right-click > Open, or "System Settings > Privacy & Security > Open Anyway",
  or 'xattr -d com.apple.quarantine "$APP_DIR"'.
- Public distribution still requires: codesign with a Developer ID Application
  certificate, 'xcrun notarytool submit', then 'xcrun stapler staple' on the .dmg
  (or on Khiops.app before re-creating the dmg) - none of that is run by this script.
EOF
