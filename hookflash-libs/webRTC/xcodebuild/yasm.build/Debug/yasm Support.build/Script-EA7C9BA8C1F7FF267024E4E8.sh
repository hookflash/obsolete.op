#!/bin/sh
echo note: "Generating yasm embeddable license."
exec "${BUILT_PRODUCTS_DIR}/genstring" license_msg "${INTERMEDIATE_DIR}/third_party/yasm/license.c" source/patched-yasm/COPYING
exit 1

