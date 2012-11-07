all: \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jccolmmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jccolss2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcgrammx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcgrass2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnt3dn.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqntmmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnts2f.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnts2i.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqntsse.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcsammmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcsamss2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdcolmmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdcolss2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdmermmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdmerss2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdsammmx.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdsamss2.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jf3dnflt.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfmmxfst.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfmmxint.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfss2fst.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfss2int.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfsseflt.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/ji3dnflt.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxfst.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxint.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxred.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2flt.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2fst.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2int.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2red.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jisseflt.o \
    $(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jsimdcpu.o

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jccolmmx.o \
    : \
    simd/jccolmmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jccolmmx.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jccolmmx.o" "simd/jccolmmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jccolss2.o \
    : \
    simd/jccolss2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jccolss2.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jccolss2.o" "simd/jccolss2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcgrammx.o \
    : \
    simd/jcgrammx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcgrammx.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcgrammx.o" "simd/jcgrammx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcgrass2.o \
    : \
    simd/jcgrass2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcgrass2.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcgrass2.o" "simd/jcgrass2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnt3dn.o \
    : \
    simd/jcqnt3dn.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcqnt3dn.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnt3dn.o" "simd/jcqnt3dn.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqntmmx.o \
    : \
    simd/jcqntmmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcqntmmx.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqntmmx.o" "simd/jcqntmmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnts2f.o \
    : \
    simd/jcqnts2f.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcqnts2f.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnts2f.o" "simd/jcqnts2f.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnts2i.o \
    : \
    simd/jcqnts2i.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcqnts2i.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqnts2i.o" "simd/jcqnts2i.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqntsse.o \
    : \
    simd/jcqntsse.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcqntsse.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcqntsse.o" "simd/jcqntsse.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcsammmx.o \
    : \
    simd/jcsammmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcsammmx.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcsammmx.o" "simd/jcsammmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcsamss2.o \
    : \
    simd/jcsamss2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jcsamss2.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jcsamss2.o" "simd/jcsamss2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdcolmmx.o \
    : \
    simd/jdcolmmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jdcolmmx.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdcolmmx.o" "simd/jdcolmmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdcolss2.o \
    : \
    simd/jdcolss2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jdcolss2.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdcolss2.o" "simd/jdcolss2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdmermmx.o \
    : \
    simd/jdmermmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jdmermmx.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdmermmx.o" "simd/jdmermmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdmerss2.o \
    : \
    simd/jdmerss2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jdmerss2.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdmerss2.o" "simd/jdmerss2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdsammmx.o \
    : \
    simd/jdsammmx.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jdsammmx.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdsammmx.o" "simd/jdsammmx.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdsamss2.o \
    : \
    simd/jdsamss2.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jdsamss2.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jdsamss2.o" "simd/jdsamss2.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jf3dnflt.o \
    : \
    simd/jf3dnflt.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jf3dnflt.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jf3dnflt.o" "simd/jf3dnflt.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfmmxfst.o \
    : \
    simd/jfmmxfst.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jfmmxfst.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfmmxfst.o" "simd/jfmmxfst.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfmmxint.o \
    : \
    simd/jfmmxint.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jfmmxint.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfmmxint.o" "simd/jfmmxint.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfss2fst.o \
    : \
    simd/jfss2fst.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jfss2fst.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfss2fst.o" "simd/jfss2fst.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfss2int.o \
    : \
    simd/jfss2int.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jfss2int.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfss2int.o" "simd/jfss2int.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfsseflt.o \
    : \
    simd/jfsseflt.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jfsseflt.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jfsseflt.o" "simd/jfsseflt.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/ji3dnflt.o \
    : \
    simd/ji3dnflt.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building ji3dnflt.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/ji3dnflt.o" "simd/ji3dnflt.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxfst.o \
    : \
    simd/jimmxfst.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jimmxfst.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxfst.o" "simd/jimmxfst.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxint.o \
    : \
    simd/jimmxint.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jimmxint.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxint.o" "simd/jimmxint.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxred.o \
    : \
    simd/jimmxred.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jimmxred.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jimmxred.o" "simd/jimmxred.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2flt.o \
    : \
    simd/jiss2flt.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jiss2flt.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2flt.o" "simd/jiss2flt.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2fst.o \
    : \
    simd/jiss2fst.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jiss2fst.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2fst.o" "simd/jiss2fst.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2int.o \
    : \
    simd/jiss2int.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jiss2int.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2int.o" "simd/jiss2int.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2red.o \
    : \
    simd/jiss2red.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jiss2red.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jiss2red.o" "simd/jiss2red.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jisseflt.o \
    : \
    simd/jisseflt.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jisseflt.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jisseflt.o" "simd/jisseflt.asm"

$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jsimdcpu.o \
    : \
    simd/jsimdcpu.asm \
    $(BUILT_PRODUCTS_DIR)/yasm
	@mkdir -p "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo"
	@echo note: "Building jsimdcpu.o"
	"$(BUILT_PRODUCTS_DIR)/yasm" -fmacho -DMACHO -Imac/ -DRGBX_FILLER_0XFF -DSTRICT_MEMORY_ACCESS -Isimd/ -o "$(SHARED_INTERMEDIATE_DIR)/third_party/libjpeg_turbo/jsimdcpu.o" "simd/jsimdcpu.asm"
