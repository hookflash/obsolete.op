/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_COMMON_VIDEO_VPLIB_ROW_H_
#define WEBRTC_COMMON_VIDEO_VPLIB_ROW_H_

#include "typedefs.h"

#define kMaxStride (2048 * 4)
#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))

void FastConvertYUVToARGBRow_NEON(const WebRtc_UWord8* y_buf,
                                  const WebRtc_UWord8* u_buf,
                                  const WebRtc_UWord8* v_buf,
                                  WebRtc_UWord8* rgb_buf,
                                  WebRtc_UWord32 width);
void FastConvertYUVToBGRARow_NEON(const WebRtc_UWord8* y_buf,
                                  const WebRtc_UWord8* u_buf,
                                  const WebRtc_UWord8* v_buf,
                                  WebRtc_UWord8* rgb_buf,
                                  WebRtc_UWord32 width);
void FastConvertYUVToABGRRow_NEON(const WebRtc_UWord8* y_buf,
                                  const WebRtc_UWord8* u_buf,
                                  const WebRtc_UWord8* v_buf,
                                  WebRtc_UWord8* rgb_buf,
                                  WebRtc_UWord32 width);
void FastConvertABGRToYUVRow2_NEON(const WebRtc_UWord8* rgb_buf,
                                   WebRtc_UWord8* y_buf,
                                   WebRtc_UWord8* u_buf,
                                   WebRtc_UWord8* v_buf,
                                   WebRtc_UWord32 width,
                                   WebRtc_UWord32 stride);

typedef signed char __attribute__((vector_size(16))) vec8;
typedef unsigned char __attribute__((vector_size(16))) uvec8;
typedef signed short __attribute__((vector_size(16))) vec16;

#endif  // WEBRTC_COMMON_VIDEO_VPLIB_ROW_H_
