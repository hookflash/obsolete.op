/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 *  video_capture_iphone.cc
 *
 */

// self header
#include "video_capture_iphone.h"

// super class stuff
#include "../video_capture_impl.h"
#include "../device_info_impl.h"
#include "../video_capture_config.h"

#include "trace.h"

#include "AVFoundation/video_capture_avfoundation.h"
#include "AVFoundation/video_capture_avfoundation_info.h"

namespace webrtc
{
namespace videocapturemodule
{

// static
bool VideoCaptureIPhone::CheckOSVersion()
{
    return true;
}
}  // videocapturemodule

/**************************************************************************
 *
 *    Create/Destroy a VideoCaptureModule
 *
 ***************************************************************************/

/*
 *   Returns version of the module and its components
 *
 *   version                 - buffer to which the version will be written
 *   remainingBufferInBytes  - remaining number of WebRtc_Word8 in the version
 *                             buffer
 *   position                - position of the next empty WebRtc_Word8 in the
 *                             version buffer
 */

VideoCaptureModule* VideoCaptureModule::Create(
    const WebRtc_Word32 id, const WebRtc_UWord8* deviceUniqueIdUTF8)
{
    WEBRTC_TRACE(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, id,
                 "Create %s", deviceUniqueIdUTF8);

    if (webrtc::videocapturemodule::VideoCaptureIPhone::CheckOSVersion == false)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, id,
                     "OS version is too old. Could not create video capture "
                     "module. Returning NULL");
        return NULL;
    }

    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, id,
                 "Using AVFoundation framework to capture video", id);

    webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundation* newCaptureModule =
        new webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundation(id);
    if(!newCaptureModule)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, id,
                     "could not Create for unique device %s, !newCaptureModule",
                     deviceUniqueIdUTF8);
        Destroy(newCaptureModule);
        newCaptureModule = NULL;
    }
    if(newCaptureModule->Init(id, deviceUniqueIdUTF8) != 0)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, id,
                     "could not Create for unique device %s, "
                     "newCaptureModule->Init()!=0", deviceUniqueIdUTF8);
        Destroy(newCaptureModule);
        newCaptureModule = NULL;
    }

    // Successfully created VideoCaptureIPhoneAVFoundation. Return it
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, id,
                 "Module created for unique device %s, will use AVFoundation "
                 "framework",deviceUniqueIdUTF8);
    return newCaptureModule;
}

void Destroy(VideoCaptureModule* module)
{
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, 0,
                 "%s:%d Destroying GISPModuleVideoCapture", __FUNCTION__,
                 __LINE__);

    webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundation* captureDevice =
        static_cast<webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundation*> (module);
    delete captureDevice;
    captureDevice = NULL;
}
/**************************************************************************
 *
 *    End Create/Destroy VideoCaptureModule
 *
 ***************************************************************************/

//  VideoCaptureMac class
namespace videocapturemodule
{

/**************************************************************************
 *
 *    These will just delegate to the appropriate class
 *
 ***************************************************************************/

VideoCaptureIPhone::VideoCaptureIPhone(const WebRtc_Word32 id) :
    VideoCaptureImpl(id), // super class constructor
        _isFrameworkSupported(false), _captureClass(NULL)
{

    _isFrameworkSupported = true;
    _captureClass = new VideoCaptureIPhoneAVFoundation(_id);
}

VideoCaptureIPhone::~VideoCaptureIPhone()
{
    delete _captureClass;
}

WebRtc_Word32 VideoCaptureIPhone::Init(const WebRtc_Word32 id,
                                    const WebRtc_UWord8* deviceUniqueIdUTF8)
{
    return static_cast<VideoCaptureIPhoneAVFoundation*>(_captureClass)->Init(id, deviceUniqueIdUTF8);
}

WebRtc_Word32 VideoCaptureIPhone::StartCapture(
    const VideoCaptureCapability& capability)
{
    return static_cast<VideoCaptureIPhoneAVFoundation*>
        (_captureClass)->StartCapture(capability);
}

WebRtc_Word32 VideoCaptureIPhone::StopCapture()
{
    return static_cast<VideoCaptureIPhoneAVFoundation*>(_captureClass)->StopCapture();
}

bool VideoCaptureIPhone::CaptureStarted()
{
    return static_cast<VideoCaptureIPhoneAVFoundation*>(_captureClass)->CaptureStarted();
}

WebRtc_Word32 VideoCaptureIPhone::CaptureSettings(VideoCaptureCapability& settings)
{
    return static_cast<VideoCaptureIPhoneAVFoundation*>
        (_captureClass)->CaptureSettings(settings);
}
} // namespace videocapturemodule


/**************************************************************************
 *
 *    Create/Destroy a DeviceInfo
 *
 ***************************************************************************/

VideoCaptureModule::DeviceInfo*
VideoCaptureModule::CreateDeviceInfo(const WebRtc_Word32 id)
{

    WEBRTC_TRACE(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, id,
                 "Create %d", id);

    if (webrtc::videocapturemodule::VideoCaptureIPhone::CheckOSVersion == false)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, id,
                     "OS version is too old. Could not create video capture "
                     "module. Returning NULL");
        return NULL;
    }

    webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundationInfo* newCaptureInfoModule =
        new webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundationInfo(id);

    if(!newCaptureInfoModule || newCaptureInfoModule->Init() != 0)
    {
        //Destroy(newCaptureInfoModule);
        delete newCaptureInfoModule;
        newCaptureInfoModule = NULL;
        WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, id,
                     "Failed to Init newCaptureInfoModule created with id %d "
                     "and device \"\" ", id);
        return NULL;
    }
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, id,
                 "VideoCaptureModule created for id", id);
    return newCaptureInfoModule;
}

void VideoCaptureModule::DestroyDeviceInfo(DeviceInfo* deviceInfo)
{
    WEBRTC_TRACE(webrtc::kTraceModuleCall, webrtc::kTraceVideoCapture, 0,
                 "%s:%d", __FUNCTION__, __LINE__);

    webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundationInfo* captureDeviceInfo =
        static_cast<webrtc::videocapturemodule::VideoCaptureIPhoneAVFoundationInfo*> (deviceInfo);
    delete captureDeviceInfo;
    captureDeviceInfo = NULL;
}

/**************************************************************************
 *
 *    End Create/Destroy VideoCaptureModule
 *
 ***************************************************************************/

// VideoCaptureMacInfo class

namespace videocapturemodule {

/**************************************************************************
 *
 *    These will just delegate to the appropriate class
 *
 ***************************************************************************/

VideoCaptureIPhoneInfo::VideoCaptureIPhoneInfo(const WebRtc_Word32 id) :
    DeviceInfoImpl(id), _isFrameworkSupported(false)//,
//_captureInfoClass(        NULL) // special init below
{
    _isFrameworkSupported = true;
    _captureInfoClass = new VideoCaptureIPhoneAVFoundationInfo(_id);
}

VideoCaptureIPhoneInfo::~VideoCaptureIPhoneInfo()
{
    delete _captureInfoClass;
}

WebRtc_Word32 VideoCaptureIPhoneInfo::Init()
{
    return static_cast<VideoCaptureIPhoneAVFoundationInfo*>(_captureInfoClass)->Init();
}

WebRtc_UWord32 VideoCaptureIPhoneInfo::NumberOfDevices()
{
    return static_cast<VideoCaptureIPhoneAVFoundationInfo*>
        (_captureInfoClass)->NumberOfDevices();
}

WebRtc_Word32 VideoCaptureIPhoneInfo::GetDeviceName(
    WebRtc_UWord32 deviceNumber, WebRtc_UWord8* deviceNameUTF8,
    WebRtc_UWord32 deviceNameLength, WebRtc_UWord8* deviceUniqueIdUTF8,
    WebRtc_UWord32 deviceUniqueIdUTF8Length, WebRtc_UWord8* productUniqueIdUTF8,
    WebRtc_UWord32 productUniqueIdUTF8Length)
{
    return static_cast<VideoCaptureIPhoneAVFoundationInfo*>
        (_captureInfoClass)->GetDeviceName(deviceNumber, deviceNameUTF8,
                                           deviceNameLength, deviceUniqueIdUTF8,
                                           deviceUniqueIdUTF8Length,
                                           productUniqueIdUTF8,
                                           productUniqueIdUTF8Length);
}

WebRtc_Word32 VideoCaptureIPhoneInfo::NumberOfCapabilities(
    const WebRtc_UWord8* deviceUniqueIdUTF8)
{
    return static_cast<VideoCaptureIPhoneAVFoundationInfo*>
        (_captureInfoClass)->NumberOfCapabilities(deviceUniqueIdUTF8);
}

WebRtc_Word32 VideoCaptureIPhoneInfo::GetCapability(
    const WebRtc_UWord8* deviceUniqueIdUTF8,
    const WebRtc_UWord32 deviceCapabilityNumber,
    VideoCaptureCapability& capability)
{
    return static_cast<VideoCaptureIPhoneAVFoundationInfo*>
        (_captureInfoClass)->GetCapability(deviceUniqueIdUTF8,
                                           deviceCapabilityNumber, capability);
}

WebRtc_Word32 VideoCaptureIPhoneInfo::GetBestMatchedCapability(
    const WebRtc_UWord8*deviceUniqueIdUTF8,
    const VideoCaptureCapability requested, VideoCaptureCapability& resulting)
{
    return static_cast<VideoCaptureIPhoneAVFoundationInfo*>
        (_captureInfoClass)->GetBestMatchedCapability(deviceUniqueIdUTF8,
                                                      requested, resulting);
}

WebRtc_Word32 VideoCaptureIPhoneInfo::DisplayCaptureSettingsDialogBox(
    const WebRtc_UWord8* deviceUniqueIdUTF8,
    const WebRtc_UWord8* dialogTitleUTF8, void* parentWindow,
    WebRtc_UWord32 positionX, WebRtc_UWord32 positionY)
{
    return static_cast<VideoCaptureIPhoneAVFoundationInfo*>
        (_captureInfoClass)->DisplayCaptureSettingsDialogBox(deviceUniqueIdUTF8,
                                                             dialogTitleUTF8,
                                                             parentWindow,
                                                             positionX,
                                                             positionY);
}

WebRtc_Word32 VideoCaptureIPhoneInfo::CreateCapabilityMap(
    const WebRtc_UWord8* deviceUniqueIdUTF8)
{
    // not supported. The call stack should never make it this deep.
    // This call should be returned higher in the order
    return -1;

}
}  // namespace webrtc
}  // namespace videocapturemodule
