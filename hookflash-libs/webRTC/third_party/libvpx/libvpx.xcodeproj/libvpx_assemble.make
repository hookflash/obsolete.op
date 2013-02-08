all: \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dequantize_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/idctllm_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/idctllm_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/iwalsh_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/iwalsh_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/loopfilter_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/loopfilter_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/mfqe_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/postproc_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/postproc_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/recon_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/recon_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse3.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse4.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_ssse3.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_ssse3.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_ssse3.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dct_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dct_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/encodeopt.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/fwalsh_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_sse4.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_ssse3.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subtract_mmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subtract_sse2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/emms.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/x86_abi_support.o

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dequantize_mmx.o \
    : \
    source/libvpx/vp8/common/x86/dequantize_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/dequantize_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dequantize_mmx.o" "source/libvpx/vp8/common/x86/dequantize_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/idctllm_mmx.o \
    : \
    source/libvpx/vp8/common/x86/idctllm_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/idctllm_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/idctllm_mmx.o" "source/libvpx/vp8/common/x86/idctllm_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/idctllm_sse2.o \
    : \
    source/libvpx/vp8/common/x86/idctllm_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/idctllm_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/idctllm_sse2.o" "source/libvpx/vp8/common/x86/idctllm_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/iwalsh_mmx.o \
    : \
    source/libvpx/vp8/common/x86/iwalsh_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/iwalsh_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/iwalsh_mmx.o" "source/libvpx/vp8/common/x86/iwalsh_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/iwalsh_sse2.o \
    : \
    source/libvpx/vp8/common/x86/iwalsh_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/iwalsh_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/iwalsh_sse2.o" "source/libvpx/vp8/common/x86/iwalsh_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/loopfilter_mmx.o \
    : \
    source/libvpx/vp8/common/x86/loopfilter_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/loopfilter_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/loopfilter_mmx.o" "source/libvpx/vp8/common/x86/loopfilter_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/loopfilter_sse2.o \
    : \
    source/libvpx/vp8/common/x86/loopfilter_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/loopfilter_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/loopfilter_sse2.o" "source/libvpx/vp8/common/x86/loopfilter_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/mfqe_sse2.o \
    : \
    source/libvpx/vp8/common/x86/mfqe_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/mfqe_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/mfqe_sse2.o" "source/libvpx/vp8/common/x86/mfqe_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/postproc_mmx.o \
    : \
    source/libvpx/vp8/common/x86/postproc_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/postproc_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/postproc_mmx.o" "source/libvpx/vp8/common/x86/postproc_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/postproc_sse2.o \
    : \
    source/libvpx/vp8/common/x86/postproc_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/postproc_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/postproc_sse2.o" "source/libvpx/vp8/common/x86/postproc_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/recon_mmx.o \
    : \
    source/libvpx/vp8/common/x86/recon_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/recon_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/recon_mmx.o" "source/libvpx/vp8/common/x86/recon_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/recon_sse2.o \
    : \
    source/libvpx/vp8/common/x86/recon_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/recon_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/recon_sse2.o" "source/libvpx/vp8/common/x86/recon_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_mmx.o \
    : \
    source/libvpx/vp8/common/x86/sad_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/sad_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_mmx.o" "source/libvpx/vp8/common/x86/sad_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse2.o \
    : \
    source/libvpx/vp8/common/x86/sad_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/sad_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse2.o" "source/libvpx/vp8/common/x86/sad_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse3.o \
    : \
    source/libvpx/vp8/common/x86/sad_sse3.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/sad_sse3.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse3.o" "source/libvpx/vp8/common/x86/sad_sse3.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse4.o \
    : \
    source/libvpx/vp8/common/x86/sad_sse4.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/sad_sse4.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_sse4.o" "source/libvpx/vp8/common/x86/sad_sse4.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_ssse3.o \
    : \
    source/libvpx/vp8/common/x86/sad_ssse3.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/sad_ssse3.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/sad_ssse3.o" "source/libvpx/vp8/common/x86/sad_ssse3.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_mmx.o \
    : \
    source/libvpx/vp8/common/x86/subpixel_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/subpixel_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_mmx.o" "source/libvpx/vp8/common/x86/subpixel_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_sse2.o \
    : \
    source/libvpx/vp8/common/x86/subpixel_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/subpixel_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_sse2.o" "source/libvpx/vp8/common/x86/subpixel_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_ssse3.o \
    : \
    source/libvpx/vp8/common/x86/subpixel_ssse3.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/subpixel_ssse3.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subpixel_ssse3.o" "source/libvpx/vp8/common/x86/subpixel_ssse3.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_mmx.o \
    : \
    source/libvpx/vp8/common/x86/variance_impl_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/variance_impl_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_mmx.o" "source/libvpx/vp8/common/x86/variance_impl_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_sse2.o \
    : \
    source/libvpx/vp8/common/x86/variance_impl_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/variance_impl_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_sse2.o" "source/libvpx/vp8/common/x86/variance_impl_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_ssse3.o \
    : \
    source/libvpx/vp8/common/x86/variance_impl_ssse3.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/common/x86/variance_impl_ssse3.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/variance_impl_ssse3.o" "source/libvpx/vp8/common/x86/variance_impl_ssse3.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dct_mmx.o \
    : \
    source/libvpx/vp8/encoder/x86/dct_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/dct_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dct_mmx.o" "source/libvpx/vp8/encoder/x86/dct_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dct_sse2.o \
    : \
    source/libvpx/vp8/encoder/x86/dct_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/dct_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/dct_sse2.o" "source/libvpx/vp8/encoder/x86/dct_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/encodeopt.o \
    : \
    source/libvpx/vp8/encoder/x86/encodeopt.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/encodeopt.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/encodeopt.o" "source/libvpx/vp8/encoder/x86/encodeopt.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/fwalsh_sse2.o \
    : \
    source/libvpx/vp8/encoder/x86/fwalsh_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/fwalsh_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/fwalsh_sse2.o" "source/libvpx/vp8/encoder/x86/fwalsh_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_mmx.o \
    : \
    source/libvpx/vp8/encoder/x86/quantize_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/quantize_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_mmx.o" "source/libvpx/vp8/encoder/x86/quantize_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_sse2.o \
    : \
    source/libvpx/vp8/encoder/x86/quantize_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/quantize_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_sse2.o" "source/libvpx/vp8/encoder/x86/quantize_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_sse4.o \
    : \
    source/libvpx/vp8/encoder/x86/quantize_sse4.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/quantize_sse4.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_sse4.o" "source/libvpx/vp8/encoder/x86/quantize_sse4.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_ssse3.o \
    : \
    source/libvpx/vp8/encoder/x86/quantize_ssse3.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/quantize_ssse3.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/quantize_ssse3.o" "source/libvpx/vp8/encoder/x86/quantize_ssse3.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subtract_mmx.o \
    : \
    source/libvpx/vp8/encoder/x86/subtract_mmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/subtract_mmx.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subtract_mmx.o" "source/libvpx/vp8/encoder/x86/subtract_mmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subtract_sse2.o \
    : \
    source/libvpx/vp8/encoder/x86/subtract_sse2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vp8/encoder/x86/subtract_sse2.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/subtract_sse2.o" "source/libvpx/vp8/encoder/x86/subtract_sse2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/emms.o \
    : \
    source/libvpx/vpx_ports/emms.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vpx_ports/emms.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/emms.o" "source/libvpx/vpx_ports/emms.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/x86_abi_support.o \
    : \
    source/libvpx/vpx_ports/x86_abi_support.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx"
	@echo note: "Compile assemly source/libvpx/vpx_ports/x86_abi_support.asm."
	"$(BUILT_PRODUCTS_DIR)/yasm" -I source/config/mac/ia32 -I source/libvpx -fmacho32 -m x86 -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libvpx/x86_abi_support.o" "source/libvpx/vpx_ports/x86_abi_support.asm"
