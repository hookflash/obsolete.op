@ This file was created from a .asm file
@  using the ads2gas_apple.pl script.

	.set WIDE_REFERENCE, 0
	.set ARCHITECTURE, 5
	.set DO1STROUNDING, 0
@
@  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
@
@  Use of this source code is governed by a BSD-style license
@  that can be found in the LICENSE file in the root of the source
@  tree. An additional intellectual property rights grant can be found
@  in the file PATENTS.  All contributing project authors may
@  be found in the AUTHORS file in the root of the source tree.
@


    .globl _vp8_short_idct4x4llm_1_neon
	.globl vp8_short_idct4x4llm_1_neon
    .globl _vp8_dc_only_idct_neon
	.globl vp8_dc_only_idct_neon
   @ ARM
   @ 
   @ PRESERVE8

.text
.p2align 2
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@void vp8_short_idct4x4llm_1_c(short *input, short *output, int pitch)@
@ r0    short *input@
@ r1    short *output@
@ r2    int pitch@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
_vp8_short_idct4x4llm_1_neon:
	vp8_short_idct4x4llm_1_neon: @
    vld1.16         {d0[]}, [r0]            @load input[0]

    add             r3, r1, r2
    add             r12, r3, r2

    vrshr.s16       d0, d0, #3

    add             r0, r12, r2

    vst1.16         {d0}, [r1]
    vst1.16         {d0}, [r3]
    vst1.16         {d0}, [r12]
    vst1.16         {d0}, [r0]

    bx             lr
    @

@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@void vp8_dc_only_idct_c(short input_dc, short *output, int pitch)@
@ r0    short input_dc@
@ r1    short *output@
@ r2    int pitch@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
_vp8_dc_only_idct_neon:
	vp8_dc_only_idct_neon: @
    vdup.16         d0, r0

    add             r3, r1, r2
    add             r12, r3, r2

    vrshr.s16       d0, d0, #3

    add             r0, r12, r2

    vst1.16         {d0}, [r1]
    vst1.16         {d0}, [r3]
    vst1.16         {d0}, [r12]
    vst1.16         {d0}, [r0]

    bx             lr

    @
