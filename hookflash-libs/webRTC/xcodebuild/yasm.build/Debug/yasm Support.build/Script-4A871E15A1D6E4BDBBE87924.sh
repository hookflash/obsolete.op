#!/bin/sh
echo note: "yasm genmacro for source/patched-yasm/modules/objfmts/coff/win64-gas.mac."
exec "${BUILT_PRODUCTS_DIR}/genmacro" "${INTERMEDIATE_DIR}/third_party/yasm/win64-gas.c" win64_gas_stdmac source/patched-yasm/modules/objfmts/coff/win64-gas.mac
exit 1

