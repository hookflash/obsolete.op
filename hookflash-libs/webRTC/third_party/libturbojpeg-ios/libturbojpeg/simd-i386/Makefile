.DEFAULT_GOAL: libsimd

libsimd_objects = jsimdcpu.o \
	jccolmmx.o jdcolmmx.o jcgrammx.o \
	jcsammmx.o jdsammmx.o jdmermmx.o \
	jcqntmmx.o jfmmxfst.o jfmmxint.o \
	jimmxred.o jimmxint.o jimmxfst.o \
	jcqnt3dn.o jf3dnflt.o ji3dnflt.o \
	jcqntsse.o jfsseflt.o jisseflt.o \
	jccolss2.o jdcolss2.o jcgrass2.o \
	jcsamss2.o jdsamss2.o jdmerss2.o \
	jcqnts2i.o jfss2fst.o jfss2int.o \
	jiss2red.o jiss2int.o jiss2fst.o \
	jcqnts2f.o jiss2flt.o

BUILD_DIR=build

libsimd: prepare build/jsimdcfg.inc jsimd_i386
	cd build && ar -r libsimd-i386.a $(libsimd_objects) "jsimd_i386.o"
	
.PHONY: clean

clean:
	rm -rf "$(BUILD_DIR)"

prepare:
	mkdir -p "$(BUILD_DIR)"
	
jsimd_i386: $(libsimd_objects)
	xcrun llvm-gcc -m32 -march=i386 -I".." -I"../.." -I"../../libjpeg" -c "jsimd_i386.c" -o "$(BUILD_DIR)/jsimd_i386.o"
	
build/jsimdcfg.inc: jsimdcfg.inc.h ../jpeglib.h ../jconfig.h ../jmorecfg.h
	xcrun cpp -I"../../libjpeg" $< | egrep "^[\;%]|^\ %" | sed 's%_cpp_protection_%%' | sed 's@% define@%define@g' >"$@"

%.o: %.asm 
	/usr/local/bin/nasm -fmacho -DMACHO -DPIC -I. -I"$(BUILD_DIR)/" -o "$(BUILD_DIR)/$@" "$<"	
	
jccolmmx.o: jcclrmmx.asm
jccolss2.o: jcclrss2.asm
jcgrammx.o: jcgrymmx.asm
jcgrass2.o: jcgryss2.asm
jdmermmx.o: jdmrgmmx.asm
jdmerss2.o: jdmrgss2.asm
