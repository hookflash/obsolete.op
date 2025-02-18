/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This sub-API supports the following functionalities:
//
//  - Allocating capture devices.
//  - Connect a capture device with one or more channels.
//  - Start and stop capture devices.
//  - Getting capture device capabilities.

#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_CAPTURE_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_CAPTURE_H_

#include "common_types.h"

namespace webrtc {

class VideoEngine;
class VideoCaptureModule;

// This structure describes one set of the supported capabilities for a capture
// device.
struct CaptureCapability {
  unsigned int width;
  unsigned int height;
  unsigned int maxFPS;
  RawVideoType rawType;
  VideoCodecType codecType;
  unsigned int expectedCaptureDelay;
  bool interlaced;
  bool faceDetection;
  CaptureCapability() {
    width = 0;
    height = 0;
    maxFPS = 0;
    rawType = kVideoI420;
    codecType = kVideoCodecUnknown;
    expectedCaptureDelay = 0;
    interlaced = false;
    faceDetection = false;
  }
};

// This enumerator tells the current brightness alarm mode.
enum Brightness {
  Normal = 0,
  Bright = 1,
  Dark = 2
};

// This enumerator describes the capture alarm mode.
enum CaptureAlarm {
  AlarmRaised = 0,
  AlarmCleared = 1
};

enum RotateCapturedFrame {
  RotateCapturedFrame_0 = 0,
  RotateCapturedFrame_90 = 90,
  RotateCapturedFrame_180 = 180,
  RotateCapturedFrame_270 = 270
};
  
enum CapturedFrameOrientation
{
  CapturedFrameOrientation_LandscapeLeft,
  CapturedFrameOrientation_PortraitUpsideDown,
  CapturedFrameOrientation_LandscapeRight,
  CapturedFrameOrientation_Portrait
};

struct ViEVideoFrameI420 {
  ViEVideoFrameI420() {
    y_plane = NULL;
    u_plane = NULL;
    v_plane = NULL;
    y_pitch = 0;
    u_pitch = 0;
    v_pitch = 0;
    width = 0;
    height = 0;
  }

  unsigned char* y_plane;
  unsigned char* u_plane;
  unsigned char* v_plane;

  int y_pitch;
  int u_pitch;
  int v_pitch;

  unsigned short width;
  unsigned short height;
};

// This class declares an abstract interface to be used when implementing
// a user-defined capture device. This interface is not meant to be
// implemented by the user. Instead, the user should call AllocateCaptureDevice
// in the ViECapture interface, which will create a suitable implementation.
// The user should then call IncomingFrame in this interface to deliver
// captured frames to the system.
class WEBRTC_DLLEXPORT ViEExternalCapture {
 public:
  ViEExternalCapture() {}
  virtual ~ViEExternalCapture() {}

  // This method is called by the user to deliver a new captured frame to
  // VideoEngine.
  virtual int IncomingFrame(unsigned char* video_frame,
                            unsigned int video_frame_length,
                            unsigned short width,
                            unsigned short height,
                            RawVideoType video_type,
                            unsigned long long capture_time = 0) = 0;

  // This method is specifically for delivering a new captured I420 frame to
  // VideoEngine.
  virtual int IncomingFrameI420(
      const ViEVideoFrameI420& video_frame,
      unsigned long long capture_time = 0) = 0;
};

// This class declares an abstract interface for a user defined observer. It is
// up to the VideoEngine user to implement a derived class which implements the
// observer class. The observer is registered using RegisterObserver() and
// deregistered using DeregisterObserver().
class WEBRTC_DLLEXPORT ViECaptureObserver {
 public:
  // This method is called if a bright or dark captured image is detected.
  virtual void BrightnessAlarm(const int capture_id,
                               const Brightness brightness) = 0;

  // This method is called periodically telling the capture device frame rate.
  virtual void CapturedFrameRate(const int capture_id,
                                 const unsigned char frame_rate) = 0;

  // This method is called if the capture device stops delivering images to
  // VideoEngine.
  virtual void NoPictureAlarm(const int capture_id,
                              const CaptureAlarm alarm) = 0;
  
  // This method is called when face is detected on capture device
  // VideoEngine.
  virtual void FaceDetected(const int capture_id) = 0;

 protected:
  virtual ~ViECaptureObserver() {}
};

class WEBRTC_DLLEXPORT ViECapture {
 public:
  // Factory for the ViECapture sub‐API and increases an internal reference
  // counter if successful. Returns NULL if the API is not supported or if
  // construction fails.
  static ViECapture* GetInterface(VideoEngine* video_engine);

  // Releases the ViECapture sub-API and decreases an internal reference
  // counter.
  // Returns the new reference count. This value should be zero
  // for all sub-API:s before the VideoEngine object can be safely deleted.
  virtual int Release() = 0;

  // Gets the number of available capture devices.
  virtual int NumberOfCaptureDevices() = 0;

  // Gets the name and unique id of a capture device.
  virtual int GetCaptureDevice(unsigned int list_number,
                               char* device_nameUTF8,
                               const unsigned int device_nameUTF8Length,
                               char* unique_idUTF8,
                               const unsigned int unique_idUTF8Length) = 0;

  // Allocates a capture device to be used in VideoEngine.
  virtual int AllocateCaptureDevice(const char* unique_idUTF8,
                                    const unsigned int unique_idUTF8Length,
                                    int& capture_id) = 0;

  // Registers an external capture device to be used in VideoEngine
  virtual int AllocateExternalCaptureDevice(
      int& capture_id,
      ViEExternalCapture *&external_capture) = 0;

  // Use capture device using external capture module.
  virtual int AllocateCaptureDevice(VideoCaptureModule& capture_module,
                                    int& capture_id) = 0;

  // Releases a capture device and makes it available for other applications.
  virtual int ReleaseCaptureDevice(const int capture_id) = 0;

  // This function connects a capture device with a channel. Multiple channels
  // can be connected to the same capture device.
  virtual int ConnectCaptureDevice(const int capture_id,
                                   const int video_channel) = 0;

  // Disconnects a capture device as input for a specified channel.
  virtual int DisconnectCaptureDevice(const int video_channel) = 0;

  // Makes a capture device start capturing video frames.
  virtual int StartCapture(
      const int capture_id,
      const CaptureCapability& capture_capability = CaptureCapability()) = 0;

  // Stops a started capture device from capturing video frames.
  virtual int StopCapture(const int capture_id) = 0;

  // Rotates captured frames before encoding and sending.
  // Used on mobile devices with rotates cameras.
  virtual int SetRotateCapturedFrames(const int capture_id,
                                      const RotateCapturedFrame rotation) = 0;
  
  // Sets default value for video frame orientation. This value is used when a device is in
  // face up / face down position.
  // Used on mobile devices with rotates cameras.
  virtual int SetDefaultCapturedFrameOrientation(const int capture_id,
                                                 const CapturedFrameOrientation orientation) = 0;
  
  // Sets value for video frame orientation when video rotation is locked.
  // Used on mobile devices with rotates cameras.
  virtual int SetCapturedFrameLockedOrientation(const int capture_id,
                                                const CapturedFrameOrientation orientation) = 0;
  
  // Enables captured frame orientation locking.
  // Used on mobile devices with rotates cameras.
  virtual int EnableCapturedFrameOrientationLock(const int capture_id,
                                                 const bool enable) = 0;

  // This function sets the expected delay from when a video frame is captured
  // to when that frame is delivered to VideoEngine.
  virtual int SetCaptureDelay(const int capture_id,
                              const unsigned int capture_delay_ms) = 0;

  // Returns the number of sets of capture capabilities the capture device
  // supports.
  virtual int NumberOfCapabilities(
      const char* unique_id_utf8,
      const unsigned int unique_id_utf8_length) = 0;

  // Gets a set of capture capabilities for a specified capture device.
  virtual int GetCaptureCapability(const char* unique_id_utf8,
                                   const unsigned int unique_id_utf8_length,
                                   const unsigned int capability_number,
                                   CaptureCapability& capability) = 0;

  // Displays the capture device property dialog box for the specified capture
  // device. Windows only.
  virtual int ShowCaptureSettingsDialogBox(
      const char* unique_idUTF8,
      const unsigned int unique_id_utf8_length,
      const char* dialog_title,
      void* parent_window = NULL,
      const unsigned int x = 200,
      const unsigned int y = 200) = 0;

  // Gets the clockwise angle the frames from the camera must be rotated in
  // order to display the frames correctly if the display is rotated in its
  // natural orientation.
  virtual int GetOrientation(const char* unique_id_utf8,
                             RotateCapturedFrame& orientation) = 0;

  // Enables brightness alarm detection and the brightness alarm callback.
  virtual int EnableBrightnessAlarm(const int capture_id,
                                    const bool enable) = 0;

  // Registers an instance of a user implementation of the ViECaptureObserver.
  virtual int RegisterObserver(const int capture_id,
                               ViECaptureObserver& observer) = 0;

  // Removes an already registered instance of ViECaptureObserver.
  virtual int DeregisterObserver(const int capture_id) = 0;

 protected:
  ViECapture() {}
  virtual ~ViECapture() {}
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_CAPTURE_H_
