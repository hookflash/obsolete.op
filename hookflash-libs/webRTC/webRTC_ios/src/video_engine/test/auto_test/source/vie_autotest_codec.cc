/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_autotest_defines.h"
#include "vie_autotest.h"
#include "engine_configurations.h"

#include "common_types.h"
#include "tb_capture_device.h"
#include "tb_I420_codec.h"
#include "tb_interfaces.h"
#include "tb_video_channel.h"
#include "vie_base.h"
#include "vie_capture.h"
#include "vie_codec.h"
#include "vie_network.h"
#include "vie_render.h"
#include "vie_rtp_rtcp.h"
#include "voe_base.h"

#include <sys/sysctl.h>
#include <string.h>

class TestCodecObserver : public webrtc::ViEEncoderObserver,
    public webrtc::ViEDecoderObserver {
public:
  int incoming_codec_called_;
  int incoming_rate_called_;
  int outgoing_rate_called_;

  unsigned char last_payload_type_;
  unsigned short last_width_;
  unsigned short last_height_;

  unsigned int last_outgoing_framerate_;
  unsigned int last_outgoing_bitrate_;
  unsigned int last_incoming_framerate_;
  unsigned int last_incoming_bitrate_;

  webrtc::VideoCodec incoming_codec_;

  TestCodecObserver() {
    incoming_codec_called_ = 0;
    incoming_rate_called_ = 0;
    outgoing_rate_called_ = 0;
    last_payload_type_ = 0;
    last_width_ = 0;
    last_height_ = 0;
    last_outgoing_framerate_ = 0;
    last_outgoing_bitrate_ = 0;
    last_incoming_framerate_ = 0;
    last_incoming_bitrate_ = 0;
    memset(&incoming_codec_, 0, sizeof(incoming_codec_));
  }
  virtual void IncomingCodecChanged(const int videoChannel,
      const webrtc::VideoCodec& videoCodec) {
    incoming_codec_called_++;
    last_payload_type_ = videoCodec.plType;
    last_width_ = videoCodec.width;
    last_height_ = videoCodec.height;

    memcpy(&incoming_codec_, &videoCodec, sizeof(videoCodec));
  }

  virtual void IncomingRate(const int videoChannel,
      const unsigned int framerate, const unsigned int bitrate) {
    incoming_rate_called_++;
    last_incoming_framerate_ += framerate;
    last_incoming_bitrate_ += bitrate;
  }

  virtual void OutgoingRate(const int videoChannel,
      const unsigned int framerate, const unsigned int bitrate) {
    outgoing_rate_called_++;
    last_outgoing_framerate_ += framerate;
    last_outgoing_bitrate_ += bitrate;
  }

  virtual void RequestNewKeyFrame(const int videoChannel) {
  }
};

class RenderFilter : public webrtc::ViEEffectFilter {
public:
  int num_frames_;
  unsigned int last_render_width_;
  unsigned int last_render_height_;

  RenderFilter()
      : num_frames_(0),
        last_render_width_(0),
        last_render_height_(0) {
  }

  virtual ~RenderFilter() {
  }

  virtual int Transform(int size, unsigned char* frameBuffer,
      unsigned int timeStamp90KHz, unsigned int width, unsigned int height) {
    num_frames_++;
    last_render_width_ = width;
    last_render_height_ = height;
    return 0;
  }
};

void ViEAutoTest::ViECodecStandardTest(int captureIndex,
                                       const char* sendIpAddress, int sendPort, int receivePort) {
  TbInterfaces interfaces("ViECodecStandardTest");

  TbCaptureDevice capture_device = TbCaptureDevice(interfaces, captureIndex);
  int capture_id = capture_device.captureId;
  _vieCaptureId = capture_id;
  
  int name[] = {CTL_HW, HW_MACHINE};
  size_t size;
  sysctl(name, 2, NULL, &size, NULL, 0);
  char* machine = (char*)malloc(size);
  sysctl(name, 2, machine, &size, NULL, 0);
  std::string machineName(machine);
  free(machine);

  webrtc::VideoEngine* video_engine = interfaces.video_engine;
  webrtc::ViEBase* base = interfaces.base;
  webrtc::ViECapture* capture = interfaces.capture;
  webrtc::ViERender* render = interfaces.render;
  webrtc::ViECodec* codec = interfaces.codec;
  webrtc::ViERTP_RTCP* rtp_rtcp = interfaces.rtp_rtcp;
  webrtc::ViENetwork* network = interfaces.network;
  
  video_engine->SetTraceCallback(this);
  
  _vieBase = base;
  _vieCapture = capture;
  _vieCodec = codec;
  _vieNetwork = network;
  _vieRtpRtcp = rtp_rtcp;

  int video_channel = -1;
  EXPECT_EQ(0, base->CreateChannel(video_channel));
  
  _vieChannel = video_channel;
  
  base->RegisterObserver(*this);
  capture->RegisterObserver(capture_id, *this);
  codec->RegisterEncoderObserver(video_channel, *this);
  codec->RegisterDecoderObserver(video_channel, *this);
  network->RegisterObserver(video_channel, *this);
  rtp_rtcp->RegisterRTPObserver(video_channel, *this);
  rtp_rtcp->RegisterRTCPObserver(video_channel, *this);
  
//  int discardedPackets = ptrViECodec->GetDiscardedPackets(videoChannel);
//  unsigned int estimated_bandwidth;
//  ptrViERtpRtcp->GetEstimatedSendBandwidth(videoChannel, &estimated_bandwidth);
//  ptrViERtpRtcp->GetEstimatedReceiveBandwidth(videoChannel, &estimated_bandwidth);
  
  EXPECT_EQ(0, network->SetMTU(video_channel, 576));

  EXPECT_EQ(0, capture->ConnectCaptureDevice(capture_id, video_channel));
  EXPECT_EQ( 0, rtp_rtcp->SetRTCPStatus(
      video_channel, webrtc::kRtcpCompound_RFC4585));

  EXPECT_EQ(0, rtp_rtcp->SetKeyFrameRequestMethod(
      video_channel, webrtc::kViEKeyFrameRequestPliRtcp));
  EXPECT_EQ(0, rtp_rtcp->SetTMMBRStatus(video_channel, true));
  EXPECT_EQ(0, render->AddRenderer(
      capture_id, _window1, 0, 0.0, 0.0, 1.0, 1.0));
  EXPECT_EQ(0, render->AddRenderer(
      video_channel, _window2, 1, 0.0, 0.0, 1.0, 1.0));
  EXPECT_EQ(0, render->StartRender(capture_id));
  EXPECT_EQ(0, render->StartRender(video_channel));

  webrtc::VideoCodec video_codec;
  memset(&video_codec, 0, sizeof(webrtc::VideoCodec));
//  for (int idx = 0; idx < codec->NumberOfCodecs(); idx++) {
//    EXPECT_EQ(0, codec->GetCodec(idx, video_codec));
//
//    if (video_codec.codecType != webrtc::kVideoCodecI420) {
//      video_codec.width = 640;
//      video_codec.height = 480;
//    }
//    if (video_codec.codecType == webrtc::kVideoCodecI420) {
//      video_codec.width = 176;
//      video_codec.height = 144;
//    }
//    EXPECT_EQ(0, codec->SetReceiveCodec(video_channel, video_codec));
//  }

  for (int idx = 0; idx < codec->NumberOfCodecs(); idx++) {
    EXPECT_EQ(0, codec->GetCodec(idx, video_codec));
    if (video_codec.codecType == webrtc::kVideoCodecVP8) {
      webrtc::RotateCapturedFrame orientation;
      EXPECT_EQ(0, capture->GetOrientation(NULL, orientation));
      int width = 0, height = 0, maxFramerate = 0, maxBitrate = 0;
      std::string iPadString("iPad");
      std::string iPad2String("iPad2");
      std::string iPad3String("iPad3");
      std::string iPhoneString("iPhone");
      std::string iPodString("iPod");
      if (captureIndex == 0) {
        if (orientation == webrtc::RotateCapturedFrame_0 || orientation == webrtc::RotateCapturedFrame_180) {
          if (machineName.compare(0, iPad2String.size(), iPad2String) == 0)
          {
            width = 320;
            height = 180;
            maxFramerate = 15;
            maxBitrate = 250;
          }
          else if (machineName.compare(0, iPad3String.size(), iPad3String) == 0)
          {
            width = 480;
            height = 270;
            maxFramerate = 15;
            maxBitrate = 250;
          }
          else if (machineName.compare(0, iPhoneString.size(), iPhoneString) == 0)
          {
            width = 160;
            height = 90;
            maxFramerate = 10;
            maxBitrate = 150;
          }
          else if (machineName.compare(0, iPodString.size(), iPodString) == 0)
          {
            width = 160;
            height = 90;
            maxFramerate = 10;
            maxBitrate = 150;
          }
        } else if (orientation == webrtc::RotateCapturedFrame_90 || orientation == webrtc::RotateCapturedFrame_270) {
          if (machineName.compare(0, iPad2String.size(), iPad2String) == 0)
          {
            width = 180;
            height = 320;
            maxFramerate = 15;
            maxBitrate = 250;
          }
          else if (machineName.compare(0, iPad3String.size(), iPad3String) == 0)
          {
            width = 270;
            height = 480;
            maxFramerate = 15;
            maxBitrate = 250;
          }
          else if (machineName.compare(0, iPhoneString.size(), iPhoneString) == 0)
          {
            width = 90;
            height = 160;
            maxFramerate = 10;
            maxBitrate = 150;
          }
          else if (machineName.compare(0, iPodString.size(), iPodString) == 0)
          {
            width = 90;
            height = 160;
            maxFramerate = 10;
            maxBitrate = 150;
          }
        }
      } else if (captureIndex == 1) {
        if (orientation == webrtc::RotateCapturedFrame_0 || orientation == webrtc::RotateCapturedFrame_180) {
          if (machineName.compare(0, iPadString.size(), iPadString) == 0)
          {
            width = 320;
            height = 240;
            maxFramerate = 15;
            maxBitrate = 250;
          }
          else if (machineName.compare(0, iPhoneString.size(), iPhoneString) == 0)
          {
            width = 160;
            height = 120;
            maxFramerate = 10;
            maxBitrate = 150;
          }
          else if (machineName.compare(0, iPodString.size(), iPodString) == 0)
          {
            width = 160;
            height = 120;
            maxFramerate = 10;
            maxBitrate = 150;
          }
        } else if (orientation == webrtc::RotateCapturedFrame_90 || orientation == webrtc::RotateCapturedFrame_270) {
          if (machineName.compare(0, iPadString.size(), iPadString) == 0)
          {
            width = 240;
            height = 320;
            maxFramerate = 15;
            maxBitrate = 250;
          }
          else if (machineName.compare(0, iPhoneString.size(), iPhoneString) == 0)
          {
            width = 120;
            height = 160;
            maxFramerate = 10;
            maxBitrate = 150;
          }
          else if (machineName.compare(0, iPodString.size(), iPodString) == 0)
          {
            width = 120;
            height = 160;
            maxFramerate = 10;
            maxBitrate = 150;
          }
        }
      } else {
        return;
      }
      video_codec.width = width;
      video_codec.height = height;
      video_codec.maxFramerate = maxFramerate;
      video_codec.maxBitrate = maxBitrate;
      video_codec.codecSpecific.VP8.feedbackModeOn = true;
      video_codec.codecSpecific.VP8.pictureLossIndicationOn = true;
      EXPECT_EQ(0, codec->SetSendCodec(video_channel, video_codec));
      break;
    }
  }

//  const char* ip_address = sendIpAddress;
  const char* ip_address = "127.0.0.1";
//  const unsigned short send_rtp_port = sendPort;
  const unsigned short send_rtp_port = 6000;
//  const unsigned short receive_rtp_port = receivePort;
  const unsigned short receive_rtp_port = 6000;
  EXPECT_EQ(0, network->SetLocalReceiver(video_channel, receive_rtp_port));
  EXPECT_EQ(0, base->StartReceive(video_channel));
  EXPECT_EQ(0, network->SetSendDestination(
      video_channel, ip_address, send_rtp_port));
  EXPECT_EQ(0, base->StartSend(video_channel));
  
  sleep(50000);
  
/*
  //
  // Make sure all codecs runs
  //
  {
    webrtc::ViEImageProcess* image_process =
        webrtc::ViEImageProcess::GetInterface(video_engine);
    TestCodecObserver codec_observer;
    EXPECT_EQ(0, codec->RegisterDecoderObserver(
        video_channel, codec_observer));
    ViETest::Log("Loop through all codecs for %d seconds",
        KAutoTestSleepTimeMs / 1000);

    for (int i = 0; i < codec->NumberOfCodecs() - 2; i++) {
      EXPECT_EQ(0, codec->GetCodec(i, video_codec));
      if (video_codec.codecType == webrtc::kVideoCodecI420) {
        // Lower resolution to sockets keep up.
        video_codec.width = 176;
        video_codec.height = 144;
        video_codec.maxFramerate = 15;
      }
      EXPECT_EQ(0, codec->SetSendCodec(video_channel, video_codec));
      ViETest::Log("\t %d. %s", i, video_codec.plName);

      RenderFilter frame_counter;
      EXPECT_EQ(0, image_process->RegisterRenderEffectFilter(
          video_channel, frame_counter));
      AutoTestSleep(KAutoTestSleepTimeMs);

      // Verify we've received and decoded correct payload.
      EXPECT_EQ(video_codec.codecType,
          codec_observer.incoming_codec_.codecType);

      int max_number_of_possible_frames = video_codec.maxFramerate
          * KAutoTestSleepTimeMs / 1000;

      if (video_codec.codecType == webrtc::kVideoCodecI420) {
        // Don't expect too much from I420, it requires a lot of bandwidth.
        EXPECT_GT(frame_counter.num_frames_, 0);
      } else {
#ifdef WEBRTC_ANDROID
        // To get the autotest to pass on some slow devices
        EXPECT_GT(
            frame_counter.num_frames_, max_number_of_possible_frames / 6);
#else
        EXPECT_GT(frame_counter.num_frames_, max_number_of_possible_frames / 4);
#endif
      }

      EXPECT_EQ(0, image_process->DeregisterRenderEffectFilter(
          video_channel));
    }
    image_process->Release();
    EXPECT_EQ(0, codec->DeregisterDecoderObserver(video_channel));
    ViETest::Log("Done!");
  }

  //
  // Test Callbacks
  //

  TestCodecObserver codecObserver;
  EXPECT_EQ(0, codec->RegisterEncoderObserver(
      video_channel, codecObserver));
  EXPECT_EQ(0, codec->RegisterDecoderObserver(
      video_channel, codecObserver));

  ViETest::Log("\nTesting codec callbacks...");

  for (int idx = 0; idx < codec->NumberOfCodecs(); idx++) {
    EXPECT_EQ(0, codec->GetCodec(idx, video_codec));
    if (video_codec.codecType == webrtc::kVideoCodecVP8) {
      EXPECT_EQ(0, codec->SetSendCodec(video_channel, video_codec));
      break;
    }
  }
  AutoTestSleep(KAutoTestSleepTimeMs);
*/
  
  EXPECT_EQ(0, base->StopSend(video_channel));
//  EXPECT_EQ(0, codec->DeregisterEncoderObserver(video_channel));
//  EXPECT_EQ(0, codec->DeregisterDecoderObserver(video_channel));

//  EXPECT_GT(codecObserver.incoming_codec_called_, 0);
//  EXPECT_GT(codecObserver.incoming_rate_called_, 0);
//  EXPECT_GT(codecObserver.outgoing_rate_called_, 0);

  EXPECT_EQ(0, base->StopReceive(video_channel));
  EXPECT_EQ(0, render->StopRender(video_channel));
  EXPECT_EQ(0, render->RemoveRenderer(capture_id));
  EXPECT_EQ(0, render->RemoveRenderer(video_channel));
  EXPECT_EQ(0, capture->DisconnectCaptureDevice(video_channel));
  EXPECT_EQ(0, base->DeleteChannel(video_channel));
}

int ViEAutoTest::ViECodecSetCodecSize(int width, int height)
{
  webrtc::VideoCodec videoCodec;
  memset(&videoCodec, 0, sizeof(webrtc::VideoCodec));
  _vieCodec->GetSendCodec(_vieChannel, videoCodec);
  videoCodec.width = width;
  videoCodec.height = height;
  _vieCodec->SetSendCodec(_vieChannel, videoCodec);
  return 0;
}

int ViEAutoTest::ViECodecSetCaptureRotation()
{
  webrtc::RotateCapturedFrame orientation;
  _vieCapture->GetOrientation(NULL, orientation);
  _vieCapture->SetRotateCapturedFrames(_vieCaptureId, orientation);
  return 0;
}

int ViEAutoTest::ViECodecSetEventCallback(void (*eventCallback)(const char*, void*))
{
  _eventCallback = eventCallback;
  return 0;
}

int ViEAutoTest::GetAverageSystemCPU(int& systemCPU)
{
  _vieBase->GetAverageSystemCPU(systemCPU);
  return 0;
}

int ViEAutoTest::GetSendCodecStastistics(unsigned int& keyFrames, unsigned int& deltaFrames)
{
  _vieCodec->GetSendCodecStastistics(_vieChannel, keyFrames, deltaFrames);
  return 0;
}

int ViEAutoTest::GetReceiveCodecStastistics(unsigned int& keyFrames, unsigned int& deltaFrames)
{
  _vieCodec->GetReceiveCodecStastistics(_vieChannel, keyFrames, deltaFrames);
  return 0;
}

int ViEAutoTest::GetReceivedRTCPStatistics(unsigned short& fractionLost,
                                           unsigned int& cumulativeLost, unsigned int& extendedMax,
                                           unsigned int& jitter, int& rttMs)
{
  _vieRtpRtcp->GetReceivedRTCPStatistics(_vieChannel, fractionLost, cumulativeLost, extendedMax,
                                         jitter, rttMs);
  return 0;
}

int ViEAutoTest::GetSentRTCPStatistics(unsigned short& fractionLost,
                                       unsigned int& cumulativeLost,
                                       unsigned int& extendedMax,
                                       unsigned int& jitter,
                                       int& rttMs)
{
  _vieRtpRtcp->GetSentRTCPStatistics(_vieChannel, fractionLost, cumulativeLost, extendedMax,
                                     jitter, rttMs);
  return 0;
}

int ViEAutoTest::GetRTPStatistics(unsigned int& bytesSent,
                                  unsigned int& packetsSent,
                                  unsigned int& bytesReceived,
                                  unsigned int& packetsReceived)
{
  _vieRtpRtcp->GetRTPStatistics(_vieChannel, bytesSent, packetsSent,
                                bytesReceived, packetsReceived);
  return 0;
}

void ViEAutoTest::ViECodecExtendedTest() {
  {
    ViETest::Log(" ");
    ViETest::Log("========================================");
    ViETest::Log(" ViECodec Extended Test\n");

    ViECodecAPITest();
    ViECodecStandardTest(0, NULL, 0, 0);
    ViECodecExternalCodecTest();

    TbInterfaces interfaces("ViECodecExtendedTest");
    webrtc::ViEBase* base = interfaces.base;
    webrtc::ViECapture* capture = interfaces.capture;
    webrtc::ViERender* render = interfaces.render;
    webrtc::ViECodec* codec = interfaces.codec;
    webrtc::ViERTP_RTCP* rtp_rtcp = interfaces.rtp_rtcp;
    webrtc::ViENetwork* network = interfaces.network;

    TbCaptureDevice capture_device = TbCaptureDevice(interfaces, 0);
    int capture_id = capture_device.captureId;

    int video_channel = -1;
    EXPECT_EQ(0, base->CreateChannel(video_channel));
    EXPECT_EQ(0, capture->ConnectCaptureDevice(capture_id, video_channel));
    EXPECT_EQ(0, rtp_rtcp->SetRTCPStatus(
        video_channel, webrtc::kRtcpCompound_RFC4585));
    EXPECT_EQ(0, rtp_rtcp->SetKeyFrameRequestMethod(
        video_channel, webrtc::kViEKeyFrameRequestPliRtcp));
    EXPECT_EQ(0, rtp_rtcp->SetTMMBRStatus(video_channel, true));
    EXPECT_EQ(0, render->AddRenderer(
        capture_id, _window1, 0, 0.0, 0.0, 1.0, 1.0));

    EXPECT_EQ(0, render->AddRenderer(
        video_channel, _window2, 1, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, render->StartRender(capture_id));
    EXPECT_EQ(0, render->StartRender(video_channel));

    webrtc::VideoCodec video_codec;
    memset(&video_codec, 0, sizeof(webrtc::VideoCodec));
    for (int idx = 0; idx < codec->NumberOfCodecs(); idx++) {
      EXPECT_EQ(0, codec->GetCodec(idx, video_codec));
      if (video_codec.codecType != webrtc::kVideoCodecI420) {
        video_codec.width = 640;
        video_codec.height = 480;
      }
      EXPECT_EQ(0, codec->SetReceiveCodec(video_channel, video_codec));
    }

    const char* ip_address = "127.0.0.1";
    const unsigned short rtp_port = 6000;
    EXPECT_EQ(0, network->SetLocalReceiver(video_channel, rtp_port));
    EXPECT_EQ(0, base->StartReceive(video_channel));
    EXPECT_EQ(0, network->SetSendDestination(
        video_channel, ip_address, rtp_port));
    EXPECT_EQ(0, base->StartSend(video_channel));

    //
    // Codec specific tests
    //
    memset(&video_codec, 0, sizeof(webrtc::VideoCodec));
    EXPECT_EQ(0, base->StopSend(video_channel));

    TestCodecObserver codec_observer;
    EXPECT_EQ(0, codec->RegisterEncoderObserver(video_channel, codec_observer));
    EXPECT_EQ(0, codec->RegisterDecoderObserver(video_channel, codec_observer));
    EXPECT_EQ(0, base->StopReceive(video_channel));

    EXPECT_EQ(0, render->StopRender(video_channel));
    EXPECT_EQ(0, render->RemoveRenderer(capture_id));
    EXPECT_EQ(0, render->RemoveRenderer(video_channel));
    EXPECT_EQ(0, capture->DisconnectCaptureDevice(video_channel));
    EXPECT_EQ(0, base->DeleteChannel(video_channel));
  }

  //
  // Multiple send channels.
  //
  {
    // Create two channels, where the second channel is created from the
    // first channel. Send different resolutions on the channels and verify
    // the received streams.

    TbInterfaces video_engine("ViECodecExtendedTest2");
    TbCaptureDevice tbCapture(video_engine, 0);

    // Create channel 1.
    int video_channel_1 = -1;
    EXPECT_EQ(0, video_engine.base->CreateChannel(video_channel_1));

    // Create channel 2 based on the first channel.
    int video_channel_2 = -1;
    EXPECT_EQ(0, video_engine.base->CreateChannel(
        video_channel_2, video_channel_1));
    EXPECT_NE(video_channel_1, video_channel_2)
      << "Channel 2 should be unique.";

    unsigned short rtp_port_1 = 12000;
    unsigned short rtp_port_2 = 13000;
    EXPECT_EQ(0, video_engine.network->SetLocalReceiver(
        video_channel_1, rtp_port_1));
    EXPECT_EQ(0, video_engine.network->SetSendDestination(
        video_channel_1, "127.0.0.1", rtp_port_1));
    EXPECT_EQ(0, video_engine.network->SetLocalReceiver(
        video_channel_2, rtp_port_2));
    EXPECT_EQ(0, video_engine.network->SetSendDestination(
        video_channel_2, "127.0.0.1", rtp_port_2));

    tbCapture.ConnectTo(video_channel_1);
    tbCapture.ConnectTo(video_channel_2);

    EXPECT_EQ(0, video_engine.rtp_rtcp->SetKeyFrameRequestMethod(
        video_channel_1, webrtc::kViEKeyFrameRequestPliRtcp));
    EXPECT_EQ(0, video_engine.rtp_rtcp->SetKeyFrameRequestMethod(
        video_channel_2, webrtc::kViEKeyFrameRequestPliRtcp));
    EXPECT_EQ(0, video_engine.render->AddRenderer(
        video_channel_1, _window1, 0, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, video_engine.render->StartRender(video_channel_1));
    EXPECT_EQ(0, video_engine.render->AddRenderer(
        video_channel_2, _window2, 0, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, video_engine.render->StartRender(video_channel_2));

    // Set Send codec
    unsigned short codecWidth = 320;
    unsigned short codecHeight = 240;
    bool codecSet = false;
    webrtc::VideoCodec videoCodec;
    webrtc::VideoCodec sendCodec1;
    webrtc::VideoCodec sendCodec2;
    for (int idx = 0; idx < video_engine.codec->NumberOfCodecs(); idx++) {
      EXPECT_EQ(0, video_engine.codec->GetCodec(idx, videoCodec));
      EXPECT_EQ(0, video_engine.codec->SetReceiveCodec(
          video_channel_1, videoCodec));
      if (videoCodec.codecType == webrtc::kVideoCodecVP8) {
        memcpy(&sendCodec1, &videoCodec, sizeof(videoCodec));
        sendCodec1.width = codecWidth;
        sendCodec1.height = codecHeight;
        EXPECT_EQ(0, video_engine.codec->SetSendCodec(
            video_channel_1, sendCodec1));
        memcpy(&sendCodec2, &videoCodec, sizeof(videoCodec));
        sendCodec2.width = 2 * codecWidth;
        sendCodec2.height = 2 * codecHeight;
        EXPECT_EQ(0, video_engine.codec->SetSendCodec(
            video_channel_2, sendCodec2));
        codecSet = true;
        break;
      }
    }EXPECT_TRUE(codecSet);

    // We need to verify using render effect filter since we won't trigger
    // a decode reset in loopback (due to using the same SSRC).
    RenderFilter filter1;
    RenderFilter filter2;
    EXPECT_EQ(0, video_engine.image_process->RegisterRenderEffectFilter(
        video_channel_1, filter1));
    EXPECT_EQ(0, video_engine.image_process->RegisterRenderEffectFilter(
        video_channel_2, filter2));

    EXPECT_EQ(0, video_engine.base->StartReceive(video_channel_1));
    EXPECT_EQ(0, video_engine.base->StartSend(video_channel_1));
    EXPECT_EQ(0, video_engine.base->StartReceive(video_channel_2));
    EXPECT_EQ(0, video_engine.base->StartSend(video_channel_2));

    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(0, video_engine.base->StopReceive(video_channel_1));
    EXPECT_EQ(0, video_engine.base->StopSend(video_channel_1));
    EXPECT_EQ(0, video_engine.base->StopReceive(video_channel_2));
    EXPECT_EQ(0, video_engine.base->StopSend(video_channel_2));

    EXPECT_EQ(0, video_engine.image_process->DeregisterRenderEffectFilter(
        video_channel_1));
    EXPECT_EQ(0, video_engine.image_process->DeregisterRenderEffectFilter(
        video_channel_2));
    EXPECT_EQ(sendCodec1.width, filter1.last_render_width_);
    EXPECT_EQ(sendCodec1.height, filter1.last_render_height_);
    EXPECT_EQ(sendCodec2.width, filter2.last_render_width_);
    EXPECT_EQ(sendCodec2.height, filter2.last_render_height_);

    EXPECT_EQ(0, video_engine.base->DeleteChannel(video_channel_1));
    EXPECT_EQ(0, video_engine.base->DeleteChannel(video_channel_2));
  }
}

void ViEAutoTest::ViECodecAPITest() {
  webrtc::VideoEngine* video_engine = NULL;
  video_engine = webrtc::VideoEngine::Create();
  EXPECT_TRUE(video_engine != NULL);

  webrtc::ViEBase* base = webrtc::ViEBase::GetInterface(video_engine);
  EXPECT_EQ(0, base->Init());

  int video_channel = -1;
  EXPECT_EQ(0, base->CreateChannel(video_channel));

  webrtc::ViECodec* codec = webrtc::ViECodec::GetInterface(video_engine);
  EXPECT_TRUE(codec != NULL);

  webrtc::VideoCodec video_codec;
  memset(&video_codec, 0, sizeof(webrtc::VideoCodec));

  const int number_of_codecs = codec->NumberOfCodecs();

  for (int i = 0; i < number_of_codecs; i++) {
    EXPECT_EQ(0, codec->GetCodec(i, video_codec));

    if (video_codec.codecType == webrtc::kVideoCodecVP8) {
      EXPECT_EQ(0, codec->SetSendCodec(video_channel, video_codec));
      break;
    }
  }
  memset(&video_codec, 0, sizeof(video_codec));
  EXPECT_EQ(0, codec->GetSendCodec(video_channel, video_codec));
  EXPECT_EQ(webrtc::kVideoCodecVP8, video_codec.codecType);

  for (int i = 0; i < number_of_codecs; i++) {
    EXPECT_EQ(0, codec->GetCodec(i, video_codec));
    if (video_codec.codecType == webrtc::kVideoCodecI420) {
      EXPECT_EQ(0, codec->SetSendCodec(video_channel, video_codec));
      break;
    }
  }
  memset(&video_codec, 0, sizeof(video_codec));
  EXPECT_EQ(0, codec->GetSendCodec(video_channel, video_codec));
  EXPECT_EQ(webrtc::kVideoCodecI420, video_codec.codecType);

  EXPECT_EQ(0, base->DeleteChannel(video_channel));

  EXPECT_EQ(0, codec->Release());
  EXPECT_EQ(0, base->Release());
  EXPECT_TRUE(webrtc::VideoEngine::Delete(video_engine));
}

void ViEAutoTest::PerformanceAlarm(const unsigned int cpuLoad)
{
  if (_eventCallback != NULL)
    _eventCallback("PerformanceAlarm", NULL);
}

void ViEAutoTest::BrightnessAlarm(const int captureId, const webrtc::Brightness brightness)
{
  if (_eventCallback != NULL)
    _eventCallback("BrightnessAlarm", NULL);
}

void ViEAutoTest::CapturedFrameRate(const int captureId, const unsigned char frameRate)
{
  if (_eventCallback != NULL)
    _eventCallback("CapturedFrameRate", (void*)&frameRate);
}

void ViEAutoTest::NoPictureAlarm(const int captureId, const webrtc::CaptureAlarm alarm)
{
  if (_eventCallback != NULL)
    _eventCallback("NoPictureAlarm", NULL);
}

void ViEAutoTest::OutgoingRate(const int videoChannel, const unsigned int framerate, const unsigned int bitrate)
{
  if (_eventCallback != NULL)
  {
    unsigned int data[2];
    data[0] = framerate;
    data[1] = bitrate;
    _eventCallback("OutgoingRate", (void*)data);
  }
}

void ViEAutoTest::IncomingCodecChanged(const int videoChannel, const webrtc::VideoCodec& videoCodec)
{
  if (_eventCallback != NULL)
    _eventCallback("IncomingCodecChanged", NULL);
}

void ViEAutoTest::IncomingRate(const int videoChannel, const unsigned int framerate, const unsigned int bitrate)
{
  if (_eventCallback != NULL)
  {
    unsigned int data[2];
    data[0] = framerate;
    data[1] = bitrate;
    _eventCallback("IncomingRate", (void*)data);
  }
}

void ViEAutoTest::RequestNewKeyFrame(const int videoChannel)
{
  if (_eventCallback != NULL)
    _eventCallback("RequestNewKeyFrame", NULL);
}

void ViEAutoTest::OnPeriodicDeadOrAlive(const int videoChannel, const bool alive)
{
  if (_eventCallback != NULL)
    _eventCallback("OnPeriodicDeadOrAlive", NULL);
}

void ViEAutoTest::PacketTimeout(const int videoChannel, const webrtc::ViEPacketTimeout timeout)
{
  if (_eventCallback != NULL)
    _eventCallback("PacketTimeout", NULL);
}

void ViEAutoTest::IncomingSSRCChanged(const int videoChannel, const unsigned int SSRC)
{
  if (_eventCallback != NULL)
    _eventCallback("IncomingSSRCChanged", NULL);
}

void ViEAutoTest::IncomingCSRCChanged(const int videoChannel, const unsigned int CSRC, const bool added)
{
  if (_eventCallback != NULL)
    _eventCallback("IncomingCSRCChanged", NULL);
}

void ViEAutoTest::OnApplicationDataReceived(const int videoChannel, const unsigned char subType,
                                            const unsigned int name, const char* data,
                                            const unsigned short dataLengthInBytes)
{
  if (_eventCallback != NULL)
    _eventCallback("OnApplicationDataReceived", NULL);  
}

void ViEAutoTest::Print(const webrtc::TraceLevel level, const char *traceString, const int length)
{
  printf("WEBRTC LOG: %s\n", traceString);
}

#ifdef WEBRTC_VIDEO_ENGINE_EXTERNAL_CODEC_API
#include "vie_external_codec.h"
#endif
void ViEAutoTest::ViECodecExternalCodecTest() {
  ViETest::Log(" ");
  ViETest::Log("========================================");
  ViETest::Log(" ViEExternalCodec Test\n");

  //***************************************************************
  //  Begin create/initialize WebRTC Video Engine for testing
  //***************************************************************

  //***************************************************************
  //  Engine ready. Begin testing class
  //***************************************************************

#ifdef WEBRTC_VIDEO_ENGINE_EXTERNAL_CODEC_API
  int numberOfErrors=0;
  {
    int error=0;
    TbInterfaces ViE("ViEExternalCodec");
    TbCaptureDevice captureDevice(ViE);
    TbVideoChannel channel(ViE, webrtc::kVideoCodecI420,
        352,288,30,(352*288*3*8*30)/(2*1000));

    captureDevice.ConnectTo(channel.videoChannel);

    error = ViE.render->AddRenderer(channel.videoChannel, _window1, 0,
        0.0, 0.0, 1.0, 1.0);
    numberOfErrors += ViETest::TestError(error == 0, "ERROR: %s at line %d",
        __FUNCTION__, __LINE__);
    error = ViE.render->StartRender(channel.videoChannel);
    numberOfErrors += ViETest::TestError(error == 0, "ERROR: %s at line %d",
        __FUNCTION__, __LINE__);

    channel.StartReceive();
    channel.StartSend();

    ViETest::Log("Using internal I420 codec");
    AutoTestSleep(KAutoTestSleepTimeMs/2);

    webrtc::ViEExternalCodec* ptrViEExtCodec =
      webrtc::ViEExternalCodec::GetInterface(ViE.video_engine);
    numberOfErrors += ViETest::TestError(ptrViEExtCodec != NULL,
        "ERROR: %s at line %d",
        __FUNCTION__, __LINE__);

    webrtc::VideoCodec codecStruct;

    error=ViE.codec->GetSendCodec(channel.videoChannel,codecStruct);
    numberOfErrors += ViETest::TestError(ptrViEExtCodec != NULL,
        "ERROR: %s at line %d",
        __FUNCTION__, __LINE__);

    // Use external encoder instead
    {
      TbI420Encoder extEncoder;

      // Test to register on wrong channel
      error = ptrViEExtCodec->RegisterExternalSendCodec(
          channel.videoChannel+5,codecStruct.plType,&extEncoder);
      numberOfErrors += ViETest::TestError(error == -1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(
          ViE.LastError() == kViECodecInvalidArgument,
          "ERROR: %s at line %d", __FUNCTION__, __LINE__);

      error = ptrViEExtCodec->RegisterExternalSendCodec(
          channel.videoChannel,codecStruct.plType,&extEncoder);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      // Use new external encoder
      error = ViE.codec->SetSendCodec(channel.videoChannel,codecStruct);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      TbI420Decoder extDecoder;
      error = ptrViEExtCodec->RegisterExternalReceiveCodec(
          channel.videoChannel,codecStruct.plType,&extDecoder);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      error = ViE.codec->SetReceiveCodec(channel.videoChannel,
          codecStruct);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      ViETest::Log("Using external I420 codec");
      AutoTestSleep(KAutoTestSleepTimeMs);

      // Test to deregister on wrong channel
      error = ptrViEExtCodec->DeRegisterExternalSendCodec(
          channel.videoChannel+5,codecStruct.plType);
      numberOfErrors += ViETest::TestError(error == -1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(
          ViE.LastError() == kViECodecInvalidArgument,
          "ERROR: %s at line %d", __FUNCTION__, __LINE__);

      // Test to deregister wrong payload type.
      error = ptrViEExtCodec->DeRegisterExternalSendCodec(
          channel.videoChannel,codecStruct.plType-1);
      numberOfErrors += ViETest::TestError(error == -1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      // Deregister external send codec
      error = ptrViEExtCodec->DeRegisterExternalSendCodec(
          channel.videoChannel,codecStruct.plType);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d", __FUNCTION__, __LINE__);

      error = ptrViEExtCodec->DeRegisterExternalReceiveCodec(
          channel.videoChannel,codecStruct.plType);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      // Verify that the encoder and decoder has been used
      TbI420Encoder::FunctionCalls encodeCalls =
        extEncoder.GetFunctionCalls();
      numberOfErrors += ViETest::TestError(encodeCalls.InitEncode == 1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(encodeCalls.Release == 1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(encodeCalls.Encode > 30,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(
          encodeCalls.RegisterEncodeCompleteCallback ==1,
          "ERROR: %s at line %d", __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(encodeCalls.SetRates > 1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(encodeCalls.SetPacketLoss > 1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      TbI420Decoder::FunctionCalls decodeCalls =
        extDecoder.GetFunctionCalls();
      numberOfErrors += ViETest::TestError(decodeCalls.InitDecode == 1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(decodeCalls.Release == 1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(decodeCalls.Decode > 30,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(
          decodeCalls.RegisterDecodeCompleteCallback ==1,
          "ERROR: %s at line %d", __FUNCTION__, __LINE__);

      ViETest::Log("Changing payload type Using external I420 codec");

      codecStruct.plType=codecStruct.plType-1;
      error = ptrViEExtCodec->RegisterExternalReceiveCodec(
          channel.videoChannel, codecStruct.plType, &extDecoder);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      error = ViE.codec->SetReceiveCodec(channel.videoChannel,
          codecStruct);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      error = ptrViEExtCodec->RegisterExternalSendCodec(
          channel.videoChannel, codecStruct.plType, &extEncoder);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      // Use new external encoder
      error = ViE.codec->SetSendCodec(channel.videoChannel,
          codecStruct);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      AutoTestSleep(KAutoTestSleepTimeMs/2);

      //***************************************************************
      //  Testing finished. Tear down Video Engine
      //***************************************************************

      error = ptrViEExtCodec->DeRegisterExternalSendCodec(
          channel.videoChannel,codecStruct.plType);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      error = ptrViEExtCodec->DeRegisterExternalReceiveCodec(
          channel.videoChannel,codecStruct.plType);
      numberOfErrors += ViETest::TestError(error == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      // Verify that the encoder and decoder has been used
      encodeCalls = extEncoder.GetFunctionCalls();
      numberOfErrors += ViETest::TestError(encodeCalls.InitEncode == 2,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(encodeCalls.Release == 2,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(encodeCalls.Encode > 30,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(
          encodeCalls.RegisterEncodeCompleteCallback == 2,
          "ERROR: %s at line %d", __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(encodeCalls.SetRates > 1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(encodeCalls.SetPacketLoss > 1,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);

      decodeCalls = extDecoder.GetFunctionCalls();
      numberOfErrors += ViETest::TestError(decodeCalls.InitDecode == 2,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(decodeCalls.Release == 2,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(decodeCalls.Decode > 30,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
      numberOfErrors += ViETest::TestError(
          decodeCalls.RegisterDecodeCompleteCallback == 2,
          "ERROR: %s at line %d", __FUNCTION__, __LINE__);

      int remainingInterfaces = ptrViEExtCodec->Release();
      numberOfErrors += ViETest::TestError(remainingInterfaces == 0,
          "ERROR: %s at line %d",
          __FUNCTION__, __LINE__);
    } // tbI420Encoder and extDecoder goes out of scope

    ViETest::Log("Using internal I420 codec");
    AutoTestSleep(KAutoTestSleepTimeMs/2);

  }
  if (numberOfErrors > 0)
  {
    // Test failed
    ViETest::Log(" ");
    ViETest::Log(" ERROR ViEExternalCodec Test FAILED!");
    ViETest::Log(" Number of errors: %d", numberOfErrors);
    ViETest::Log("========================================");
    ViETest::Log(" ");
    return;
  }

  ViETest::Log(" ");
  ViETest::Log(" ViEExternalCodec Test PASSED!");
  ViETest::Log("========================================");
  ViETest::Log(" ");
  return;

#else
  ViETest::Log(" ViEExternalCodec not enabled\n");
  return;
#endif
}
