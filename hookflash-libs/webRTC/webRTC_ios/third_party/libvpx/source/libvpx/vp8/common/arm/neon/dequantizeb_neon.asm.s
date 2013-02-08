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


    .globl _vp8_dequantize_b_loop_neon
	.globl vp8_dequantize_b_loop_neon
   @ ARM
   @ 
   @ PRESERVE8

.text
.p2align 2
@ r0    short *Q,
@ r1    short *DQC
@ r2    short *DQ
_vp8_dequantize_b_loop_neon:
	vp8_dequantize_b_loop_neon: @
    vld1.16         {q0, q1}, [r0]
    vld1.16         {q2, q3}, [r1]

    vmul.i16        q4, q0, q2
    vmul.i16        q5, q1, q3

    vst1.16         {q4, q5}, [r2]

    bx             lr

    @

