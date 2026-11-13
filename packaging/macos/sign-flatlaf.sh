#!/bin/bash
set -euo pipefail

# Sign the macOS native libraries inside flatlaf.jar before signing the app bundle.
# Apple notarization requires both architectures to have Developer ID signatures
# with secure timestamps, even when only one architecture is used at runtime.
#
# Usage: bash sign-flatlaf.sh <flatlaf.jar> [signing-identity]
# The JAR is updated in place; the default signing identity is ad hoc ("-").

JAR_PATH="${1:?Usage: sign-flatlaf.sh <flatlaf.jar> [signing-identity]}"
CODESIGN_IDENTITY="${2:--}"
JAR_PATH="$(cd -- "$(dirname -- "$JAR_PATH")" && pwd)/$(basename -- "$JAR_PATH")"
WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/khiops-flatlaf.XXXXXX")"
trap 'rm -rf "$WORK_DIR"' EXIT

NATIVE_DIR="com/formdev/flatlaf/natives"
NATIVE_LIBRARIES=(
    "$NATIVE_DIR/libflatlaf-macos-x86_64.dylib"
    "$NATIVE_DIR/libflatlaf-macos-arm64.dylib"
)

if [[ "$CODESIGN_IDENTITY" == "-" ]]; then
    SIGN_FLAGS=(--force --sign -)
else
    SIGN_FLAGS=(--force --options runtime --timestamp --sign "$CODESIGN_IDENTITY")
fi

mkdir -p "$WORK_DIR/$NATIVE_DIR"
for library in "${NATIVE_LIBRARIES[@]}"; do
    unzip -p "$JAR_PATH" "$library" > "$WORK_DIR/$library"
    codesign "${SIGN_FLAGS[@]}" "$WORK_DIR/$library"
    codesign --verify --strict "$WORK_DIR/$library"
done

(
    cd "$WORK_DIR"
    zip -q "$JAR_PATH" "${NATIVE_LIBRARIES[@]}"
)
