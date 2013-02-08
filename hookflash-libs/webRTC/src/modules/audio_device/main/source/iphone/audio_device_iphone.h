/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_IPHONE_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_IPHONE_H

#include "typedefs_iphone.h"
#include "audio_device_generic.h"
#include "critical_section_wrapper.h"
#include "audio_mixer_manager_iphone.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioConverter.h>
#include <AudioToolbox/AudioServices.h>
#include <mach/semaphore.h>

struct PaUtilRingBuffer;

@class AudioDeviceInfoIPhoneObjC;

namespace webrtc
{
class EventWrapper;
class ThreadWrapper;

const WebRtc_UWord32 N_REC_SAMPLES_PER_SEC = 44000;
const WebRtc_UWord32 N_PLAY_SAMPLES_PER_SEC = 44000;

const WebRtc_UWord32 N_REC_CHANNELS = 1; // default is mono recording
const WebRtc_UWord32 N_PLAY_CHANNELS = 1; // default is stereo playout
const WebRtc_UWord32 N_DEVICE_CHANNELS = 8;

enum
{
    N_BLOCKS_IO = 2
};
enum
{
    N_BUFFERS_IN = 10
};
enum
{
    N_BUFFERS_OUT = 3
}; // Must be at least N_BLOCKS_IO

const WebRtc_UWord32 TIMER_PERIOD_MS = (2 * 10 * N_BLOCKS_IO * 1000000);

class AudioDeviceIPhone: public AudioDeviceGeneric
{
public:
    AudioDeviceIPhone(const WebRtc_Word32 id);
    ~AudioDeviceIPhone();

    // Retrieve the currently utilized audio layer
    virtual WebRtc_Word32
        ActiveAudioLayer(AudioDeviceModule::AudioLayer& audioLayer) const;

    // Main initializaton and termination
    virtual WebRtc_Word32 Init();
    virtual WebRtc_Word32 Terminate();
    virtual bool Initialized() const;

    // Device enumeration
    virtual WebRtc_Word16 PlayoutDevices();
    virtual WebRtc_Word16 RecordingDevices();
    virtual WebRtc_Word32 PlayoutDeviceName(
        WebRtc_UWord16 index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]);
    virtual WebRtc_Word32 RecordingDeviceName(
        WebRtc_UWord16 index,
        char name[kAdmMaxDeviceNameSize],
        char guid[kAdmMaxGuidSize]);

    // Device selection
    virtual WebRtc_Word32 SetPlayoutDevice(WebRtc_UWord16 index);
    virtual WebRtc_Word32 SetPlayoutDevice(
        AudioDeviceModule::WindowsDeviceType device);
    virtual WebRtc_Word32 SetRecordingDevice(WebRtc_UWord16 index);
    virtual WebRtc_Word32 SetRecordingDevice(
        AudioDeviceModule::WindowsDeviceType device);

    // Audio transport initialization
    virtual WebRtc_Word32 PlayoutIsAvailable(bool& available);
    virtual WebRtc_Word32 InitPlayout();
    virtual bool PlayoutIsInitialized() const;
    virtual WebRtc_Word32 RecordingIsAvailable(bool& available);
    virtual WebRtc_Word32 InitRecording();
    virtual bool RecordingIsInitialized() const;

    // Audio transport control
    virtual WebRtc_Word32 StartPlayout();
    virtual WebRtc_Word32 StopPlayout();
    virtual bool Playing() const;
    virtual WebRtc_Word32 StartRecording();
    virtual WebRtc_Word32 StopRecording();
    virtual bool Recording() const;

    // Microphone Automatic Gain Control (AGC)
    virtual WebRtc_Word32 SetAGC(bool enable);
    virtual bool AGC() const;

    // Volume control based on the Windows Wave API (Windows only)
    virtual WebRtc_Word32 SetWaveOutVolume(WebRtc_UWord16 volumeLeft,
                                           WebRtc_UWord16 volumeRight);
    virtual WebRtc_Word32 WaveOutVolume(WebRtc_UWord16& volumeLeft,
                                        WebRtc_UWord16& volumeRight) const;

    // Audio mixer initialization
    virtual WebRtc_Word32 SpeakerIsAvailable(bool& available);
    virtual WebRtc_Word32 InitSpeaker();
    virtual bool SpeakerIsInitialized() const;
    virtual WebRtc_Word32 MicrophoneIsAvailable(bool& available);
    virtual WebRtc_Word32 InitMicrophone();
    virtual bool MicrophoneIsInitialized() const;

    // Speaker volume controls
    virtual WebRtc_Word32 SpeakerVolumeIsAvailable(bool& available);
    virtual WebRtc_Word32 SetSpeakerVolume(WebRtc_UWord32 volume);
    virtual WebRtc_Word32 SpeakerVolume(WebRtc_UWord32& volume) const;
    virtual WebRtc_Word32 MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const;
    virtual WebRtc_Word32 MinSpeakerVolume(WebRtc_UWord32& minVolume) const;
    virtual WebRtc_Word32 SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const;

    // Microphone volume controls
    virtual WebRtc_Word32 MicrophoneVolumeIsAvailable(bool& available);
    virtual WebRtc_Word32 SetMicrophoneVolume(WebRtc_UWord32 volume);
    virtual WebRtc_Word32 MicrophoneVolume(WebRtc_UWord32& volume) const;
    virtual WebRtc_Word32 MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const;
    virtual WebRtc_Word32 MinMicrophoneVolume(WebRtc_UWord32& minVolume) const;
    virtual WebRtc_Word32
        MicrophoneVolumeStepSize(WebRtc_UWord16& stepSize) const;

    // Microphone mute control
    virtual WebRtc_Word32 MicrophoneMuteIsAvailable(bool& available);
    virtual WebRtc_Word32 SetMicrophoneMute(bool enable);
    virtual WebRtc_Word32 MicrophoneMute(bool& enabled) const;

    // Speaker mute control
    virtual WebRtc_Word32 SpeakerMuteIsAvailable(bool& available);
    virtual WebRtc_Word32 SetSpeakerMute(bool enable);
    virtual WebRtc_Word32 SpeakerMute(bool& enabled) const;

    // Microphone boost control
    virtual WebRtc_Word32 MicrophoneBoostIsAvailable(bool& available);
    virtual WebRtc_Word32 SetMicrophoneBoost(bool enable);
    virtual WebRtc_Word32 MicrophoneBoost(bool& enabled) const;

    // Stereo support
    virtual WebRtc_Word32 StereoPlayoutIsAvailable(bool& available);
    virtual WebRtc_Word32 SetStereoPlayout(bool enable);
    virtual WebRtc_Word32 StereoPlayout(bool& enabled) const;
    virtual WebRtc_Word32 StereoRecordingIsAvailable(bool& available);
    virtual WebRtc_Word32 SetStereoRecording(bool enable);
    virtual WebRtc_Word32 StereoRecording(bool& enabled) const;

    // Delay information and control
    virtual WebRtc_Word32
        SetPlayoutBuffer(const AudioDeviceModule::BufferType type,
                         WebRtc_UWord16 sizeMS);
    virtual WebRtc_Word32 PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                        WebRtc_UWord16& sizeMS) const;
    virtual WebRtc_Word32 PlayoutDelay(WebRtc_UWord16& delayMS) const;
    virtual WebRtc_Word32 RecordingDelay(WebRtc_UWord16& delayMS) const;

    // CPU load
    virtual WebRtc_Word32 CPULoad(WebRtc_UWord16& load) const;
  
    virtual WebRtc_Word32 SetLoudspeakerStatus(bool enable);
    virtual WebRtc_Word32 GetLoudspeakerStatus(bool& enable) const;
    virtual WebRtc_Word32 GetOutputAudioRoute(OutputAudioRoute& route) const;

public:
    virtual bool PlayoutWarning() const;
    virtual bool PlayoutError() const;
    virtual bool PlayoutRouteChanged() const;
    virtual bool RecordingWarning() const;
    virtual bool RecordingError() const;
    virtual void ClearPlayoutWarning();
    virtual void ClearPlayoutError();
    virtual void ClearPlayoutRouteChanged();
    virtual void ClearRecordingWarning();
    virtual void ClearRecordingError();

public:
    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

private:
    void Lock()
    {
        _critSect.Enter();
    }
    ;
    void UnLock()
    {
        _critSect.Leave();
    }
    ;
    WebRtc_Word32 Id()
    {
        return _id;
    }

    static void AtomicSet32(int32_t* theValue, int32_t newValue);
    static int32_t AtomicGet32(int32_t* theValue);

    static void logCAMsg(const TraceLevel level,
                         const TraceModule module,
                         const WebRtc_Word32 id, const char *msg,
                         const char *err);

    WebRtc_Word32 GetNumberDevices();

    WebRtc_Word32 GetDeviceName(char* name);

    static void 
        interruptionListenerCallback(void *inUserData, UInt32 interruptionState);
    
    static void 
        propertyListenerCallback(void *inClientData, AudioSessionPropertyID	inID,
                                 UInt32 inDataSize, const void* inData);
private:
    static OSStatus 
        captureCallback(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 inBusNumber,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData);
    static OSStatus 
        renderCallback(void *inRefCon,
                       AudioUnitRenderActionFlags *ioActionFlags,
                       const AudioTimeStamp *inTimeStamp,
                       UInt32 inBusNumber,
                       UInt32 inNumberFrames,
                       AudioBufferList *ioData);
    OSStatus 
        implCaptureCallback(AudioUnitRenderActionFlags *ioActionFlags,
                            const AudioTimeStamp *inTimeStamp,
                            UInt32 inBusNumber,
                            UInt32 inNumberFrames);
    OSStatus 
        implRenderCallback(UInt32 inNumberFrames, AudioBufferList *ioData);
    
private:
    AudioDeviceBuffer* _ptrAudioBuffer;

    CriticalSectionWrapper& _critSect;
    CriticalSectionWrapper& _critSectCb;

    EventWrapper& _stopEventRec;
    EventWrapper& _stopEvent;

    WebRtc_Word32 _id;

    AudioMixerManagerIPhone _mixerManager;

    WebRtc_UWord8 _recChannels;
    WebRtc_UWord8 _playChannels;
    
    WebRtc_Word8 _playBufferRemainingSamples[2 * 440];
    WebRtc_UWord16 _remainingPlayoutSamplesLength;
    
    WebRtc_Word8 _recordBufferRemainingSamples[2 * 440];
    WebRtc_UWord16 _remainingRecordedSamplesLength;

    AudioDeviceModule::BufferType _playBufType;

private:
    bool _initialized;
    bool _audioSessionInitialized;
    bool _audioUnitInitialized;
    bool _isShutDown;
    bool _recording;
    bool _playing;
    bool _recIsInitialized;
    bool _playIsInitialized;
    bool _startRec;
    bool _stopRec;
    bool _stopPlay;
    bool _AGC;

    // Atomically set varaibles
    int32_t _renderDeviceIsAlive;
    int32_t _captureDeviceIsAlive;

    bool _doStop; // For play if not shared device or play+rec if shared device
    bool _doStopRec; // For rec if not shared device
    bool _stereoRender;
    bool _stereoRenderRequested;
    
    AudioComponentInstance _audioUnitComponent;

    AudioStreamBasicDescription _outStreamFormat;
    AudioStreamBasicDescription _inStreamFormat;

    WebRtc_UWord32 _captureLatencyUs;
    WebRtc_UWord32 _renderLatencyUs;

    // Atomically set variables
    mutable int32_t _captureDelayUs;
    mutable int32_t _renderDelayUs;

    WebRtc_Word32 _renderDelayOffsetSamples;

private:
    WebRtc_UWord16 _playBufDelay; // playback delay
    WebRtc_UWord16 _playBufDelayFixed; // fixed playback delay

    WebRtc_UWord16 _playWarning;
    WebRtc_UWord16 _playError;
    bool _playoutRouteChanged;
    WebRtc_UWord16 _recWarning;
    WebRtc_UWord16 _recError;
  
    AudioDeviceInfoIPhoneObjC* _audioDeviceInfo;
};

} //  namespace webrtc

#endif  // WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_IPHONE_H
