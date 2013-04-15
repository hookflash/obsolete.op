#!/bin/sh
echo note: "Running source/patched-yasm/modules/arch/x86/gen_x86_insn.py."
exec python source/patched-yasm/modules/arch/x86/gen_x86_insn.py "${SHARED_INTERMEDIATE_DIR}/third_party/yasm"
exit 1

