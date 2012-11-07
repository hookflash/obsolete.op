/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "scale.h"

void ScaleDownRGBARow2_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                            WebRtc_UWord8* dst, WebRtc_UWord32 dst_width)
{
  asm volatile (
    "add        %1, %0, %1, lsl #2             \n"  // change the stride to row 2 pointer
    "1:                                        \n"
    "vld4.8     {d0,d2,d4,d6}, [%0]!           \n"  // load row 1 and post increment
    "vld4.8     {d1,d3,d5,d7}, [%0]!           \n"
    "vld4.8     {d8,d10,d12,d14}, [%1]!        \n"  // load row 2 and post increment
    "vld4.8     {d9,d11,d13,d15}, [%1]!        \n"
    "vpaddl.u8  q0, q0                         \n"  // row 1 add adjacent
    "vpaddl.u8  q1, q1                         \n"
    "vpaddl.u8  q2, q2                         \n"
    "vpaddl.u8  q3, q3                         \n"
    "vpadal.u8  q0, q4                         \n"  // row 2 add adjacent, add row 1 to row 2
    "vpadal.u8  q1, q5                         \n"
    "vpadal.u8  q2, q6                         \n"
    "vpadal.u8  q3, q7                         \n"
    "vrshrn.u16 d0, q0, #2                     \n"  // downshift, round and pack
    "vrshrn.u16 d1, q1, #2                     \n"
    "vrshrn.u16 d2, q2, #2                     \n"
    "vrshrn.u16 d3, q3, #2                     \n"
    "vst4.u8    {d0,d1,d2,d3}, [%2]!           \n"
    "subs       %3, %3, #8                     \n"  // 8 processed per loop
    "bhi        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(src_stride),       // %1
      "+r"(dst),              // %2
      "+r"(dst_width)         // %3
    :
    : "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7"     // Clobber List
    );
}

void ScaleDownRow2_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                        WebRtc_UWord8* dst, WebRtc_UWord32 dst_width) 
{
  asm volatile (
    "add        %1, %0                         \n"  // change the stride to row 2 pointer
    "1:                                        \n"
    "vld1.u8    {q0,q1}, [%0]!                 \n"  // load row 1 and post increment
    "vld1.u8    {q2,q3}, [%1]!                 \n"  // load row 2 and post increment
    "vpaddl.u8  q0, q0                         \n"  // row 1 add adjacent
    "vpaddl.u8  q1, q1                         \n"
    "vpadal.u8  q0, q2                         \n"  // row 2 add adjacent, add row 1 to row 2
    "vpadal.u8  q1, q3                         \n"
    "vrshrn.u16 d0, q0, #2                     \n"  // downshift, round and pack
    "vrshrn.u16 d1, q1, #2                     \n"
    "vst1.u8    {q0}, [%2]!                    \n"
    "subs       %3, %3, #16                    \n"  // 16 processed per loop
    "bhi        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(src_stride),       // %1
      "+r"(dst),              // %2
      "+r"(dst_width)         // %3
    :
    : "q0", "q1", "q2", "q3"     // Clobber List
    );
}

void ScaleDownAndDeinterleaveRow2_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                                       WebRtc_UWord8* dstU, WebRtc_UWord8* dstV, WebRtc_UWord32 dst_width)
{
  asm volatile (
    "add        %1, %0, %1, lsl #1             \n"  // change the stride to row 2 pointer
    "1:                                        \n"
    "vld2.u8    {q0,q1}, [%0]!                 \n"  // load row 1 and post increment
    "vld2.u8    {q2,q3}, [%1]!                 \n"  // load row 2 and post increment
    "vpaddl.u8  q0, q0                         \n"  // row 1 add adjacent
    "vpaddl.u8  q1, q1                         \n"
    "vpadal.u8  q0, q2                         \n"  // row 2 add adjacent, add row 1 to row 2
    "vpadal.u8  q1, q3                         \n"
    "vrshrn.u16 d0, q0, #2                     \n"  // downshift, round and pack
    "vrshrn.u16 d1, q1, #2                     \n"
    "vst1.u8    {d0}, [%2]!                    \n"
    "vst1.u8    {d1}, [%3]!                    \n"
    "subs       %4, %4, #8                     \n"  // 8 processed per loop
    "bhi        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(src_stride),       // %1
      "+r"(dstU),             // %2
      "+r"(dstV),             // %3
      "+r"(dst_width)         // %4
    :
    : "q0", "q1", "q2", "q3"     // Clobber List
    );
}

void ScaleDownRow4_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                        WebRtc_UWord8* dst_ptr, WebRtc_UWord32 dst_width) 
{
  asm volatile (
    "add        r4, %0, %3                     \n"
    "add        r5, r4, %3                     \n"
    "add        %3, r5, %3                     \n"
    "1:                                        \n"
    "vld1.u8    {q0}, [%0]!                    \n"   // load up 16x4 block of input data
    "vld1.u8    {q1}, [r4]!                    \n"
    "vld1.u8    {q2}, [r5]!                    \n"
    "vld1.u8    {q3}, [%3]!                    \n"
    "vpaddl.u8  q0, q0                         \n"
    "vpadal.u8  q0, q1                         \n"
    "vpadal.u8  q0, q2                         \n"
    "vpadal.u8  q0, q3                         \n"
    "vpaddl.u16 q0, q0                         \n"
    "vrshrn.u32 d0, q0, #4                     \n"   // divide by 16 w/rounding
    "vmovn.u16  d0, q0                         \n"
    "vst1.u32   {d0[0]}, [%1]!                 \n"
    "subs       %2, #4                         \n"
    "bhi        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(dst_ptr),          // %1
      "+r"(dst_width)         // %2
    : "r"(src_stride)         // %3
    : "r4", "r5", "q0", "q1", "q2", "q3", "memory", "cc"
    );
}

void ScaleDownAndDeinterleaveRow4_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                                       WebRtc_UWord8* dstU, WebRtc_UWord8* dstV, WebRtc_UWord32 dst_width)
{
  asm volatile (
    "add        %1, %0                         \n"  // change the stride to row 2 pointer
    "1:                                        \n"
    "vld1.u8    {q0,q1}, [%0]!                 \n"  // load row 1 and post increment
    "vld1.u8    {q2,q3}, [%1]!                 \n"  // load row 2 and post increment
    "vpaddl.u8  q0, q0                         \n"  // row 1 add adjacent
    "vpaddl.u8  q1, q1                         \n"
    "vpaddl.u8  q2, q2                         \n"
    "vpaddl.u8  q3, q3                         \n"
    "vpadal.u8  q0, q4                         \n"  // row 2 add adjacent, add row 1 to row 2
    "vpadal.u8  q1, q5                         \n"
    "vpadal.u8  q2, q6                         \n"
    "vpadal.u8  q3, q7                         \n"
    "vrshrn.u16 d0, q0, #2                     \n"  // downshift, round and pack
    "vrshrn.u16 d1, q1, #2                     \n"
    "vrshrn.u16 d2, q2, #2                     \n"
    "vrshrn.u16 d3, q3, #2                     \n"
    "vst4.u8    {d0,d1,d2,d3}, [%2]!           \n"
    "subs       %3, %3, #8                     \n"  // 8 processed per loop
    "bhi        1b                             \n"
    : "+r"(src_ptr),          // %0
      "+r"(src_stride),       // %1
      "+r"(dstU),             // %2
      "+r"(dstV),             // %3
      "+r"(dst_width)         // %4
    :
    : "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7"     // Clobber List
    );
}

