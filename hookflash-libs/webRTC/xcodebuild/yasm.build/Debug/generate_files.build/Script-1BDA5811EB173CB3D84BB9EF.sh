#!/bin/sh
echo note: "Generating yasm version file: ${SHARED_INTERMEDIATE_DIR}/third_party/yasm/version.mac."
exec "${BUILT_PRODUCTS_DIR}/genversion" "${SHARED_INTERMEDIATE_DIR}/third_party/yasm/version.mac"
exit 1

