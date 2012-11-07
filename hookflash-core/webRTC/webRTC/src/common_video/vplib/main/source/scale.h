/*
 *  Copyright (c) 2011 The LibYuv project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_COMMON_VIDEO_VPLIB_SCALE_H
#define WEBRTC_COMMON_VIDEO_VPLIB_SCALE_H

#include "typedefs.h"

void ScaleDownRGBARow2_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                            WebRtc_UWord8* dst, WebRtc_UWord32 dst_width);
void ScaleDownRow2_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                        WebRtc_UWord8* dst, WebRtc_UWord32 dst_width);
void ScaleDownAndDeinterleaveRow2_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                                       WebRtc_UWord8* dstU, WebRtc_UWord8* dstV, WebRtc_UWord32 dst_width);
void ScaleDownRow4_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                        WebRtc_UWord8* dst_ptr, WebRtc_UWord32 dst_width);
void ScaleDownAndDeinterleaveRow4_NEON(const WebRtc_UWord8* src_ptr, WebRtc_UWord32 src_stride,
                                       WebRtc_UWord8* dstU, WebRtc_UWord8* dstV, WebRtc_UWord32 dst_width);

#endif  // WEBRTC_COMMON_VIDEO_VPLIB_SCALE_H
