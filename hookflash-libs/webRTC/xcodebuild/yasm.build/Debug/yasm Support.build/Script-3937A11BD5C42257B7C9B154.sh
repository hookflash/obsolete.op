#!/bin/sh
echo note: "yasm genmacro for ${SHARED_INTERMEDIATE_DIR}/third_party/yasm/version.mac."
exec "${BUILT_PRODUCTS_DIR}/genmacro" "${INTERMEDIATE_DIR}/third_party/yasm/nasm-version.c" nasm_version_mac "${SHARED_INTERMEDIATE_DIR}/third_party/yasm/version.mac"
exit 1

