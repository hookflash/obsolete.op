/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "row.h"

#define YUVTORGB                                                               \
    "vld1.u8    {d0}, [%0]!                    \n"                             \
    "vld1.u32   {d2[0]}, [%1]!                 \n"                             \
    "vld1.u32   {d2[1]}, [%2]!                 \n"                             \
                                                                               \
    "veor.u8    d2, d26                        \n"/*subtract 128 from u and v*/\
                                                                               \
    "vmull.s8   q8, d2, d24                    \n"/*  u/v B/R component      */\
                                                                               \
    "vmull.s8   q9, d2, d25                    \n"/*  u/v G component        */\
                                                                               \
    "vmov.u8    d1, #0                         \n"/*  split odd/even y apart */\
    "vtrn.u8    d0, d1                         \n"                             \
                                                                               \
    "vsub.s16   q0, q0, q15                    \n"/*  offset y               */\
    "vmul.s16   q0, q0, q14                    \n"                             \
                                                                               \
    "vadd.s16   d18, d19                       \n"                             \
                                                                               \
    "vqadd.s16  d20, d0, d16                   \n"                             \
    "vqadd.s16  d21, d1, d16                   \n"                             \
                                                                               \
    "vqadd.s16  d22, d0, d17                   \n"                             \
    "vqadd.s16  d23, d1, d17                   \n"                             \
                                                                               \
    "vqadd.s16  d16, d0, d18                   \n"                             \
    "vqadd.s16  d17, d1, d18                   \n"                             \
                                                                               \
    "vqrshrun.s16 d0, q10, #6                  \n"                             \
    "vqrshrun.s16 d1, q11, #6                  \n"                             \
    "vqrshrun.s16 d2, q8, #6                   \n"                             \
                                                                               \
    "vmovl.u8   q10, d0                        \n"/*  set up for reinterleave*/\
    "vmovl.u8   q11, d1                        \n"                             \
    "vmovl.u8   q8, d2                         \n"                             \
                                                                               \
    "vtrn.u8    d20, d21                       \n"                             \
    "vtrn.u8    d22, d23                       \n"                             \
    "vtrn.u8    d16, d17                       \n"                             \

static const vec8 kUVToRB[8]  = { 127, 127, 127, 127, 102, 102, 102, 102 };
static const vec8 kUVToG[8]   = { -25, -25, -25, -25, -52, -52, -52, -52 };

void FastConvertYUVToARGBRow_NEON(const WebRtc_UWord8* y_buf,
                                  const WebRtc_UWord8* u_buf,
                                  const WebRtc_UWord8* v_buf,
                                  WebRtc_UWord8* rgb_buf,
                                  WebRtc_UWord32 width) {
  asm volatile (
    "vld1.u8    {d24}, [%5]                    \n"
    "vld1.u8    {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
  "1:                                          \n"
YUVTORGB
    "vmov.u8    d21, d16                       \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.u8    {d20, d21, d22, d23}, [%3]!    \n"
    "subs       %4, %4, #8                     \n"
    "bhi        1b                             \n"
    : "+r"(y_buf),          // %0
      "+r"(u_buf),          // %1
      "+r"(v_buf),          // %2
      "+r"(rgb_buf),        // %3
      "+r"(width)           // %4
    : "r"(kUVToRB),
      "r"(kUVToG)
    : "cc", "memory", "q0", "q1", "q2", "q3", "q8", "q9",
                      "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void FastConvertYUVToBGRARow_NEON(const WebRtc_UWord8* y_buf,
                                  const WebRtc_UWord8* u_buf,
                                  const WebRtc_UWord8* v_buf,
                                  WebRtc_UWord8* rgb_buf,
                                  WebRtc_UWord32 width) {
  asm volatile (
    "vld1.u8    {d24}, [%5]                    \n"
    "vld1.u8    {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
  "1:                                          \n"
YUVTORGB
    "vswp.u8    d20, d22                       \n"
    "vmov.u8    d21, d16                       \n"
    "vmov.u8    d19, #255                      \n"
    "vst4.u8    {d19, d20, d21, d22}, [%3]!    \n"
    "subs       %4, %4, #8                     \n"
    "bhi        1b                             \n"
    : "+r"(y_buf),          // %0
      "+r"(u_buf),          // %1
      "+r"(v_buf),          // %2
      "+r"(rgb_buf),        // %3
      "+r"(width)           // %4
    : "r"(kUVToRB),
      "r"(kUVToG)
    : "cc", "memory", "q0", "q1", "q2", "q3", "q8", "q9",
                      "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void FastConvertYUVToABGRRow_NEON(const WebRtc_UWord8* y_buf,
                                  const WebRtc_UWord8* u_buf,
                                  const WebRtc_UWord8* v_buf,
                                  WebRtc_UWord8* rgb_buf,
                                  WebRtc_UWord32 width) {
  asm volatile (
    "vld1.u8    {d24}, [%5]                    \n"
    "vld1.u8    {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
  "1:                                          \n"
YUVTORGB
    "vswp.u8    d20, d22                       \n"
    "vmov.u8    d21, d16                       \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.u8    {d20, d21, d22, d23}, [%3]!    \n"
    "subs       %4, %4, #8                     \n"
    "bhi        1b                             \n"
    : "+r"(y_buf),          // %0
      "+r"(u_buf),          // %1
      "+r"(v_buf),          // %2
      "+r"(rgb_buf),        // %3
      "+r"(width)           // %4
    : "r"(kUVToRB),
      "r"(kUVToG)
    : "cc", "memory", "q0", "q1", "q2", "q3", "q8", "q9",
                      "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void FastConvertABGRToYUVRow2_NEON(const WebRtc_UWord8* rgb_buf,
                                   WebRtc_UWord8* y_buf,
                                   WebRtc_UWord8* u_buf,
                                   WebRtc_UWord8* v_buf,
                                   WebRtc_UWord32 width,
                                   WebRtc_UWord32 stride) {

  asm volatile ("    mov            r11, %0                         ;                                 \n"
                "    add            r11, r11, %5, lsl #2            ;                                 \n"
                "    mov            r12, %1                         ;                                 \n"
                "    add            r12, r12, %5                    ;                                 \n"
                "    ;                                                                                \n"
                "    vmov.i16       q11, #(128<<8)                  ;                                 \n"
                "    vmov.i8        d24, #77                        ;                                 \n"
                "    vmov.i8        d25, #150                       ;                                 \n"
                "    vmov.i8        d26, #29                        ;                                 \n"
                "    vmov.i8        d27, #43                        ;                                 \n"
                "    vmov.i8        d28, #85                        ;                                 \n"
                "    vmov.i8        d29, #128                       ;                                 \n"
                "    vmov.i8        d30, #107                       ;                                 \n"
                "    vmov.i8        d31, #21                        ;                                 \n"
                "    ;                                                                                \n"
                "1:                                                                                   \n"
                "    vld4.8         {d8,d9,d10,d11}, [%0]!          ;                                 \n"
                "    vld4.8         {d12,d13,d14,d15}, [r11]!       ;                                 \n"
                "    vpaddl.u8      d16, d8                         ;                                 \n"
                "    vpadal.u8      d16, d12                        ;                                 \n"
                "    vpaddl.u8      d17, d9                         ;                                 \n"
                "    vpadal.u8      d17, d13                        ;                                 \n"
                "    vpaddl.u8      d18, d10                        ;                                 \n"
                "    vpadal.u8      d18, d14                        ;                                 \n"
                "    vrshr.u16      d16, d16, #2                    ;                                 \n"
                "    vrshr.u16      d17, d17, #2                    ;                                 \n"
                "    vrshr.u16      d18, d18, #2                    ;                                 \n"
                "    ;                                                                                \n"
                "    vmull.u8       q0, d10, d24                    ;                                 \n"
                "    vmull.u8       q1, d14, d24                    ;                                 \n"
                "    vmlal.u8       q0, d9, d25                     ;                                 \n"
                "    vmlal.u8       q1, d13, d25                    ;                                 \n"
                "    vmlal.u8       q0, d8, d26                     ;                                 \n"
                "    vmlal.u8       q1, d12, d26                    ;                                 \n"
                "    vsub.i16       q0, q0, q11                     ;                                 \n"
                "    vsub.i16       q1, q1, q11                     ;                                 \n"
                "    vqrshrn.s16    d4, q0, #8                      ;                                 \n"
                "    vqrshrn.s16    d5, q1, #8                      ;                                 \n"
                "    vadd.i8        d4, d4, d29                     ;                                 \n"
                "    vadd.i8        d5, d5, d29                     ;                                 \n"
                "    ;                                                                                \n"
                "    vld4.8         {d8,d9,d10,d11}, [%0]!          ;                                 \n"
                "    vld4.8         {d12,d13,d14,d15}, [r11]!       ;                                 \n"
                "    vpaddl.u8      d19, d8                         ;                                 \n"
                "    vpadal.u8      d19, d12                        ;                                 \n"
                "    vpaddl.u8      d20, d9                         ;                                 \n"
                "    vpadal.u8      d20, d13                        ;                                 \n"
                "    vpaddl.u8      d21, d10                        ;                                 \n"
                "    vpadal.u8      d21, d14                        ;                                 \n"
                "    vrshr.u16      d19, d19, #2                    ;                                 \n"
                "    vrshr.u16      d20, d20, #2                    ;                                 \n"
                "    vrshr.u16      d21, d21, #2                    ;                                 \n"
                "    ;                                                                                \n"
                "    vmull.u8       q0, d10, d24                    ;                                 \n"
                "    vmull.u8       q1, d14, d24                    ;                                 \n"
                "    vmlal.u8       q0, d9, d25                     ;                                 \n"
                "    vmlal.u8       q1, d13, d25                    ;                                 \n"
                "    vmlal.u8       q0, d8, d26                     ;                                 \n"
                "    vmlal.u8       q1, d12, d26                    ;                                 \n"
                "    vsub.i16       q0, q0, q11                     ;                                 \n"
                "    vsub.i16       q1, q1, q11                     ;                                 \n"
                "    vqrshrn.s16    d6, q0, #8                      ;                                 \n"
                "    vqrshrn.s16    d7, q1, #8                      ;                                 \n"
                "    vadd.i8        d6, d6, d29                     ;                                 \n"
                "    vadd.i8        d7, d7, d29                     ;                                 \n"
                "    ;                                                                                \n"
                "    vst1.u8        {d4}, [%1]!                     ;                                 \n"
                "    vst1.u8        {d6}, [%1]!                     ;                                 \n"
                "    vst1.u8        {d5}, [r12]!                    ;                                 \n"
                "    vst1.u8        {d7}, [r12]!                    ;                                 \n"
                "    ;                                                                                \n"
                "    vuzp.8         d16, d19                        ;                                 \n"
                "    vuzp.8         d17, d20                        ;                                 \n"
                "    vuzp.8         d18, d21                        ;                                 \n"
                "    vmull.u8       q0, d16, d29                    ;                                 \n"
                "    vmlsl.u8       q0, d17, d28                    ;                                 \n"
                "    vmlsl.u8       q0, d18, d27                    ;                                 \n"
                "    vqrshrn.s16    d4, q0, #8                      ;                                 \n"
                "    vadd.i8        d4, d4, d29                     ;                                 \n"
                "    vmull.u8       q1, d18, d29                    ;                                 \n"
                "    vmlsl.u8       q1, d17, d30                    ;                                 \n"
                "    vmlsl.u8       q1, d16, d31                    ;                                 \n"
                "    vqrshrn.s16    d5, q1, #8                      ;                                 \n"
                "    vadd.i8        d5, d5, d29                     ;                                 \n"
                "    vst1.u8        {d4}, [%2]!                     ;                                 \n"
                "    vst1.u8        {d5}, [%3]!                     ;                                 \n"
                "    ;                                                                                \n"
                "    subs           %4, %4, #16                     ;                                 \n"
                "    bhi            1b                              ;                                 \n"
                : 
                "+r"(rgb_buf),        // %0
                "+r"(y_buf),          // %1
                "+r"(u_buf),          // %2
                "+r"(v_buf),          // %3
                "+r"(width),          // %4
                "+r"(stride)          // %5
                :
                : 
                "cc", "memory", "r11", "r12", "q0", "q1", "d4", "d5", "d6", "d7", "d8", "d9",
                "d10", "d11", "d12", "d13", "d14", "d15", "d16", "d17", "d18", "d19", "d20",
                "d21", "q11", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
                ); 
}
