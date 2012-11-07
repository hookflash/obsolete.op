/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_COMMON_VIDEO_VPLIB_ROTATE_H_
#define WEBRTC_COMMON_VIDEO_VPLIB_ROTATE_H_

#include "typedefs.h"

// Supported rotation
enum RotationMode {
  kRotate0 = 0, // No rotation
  kRotate90 = 90,  // Rotate 90 degrees clockwise
  kRotate180 = 180,  // Rotate 180 degrees
  kRotate270 = 270,  // Rotate 270 degrees clockwise
};

void ReverseRow_NEON(const WebRtc_UWord8* src, WebRtc_UWord8* dst, WebRtc_Word32 width);
void TransposeWx8_NEON(const WebRtc_UWord8* src, WebRtc_Word32 src_stride,
                       WebRtc_UWord8* dst, WebRtc_Word32 dst_stride,
                       WebRtc_Word32 width);
void ReverseRowUV_NEON(const WebRtc_UWord8* src,
                       WebRtc_UWord8* dst_a, WebRtc_UWord8* dst_b,
                       WebRtc_Word32 width);
void TransposeUVWx8_NEON(const WebRtc_UWord8* src, WebRtc_Word32 src_stride,
                         WebRtc_UWord8* dst_a, WebRtc_Word32 dst_stride_a,
                         WebRtc_UWord8* dst_b, WebRtc_Word32 dst_stride_b,
                         WebRtc_Word32 width);

#endif  // WEBRTC_COMMON_VIDEO_VPLIB_ROTATE_H_
