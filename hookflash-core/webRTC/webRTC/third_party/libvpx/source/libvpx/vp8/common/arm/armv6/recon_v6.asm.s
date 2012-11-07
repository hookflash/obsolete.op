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


    .globl _vp8_recon_b_armv6
	.globl vp8_recon_b_armv6
    .globl _vp8_recon2b_armv6
	.globl vp8_recon2b_armv6
    .globl _vp8_recon4b_armv6
	.globl vp8_recon4b_armv6

.text
.p2align 2

@void recon_b(unsigned char *pred_ptr, short *diff_ptr, unsigned char *dst_ptr, int r3)
@ R0 char* pred_ptr
@ R1 short * dif_ptr
@ R2 char * dst_ptr
@ R3 int r3

@ Description:
@ Loop through the block adding the Pred and Diff together.  Clamp and then
@ store back into the Dst.

@ Restrictions :
@ all buffers are expected to be 4 byte aligned coming in and
@ going out.
@-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
@
@
@
@-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
_vp8_recon_b_armv6:
	vp8_recon_b_armv6: @
    stmdb   sp!, {r4 - r9, lr}

    @0, 1, 2, 3
    ldr     r4, [r0], #16          @ 3 | 2 | 1 | 0
    ldr     r6, [r1, #0]           @     1 |     0
    ldr     r7, [r1, #4]           @     3 |     2

    pkhbt   r8, r6, r7, lsl #16     @     2 |     0
    pkhtb   r9, r7, r6, asr #16     @     3 |     1

    uxtab16 r8, r8, r4              @     2 |     0  +  3 | 2 | 2 | 0
    uxtab16 r9, r9, r4, ror #8      @     3 |     1  +  0 | 3 | 2 | 1

    usat16  r8, #8, r8
    usat16  r9, #8, r9
    add     r1, r1, #32
    orr     r8, r8, r9, lsl #8

    str     r8, [r2], r3

    @0, 1, 2, 3
    ldr     r4, [r0], #16          @ 3 | 2 | 1 | 0
@@  ldr     r6, [r1, #8]           @     1 |     0
@@  ldr     r7, [r1, #12]          @     3 |     2
    ldr     r6, [r1, #0]           @     1 |     0
    ldr     r7, [r1, #4]           @     3 |     2

    pkhbt   r8, r6, r7, lsl #16     @     2 |     0
    pkhtb   r9, r7, r6, asr #16     @     3 |     1

    uxtab16 r8, r8, r4              @     2 |     0  +  3 | 2 | 2 | 0
    uxtab16 r9, r9, r4, ror #8      @     3 |     1  +  0 | 3 | 2 | 1

    usat16  r8, #8, r8
    usat16  r9, #8, r9
    add     r1, r1, #32
    orr     r8, r8, r9, lsl #8

    str     r8, [r2], r3

    @0, 1, 2, 3
    ldr     r4, [r0], #16          @ 3 | 2 | 1 | 0
@@  ldr     r6, [r1, #16]          @     1 |     0
@@  ldr     r7, [r1, #20]          @     3 |     2
    ldr     r6, [r1, #0]           @     1 |     0
    ldr     r7, [r1, #4]           @     3 |     2

    pkhbt   r8, r6, r7, lsl #16     @     2 |     0
    pkhtb   r9, r7, r6, asr #16     @     3 |     1

    uxtab16 r8, r8, r4              @     2 |     0  +  3 | 2 | 2 | 0
    uxtab16 r9, r9, r4, ror #8      @     3 |     1  +  0 | 3 | 2 | 1

    usat16  r8, #8, r8
    usat16  r9, #8, r9
    add     r1, r1, #32
    orr     r8, r8, r9, lsl #8

    str     r8, [r2], r3

    @0, 1, 2, 3
    ldr     r4, [r0], #16          @ 3 | 2 | 1 | 0
@@  ldr     r6, [r1, #24]          @     1 |     0
@@  ldr     r7, [r1, #28]          @     3 |     2
    ldr     r6, [r1, #0]           @     1 |     0
    ldr     r7, [r1, #4]           @     3 |     2

    pkhbt   r8, r6, r7, lsl #16     @     2 |     0
    pkhtb   r9, r7, r6, asr #16     @     3 |     1

    uxtab16 r8, r8, r4              @     2 |     0  +  3 | 2 | 2 | 0
    uxtab16 r9, r9, r4, ror #8      @     3 |     1  +  0 | 3 | 2 | 1

    usat16  r8, #8, r8
    usat16  r9, #8, r9
    orr     r8, r8, r9, lsl #8

    str     r8, [r2], r3

    ldmia   sp!, {r4 - r9, pc}

    @    @ |recon_b|

@-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
@
@
@
@-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
@ R0 char  *pred_ptr
@ R1 short *dif_ptr
@ R2 char  *dst_ptr
@ R3 int r3
_vp8_recon4b_armv6:
	vp8_recon4b_armv6: @
    stmdb   sp!, {r4 - r9, lr}

    mov     lr, #4

recon4b_loop:
    @0, 1, 2, 3
    ldr     r4, [r0], #4           @ 3 | 2 | 1 | 0
    ldr     r6, [r1, #0]           @     1 |     0
    ldr     r7, [r1, #4]           @     3 |     2

    pkhbt   r8, r6, r7, lsl #16     @     2 |     0
    pkhtb   r9, r7, r6, asr #16     @     3 |     1

    uxtab16 r8, r8, r4              @     2 |     0  +  3 | 2 | 2 | 0
    uxtab16 r9, r9, r4, ror #8      @     3 |     1  +  0 | 3 | 2 | 1

    usat16  r8, #8, r8
    usat16  r9, #8, r9
    orr     r8, r8, r9, lsl #8

    str     r8, [r2]

    @4, 5, 6, 7
    ldr     r4, [r0], #4
@@  ldr     r6, [r1, #32]
@@  ldr     r7, [r1, #36]
    ldr     r6, [r1, #8]
    ldr     r7, [r1, #12]

    pkhbt   r8, r6, r7, lsl #16
    pkhtb   r9, r7, r6, asr #16

    uxtab16 r8, r8, r4
    uxtab16 r9, r9, r4, ror #8
    usat16  r8, #8, r8
    usat16  r9, #8, r9
    orr     r8, r8, r9, lsl #8

    str     r8, [r2, #4]

    @8, 9, 10, 11
    ldr     r4, [r0], #4
@@  ldr     r6, [r1, #64]
@@  ldr     r7, [r1, #68]
    ldr     r6, [r1, #16]
    ldr     r7, [r1, #20]

    pkhbt   r8, r6, r7, lsl #16
    pkhtb   r9, r7, r6, asr #16

    uxtab16 r8, r8, r4
    uxtab16 r9, r9, r4, ror #8
    usat16  r8, #8, r8
    usat16  r9, #8, r9
    orr     r8, r8, r9, lsl #8

    str     r8, [r2, #8]

    @12, 13, 14, 15
    ldr     r4, [r0], #4
@@  ldr     r6, [r1, #96]
@@  ldr     r7, [r1, #100]
    ldr     r6, [r1, #24]
    ldr     r7, [r1, #28]

    pkhbt   r8, r6, r7, lsl #16
    pkhtb   r9, r7, r6, asr #16

    uxtab16 r8, r8, r4
    uxtab16 r9, r9, r4, ror #8
    usat16  r8, #8, r8
    usat16  r9, #8, r9
    orr     r8, r8, r9, lsl #8

    str     r8, [r2, #12]

    add     r2, r2, r3
@@  add     r1, r1, #8
    add     r1, r1, #32

    subs    lr, lr, #1
    bne     recon4b_loop

    ldmia   sp!, {r4 - r9, pc}

    @    @ |Recon4B|

@-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
@
@
@
@-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
@ R0 char  *pred_ptr
@ R1 short *dif_ptr
@ R2 char  *dst_ptr
@ R3 int r3
_vp8_recon2b_armv6:
	vp8_recon2b_armv6: @
    stmdb   sp!, {r4 - r9, lr}

    mov     lr, #4

recon2b_loop:
    @0, 1, 2, 3
    ldr     r4, [r0], #4
    ldr     r6, [r1, #0]
    ldr     r7, [r1, #4]

    pkhbt   r8, r6, r7, lsl #16
    pkhtb   r9, r7, r6, asr #16

    uxtab16 r8, r8, r4
    uxtab16 r9, r9, r4, ror #8
    usat16  r8, #8, r8
    usat16  r9, #8, r9
    orr     r8, r8, r9, lsl #8

    str     r8, [r2]

    @4, 5, 6, 7
    ldr     r4, [r0], #4
@@  ldr     r6, [r1, #32]
@@  ldr     r7, [r1, #36]
    ldr     r6, [r1, #8]
    ldr     r7, [r1, #12]

    pkhbt   r8, r6, r7, lsl #16
    pkhtb   r9, r7, r6, asr #16

    uxtab16 r8, r8, r4
    uxtab16 r9, r9, r4, ror #8
    usat16  r8, #8, r8
    usat16  r9, #8, r9
    orr     r8, r8, r9, lsl #8

    str     r8, [r2, #4]

    add     r2, r2, r3
@@  add     r1, r1, #8
    add     r1, r1, #16

    subs    lr, lr, #1
    bne     recon2b_loop

    ldmia   sp!, {r4 - r9, pc}

    @    @ |Recon2B|

