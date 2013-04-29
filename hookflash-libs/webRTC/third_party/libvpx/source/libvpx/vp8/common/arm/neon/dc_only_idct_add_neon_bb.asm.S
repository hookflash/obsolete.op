@ This file was created from a .asm file
@  using the ads2gas_apple.pl script.

	.set WIDE_REFERENCE, 0
	.set ARCHITECTURE, 5
	.set DO1STROUNDING, 0
@
@  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
@
@  Use of this source code is governed by a BSD-style license and patent
@  grant that can be found in the LICENSE file in the root of the source
@  tree. All contributing project authors may be found in the AUTHORS
@  file in the root of the source tree.
@


    .globl _vp8_dc_only_idct_add_neon
	.globl vp8_dc_only_idct_add_neon
   @ ARM
   @ 
   @ PRESERVE8

.text
.p2align 2

@void vp8_dc_only_idct_add_c(short input_dc, unsigned char *pred_ptr,
@                            int pred_stride, unsigned char *dst_ptr,
@                            int dst_stride)

@ r0  input_dc
@ r1  pred_ptr
@ r2  pred_stride
@ r3  dst_ptr
@ sp  dst_stride

_vp8_dc_only_idct_add_neon:
	vp8_dc_only_idct_add_neon: @
    add             r0, r0, #4
    asr             r0, r0, #3
    ldr             r12, [sp]
    vdup.16         q0, r0

    vld1.32         {d2[0]}, [r1], r2
    vld1.32         {d2[1]}, [r1], r2
    vld1.32         {d4[0]}, [r1], r2
    vld1.32         {d4[1]}, [r1]

    vaddw.u8        q1, q0, d2
    vaddw.u8        q2, q0, d4

    vqmovun.s16     d2, q1
    vqmovun.s16     d4, q2

    vst1.32         {d2[0]}, [r3], r12
    vst1.32         {d2[1]}, [r3], r12
    vst1.32         {d4[0]}, [r3], r12
    vst1.32         {d4[1]}, [r3]
 
    bx              lr

    @

