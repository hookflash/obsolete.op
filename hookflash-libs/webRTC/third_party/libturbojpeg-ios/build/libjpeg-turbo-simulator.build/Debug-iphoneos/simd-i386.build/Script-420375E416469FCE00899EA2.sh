#!/bin/bash
cd "$SRCROOT/libturbojpeg/simd-i386"
make clean
make

mkdir -p "$(dirname "$SCRIPT_INPUT_FILE_0")/lib"
cp "$SCRIPT_INPUT_FILE_0" "$SCRIPT_OUTPUT_FILE_0"
