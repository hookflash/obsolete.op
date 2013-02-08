/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//
// vie_autotest.h
//

#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_

#include "common_types.h"

#include "voe_base.h"
#include "voe_codec.h"
#include "voe_hardware.h"
#include "voe_audio_processing.h"

#include "vie_base.h"
#include "vie_capture.h"
#include "vie_codec.h"
#include "vie_file.h"
#include "vie_network.h"
#include "vie_render.h"
#include "vie_rtp_rtcp.h"
#include "vie_defines.h"
#include "vie_errors.h"
#include "video_render_defines.h"

#include "vie_autotest_defines.h"

#ifndef WEBRTC_ANDROID
#include <string>
#endif

class TbCaptureDevice;
class TbInterfaces;
class TbVideoChannel;
class ViEToFileRenderer;

// This class provides a bunch of methods, implemented across several .cc
// files, which runs tests on the video engine. All methods will report
// errors using standard googletest macros, except when marked otherwise.
class ViEAutoTest : public webrtc::ViEBaseObserver, webrtc::ViECaptureObserver, webrtc::ViEEncoderObserver, webrtc::ViEDecoderObserver,
                    webrtc::ViENetworkObserver, webrtc::ViERTPObserver, webrtc::ViERTCPObserver, webrtc::TraceCallback
{
public:
    ViEAutoTest(void* window1, void* window2);
    ~ViEAutoTest();

    // These three are special and should not be run in a googletest harness.
    // They keep track of their errors by themselves and return the number
    // of errors.
    int ViELoopbackCall();
    int ViESimulcastCall();
    int ViECustomCall();

    // All functions except the three above are meant to run in a
    // googletest harness.
    void ViEStandardTest();
    void ViEExtendedTest();
    void ViEAPITest();

    // vie_autotest_base.cc
    void ViEBaseStandardTest();
    void ViEBaseExtendedTest();
    void ViEBaseAPITest();

    // vie_autotest_capture.cc
    void ViECaptureStandardTest();
    void ViECaptureExtendedTest();
    void ViECaptureAPITest();
    void ViECaptureExternalCaptureTest();

    // vie_autotest_codec.cc
    void ViECodecStandardTest(int captureIndex, 
                              const char* sendIpAddress, int sendPort, int receivePort);
    int ViECodecSetCodecSize(int width, int height);
    int ViECodecSetCaptureRotation();
    int ViECodecSetEventCallback(void (*eventCallback)(const char*, void*));
    int GetAverageSystemCPU(int& systemCPU);
    int GetSendCodecStastistics(unsigned int& keyFrames, unsigned int& deltaFrames);
    int GetReceiveCodecStastistics(unsigned int& keyFrames, unsigned int& deltaFrames);
    int GetReceivedRTCPStatistics(unsigned short& fractionLost,
                                  unsigned int& cumulativeLost, unsigned int& extendedMax,
                                  unsigned int& jitter, int& rttMs);
    int GetSentRTCPStatistics(unsigned short& fractionLost,
                              unsigned int& cumulativeLost,
                              unsigned int& extendedMax,
                              unsigned int& jitter,
                              int& rttMs);
    int GetRTPStatistics(unsigned int& bytesSent,
                         unsigned int& packetsSent,
                         unsigned int& bytesReceived,
                         unsigned int& packetsReceived);
    void ViECodecExtendedTest();
    void ViECodecExternalCodecTest();
    void ViECodecAPITest();
    void PerformanceAlarm(const unsigned int cpuLoad);
    void BrightnessAlarm(const int captureId, const webrtc::Brightness brightness);
    void CapturedFrameRate(const int captureId, const unsigned char frameRate);
    void NoPictureAlarm(const int captureId, const webrtc::CaptureAlarm alarm);
    void OutgoingRate(const int videoChannel, const unsigned int framerate, const unsigned int bitrate);
    void IncomingCodecChanged(const int videoChannel, const webrtc::VideoCodec& videoCodec);
    void IncomingRate(const int videoChannel, const unsigned int framerate, const unsigned int bitrate);
    void RequestNewKeyFrame(const int videoChannel);
    void OnPeriodicDeadOrAlive(const int videoChannel, const bool alive);
    void PacketTimeout(const int videoChannel, const webrtc::ViEPacketTimeout timeout);
    void IncomingSSRCChanged(const int videoChannel, const unsigned int SSRC);
    void IncomingCSRCChanged(const int videoChannel, const unsigned int CSRC, const bool added);
    void OnApplicationDataReceived(const int videoChannel, const unsigned char subType,
                                   const unsigned int name, const char* data,
                                   const unsigned short dataLengthInBytes);
    void Print(const webrtc::TraceLevel level, const char *traceString, const int length);

    // vie_autotest_encryption.cc
    void ViEEncryptionStandardTest();
    void ViEEncryptionExtendedTest();
    void ViEEncryptionAPITest();

    // vie_autotest_file.ccs
    void ViEFileStandardTest();
    void ViEFileExtendedTest();
    void ViEFileAPITest();

    // vie_autotest_image_process.cc
    void ViEImageProcessStandardTest();
    void ViEImageProcessExtendedTest();
    void ViEImageProcessAPITest();

    // vie_autotest_network.cc
    void ViENetworkStandardTest();
    void ViENetworkExtendedTest();
    void ViENetworkAPITest();

    // vie_autotest_render.cc
    void ViERenderStandardTest();
    void ViERenderExtendedTest();
    void ViERenderAPITest();

    // vie_autotest_rtp_rtcp.cc
    void ViERtpRtcpStandardTest();
    void ViERtpRtcpExtendedTest();
    void ViERtpRtcpAPITest();

    // vie_autotest_rtp_fuzz.cc
    void ViERtpTryInjectingRandomPacketsIntoRtpStream(long rand_seed);

private:
    void PrintAudioCodec(const webrtc::CodecInst audioCodec);
    void PrintVideoCodec(const webrtc::VideoCodec videoCodec);

    // Sets up rendering so the capture device output goes to window 1 and
    // the video engine output goes to window 2.
    void RenderCaptureDeviceAndOutputStream(TbInterfaces* video_engine,
                                            TbVideoChannel* video_channel,
                                            TbCaptureDevice* capture_device);

    void* _window1;
    void* _window2;
  
    int _vieChannel;
    int _vieCaptureId;
    webrtc::ViEBase* _vieBase;
    webrtc::ViECapture* _vieCapture;
    webrtc::ViECodec* _vieCodec;
    webrtc::ViENetwork* _vieNetwork;
    webrtc::ViERTP_RTCP* _vieRtpRtcp;

    webrtc::VideoRenderType _renderType;
    webrtc::VideoRender* _vrm1;
    webrtc::VideoRender* _vrm2;
  
    void (*_eventCallback)(const char*, void*);
};

#endif  // WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_
