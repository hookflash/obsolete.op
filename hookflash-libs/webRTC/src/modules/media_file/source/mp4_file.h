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

#ifndef WEBRTC_MODULES_MEDIA_FILE_SOURCE_MP4_FILE_H_
#define WEBRTC_MODULES_MEDIA_FILE_SOURCE_MP4_FILE_H_

#include <stdio.h>

#include "typedefs.h"
#include "mp4_file_objc.h"

namespace webrtc {
class CriticalSectionWrapper;
class ListWrapper;

class MP4File
{
public:
    
    MP4File();
    ~MP4File();
    
    WebRtc_Word32 CreateVideoStream(WebRtc_UWord16 width, WebRtc_UWord16 height, WebRtc_UWord32 bitrate);
    WebRtc_Word32 CreateAudioStream(WebRtc_Word32 samplerate, WebRtc_UWord16 bitrate);
    WebRtc_Word32 Create(const char* fileName, bool saveVideoToLibrary);
    
    WebRtc_Word32 WriteAudio(const WebRtc_UWord8* data, WebRtc_Word32 length,
                             WebRtc_UWord32 timeStamp);
    WebRtc_Word32 WriteVideo(const WebRtc_UWord8* data, WebRtc_Word32 length,
                             WebRtc_UWord32 timeStamp);
    
    WebRtc_Word32 Close();
    
private:
    enum MP4FileMode
    {
        NotSet,
        Write
    };

    void CloseWrite();
    
    void ResetMembers();
    
private:
    
    CriticalSectionWrapper* _crit;
    
    MP4FileObjC* _mp4File;
    NSURL* _movieURL;

    WebRtc_Word32 _videoFrames;
    WebRtc_Word32 _audioFrames;
    
    size_t _bytesWritten;
    
    bool _writeAudioStream;
    bool _writeVideoStream;
    
    MP4FileMode _mp4Mode;
    bool _created;
};
} // namespace webrtc

#endif // WEBRTC_MODULES_MEDIA_FILE_SOURCE_MP4_FILE_H_
