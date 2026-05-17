#!/bin/bash
# Author  : Sylvain Deguire (VA2OPS)
# Date    : May 2026
# Purpose : Build QtPatWinlink out-of-source

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../QtPatWinlink-build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

qmake6 "$SCRIPT_DIR/QtPatWinlink.pro"
make -j$(nproc)

echo "Binary: $BUILD_DIR/QtPatWinlink"
