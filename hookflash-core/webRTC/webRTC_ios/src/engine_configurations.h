/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_ENGINE_CONFIGURATIONS_H_
#define WEBRTC_ENGINE_CONFIGURATIONS_H_

// ============================================================================
//                              Voice and Video
// ============================================================================

// Don't link in socket support in Chrome
#ifdef WEBRTC_CHROMIUM_BUILD
#define WEBRTC_EXTERNAL_TRANSPORT
#endif

// Optional to enable stand-alone
#define WEBRTC_EXTERNAL_TRANSPORT

// ----------------------------------------------------------------------------
//  [Voice] Codec settings
// ----------------------------------------------------------------------------

#ifdef WEBRTC_ANDROID
#define WEBRTC_CODEC_ISACFX     // fix-point iSAC implementation
#elif MAC_IPHONE
#define WEBRTC_CODEC_ISACFX
#else
#define WEBRTC_CODEC_ISAC       // floating-point iSAC implementation (default)
#endif
#define WEBRTC_CODEC_AVT
#ifdef MAC_IPHONE
#define WEBRTC_CODEC_VORBIS
#define WEBRTC_CODEC_OPUS
#endif

#ifndef WEBRTC_CHROMIUM_BUILD
#define WEBRTC_CODEC_ILBC
#define WEBRTC_CODEC_G722
#define WEBRTC_CODEC_PCM16
#define WEBRTC_CODEC_RED
#endif

// ----------------------------------------------------------------------------
//  [Video] Codec settings
// ----------------------------------------------------------------------------

#define VIDEOCODEC_I420
#define VIDEOCODEC_VP8
#define VIDEOCODEC_H264

// ============================================================================
//                                 VoiceEngine
// ============================================================================

// ----------------------------------------------------------------------------
//  Settings for VoiceEngine
// ----------------------------------------------------------------------------

#define WEBRTC_VOICE_ENGINE_AGC                 // Near-end AGC
#define WEBRTC_VOICE_ENGINE_ECHO                // Near-end AEC
#define WEBRTC_VOICE_ENGINE_NR                  // Near-end NS
#define WEBRTC_VOE_EXTERNAL_REC_AND_PLAYOUT

#ifndef WEBRTC_CHROMIUM_BUILD
#define WEBRTC_VOICE_ENGINE_TYPING_DETECTION    // Typing detection
#endif

// ----------------------------------------------------------------------------
//  VoiceEngine sub-APIs
// ----------------------------------------------------------------------------

#define WEBRTC_VOICE_ENGINE_AUDIO_PROCESSING_API
#define WEBRTC_VOICE_ENGINE_CODEC_API
#define WEBRTC_VOICE_ENGINE_DTMF_API
#define WEBRTC_VOICE_ENGINE_EXTERNAL_MEDIA_API
#define WEBRTC_VOICE_ENGINE_FILE_API
#define WEBRTC_VOICE_ENGINE_HARDWARE_API
#define WEBRTC_VOICE_ENGINE_NETEQ_STATS_API
#define WEBRTC_VOICE_ENGINE_NETWORK_API
#define WEBRTC_VOICE_ENGINE_RTP_RTCP_API
#define WEBRTC_VOICE_ENGINE_VIDEO_SYNC_API
#define WEBRTC_VOICE_ENGINE_VOLUME_CONTROL_API

#ifndef WEBRTC_CHROMIUM_BUILD
#define WEBRTC_VOICE_ENGINE_CALL_REPORT_API
#define WEBRTC_VOICE_ENGINE_ENCRYPTION_API
#endif

// ============================================================================
//                                 VideoEngine
// ============================================================================

// ----------------------------------------------------------------------------
//  Settings for special VideoEngine configurations
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//  VideoEngine sub-API:s
// ----------------------------------------------------------------------------

#define WEBRTC_VIDEO_ENGINE_CAPTURE_API
#define WEBRTC_VIDEO_ENGINE_CODEC_API
#define WEBRTC_VIDEO_ENGINE_ENCRYPTION_API
#define WEBRTC_VIDEO_ENGINE_IMAGE_PROCESS_API
#define WEBRTC_VIDEO_ENGINE_NETWORK_API
#define WEBRTC_VIDEO_ENGINE_RENDER_API
#define WEBRTC_VIDEO_ENGINE_RTP_RTCP_API
// #define WEBRTC_VIDEO_ENGINE_EXTERNAL_CODEC_API

#ifndef WEBRTC_CHROMIUM_BUILD
#define WEBRTC_VIDEO_ENGINE_FILE_API
#endif

// ============================================================================
//                       Platform specific configurations
// ============================================================================

// ----------------------------------------------------------------------------
//  VideoEngine Windows
// ----------------------------------------------------------------------------

#if defined(_WIN32)
// #define DIRECTDRAW_RENDERING
#define DIRECT3D9_RENDERING  // Requires DirectX 9.
#endif

// ----------------------------------------------------------------------------
//  VideoEngine MAC
// ----------------------------------------------------------------------------

#if defined(WEBRTC_MAC) && !defined(MAC_IPHONE)
// #define CARBON_RENDERING
#define COCOA_RENDERING
#endif

// ----------------------------------------------------------------------------
//  VideoEngine Mobile iPhone
// ----------------------------------------------------------------------------

#if defined(MAC_IPHONE)
#define EAGL_RENDERING
#endif

// ----------------------------------------------------------------------------
//  Deprecated
// ----------------------------------------------------------------------------

// #define WEBRTC_CODEC_G729
// #define WEBRTC_DTMF_DETECTION
// #define WEBRTC_SRTP
// #define WEBRTC_SRTP_ALLOW_ROC_ITERATION

#endif  // WEBRTC_ENGINE_CONFIGURATIONS_H_
