/*

 Copyright (c) 2013, SMB Phone Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.

 */

#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#include <iostream>
#include <new>

#include "video_capture_bb.h"

#include "ref_count.h"
#include "trace.h"
#include "thread_wrapper.h"
#include "critical_section_wrapper.h"

namespace webrtc
{
namespace videocapturemodule
{

CriticalSectionWrapper* capture_crit_sect;
VideoCaptureModuleBB* video_capture_module;

void viewfinder_callback(camera_handle_t camera_handle, camera_buffer_t* camera_buffer, void* arg)
{
    capture_crit_sect->Enter();

    camera_frametype_t frame_type = camera_buffer->frametype;
    uint64_t frame_size = camera_buffer->framesize;
    uint8_t* frame_buffer = camera_buffer->framebuf;
    uint64_t frame_meta_size = camera_buffer->framemetasize;
    void* frame_meta = camera_buffer->framemeta;
    int64_t frame_timestamp = camera_buffer->frametimestamp;
    camera_frame_nv12_t frame_desc = camera_buffer->framedesc.nv12;

    uint32_t height = frame_desc.height;
    uint32_t width = frame_desc.width;
    uint32_t stride = frame_desc.stride;
    int64_t uv_offset = frame_desc.uv_offset;
    int64_t uv_stride = frame_desc.uv_stride;

    VideoCaptureCapability frame_info;
    frame_info.width = width;
    frame_info.height = height;
    frame_info.rawType = kVideoI420;

    WebRtc_Word32 output_buffer_size = width * height * 3 / 2;
    WebRtc_UWord8* output_buffer = (WebRtc_UWord8*)malloc(output_buffer_size);

    WebRtc_UWord8* source_data_y = frame_buffer;
    WebRtc_UWord8* source_data_uv = frame_buffer + uv_offset;
    WebRtc_UWord8* destination_data_y = output_buffer;
    WebRtc_UWord8* destination_data_u = output_buffer + width * height;
    WebRtc_UWord8* destination_data_v = output_buffer + width * height * 5 / 4;

    for (int i = 0; i < (int)height; i++)
    {
        memcpy(destination_data_y, source_data_y, stride);
        source_data_y += stride;
        destination_data_y += width;
    }

    // NV12 -> i420 conversion
    for (int j = 0; j < (height >> 1); j++)
    {
        for (int i = 0; i < (width >> 1); i++)
        {
        	destination_data_u[i] = source_data_uv[2*i];
        	destination_data_v[i] = source_data_uv[2*i+1];
        }
        source_data_uv += uv_stride;
        destination_data_u += width >> 1;
        destination_data_v += width >> 1;
    }

	video_capture_module->IncomingFrameBB((unsigned char*)output_buffer, output_buffer_size, frame_info);

	free(output_buffer);

	capture_crit_sect->Leave();
}

VideoCaptureModule* VideoCaptureImpl::Create(const WebRtc_Word32 id,
                                             const char* deviceUniqueId)
{
    RefCountImpl<videocapturemodule::VideoCaptureModuleBB>* implementation =
        new RefCountImpl<videocapturemodule::VideoCaptureModuleBB>(id);

    if (!implementation || implementation->Init(deviceUniqueId) != 0)
    {
        delete implementation;
        implementation = NULL;
    }

    return implementation;
}

VideoCaptureModuleBB::VideoCaptureModuleBB(const WebRtc_Word32 id)
    : VideoCaptureImpl(id), 
      _cameraHandle(CAMERA_HANDLE_INVALID),
      _currentWidth(-1), 
      _currentHeight(-1),
      _currentFrameRate(-1), 
      _captureStarted(false),
      _captureVideoType(kVideoI420)
{
	video_capture_module = this;
    capture_crit_sect = CriticalSectionWrapper::CreateCriticalSection();
}

WebRtc_Word32 VideoCaptureModuleBB::Init(const char* deviceUniqueIdUTF8)
{
    int len = strlen((const char*) deviceUniqueIdUTF8);
    _deviceUniqueId = new (std::nothrow) char[len + 1];
    if (_deviceUniqueId)
    {
        memcpy(_deviceUniqueId, deviceUniqueIdUTF8, len + 1);
    }

    camera_error_t error;
    camera_handle_t cameraHandle;

    error = camera_open(CAMERA_UNIT_FRONT,
            CAMERA_MODE_RW,
            &cameraHandle);
    if (error != CAMERA_EOK)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id, "cannot open camera - error: %d", error);
        return -1;
    }

    _cameraHandle = cameraHandle;

//    unsigned int numasked = 0;
//    unsigned int numsupported;
//    camera_res_t* resolutions = NULL;
//
//    error = camera_get_video_output_resolutions(cameraHandle, numasked, &numsupported, resolutions);
//    if (error != CAMERA_EOK)
//    {
//        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id, "cannot get camera resolutions - error: %d", error);
//    	return -1;
//    }
//
//    numasked = numsupported;
//    resolutions = (camera_res_t*)malloc(numasked * sizeof(camera_res_t));
//
//    error = camera_get_video_output_resolutions(cameraHandle, numasked, &numsupported, resolutions);
//    if (error != CAMERA_EOK)
//    {
//        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id, "cannot get camera resolutions - error: %d", error);
//    	return -1;
//    }
//
//    free(resolutions);

/*
    error = camera_set_video_property(cameraHandle,
        CAMERA_IMGPROP_WIDTH, 240,
        CAMERA_IMGPROP_HEIGHT, 320,
        CAMERA_IMGPROP_ROTATION, 90,
        CAMERA_IMGPROP_FRAMERATE, (double)15.0);
        */
    error = camera_set_video_property(cameraHandle,
        CAMERA_IMGPROP_WIDTH, 480,
        CAMERA_IMGPROP_HEIGHT, 640,
        CAMERA_IMGPROP_ROTATION, 90,
        CAMERA_IMGPROP_FRAMERATE, (double)15.0);
//    error = camera_set_video_property(cameraHandle,
//    		CAMERA_IMGPROP_WIDTH, 240,
//    		CAMERA_IMGPROP_HEIGHT, 320);
//    error = camera_set_video_property(cameraHandle,
//    		CAMERA_IMGPROP_ROTATION, 90);
//    error = camera_set_video_property(cameraHandle,
//    		CAMERA_IMGPROP_FRAMERATE, (double)15.0);
//    error = camera_set_video_property(cameraHandle,
//    		CAMERA_IMGPROP_VIDEOCODEC, CAMERA_VIDEOCODEC_NONE);
//    error = camera_set_video_property(cameraHandle,
//    		CAMERA_IMGPROP_AUDIOCODEC, CAMERA_AUDIOCODEC_NONE);
    if (error != CAMERA_EOK)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id, "cannot set video properties - error: %d", error);
        return -1;
    }

    return 0;
}

VideoCaptureModuleBB::~VideoCaptureModuleBB()
{
    StopCapture();
    if (capture_crit_sect)
    {
        delete capture_crit_sect;
    }
    if (_cameraHandle != CAMERA_HANDLE_INVALID)
    {
        camera_error_t error;
        error = camera_close(_cameraHandle);
        if (error != CAMERA_EOK)
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id, "cannot close camera - error: %d", error);
        _cameraHandle = CAMERA_HANDLE_INVALID;
    }
}

WebRtc_Word32 VideoCaptureModuleBB::StartCapture(
                                        const VideoCaptureCapability& capability)
{
    camera_error_t error;

    error = camera_start_video_viewfinder(_cameraHandle,
    		viewfinder_callback,
            NULL,
            NULL);
    if (error != CAMERA_EOK) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id, "cannot start video viewfinder - error: %d", error);
    }

    _captureStarted = true;

    return 0;
}

WebRtc_Word32 VideoCaptureModuleBB::StopCapture()
{
    camera_error_t error;

    error = camera_stop_video_viewfinder(_cameraHandle);
    if (error != CAMERA_EOK) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id, "cannot stop video viewfinder - error: %d", error);
    }

    return 0;
}

bool VideoCaptureModuleBB::CaptureStarted()
{
    return _captureStarted;
}

WebRtc_Word32 VideoCaptureModuleBB::CaptureSettings(VideoCaptureCapability& settings)
{
    settings.width = _currentWidth;
    settings.height = _currentHeight;
    settings.maxFPS = _currentFrameRate;
    settings.rawType = _captureVideoType;

    return 0;
}

WebRtc_Word32 VideoCaptureModuleBB::IncomingFrameBB(WebRtc_UWord8* videoFrame,
		WebRtc_Word32 videoFrameLength,
        const VideoCaptureCapability& frameInfo,
        WebRtc_Word64 captureTime,
        bool faceDetected)
{
	return IncomingFrame(videoFrame, videoFrameLength, frameInfo, captureTime, faceDetected);
}

} // namespace videocapturemodule
} // namespace webrtc
