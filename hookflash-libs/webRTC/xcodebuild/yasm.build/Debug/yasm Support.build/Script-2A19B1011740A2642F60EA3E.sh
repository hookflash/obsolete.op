#!/bin/sh
echo note: "yasm genmacro for source/patched-yasm/modules/objfmts/coff/win64-nasm.mac."
exec "${BUILT_PRODUCTS_DIR}/genmacro" "${INTERMEDIATE_DIR}/third_party/yasm/win64-nasm.c" win64_nasm_stdmac source/patched-yasm/modules/objfmts/coff/win64-nasm.mac
exit 1

