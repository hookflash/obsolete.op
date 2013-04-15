#!/bin/sh
echo note: "Generating yasm tokens for lc3b."
exec "${BUILT_PRODUCTS_DIR}/re2c" -s -o "${INTERMEDIATE_DIR}/third_party/yasm/lc3bid.c" source/patched-yasm/modules/arch/lc3b/lc3bid.re
exit 1

