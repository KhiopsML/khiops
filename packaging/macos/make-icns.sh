#!/bin/bash
set -euo pipefail

# Generate a macOS .icns from a source PNG. Usage: make-icns.sh <source.png> <output.icns>
#
# NOTE: the current source (packaging/common/images/khiops.png, 295x280) is low resolution
# and not perfectly square. This script still produces a working .icns, but upscaled sizes
# (256/512) will look soft. Replace the source with a real 1024x1024 square PNG when
# proper artwork is available, and re-run this script - no other change needed.

SRC="${1:?Usage: make-icns.sh <source.png> <output.icns>}"
OUT="${2:?Usage: make-icns.sh <source.png> <output.icns>}"

ICONSET_DIR=$(mktemp -d)/khiops.iconset
mkdir -p "$ICONSET_DIR"

for size in 16 32 128 256 512; do
    sips -z "$size" "$size" "$SRC" --out "$ICONSET_DIR/icon_${size}x${size}.png" >/dev/null
    double=$((size * 2))
    sips -z "$double" "$double" "$SRC" --out "$ICONSET_DIR/icon_${size}x${size}@2x.png" >/dev/null
done

iconutil -c icns "$ICONSET_DIR" -o "$OUT"
rm -rf "$(dirname "$ICONSET_DIR")"
echo "==> Wrote $OUT"
