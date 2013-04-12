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

#include "mp4_file.h"

#include <assert.h>
#include <string.h>

#include "critical_section_wrapper.h"
#include "file_wrapper.h"
#include "list_wrapper.h"
#include "trace.h"

namespace webrtc {

MP4File::MP4File()
    : _crit(CriticalSectionWrapper::CreateCriticalSection()),
    _mp4File([[MP4FileObjC alloc] init]),
    _movieURL(NULL),
    _videoFrames(0),
    _audioFrames(0),
    _bytesWritten(0),
    _writeAudioStream(false),
    _writeVideoStream(false),
    _created(false)
{
    
}

MP4File::~MP4File()
{
    Close();

    delete _crit;
    
    [_mp4File release];
}

WebRtc_Word32 MP4File::Close()
{
    _crit->Enter();
    switch (_mp4Mode)
    {
        case Write:
            CloseWrite();
            break;
        default:
            break;
    }
    
    if (_movieURL)
        [_movieURL release];
    ResetMembers();
    _crit->Leave();
    return 0;
}

WebRtc_Word32 MP4File::WriteAudio(const WebRtc_UWord8* data,
                                  WebRtc_Word32 length,
                                  WebRtc_UWord32 timeStamp)
{
    _crit->Enter();
    size_t newBytesWritten = _bytesWritten;
    
    if (_mp4Mode != Write)
    {
        _crit->Leave();
        return -1;
    }
    if (!_created)
    {
        _crit->Leave();
        return -1;
    }
    if (!_writeAudioStream)
    {
        _crit->Leave();
        return -1;
    }
    
//    printf("WriteAudio - TS: %d\n", timeStamp);
    CMTime time = CMTimeMake(timeStamp * 1000000, 1000000000);
    
    [_mp4File writeAudioData:data withLength:length withTimeStamp:time];
    
    _bytesWritten += length;
    
    ++_audioFrames;
    newBytesWritten = _bytesWritten - newBytesWritten;
    _crit->Leave();
    return static_cast<WebRtc_Word32>(newBytesWritten);
}

WebRtc_Word32 MP4File::WriteVideo(const WebRtc_UWord8* data,
                                  WebRtc_Word32 length,
                                  WebRtc_UWord32 timeStamp)
{
    _crit->Enter();
    size_t newBytesWritten = _bytesWritten;
    if (_mp4Mode != Write)
    {
        _crit->Leave();
        return -1;
    }
    if (!_created)
    {
        _crit->Leave();
        return -1;
    }
    if (!_writeVideoStream)
    {
        _crit->Leave();
        return -1;
    }
    
    CMTime time = CMTimeMake(timeStamp * 1000000, 1000000000);

    [_mp4File writeVideoData:data withLength:length withTimeStamp:time];

    _bytesWritten += length;
    
    ++_videoFrames;
    newBytesWritten = _bytesWritten - newBytesWritten;
    _crit->Leave();
    return static_cast<WebRtc_Word32>(newBytesWritten);
}

WebRtc_Word32 MP4File::Create(const char* fileName, bool saveVideoToLibrary)
{
    _crit->Enter();
    if (_mp4Mode != Write)
    {
        _crit->Leave();
        return -1;
    }
    
    if (!_writeVideoStream && !_writeAudioStream)
    {
        _crit->Leave();
        return -1;
    }
    if (_created)
    {
        _crit->Leave();
        return -1;
    }
    
    NSURL* movieURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:fileName]];
    [movieURL retain];
    
    [_mp4File createMovie:movieURL andSaveVideoToLibrary:saveVideoToLibrary];
        
    _created = true;
    
    _crit->Leave();
    return 0;
}

WebRtc_Word32 MP4File::CreateVideoStream(WebRtc_UWord16 width, WebRtc_UWord16 height, WebRtc_UWord32 bitrate)
{
    _crit->Enter();
    
    if (_created)
    {
        _crit->Leave();
        return -1;
    }
    
    [_mp4File setVideoWidth:width andHeight:height andBitrate:bitrate];
    
    _mp4Mode = Write;
    _writeVideoStream = true;

    _crit->Leave();
    return 0;
}

WebRtc_Word32 MP4File::CreateAudioStream(WebRtc_Word32 samplerate, WebRtc_UWord16 bitrate)
{
    _crit->Enter();
    
    if (_created)
    {
        _crit->Leave();
        return -1;
    }
    
    [_mp4File setAudioSamplerate:samplerate andBitrate:bitrate];
    
    _mp4Mode = Write;
    _writeAudioStream = true;

    _crit->Leave();
    return 0;
}

void MP4File::CloseWrite()
{
    if (_created)
    {
        [_mp4File close];
    }
}

void MP4File::ResetMembers()
{
    _videoFrames   = 0;
    _audioFrames   = 0;
    
    _bytesWritten  = 0;
    
    _writeAudioStream = false;
    _writeVideoStream = false;
    
    _mp4Mode       = NotSet;
    
    _created       = false;
}
} // namespace webrtc
