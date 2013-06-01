/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "device_info_bb.h"

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


#include "ref_count.h"
#include "trace.h"


namespace webrtc
{
namespace videocapturemodule
{
VideoCaptureModule::DeviceInfo*
VideoCaptureImpl::CreateDeviceInfo(const WebRtc_Word32 id)
{
    videocapturemodule::DeviceInfoBB *deviceInfo =
                    new videocapturemodule::DeviceInfoBB(id);
    if (!deviceInfo)
    {
        deviceInfo = NULL;
    }

    return deviceInfo;
}

DeviceInfoBB::DeviceInfoBB(const WebRtc_Word32 id)
    : DeviceInfoImpl(id)
{
}

WebRtc_Word32 DeviceInfoBB::Init()
{
    return 0;
}

DeviceInfoBB::~DeviceInfoBB()
{
}

WebRtc_UWord32 DeviceInfoBB::NumberOfDevices()
{
    WEBRTC_TRACE(webrtc::kTraceApiCall, webrtc::kTraceVideoCapture, _id, "%s", __FUNCTION__);

    WebRtc_UWord32 count = 0;
    char device[20];

    return count;
}

WebRtc_Word32 DeviceInfoBB::GetDeviceName(
                                         WebRtc_UWord32 deviceNumber,
                                         char* deviceNameUTF8,
                                         WebRtc_UWord32 deviceNameLength,
                                         char* deviceUniqueIdUTF8,
                                         WebRtc_UWord32 deviceUniqueIdUTF8Length,
                                         char* /*productUniqueIdUTF8*/,
                                         WebRtc_UWord32 /*productUniqueIdUTF8Length*/)
{
    WEBRTC_TRACE(webrtc::kTraceApiCall, webrtc::kTraceVideoCapture, _id, "%s", __FUNCTION__);
    return 0;
}

WebRtc_Word32 DeviceInfoBB::CreateCapabilityMap(
                                        const char* deviceUniqueIdUTF8)
{
    return 0;
}

bool DeviceInfoBB::IsDeviceNameMatches(const char* name,
                                          const char* deviceUniqueIdUTF8)
{
    if (strncmp(deviceUniqueIdUTF8, name, strlen(name)) == 0)
            return true;
    return false;
}

WebRtc_Word32 DeviceInfoBB::FillCapabilityMap(int fd)
{
    return 0;
}

} // namespace videocapturemodule
} // namespace webrtc

