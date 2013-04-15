#!/bin/sh
echo note: "yasm genmacro for source/patched-yasm/modules/parsers/nasm/nasm-std.mac."
exec "${BUILT_PRODUCTS_DIR}/genmacro" "${INTERMEDIATE_DIR}/third_party/yasm/nasm-macros.c" nasm_standard_mac source/patched-yasm/modules/parsers/nasm/nasm-std.mac
exit 1

