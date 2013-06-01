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

#import "mp4_file_objc.h"
#import <UIKit/UIKit.h>
#import <MobileCoreServices/MobileCoreServices.h>
#import <AssetsLibrary/AssetsLibrary.h>
#include "trace.h"

@interface MP4FileObjC ()

@property (readwrite, getter=isRecording) BOOL recording;

@end

@implementation MP4FileObjC

@synthesize recording;

- (id)init
{
    if (self = [super init])
    {
        movieWritingQueue = dispatch_queue_create("Movie Writing Queue", DISPATCH_QUEUE_SERIAL);
        movieWritingSemaphore = dispatch_semaphore_create(0);
        
        assetWriter = nil;
        assetWriterVideoInPixelBufferAdaptor = nil;

        readyToRecord = NO;
        recordingWillBeStopped = NO;
        lastAudioTimeStamp = CMTimeMake(0, 0);
        self.recording = NO;
      
        assetWriterVideoInPixelBufferAdaptorLock = [[NSLock alloc] init];
    }
    return self;
}

- (void)dealloc
{
    if (movieWritingQueue)
    {
        dispatch_release(movieWritingQueue);
        movieWritingQueue = NULL;
    }
    if (movieWritingSemaphore)
    {
        dispatch_release(movieWritingSemaphore);
        movieWritingSemaphore = NULL;
    }
    [assetWriterVideoInPixelBufferAdaptorLock release];

    [super dealloc];
}

- (BOOL)writeAudioSampleBuffer:(CMSampleBufferRef)sampleBuffer withTimeStamp:(CMTime)timeStamp
{
    if (assetWriter.status == AVAssetWriterStatusUnknown)
    {
        if ([assetWriter startWriting])
        {
            [assetWriter startSessionAtSourceTime:lastAudioTimeStamp];
        }
        else
        {
            char errorString[1024] = "";
            [[[assetWriter error] localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                               "Failed to start writing - %s", errorString);
            return NO;
        }
    }
	
    if (assetWriter.status == AVAssetWriterStatusWriting && assetWriterAudioIn.readyForMoreMediaData)
    {
//        printf("Write - Audio sample buffer - TS: %lld\t%d\n",
//               timeStamp.value, timeStamp.timescale);
        
        if (![assetWriterAudioIn appendSampleBuffer:sampleBuffer])
        {
            char errorString[1024] = "";
            [[[assetWriter error] localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                               "Failed to append audio sample buffer - %s", errorString);
            return NO;
        }
        
        return YES;
    }
    
    return NO;
}

- (BOOL)writeVideoPixelBuffer:(CVPixelBufferRef)pixelBuffer withTimeStamp:(CMTime)timeStamp
{
    if (assetWriter.status == AVAssetWriterStatusUnknown)
    {
        if ([assetWriter startWriting])
        {
            [assetWriter startSessionAtSourceTime:lastAudioTimeStamp];
        }
        else
        {
            char errorString[1024] = "";
            [[[assetWriter error] localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                               "Failed to start writing - %s", errorString);
            return NO;
        }
    }
	
    if (assetWriter.status == AVAssetWriterStatusWriting && assetWriterVideoIn.readyForMoreMediaData)
    {
//        printf("Write - Video sample buffer - TS: %lld\t%d\n",
//               timeStamp.value, timeStamp.timescale);
        
        [assetWriterVideoInPixelBufferAdaptorLock lock];
      
        if (![assetWriterVideoInPixelBufferAdaptor appendPixelBuffer:pixelBuffer withPresentationTime:timeStamp])
        {
            char errorString[1024] = "";
            [[[assetWriter error] localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                               "Failed to append video sample buffer - %s", errorString);
            [assetWriterVideoInPixelBufferAdaptorLock unlock];
            return NO;
        }

        [assetWriterVideoInPixelBufferAdaptorLock unlock];
      
        return YES;
    }
    return NO;
}

- (BOOL)setupAssetWriterAudioInput
{
    NSDictionary *audioCompressionSettings = [NSDictionary dictionaryWithObjectsAndKeys:
                                              [NSNumber numberWithInteger:kAudioFormatMPEG4AAC], AVFormatIDKey,
                                              [NSNumber numberWithFloat:32000], AVSampleRateKey,
                                              [NSNumber numberWithInt:audioBitrate], AVEncoderBitRateKey,
                                              [NSNumber numberWithInteger:1], AVNumberOfChannelsKey,
                                              [NSData data], AVChannelLayoutKey,
                                              nil];
    if ([assetWriter canApplyOutputSettings:audioCompressionSettings forMediaType:AVMediaTypeAudio])
    {
        assetWriterAudioIn = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio outputSettings:audioCompressionSettings];
        
        assetWriterAudioIn.expectsMediaDataInRealTime = YES;
        
        if ([assetWriter canAddInput:assetWriterAudioIn])
        {
            [assetWriter addInput:assetWriterAudioIn];
        }
        else
        {
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                               "Couldn't add asset writer audio input.");
            return NO;
        }
    }
    else
    {
        webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                           "Couldn't apply audio output settings.");
        return NO;
    }
    
    return YES;
}

- (BOOL)setupAssetWriterVideoInput
{
    NSDictionary *videoCompressionSettings = [NSDictionary dictionaryWithObjectsAndKeys:
                                              AVVideoCodecH264, AVVideoCodecKey,
                                              [NSNumber numberWithInteger:videoWidth], AVVideoWidthKey,
                                              [NSNumber numberWithInteger:videoHeight], AVVideoHeightKey,
                                              [NSDictionary dictionaryWithObjectsAndKeys:
                                               [NSNumber numberWithInteger:(videoBitrate * 1000)], AVVideoAverageBitRateKey,
                                               [NSNumber numberWithInteger:30], AVVideoMaxKeyFrameIntervalKey,
                                               nil], AVVideoCompressionPropertiesKey,
                                              nil];
    if ([assetWriter canApplyOutputSettings:videoCompressionSettings forMediaType:AVMediaTypeVideo])
    {
        assetWriterVideoIn = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:videoCompressionSettings];
        
        assetWriterVideoIn.expectsMediaDataInRealTime = YES;
        
        NSDictionary *bufferAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
                                          [NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange], kCVPixelBufferPixelFormatTypeKey,
                                          [NSNumber numberWithInt:videoWidth], kCVPixelBufferWidthKey,
                                          [NSNumber numberWithInt:videoHeight], kCVPixelBufferHeightKey,
                                          nil];

        [assetWriterVideoInPixelBufferAdaptorLock lock];
      
        assetWriterVideoInPixelBufferAdaptor =
            [[AVAssetWriterInputPixelBufferAdaptor alloc] initWithAssetWriterInput:assetWriterVideoIn sourcePixelBufferAttributes:bufferAttributes];

        [assetWriterVideoInPixelBufferAdaptorLock unlock];
      
        if ([assetWriter canAddInput:assetWriterVideoIn])
        {
            [assetWriter addInput:assetWriterVideoIn];
        }
        else
        {
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                               "Couldn't add asset writer video input.");
            return NO;
        }
    }
    else
    {
        webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                           "Couldn't apply video output settings.");
        return NO;
    }
    
    return YES;
}

- (void)removeMovieFile
{
    NSError *error;
    NSFileManager *fileManager = [NSFileManager defaultManager];
  
    BOOL fileExists = [fileManager fileExistsAtPath:[movieURL path]];
    if (fileExists)
    {
        if (![fileManager removeItemAtURL:movieURL error:&error])
        {
            char errorString[1024] = "";
            [[error localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                         "Failed to delete movie file - %s", errorString);
        }
    }
}

- (void)saveMovieToPhotoAlbum
{
    ALAssetsLibrary *library = [[ALAssetsLibrary alloc] init];
    [library writeVideoAtPathToSavedPhotosAlbum:movieURL
                                completionBlock:^(NSURL *assetURL, NSError *error) {
                                    if (error)
                                    {
                                        char errorString[1024] = "";
                                        [[error localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
                                        webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                                                         "Failed to init Asset Writer - %s", errorString);
                                    }
                                    else
                                    {
                                        [self removeMovieFile];
                                    }
                                
                                    dispatch_async(movieWritingQueue, ^{
                                        [movieURL release];
                                        readyToRecord = NO;
                                        recordingWillBeStopped = NO;
                                        lastAudioTimeStamp = CMTimeMake(0, audioSamplerate);
                                        self.recording = NO;
                                        dispatch_semaphore_signal(movieWritingSemaphore);
                                    });
                                }];
    [library release];
}

- (void)createMovie:(NSURL*)url andSaveVideoToLibrary:(BOOL)saveToLibrary
{
    if (saveToLibrary)
        movieURL = [[NSURL alloc] initFileURLWithPath:[NSString stringWithFormat:@"%@%@", NSTemporaryDirectory(), [url path]]];
    else
        movieURL = [url retain];
  
    saveVideoToLibrary = saveToLibrary;
  
    dispatch_async(movieWritingQueue, ^{
        
        if (self.recording)
            return;
      
        [self removeMovieFile];
      
        NSError *error;
      
        assetWriter = [[AVAssetWriter alloc] initWithURL:movieURL fileType:(NSString*)kUTTypeMPEG4 error:&error];
        if (error)
        {
            char errorString[1024] = "";
            [[error localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                               "Failed to init Asset Writer - %s", errorString);
        }
        
        BOOL readyToRecordAudio = [self setupAssetWriterAudioInput];
        BOOL readyToRecordVideo = [self setupAssetWriterVideoInput];
        readyToRecord = readyToRecordAudio && readyToRecordVideo;
        self.recording = YES;
    });
}

- (void) setVideoWidth:(WebRtc_UWord16)width andHeight:(WebRtc_UWord16)height andBitrate:(WebRtc_UWord32)bitrate
{
    dispatch_async(movieWritingQueue, ^{
        videoWidth = width;
        videoHeight = height;
        videoBitrate = bitrate;
    });
}

- (void) setAudioSamplerate:(WebRtc_UWord32)samplerate andBitrate:(WebRtc_UWord32)bitrate
{
    dispatch_async(movieWritingQueue, ^{
        audioSamplerate = samplerate;
        audioBitrate = bitrate;
        lastAudioTimeStamp = CMTimeMake(0, audioSamplerate);
    });
}

- (void)close
{
    dispatch_async(movieWritingQueue, ^{
		
        if (recordingWillBeStopped || (self.recording == NO))
            return;
		
        recordingWillBeStopped = YES;
        
        [assetWriterAudioIn markAsFinished];
        [assetWriterVideoIn markAsFinished];

        NSString* systemVersion = [[UIDevice currentDevice] systemVersion];
        if ([systemVersion compare:@"6.0" options:NSNumericSearch] != NSOrderedAscending)
        {
            [assetWriter finishWritingWithCompletionHandler:^(){
                
                dispatch_async(movieWritingQueue, ^{
                    
                    if (assetWriter.status != AVAssetWriterStatusFailed)
                    {
                        [assetWriterVideoInPixelBufferAdaptorLock lock];
                        [assetWriterVideoInPixelBufferAdaptor release];
                        assetWriterVideoInPixelBufferAdaptor = nil;
                        [assetWriterVideoInPixelBufferAdaptorLock unlock];
                        [assetWriterAudioIn release];
                        [assetWriterVideoIn release];
                        [assetWriter release];
                        assetWriter = nil;
                      
                        if (saveVideoToLibrary)
                        {
                            // sempahore signal is dispached from this function call
                            [self saveMovieToPhotoAlbum];
                        }
                        else
                        {
                            [movieURL release];
                            readyToRecord = NO;
                            recordingWillBeStopped = NO;
                            lastAudioTimeStamp = CMTimeMake(0, audioSamplerate);
                            self.recording = NO;
                          
                            dispatch_semaphore_signal(movieWritingSemaphore);
                        }
                    }
                    else
                    {
                        char errorString[1024] = "";
                        [[[assetWriter error] localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
                        webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                                           "Failed to finish Asset Writer writing - %s", errorString);
                      
                        dispatch_semaphore_signal(movieWritingSemaphore);
                    }
                });
            }];
        }
        else
        {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            [assetWriter finishWriting];
#pragma clang diagnostic pop
            
            if (assetWriter.status != AVAssetWriterStatusFailed)
            {
                [assetWriterVideoInPixelBufferAdaptorLock lock];
                [assetWriterVideoInPixelBufferAdaptor release];
                assetWriterVideoInPixelBufferAdaptor = nil;
                [assetWriterVideoInPixelBufferAdaptorLock unlock];
                [assetWriterAudioIn release];
                [assetWriterVideoIn release];
                [assetWriter release];
                assetWriter = nil;
              
                if (saveVideoToLibrary)
                {
                    // sempahore signal is dispached from this function call
                    [self saveMovieToPhotoAlbum];
                }
                else
                {
                    [movieURL release];
                    readyToRecord = NO;
                    recordingWillBeStopped = NO;
                    lastAudioTimeStamp = CMTimeMake(0, audioSamplerate);
                    self.recording = NO;
                
                    dispatch_semaphore_signal(movieWritingSemaphore);
                }
            }
            else
            {
                char errorString[1024] = "";
                [[[assetWriter error] localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
                webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                                   "Failed to finish Asset Writer writing - %s", errorString);
              
                dispatch_semaphore_signal(movieWritingSemaphore);
            }
        }
    });
    
    // timeout is 10 s
    if (dispatch_semaphore_wait(movieWritingSemaphore, dispatch_time(DISPATCH_TIME_NOW, 10LL * NSEC_PER_SEC)) != 0)
    {
        webrtc::Trace::Add(webrtc::kTraceWarning, webrtc::kTraceUtility, -1,
                           "Movie writing semaphore timeout occurred.");
    }
}

- (void)writeAudioData:(const WebRtc_UWord8*)data withLength:(WebRtc_Word32)length withTimeStamp:(CMTime)timeStamp
{
    OSStatus status;
    CMBlockBufferRef blockBuffer1 = NULL;
    CMBlockBufferRef blockBuffer2 = NULL;
    
    status = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, (void*)data, length,
                                                kCFAllocatorNull, NULL, 0, length,
                                                0, &blockBuffer1);
    
    if (status != noErr)
    {
        webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                           "Couldn't create Block Buffer.");
        return;
    }
    
    status = CMBlockBufferCreateContiguous(kCFAllocatorDefault, blockBuffer1, kCFAllocatorDefault, NULL, 0, length,
                                           kCMBlockBufferAlwaysCopyDataFlag, &blockBuffer2);
    
    if (status != noErr)
    {
        webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                           "Couldn't create Block Buffer copy.");
        CFRelease(blockBuffer1);
        return;
    }

    dispatch_async(movieWritingQueue, ^{
        
        if (!assetWriter || recordingWillBeStopped)
        {
            CFRelease(blockBuffer1);
            CFRelease(blockBuffer2);
            return;
        }
        
        OSStatus status;
        CMSampleBufferRef sampleBuffer = NULL;
        
        // Time stamps form WebRTC sometimes overlapping each other. So we use internal TS representation.
        CMTime internalTimeStamp = CMTimeAdd(lastAudioTimeStamp, CMTimeMake(audioSamplerate / 100, audioSamplerate));
        
        CMFormatDescriptionRef formatDescription = nil;
        AudioStreamBasicDescription asbd;
        bzero(&asbd, sizeof(asbd));
        asbd.mSampleRate = audioSamplerate;
        asbd.mFormatID   = kAudioFormatLinearPCM;
        asbd.mFramesPerPacket = 1;
        asbd.mChannelsPerFrame = 1;
        asbd.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger |
                            kLinearPCMFormatFlagIsPacked;
        asbd.mBitsPerChannel = 16;
        asbd.mBytesPerPacket = 2;
        asbd.mBytesPerFrame = 2;
        
        CMAudioFormatDescriptionCreate(kCFAllocatorDefault, &asbd, 0, NULL, 0, NULL,
                                       NULL, &formatDescription);

        status = CMAudioSampleBufferCreateWithPacketDescriptions(kCFAllocatorDefault, blockBuffer2, TRUE,
                                                                 0, NULL, formatDescription, 1, internalTimeStamp,
                                                                 NULL, &sampleBuffer);
        
        if (status != noErr)
        {
            webrtc::Trace::Add(webrtc::kTraceError, webrtc::kTraceUtility, -1,
                               "Couldn't create audio sample buffer.");
            CFRelease(formatDescription);
            CFRelease(blockBuffer1);
            CFRelease(blockBuffer2);
            return;
        }

        if (readyToRecord)
        {
            if ([self writeAudioSampleBuffer:sampleBuffer withTimeStamp:internalTimeStamp])
                lastAudioTimeStamp = internalTimeStamp;
        }
      
        CFRelease(formatDescription);
        CFRelease(blockBuffer1);
        CFRelease(blockBuffer2);
        CFRelease(sampleBuffer);
    });
}

- (void)writeVideoData:(const WebRtc_UWord8*)data withLength:(WebRtc_Word32)length withTimeStamp:(CMTime)timeStamp
{
    
    CVPixelBufferRef pixelBuffer = NULL;
    CVReturn status;
    
    [assetWriterVideoInPixelBufferAdaptorLock lock];
  
    if (assetWriterVideoInPixelBufferAdaptor)
    {
        status = CVPixelBufferPoolCreatePixelBuffer(NULL, assetWriterVideoInPixelBufferAdaptor.pixelBufferPool, &pixelBuffer);
    
        if (status != kCVReturnSuccess)
        {
            char errorString[1024] = "";
            [[[assetWriter error] localizedDescription] getCString:errorString maxLength:1024 encoding:NSUTF8StringEncoding];
            webrtc::Trace::Add(webrtc::kTraceWarning, webrtc::kTraceUtility, -1,
                               "Couldn't create Pixel Buffer from pool - %s", errorString);
            [assetWriterVideoInPixelBufferAdaptorLock unlock];
            return;
        }
    }
    else
    {
        [assetWriterVideoInPixelBufferAdaptorLock unlock];
        return;
    }

    [assetWriterVideoInPixelBufferAdaptorLock unlock];
  
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
  
    void* baseAddressY = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
    void* baseAddressUV = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1);
    int bytesPerRowY = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
    int bytesPerRowUV = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1);
  
    WebRtc_UWord8* destinationDataY = (WebRtc_UWord8*)baseAddressY;
    WebRtc_UWord8* sourceDataY = (WebRtc_UWord8*)data;
    for (int i = 0; i < videoHeight; i++)
    {
        memcpy(destinationDataY, sourceDataY, videoWidth);
        destinationDataY += bytesPerRowY;
        sourceDataY += videoWidth;
    }
  
    // i420 -> NV12 conversion
    WebRtc_UWord8* destinationDataUV = (WebRtc_UWord8*)baseAddressUV;
    WebRtc_UWord8* sourceDataU = (WebRtc_UWord8*)(data + videoWidth * videoHeight);
    WebRtc_UWord8* sourceDataV = (WebRtc_UWord8*)(data + videoWidth * videoHeight + (videoWidth >> 1) * (videoHeight >> 1));
    for (int j = 0; j < (videoHeight >> 1); j++)
    {
        for (int i = 0; i < (videoWidth >> 1); i++)
        {
            destinationDataUV[2*i] = sourceDataU[i];
            destinationDataUV[2*i+1] = sourceDataV[i];
        }
        destinationDataUV += bytesPerRowUV;
        sourceDataU += videoWidth >> 1;
        sourceDataV += videoWidth >> 1;
    }
  
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

    dispatch_async(movieWritingQueue, ^{
        
        if (!assetWriter || recordingWillBeStopped)
        {
            CVPixelBufferRelease(pixelBuffer);
            return;
        }
        
        if (readyToRecord)
            [self writeVideoPixelBuffer:pixelBuffer withTimeStamp:lastAudioTimeStamp];

        CVPixelBufferRelease(pixelBuffer);
    });
}

@end
