/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
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

#ifndef WEBRTC_ANDROID
#include <string>
#endif

using namespace webrtc;

class ViEAutoTest : public ViEBaseObserver, ViECaptureObserver, ViEEncoderObserver, ViEDecoderObserver,
                           ViENetworkObserver, ViERTPObserver, ViERTCPObserver, TraceCallback
{
public:
    ViEAutoTest(void* window1, void* window2);
    ~ViEAutoTest();

    int ViEStandardTest();
    int ViEExtendedTest();
    int ViEAPITest();
    int ViELoopbackCall();

    // custom call and helper functions
    int ViECustomCall();

    // general settings functions
    bool GetVideoDevice(ViEBase* ptrViEBase, ViECapture* ptrViECapture,
                        char* captureDeviceName, char* captureDeviceUniqueId);
    bool GetIPAddress(char* IP);
#ifndef WEBRTC_ANDROID
    bool ValidateIP(std::string iStr);
#endif
    void PrintCallInformation(char* IP, char* videoCaptureDeviceName,
                              char* videoCaptureUniqueId,
                              webrtc::VideoCodec videoCodec, int videoTxPort,
                              int videoRxPort, char* audioCaptureDeviceName,
                              char* audioPlaybackDeviceName,
                              webrtc::CodecInst audioCodec, int audioTxPort,
                              int audioRxPort);

    // video settings functions
    bool GetVideoPorts(int* txPort, int* rxPort);
    bool GetVideoCodec(ViECodec* ptrViECodec, webrtc::VideoCodec& videoCodec);

    // audio settings functions
    bool GetAudioDevices(VoEBase* ptrVEBase, VoEHardware* ptrVEHardware,
                         char* recordingDeviceName, int& recordingDeviceIndex,
                         char* playbackDeviceName, int& playbackDeviceIndex);
    bool GetAudioDevices(VoEBase* ptrVEBase, VoEHardware* ptrVEHardware,
                         int& recordingDeviceIndex, int& playbackDeviceIndex);
    bool GetAudioPorts(int* txPort, int* rxPort);
    bool GetAudioCodec(VoECodec* ptrVeCodec, CodecInst& audioCodec);

    // vie_autotest_base.cc
    int ViEBaseStandardTest();
    int ViEBaseExtendedTest();
    int ViEBaseAPITest();

    // vie_autotest_capture.cc
    int ViECaptureStandardTest();
    int ViECaptureExtendedTest();
    int ViECaptureAPITest();
    int ViECaptureExternalCaptureTest();

    // vie_autotest_codec.cc
    int ViECodecStandardTest(int width, int height, const char* sendIpAddress, int sendPort, int receivePort);
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
    int ViECodecExtendedTest();
    int ViECodecExternalCodecTest();
    int ViECodecAPITest();
    void PerformanceAlarm(const unsigned int cpuLoad);
    void BrightnessAlarm(const int captureId, const Brightness brightness);
    void CapturedFrameRate(const int captureId, const unsigned char frameRate);
    void NoPictureAlarm(const int captureId, const CaptureAlarm alarm);
    void OutgoingRate(const int videoChannel, const unsigned int framerate, const unsigned int bitrate);
    void IncomingCodecChanged(const int videoChannel, const VideoCodec& videoCodec);
    void IncomingRate(const int videoChannel, const unsigned int framerate, const unsigned int bitrate);
    void RequestNewKeyFrame(const int videoChannel);
    void OnPeriodicDeadOrAlive(const int videoChannel, const bool alive);
    void PacketTimeout(const int videoChannel, const ViEPacketTimeout timeout);
    void IncomingSSRCChanged(const int videoChannel, const unsigned int SSRC);
    void IncomingCSRCChanged(const int videoChannel, const unsigned int CSRC, const bool added);
    void OnApplicationDataReceived(const int videoChannel, const unsigned char subType,
                                   const unsigned int name, const char* data,
                                   const unsigned short dataLengthInBytes);
    void Print(const TraceLevel level, const char *traceString, const int length);

    // vie_autotest_encryption.cc
    int ViEEncryptionStandardTest();
    int ViEEncryptionExtendedTest();
    int ViEEncryptionAPITest();

    // vie_autotest_file.ccs
    int ViEFileStandardTest();
    int ViEFileExtendedTest();
    int ViEFileAPITest();

    // vie_autotest_image_process.cc
    int ViEImageProcessStandardTest();
    int ViEImageProcessExtendedTest();
    int ViEImageProcessAPITest();

    // vie_autotest_network.cc
    int ViENetworkStandardTest();
    int ViENetworkExtendedTest();
    int ViENetworkAPITest();

    // vie_autotest_render.cc
    int ViERenderStandardTest();
    int ViERenderExtendedTest();
    int ViERenderAPITest();

    // vie_autotest_rtp_rtcp.cc
    int ViERtpRtcpStandardTest();
    int ViERtpRtcpExtendedTest();
    int ViERtpRtcpAPITest();

private:
    void PrintAudioCodec(const webrtc::CodecInst audioCodec);
    void PrintVideoCodec(const webrtc::VideoCodec videoCodec);

    void* _window1;
    void* _window2;
  
    int _vieChannel;
    int _vieCaptureId;
    ViEBase* _vieBase;
    ViECapture* _vieCapture;
    ViECodec* _vieCodec;
    ViENetwork* _vieNetwork;
    ViERTP_RTCP* _vieRtpRtcp;

    VideoRenderType _renderType;
    VideoRender* _vrm1;
    VideoRender* _vrm2;
  
    void (*_eventCallback)(const char*, void*);
};

#endif  // WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_
