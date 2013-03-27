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

#ifndef WEBRTC_MODULES_MEDIA_FILE_SOURCE_MP4_FILE_OBJC_H_
#define WEBRTC_MODULES_MEDIA_FILE_SOURCE_MP4_FILE_OBJC_H_

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CMBufferQueue.h>
#include "typedefs.h"

@interface MP4FileObjC : NSObject
{
    AVAssetWriter* assetWriter;
    AVAssetWriterInput* assetWriterAudioIn;
    AVAssetWriterInput* assetWriterVideoIn;
    AVAssetWriterInputPixelBufferAdaptor* assetWriterVideoInPixelBufferAdaptor;
    dispatch_queue_t movieWritingQueue;
    dispatch_semaphore_t movieWritingSemaphore;
    
    BOOL readyToRecord;
    BOOL recordingWillBeStopped;
    
    CMTime lastAudioTimeStamp;
  
    NSLock* assetWriterVideoInPixelBufferAdaptorLock;
  
    NSURL* movieURL;
    BOOL saveVideoToLibrary;
    
    BOOL recording;
    
    WebRtc_UWord16 videoWidth;
    WebRtc_UWord16 videoHeight;
    WebRtc_UWord32 videoBitrate;
    WebRtc_UWord32 audioSamplerate;
    WebRtc_UWord32 audioBitrate;
}

- (void) createMovie:(NSURL*)url andSaveVideoToLibrary:(BOOL)saveToLibrary;
- (void) setVideoWidth:(WebRtc_UWord16)width andHeight:(WebRtc_UWord16)height andBitrate:(WebRtc_UWord32)bitrate;
- (void) setAudioSamplerate:(WebRtc_UWord32)samplerate andBitrate:(WebRtc_UWord32)bitrate;
- (void) close;
- (void) writeAudioData:(const WebRtc_UWord8*)data withLength:(WebRtc_Word32)length withTimeStamp:(CMTime)timeStamp;
- (void) writeVideoData:(const WebRtc_UWord8*)data withLength:(WebRtc_Word32)length withTimeStamp:(CMTime)timeStamp;

@end

#endif // WEBRTC_MODULES_MEDIA_FILE_SOURCE_MP4_FILE_OBJC_H_
