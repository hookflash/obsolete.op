/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "tb_capture_device.h"

tbCaptureDevice::tbCaptureDevice(tbInterfaces& Engine, int& nrOfErrors, int width, int height) :
    captureId(-1),
    numberOfErrors(nrOfErrors),
    ViE(Engine),
    vcpm_(NULL)
{
    const unsigned int KMaxDeviceNameLength = 128;
    const unsigned int KMaxUniqueIdLength = 256;
    WebRtc_UWord8 deviceName[KMaxDeviceNameLength];
    memset(deviceName, 0, KMaxDeviceNameLength);
    WebRtc_UWord8 uniqueId[KMaxUniqueIdLength];
    memset(uniqueId, 0, KMaxUniqueIdLength);

    int error;
    bool captureDeviceSet = false;

    webrtc::VideoCaptureModule::DeviceInfo* devInfo =
        webrtc::VideoCaptureModule::CreateDeviceInfo(0);
    size_t captureIdx;
    if (height == 240)
        captureIdx = 1;
    else if (height == 180)
        captureIdx = 0;
    else
      return;

    error = devInfo->GetDeviceName(captureIdx, deviceName,
                                       KMaxDeviceNameLength, uniqueId,
                                       KMaxUniqueIdLength);
    numberOfErrors += ViETest::TestError(error == 0,
                                         "ERROR: %s at line %d",
                                         __FUNCTION__, __LINE__);

    vcpm_ = webrtc::VideoCaptureModule::Create(captureIdx, uniqueId);
    numberOfErrors += ViETest::TestError(vcpm_ != NULL, "ERROR: %s at line %d - Failed to open this device.e",
                                         __FUNCTION__, __LINE__);

    error = ViE.ptrViECapture->AllocateCaptureDevice(*vcpm_, captureId);
    if (error == 0)
    {
        ViETest::Log("Using capture device: %s, captureId: %d", deviceName,
                     captureId);
        captureDeviceSet = true;
    }
    
    webrtc::VideoCaptureModule::DestroyDeviceInfo(devInfo);
    numberOfErrors += ViETest::TestError(
        captureDeviceSet, "ERROR: %s at line %d - Could not set capture device",
        __FUNCTION__, __LINE__);

    ViETest::Log("Starting capture device %s with captureId %d\n", deviceName,
                 captureId);
    
    webrtc::CaptureCapability capability;
    capability.height = height;
    capability.width = width;
    capability.maxFPS = 15;

    error = ViE.ptrViECapture->StartCapture(captureId, capability);
    numberOfErrors += ViETest::TestError(error == 0, "ERROR: %s at line %d",
                                         __FUNCTION__, __LINE__);
}

tbCaptureDevice::~tbCaptureDevice(void)
{
    ViETest::Log("Stopping capture device with id %d\n", captureId);
    int error;
    error = ViE.ptrViECapture->StopCapture(captureId);
    numberOfErrors += ViETest::TestError(error == 0, "ERROR: %s at line %d",
                                         __FUNCTION__, __LINE__);

    error = ViE.ptrViECapture->ReleaseCaptureDevice(captureId);
    numberOfErrors += ViETest::TestError(error == 0, "ERROR: %s at line %d",
                                         __FUNCTION__, __LINE__);

    webrtc::VideoCaptureModule::Destroy(vcpm_);

}

void tbCaptureDevice::ConnectTo(int videoChannel)
{
    int error;
    error = ViE.ptrViECapture->ConnectCaptureDevice(captureId, videoChannel);
    numberOfErrors += ViETest::TestError(error == 0, "ERROR: %s at line %d",
                                         __FUNCTION__, __LINE__);
}

void tbCaptureDevice::Disconnect(int videoChannel)
{
    int error = 0;
    error = ViE.ptrViECapture->DisconnectCaptureDevice(videoChannel);
    numberOfErrors += ViETest::TestError(error == 0, "ERROR: %s at line %d",
                                         __FUNCTION__, __LINE__);
}
