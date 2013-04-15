#!/bin/sh
echo note: "Generating yasm module information."
exec "${BUILT_PRODUCTS_DIR}/genmodule" source/patched-yasm/libyasm/module.in source/config/mac/Makefile "${INTERMEDIATE_DIR}/third_party/yasm/module.c"
exit 1

