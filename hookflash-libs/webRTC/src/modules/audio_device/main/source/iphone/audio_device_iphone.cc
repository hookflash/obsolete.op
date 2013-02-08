/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio_device_utility.h"
#include "audio_device_iphone.h"
#include "audio_device_config.h"
#include "audio_device_info_iphone_objc.h"

#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"

#include <cassert>

#include <sys/sysctl.h>         // sysctlbyname()
#include <mach/mach.h>          // mach_task_self()
#include <libkern/OSAtomic.h>   // OSAtomicCompareAndSwap()

namespace webrtc
{

#define WEBRTC_CA_RETURN_ON_ERR(expr)                                     \
    do {                                                                \
        err = expr;                                                     \
        if (err != noErr) {                                             \
            logCAMsg(kTraceError, kTraceAudioDevice, _id,    \
                "Error in " #expr, (const char *)&err);                 \
            return -1;                                                  \
        }                                                               \
    } while(0)

#define WEBRTC_CA_LOG_ERR(expr)                                           \
    do {                                                                \
        err = expr;                                                     \
        if (err != noErr) {                                             \
            logCAMsg(kTraceError, kTraceAudioDevice, _id,    \
                "Error in " #expr, (const char *)&err);                 \
        }                                                               \
    } while(0)

#define WEBRTC_CA_LOG_WARN(expr)                                           \
    do {                                                                 \
        err = expr;                                                      \
        if (err != noErr) {                                              \
            logCAMsg(kTraceWarning, kTraceAudioDevice, _id,  \
                "Error in " #expr, (const char *)&err);                  \
        }                                                                \
    } while(0)

enum
{
    MaxNumberDevices = 64
};

void AudioDeviceIPhone::AtomicSet32(int32_t* theValue, int32_t newValue)
{
    while (1)
    {
        int32_t oldValue = *theValue;
        if (OSAtomicCompareAndSwap32Barrier(oldValue, newValue, theValue)
            == true)
        {
            return;
        }
    }
}

int32_t AudioDeviceIPhone::AtomicGet32(int32_t* theValue)
{
    while (1)
    {
        WebRtc_Word32 value = *theValue;
        if (OSAtomicCompareAndSwap32Barrier(value, value, theValue) == true)
        {
            return value;
        }
    }
}

// CoreAudio errors are best interpreted as four character strings.
void AudioDeviceIPhone::logCAMsg(const TraceLevel level,
                              const TraceModule module,
                              const WebRtc_Word32 id, const char *msg,
                              const char *err)
{
    assert(msg != NULL);
    assert(err != NULL);

#ifdef WEBRTC_BIG_ENDIAN
    WEBRTC_TRACE(level, module, id, "%s: %.4s", msg, err);
#else
    // We need to flip the characters in this case.
    WEBRTC_TRACE(level, module, id, "%s: %.1s%.1s%.1s%.1s", msg, err + 3, err
        + 2, err + 1, err);
#endif
}

AudioDeviceIPhone::AudioDeviceIPhone(const WebRtc_Word32 id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _critSectCb(*CriticalSectionWrapper::CreateCriticalSection()),
    _stopEventRec(*EventWrapper::Create()),
    _stopEvent(*EventWrapper::Create()),
    _id(id),
    _mixerManager(id),
    _recChannels(N_REC_CHANNELS),
    _playChannels(N_PLAY_CHANNELS),
    _remainingPlayoutSamplesLength(0),
    _remainingRecordedSamplesLength(0),
    _playBufType(AudioDeviceModule::kFixedBufferSize),
    _initialized(false),
    _audioSessionInitialized(false),
    _audioUnitInitialized(false),
    _isShutDown(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _startRec(false),
    _stopRec(false),
    _stopPlay(false),
    _AGC(false),
    _renderDeviceIsAlive(1),
    _captureDeviceIsAlive(1),
    _doStop(false),
    _doStopRec(false),
    _captureLatencyUs(0),
    _renderLatencyUs(0),
    _captureDelayUs(0),
    _renderDelayUs(0),
    _renderDelayOffsetSamples(0),
    _playBufDelayFixed(20),
    _playWarning(0),
    _playError(0),
    _playoutRouteChanged(false),
    _recWarning(0),
    _recError(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);

    assert(&_stopEvent != NULL);
    assert(&_stopEventRec != NULL);
  
    _audioDeviceInfo = [[AudioDeviceInfoIPhoneObjC alloc] init];
}


AudioDeviceIPhone::~AudioDeviceIPhone()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);

    if (!_isShutDown)
    {
        Terminate();
    }

    if(_audioDeviceInfo)
    {
        [_audioDeviceInfo release];
    }

    delete &_stopEvent;
    delete &_stopEventRec;
    delete &_critSect;
    delete &_critSectCb;
}

// ============================================================================
//                                     API
// ============================================================================

void AudioDeviceIPhone::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    _ptrAudioBuffer = audioBuffer;

    // inform the AudioBuffer about default settings for this implementation
    _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
    _ptrAudioBuffer->SetRecordingChannels(N_REC_CHANNELS);
    _ptrAudioBuffer->SetPlayoutChannels(N_PLAY_CHANNELS);
}

WebRtc_Word32 AudioDeviceIPhone::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    audioLayer = AudioDeviceModule::kPlatformDefaultAudio;
    return 0;
}
  
WebRtc_Word32 AudioDeviceIPhone::Init()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (_initialized)
    {
        return 0;
    }

    OSStatus err = noErr;

    _isShutDown = false;

    if (!_audioSessionInitialized)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioSessionInitialize(NULL, NULL, interruptionListenerCallback, NULL));
        _audioSessionInitialized = true;
    }
    
    UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
            sizeof(audioCategory), &audioCategory));
    
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange,
                                                            propertyListenerCallback, this));

    _playWarning = 0;
    _playError = 0;
    _playoutRouteChanged = false;
    _recWarning = 0;
    _recError = 0;

    _initialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::Terminate()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    if (!_initialized)
    {
        return 0;
    }

    if (_recording)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Recording must be stopped");
        return -1;
    }

    if (_playing)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Playback must be stopped");
        return -1;
    }

    _critSect.Enter();

    _mixerManager.Close();

    OSStatus err = noErr;
    int retVal = 0;
    
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionRemovePropertyListenerWithUserData(kAudioSessionProperty_AudioRouteChange, 
                propertyListenerCallback, this));

    _critSect.Leave();

    _isShutDown = true;
    _initialized = false;

    return retVal;
}

bool AudioDeviceIPhone::Initialized() const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_initialized);
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    // Make an attempt to open up the
    // output mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitSpeaker() == -1)
    {
        available = false;
        return 0;
    }

    // Given that InitSpeaker was successful, we know that a valid speaker exists
    // 
    available = true;

    // Close the initialized output mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::InitSpeaker()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (_playing)
    {
        return -1;
    }

    if (_mixerManager.OpenSpeaker() == -1)
    {
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Make an attempt to open up the
    // input mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitMicrophone() == -1)
    {
        available = false;
        return 0;
    }

    // Given that InitMicrophone was successful, we know that a valid microphone exists
    // 
    available = true;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}


WebRtc_Word32 AudioDeviceIPhone::InitMicrophone()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (_recording)
    {
        return -1;
    }

    if (_mixerManager.OpenMicrophone() == -1)
    {
        return -1;
    }

    return 0;
}

bool AudioDeviceIPhone::SpeakerIsInitialized() const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_mixerManager.SpeakerIsInitialized());
}

bool AudioDeviceIPhone::MicrophoneIsInitialized() const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_mixerManager.MicrophoneIsInitialized());
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerVolumeIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    // Make an attempt to open up the
    // output mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitSpeaker() == -1)
    {
        // If we end up here it means that the selected speaker has no volume
        // control.
        available = false;
        return 0;
    }

    // Given that InitSpeaker was successful, we know that a volume control exists
    //
    available = true;

    // Close the initialized output mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetSpeakerVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetSpeakerVolume(volume=%u)", volume);

    return (_mixerManager.SetSpeakerVolume(volume));
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerVolume(WebRtc_UWord32& volume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WebRtc_UWord32 level(0);

    if (_mixerManager.SpeakerVolume(level) == -1)
    {
        return -1;
    }

    volume = level;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetWaveOutVolume(WebRtc_UWord16 volumeLeft,
                                               WebRtc_UWord16 volumeRight)
{
    WEBRTC_TRACE(
                 kTraceModuleCall,
                 kTraceAudioDevice,
                 _id,
                 "AudioDeviceIPhone::SetWaveOutVolume(volumeLeft=%u, volumeRight=%u)",
                 volumeLeft, volumeRight);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32
AudioDeviceIPhone::WaveOutVolume(WebRtc_UWord16& /*volumeLeft*/,
                              WebRtc_UWord16& /*volumeRight*/) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WebRtc_UWord32 maxVol(0);

    if (_mixerManager.MaxSpeakerVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::MinSpeakerVolume(WebRtc_UWord32& minVolume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WebRtc_UWord32 minVol(0);

    if (_mixerManager.MinSpeakerVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;
    return 0;
}

WebRtc_Word32
AudioDeviceIPhone::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WebRtc_UWord16 delta(0);

    if (_mixerManager.SpeakerVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerMuteIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    // Make an attempt to open up the
    // output mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitSpeaker() == -1)
    {
        // If we end up here it means that the selected speaker has no volume
        // control, hence it is safe to state that there is no mute control
        // already at this stage.
        available = false;
        return 0;
    }

    // Check if the selected speaker has a mute control
    //
    _mixerManager.SpeakerMuteIsAvailable(isAvailable);

    available = isAvailable;

    // Close the initialized output mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetSpeakerMute(bool enable)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetSpeakerMute(enable=%u)", enable);
    return (_mixerManager.SetSpeakerMute(enable));
}

WebRtc_Word32 AudioDeviceIPhone::SpeakerMute(bool& enabled) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool muted(0);

    if (_mixerManager.SpeakerMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneMuteIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Make an attempt to open up the
    // input mixer corresponding to the currently selected input device.
    //
    if (!wasInitialized && InitMicrophone() == -1)
    {
        // If we end up here it means that the selected microphone has no volume
        // control, hence it is safe to state that there is no boost control
        // already at this stage.
        available = false;
        return 0;
    }

    // Check if the selected microphone has a mute control
    //
    _mixerManager.MicrophoneMuteIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetMicrophoneMute(bool enable)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetMicrophoneMute(enable=%u)", enable);
    return (_mixerManager.SetMicrophoneMute(enable));
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneMute(bool& enabled) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool muted(0);

    if (_mixerManager.MicrophoneMute(muted) == -1)
    {
        return -1;
    }

    enabled = muted;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneBoostIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Enumerate all avaliable microphone and make an attempt to open up the
    // input mixer corresponding to the currently selected input device.
    //
    if (!wasInitialized && InitMicrophone() == -1)
    {
        // If we end up here it means that the selected microphone has no volume
        // control, hence it is safe to state that there is no boost control
        // already at this stage.
        available = false;
        return 0;
    }

    // Check if the selected microphone has a boost control
    //
    _mixerManager.MicrophoneBoostIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetMicrophoneBoost(bool enable)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetMicrophoneBoost(enable=%u)", enable);

    return (_mixerManager.SetMicrophoneBoost(enable));
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneBoost(bool& enabled) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool onOff(0);

    if (_mixerManager.MicrophoneBoost(onOff) == -1)
    {
        return -1;
    }

    enabled = onOff;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StereoRecordingIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    if (!wasInitialized && InitMicrophone() == -1)
    {
        // Cannot open the specified device
        available = false;
        return 0;
    }

    // Check if the selected microphone can record stereo
    //
    _mixerManager.StereoRecordingIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetStereoRecording(bool enable)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetStereoRecording(enable=%u)", enable);

    if (enable)
        _recChannels = 2;
    else
        _recChannels = 1;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StereoRecording(bool& enabled) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    if (_recChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StereoPlayoutIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool isAvailable(false);
    bool wasInitialized = _mixerManager.SpeakerIsInitialized();

    if (!wasInitialized && InitSpeaker() == -1)
    {
        // Cannot open the specified device
        available = false;
        return 0;
    }

    // Check if the selected microphone can record stereo
    //
    _mixerManager.StereoPlayoutIsAvailable(isAvailable);
    available = isAvailable;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseSpeaker();
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetStereoPlayout(bool enable)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetStereoPlayout(enable=%u)", enable);

    if (enable)
        _playChannels = 2;
    else
        _playChannels = 1;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StereoPlayout(bool& enabled) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    if (_playChannels == 2)
        enabled = true;
    else
        enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetAGC(bool enable)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetAGC(enable=%d)", enable);

    _AGC = enable;

    return 0;
}

bool AudioDeviceIPhone::AGC() const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    return _AGC;
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneVolumeIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    bool wasInitialized = _mixerManager.MicrophoneIsInitialized();

    // Make an attempt to open up the
    // input mixer corresponding to the currently selected output device.
    //
    if (!wasInitialized && InitMicrophone() == -1)
    {
        // If we end up here it means that the selected microphone has no volume
        // control.
        available = false;
        return 0;
    }

    // Given that InitMicrophone was successful, we know that a volume control
    // exists
    //
    available = true;

    // Close the initialized input mixer
    //
    if (!wasInitialized)
    {
        _mixerManager.CloseMicrophone();
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetMicrophoneVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetMicrophoneVolume(volume=%u)", volume);

    return (_mixerManager.SetMicrophoneVolume(volume));
}

WebRtc_Word32 AudioDeviceIPhone::MicrophoneVolume(WebRtc_UWord32& volume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WebRtc_UWord32 level(0);

    if (_mixerManager.MicrophoneVolume(level) == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  failed to retrive current microphone level");
        return -1;
    }

    volume = level;
    return 0;
}

WebRtc_Word32
AudioDeviceIPhone::MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WebRtc_UWord32 maxVol(0);

    if (_mixerManager.MaxMicrophoneVolume(maxVol) == -1)
    {
        return -1;
    }

    maxVolume = maxVol;
    return 0;
}

WebRtc_Word32
AudioDeviceIPhone::MinMicrophoneVolume(WebRtc_UWord32& minVolume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WebRtc_UWord32 minVol(0);

    if (_mixerManager.MinMicrophoneVolume(minVol) == -1)
    {
        return -1;
    }

    minVolume = minVol;
    return 0;
}

WebRtc_Word32
AudioDeviceIPhone::MicrophoneVolumeStepSize(WebRtc_UWord16& stepSize) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WebRtc_UWord16 delta(0);

    if (_mixerManager.MicrophoneVolumeStepSize(delta) == -1)
    {
        return -1;
    }

    stepSize = delta;
    return 0;
}

WebRtc_Word16 AudioDeviceIPhone::PlayoutDevices()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    return GetNumberDevices();
}

WebRtc_Word32 AudioDeviceIPhone::SetPlayoutDevice(WebRtc_UWord16 index)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetPlayoutDevice(index=%u)", index);

    if (_playIsInitialized)
    {
        return -1;
    }

    WebRtc_UWord32 nDevices = GetNumberDevices();
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable waveform-audio output devices is %u",
                 nDevices);

    if (index > (nDevices - 1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  device index is out of range [0,%u]", (nDevices - 1));
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType /*device*/)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::PlayoutDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::PlayoutDeviceName(index=%u)", index);

    const WebRtc_UWord16 nDevices(PlayoutDevices());

    if ((index > (nDevices - 1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return GetDeviceName(&name[0]);
}

WebRtc_Word32 AudioDeviceIPhone::RecordingDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::RecordingDeviceName(index=%u)", index);

    const WebRtc_UWord16 nDevices(RecordingDevices());

    if ((index > (nDevices - 1)) || (name == NULL))
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return GetDeviceName(&name[0]);
}

WebRtc_Word16 AudioDeviceIPhone::RecordingDevices()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    return GetNumberDevices();
}

WebRtc_Word32 AudioDeviceIPhone::SetRecordingDevice(WebRtc_UWord16 index)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetRecordingDevice(index=%u)", index);

    if (_recIsInitialized)
    {
        return -1;
    }

    WebRtc_UWord32 nDevices = GetNumberDevices();
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  number of availiable waveform-audio input devices is %u",
                 nDevices);

    if (index > (nDevices - 1))
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  device index is out of range [0,%u]", (nDevices - 1));
        return -1;
    }

    return 0;
}


WebRtc_Word32
AudioDeviceIPhone::SetRecordingDevice(AudioDeviceModule::WindowsDeviceType /*device*/)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::PlayoutIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;

    // Try to initialize the playout side
    WebRtc_Word32 res = InitPlayout();

    // Cancel effect of initialization
    StopPlayout();

    if (res != -1)
    {
        available = true;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::RecordingIsAvailable(bool& available)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    available = false;

    // Try to initialize the recording side
    WebRtc_Word32 res = InitRecording();

    // Cancel effect of initialization
    StopRecording();

    if (res != -1)
    {
        available = true;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::InitPlayout()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (_playing)
    {
        return -1;
    }
    if (_playIsInitialized)
    {
        return 0;
    }

    // Initialize the speaker (devices might have been added or removed)
    if (InitSpeaker() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitSpeaker() failed");
    }

    OSStatus err = noErr;
    UInt32 size = 0;
    _renderDelayOffsetSamples = 0;
    _renderDelayUs = 0;
    _renderLatencyUs = 0;
    _renderDeviceIsAlive = 1;
    _doStop = false;

    AudioSessionPropertyID audioSessionProperty;

    // Get current stream description  
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareSampleRate;
    Float64 sampleRate = 0;
    size = sizeof(Float64);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &sampleRate));
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareOutputNumberChannels;
    UInt32 numberChannels = 0;
    size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &numberChannels));
    numberChannels = 1;
    _outStreamFormat.mSampleRate = sampleRate;
    _outStreamFormat.mFormatID = kAudioFormatLinearPCM;
    _outStreamFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    _outStreamFormat.mBitsPerChannel = sizeof(SInt16) * 8;
    _outStreamFormat.mChannelsPerFrame = numberChannels;
    _outStreamFormat.mBytesPerFrame = numberChannels * sizeof(SInt16);
    _outStreamFormat.mFramesPerPacket = 1;
    _outStreamFormat.mBytesPerPacket = numberChannels * sizeof(SInt16);;

    if (_outStreamFormat.mFormatID != kAudioFormatLinearPCM)
    {
        logCAMsg(kTraceError, kTraceAudioDevice, _id,
                 "Unacceptable output stream format -> mFormatID",
                 (const char *) &_outStreamFormat.mFormatID);
        return -1;
    }

    if (_outStreamFormat.mChannelsPerFrame > N_DEVICE_CHANNELS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "Too many channels on device -> mChannelsPerFrame = %d",
                     _outStreamFormat.mChannelsPerFrame);
        return -1;
    }

    if (_outStreamFormat.mFormatFlags & kAudioFormatFlagIsNonInterleaved)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "Non-interleaved audio data is not supported.",
                     "AudioHardware streams should not have this format.");
        return -1;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "Ouput stream format:");
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mSampleRate = %f, mChannelsPerFrame = %u",
                 _outStreamFormat.mSampleRate,
                 _outStreamFormat.mChannelsPerFrame);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mBytesPerPacket = %u, mFramesPerPacket = %u",
                 _outStreamFormat.mBytesPerPacket,
                 _outStreamFormat.mFramesPerPacket);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mBytesPerFrame = %u, mBitsPerChannel = %u",
                 _outStreamFormat.mBytesPerFrame,
                 _outStreamFormat.mBitsPerChannel);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "mFormatFlags = %u, mChannelsPerFrame = %u",
                 _outStreamFormat.mFormatFlags,
                 _outStreamFormat.mChannelsPerFrame);
    logCAMsg(kTraceInfo, kTraceAudioDevice, _id, "mFormatID",
             (const char *) &_outStreamFormat.mFormatID);

    // Our preferred format to work with 
    if (_outStreamFormat.mChannelsPerFrame >= 2 && (_playChannels == 2))
    {
    } else
    {
        // Disable stereo playout when we only have one channel on the device.
        _playChannels = 1;
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "Stereo playout unavailable on this device");
    }

    if (_ptrAudioBuffer)
    {
        // Update audio buffer with the selected parameters
        _ptrAudioBuffer->SetPlayoutSampleRate(N_PLAY_SAMPLES_PER_SEC);
        _ptrAudioBuffer->SetPlayoutChannels((WebRtc_UWord8) _playChannels);
    }
/*
    // Get render device latency
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareOutputLatency;
    UInt32 latency = 0;
    size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &latency));
    _renderLatencyUs = (WebRtc_UWord32) ((1.0e6 * latency)
                                         / _outStreamFormat.mSampleRate);
    
    // Get render stream latency
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareIOBufferDuration;
    size = sizeof(UInt32);
    latency = 0;
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &latency));
    _renderLatencyUs += (WebRtc_UWord32) ((1.0e6 * latency)
                                          / _outStreamFormat.mSampleRate);
*/
    _renderLatencyUs = 10;
    if (!_recIsInitialized)
    {
        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Output;
        //desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
        desc.componentSubType = kAudioUnitSubType_RemoteIO;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        
        AudioComponent component = AudioComponentFindNext(NULL, &desc);
        if (component == NULL)
            return -1;
        
        WEBRTC_CA_RETURN_ON_ERR(AudioComponentInstanceNew(component, &_audioUnitComponent));
    }
    
    UInt32 enable;
    enable = 1;
/*
    WEBRTC_CA_RETURN_ON_ERR(AudioUnitSetProperty(_audioUnitComponent, kAudioOutputUnitProperty_EnableIO,
                                                 kAudioUnitScope_Output, 0, &enable, sizeof(enable)));
*/     
    WEBRTC_CA_RETURN_ON_ERR(AudioUnitSetProperty(_audioUnitComponent, kAudioUnitProperty_StreamFormat,
                                                 kAudioUnitScope_Input, 0, &_outStreamFormat, sizeof(_outStreamFormat)));
    
    AURenderCallbackStruct outputCallback;
    outputCallback.inputProc = renderCallback;
    outputCallback.inputProcRefCon = this;
    WEBRTC_CA_RETURN_ON_ERR(AudioUnitSetProperty(_audioUnitComponent, kAudioUnitProperty_SetRenderCallback,
                                                 kAudioUnitScope_Input, 0, &outputCallback, sizeof(outputCallback)));

    // Mark playout side as initialized
    _playIsInitialized = true;

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  initial playout status: _renderDelayOffsetSamples=%d,"
                 " _renderDelayUs=%d, _renderLatencyUs=%d",
                 _renderDelayOffsetSamples, _renderDelayUs, _renderLatencyUs);

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::InitRecording()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (_recording)
    {
        return -1;
    }

    if (_recIsInitialized)
    {
        return 0;
    }

    // Initialize the microphone (devices might have been added or removed)
    if (InitMicrophone() == -1)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  InitMicrophone() failed");
    }

    OSStatus err = noErr;
    UInt32 size = 0;

    _captureDelayUs = 0;
    _captureLatencyUs = 0;
    _captureDeviceIsAlive = 1;
    _doStopRec = false;

    AudioSessionPropertyID audioSessionProperty;
    // Get current stream description  
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareSampleRate;
    Float64 sampleRate = 0;
    size = sizeof(Float64);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &sampleRate));
/*
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareInputNumberChannels;
    UInt32 numberChannels = 0;
    size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &numberChannels));
*/
    UInt32 numberChannels = 1;
    _inStreamFormat.mSampleRate = sampleRate;
    _inStreamFormat.mFormatID = kAudioFormatLinearPCM;
    _inStreamFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    _inStreamFormat.mBitsPerChannel = sizeof(SInt16) * 8;
    _inStreamFormat.mChannelsPerFrame = numberChannels;
    _inStreamFormat.mBytesPerFrame = numberChannels * sizeof(SInt16);
    _inStreamFormat.mFramesPerPacket = 1;
    _inStreamFormat.mBytesPerPacket = numberChannels * sizeof(SInt16);;

    if (_inStreamFormat.mFormatID != kAudioFormatLinearPCM)
    {
        logCAMsg(kTraceError, kTraceAudioDevice, _id,
                 "Unacceptable input stream format -> mFormatID",
                 (const char *) &_inStreamFormat.mFormatID);
        return -1;
    }

    if (_inStreamFormat.mChannelsPerFrame > N_DEVICE_CHANNELS)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     ", Too many channels on device (mChannelsPerFrame = %d)",
                     _inStreamFormat.mChannelsPerFrame);
        return -1;
    }

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " Input stream format:");
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " mSampleRate = %f, mChannelsPerFrame = %u",
                 _inStreamFormat.mSampleRate, _inStreamFormat.mChannelsPerFrame);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " mBytesPerPacket = %u, mFramesPerPacket = %u",
                 _inStreamFormat.mBytesPerPacket,
                 _inStreamFormat.mFramesPerPacket);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " mBytesPerFrame = %u, mBitsPerChannel = %u",
                 _inStreamFormat.mBytesPerFrame,
                 _inStreamFormat.mBitsPerChannel);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 " mFormatFlags = %u, mChannelsPerFrame = %u",
                 _inStreamFormat.mFormatFlags,
                 _inStreamFormat.mChannelsPerFrame);
    logCAMsg(kTraceInfo, kTraceAudioDevice, _id, "mFormatID",
             (const char *) &_inStreamFormat.mFormatID);

    // Our preferred format to work with
    if (_inStreamFormat.mChannelsPerFrame >= 2 && (_recChannels == 2))
    {
    } else
    {
        // Disable stereo recording when we only have one channel on the device.
        _recChannels = 1;
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "Stereo recording unavailable on this device");
    }

    if (_ptrAudioBuffer)
    {
        // Update audio buffer with the selected parameters
        _ptrAudioBuffer->SetRecordingSampleRate(N_REC_SAMPLES_PER_SEC);
        _ptrAudioBuffer->SetRecordingChannels((WebRtc_UWord8) _recChannels);
    }
/*
    // Get capture device latency
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareInputLatency;
    UInt32 latency = 0;
    size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty, 
            &size, &latency));
    _captureLatencyUs = (UInt32)((1.0e6 * latency)
                                 / _inStreamFormat.mSampleRate);

    // Get capture stream latency
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareIOBufferDuration;
    size = sizeof(UInt32);
    latency = 0;
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &latency));
    _captureLatencyUs += (UInt32)((1.0e6 * latency)
                                  / _inStreamFormat.mSampleRate);
*/
    _captureLatencyUs = 10;
    if (!_playIsInitialized)
    {
        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Output;
        //desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
        desc.componentSubType = kAudioUnitSubType_RemoteIO;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        
        AudioComponent component = AudioComponentFindNext(NULL, &desc);
        if (component == NULL)
            return -1;
        
        WEBRTC_CA_RETURN_ON_ERR(AudioComponentInstanceNew(component, &_audioUnitComponent));
    }
    
    UInt32 enable;
    enable = 1;
    WEBRTC_CA_RETURN_ON_ERR(AudioUnitSetProperty(_audioUnitComponent, kAudioOutputUnitProperty_EnableIO,
                                                 kAudioUnitScope_Input, 1, &enable, sizeof(enable)));
    
    WEBRTC_CA_RETURN_ON_ERR(AudioUnitSetProperty(_audioUnitComponent, kAudioUnitProperty_StreamFormat,
                                                 kAudioUnitScope_Output, 1, &_inStreamFormat, sizeof(_inStreamFormat)));
    
    AURenderCallbackStruct inputCallback;
    inputCallback.inputProc = captureCallback;
    inputCallback.inputProcRefCon = this;
    WEBRTC_CA_RETURN_ON_ERR(AudioUnitSetProperty(_audioUnitComponent, kAudioOutputUnitProperty_SetInputCallback,
                                                 kAudioUnitScope_Global, 1, &inputCallback, sizeof(inputCallback)));

    // Mark recording side as initialized
    _recIsInitialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StartRecording()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);
  
    if (!_recIsInitialized)
    {
        return -1;
    }

    if (_recording)
    {
        return 0;
    }

    if (!_initialized)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Recording worker thread has not been started");
        return -1;
    }
    
    OSStatus err = noErr;
    
    if (!_audioUnitInitialized)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioUnitInitialize(_audioUnitComponent));
        _audioUnitInitialized = true;
    }

    if (!_playing)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s\n", "AudioDeviceIPhone::StartRecording()");
        err = AudioSessionSetActive(true);
        if (err != noErr)
        {
            char* errChar;
            errChar = (char*)&err;
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "%s: %.1s%.1s%.1s%.1s\n", "AudioDeviceIPhone::StartRecording() - error: ", errChar + 3, errChar + 2, errChar + 1, errChar);
            return -1;            
        }
        WEBRTC_CA_RETURN_ON_ERR(AudioOutputUnitStart(_audioUnitComponent));
    }

    _recording = true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StopRecording()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (!_recIsInitialized)
    {
        return 0;
    }

    OSStatus err = noErr;
    
    if (_recording && !_playing)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioOutputUnitStop(_audioUnitComponent));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s\n", "AudioDeviceIPhone::StopRecording()");
        err = AudioSessionSetActive(false);
        if (err != noErr)
        {
            char* errChar;
            errChar = (char*)&err;
            if (err == kAudioSessionNotActiveError)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "%s: %.1s%.1s%.1s%.1s\n", "AudioDeviceIPhone::StopRecording() - error: ", errChar + 3, errChar + 2, errChar + 1, errChar);
            }
            else 
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "%s: %.1s%.1s%.1s%.1s\n", "AudioDeviceIPhone::StopRecording() - error: ", errChar + 3, errChar + 2, errChar + 1, errChar);
                return -1;
            }
        }
    }

    _audioUnitInitialized = false;
    _recIsInitialized = false;
    _recording = false;

    return 0;
}

bool AudioDeviceIPhone::RecordingIsInitialized() const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_recIsInitialized);
}

bool AudioDeviceIPhone::Recording() const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_recording);
}

bool AudioDeviceIPhone::PlayoutIsInitialized() const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_playIsInitialized);
}

WebRtc_Word32 AudioDeviceIPhone::StartPlayout()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
  
    CriticalSectionScoped lock(_critSect);
    
    if (!_playIsInitialized)
    {
        return -1;
    }

    if (_playing)
    {
        return 0;
    }

    OSStatus err = noErr;
    
    if (!_audioUnitInitialized)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioUnitInitialize(_audioUnitComponent));
        _audioUnitInitialized = true;
    }

    if (!_recording)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s", "AudioDeviceIPhone::StartPlayout()");
        err = AudioSessionSetActive(true);
        if (err != noErr)
        {
            char* errChar;
            errChar = (char*)&err;
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "%s: %.1s%.1s%.1s%.1s\n", "AudioDeviceIPhone::StartPlayout() - error: ", errChar + 3, errChar + 2, errChar + 1, errChar);
            return -1;            
        }
        WEBRTC_CA_RETURN_ON_ERR(AudioOutputUnitStart(_audioUnitComponent));
    }

    _playing = true;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::StopPlayout()
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    if (!_playIsInitialized)
    {
        return 0;
    }

    OSStatus err = noErr;
    
    if (_playing && !_recording)
    {
        WEBRTC_CA_RETURN_ON_ERR(AudioOutputUnitStop(_audioUnitComponent));
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s\n", "AudioDeviceIPhone::StopPlayout()");
        err = AudioSessionSetActive(false);
        if (err != noErr)
        {
            char* errChar;
            errChar = (char*)&err;
            if (err == kAudioSessionNotActiveError)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id, "%s: %.1s%.1s%.1s%.1s\n", "AudioDeviceIPhone::StopPlayout() - error: ", errChar + 3, errChar + 2, errChar + 1, errChar);
            }
            else 
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id, "%s: %.1s%.1s%.1s%.1s\n", "AudioDeviceIPhone::StopPlayout() - error: ", errChar + 3, errChar + 2, errChar + 1, errChar);
                return -1;
            }
        }
    }

    _audioUnitInitialized = false;
    _playIsInitialized = false;
    _playing = false;

    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    int32_t renderDelayUs = AtomicGet32(&_renderDelayUs);
    delayMS = static_cast<WebRtc_UWord16> (1e-3 * (renderDelayUs
        + _renderLatencyUs) + 0.5);
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::RecordingDelay(WebRtc_UWord16& delayMS) const
{
    int32_t captureDelayUs = AtomicGet32(&_captureDelayUs);
    delayMS = static_cast<WebRtc_UWord16> (1e-3 * (captureDelayUs
        + _captureLatencyUs) + 0.5);
    return 0;
}

bool AudioDeviceIPhone::Playing() const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    return (_playing);
}

WebRtc_Word32 AudioDeviceIPhone::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type,
    WebRtc_UWord16 sizeMS)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetPlayoutBuffer(type=%u, sizeMS=%u)", type,
                 sizeMS);

    if (type != AudioDeviceModule::kFixedBufferSize)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     " Adaptive buffer size not supported on this platform");
        return -1;
    }

    _playBufType = type;
    _playBufDelayFixed = sizeMS;
    return 0;
}

WebRtc_Word32 AudioDeviceIPhone::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,
    WebRtc_UWord16& sizeMS) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    type = _playBufType;
    sizeMS = _playBufDelayFixed;

    return 0;
}

// Not implemented for Mac.
WebRtc_Word32 AudioDeviceIPhone::CPULoad(WebRtc_UWord16& /*load*/) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");

    return -1;
}

WebRtc_Word32 AudioDeviceIPhone::SetLoudspeakerStatus(bool enable)
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "AudioDeviceIPhone::SetLoudspeakerStatus(enable=%u)", enable);
    return (_mixerManager.SetLoudspeakerStatus(enable));
}
  
WebRtc_Word32 AudioDeviceIPhone::GetLoudspeakerStatus(bool& enable) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
  
    bool status(0);
  
    if (_mixerManager.GetLoudspeakerStatus(status) == -1)
    {
        return -1;
    }
  
    enable = status;
    return 0;
}
  
WebRtc_Word32 AudioDeviceIPhone::GetOutputAudioRoute(OutputAudioRoute& route) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);
    
    if (_mixerManager.GetOutputAudioRoute(route) == -1)
    {
        return -1;
    }
  
    return 0;
}

bool AudioDeviceIPhone::PlayoutWarning() const
{
    return (_playWarning > 0);
}

bool AudioDeviceIPhone::PlayoutError() const
{
    return (_playError > 0);
}
  
bool AudioDeviceIPhone::PlayoutRouteChanged() const
{
    return _playoutRouteChanged;
}

bool AudioDeviceIPhone::RecordingWarning() const
{
    return (_recWarning > 0);
}

bool AudioDeviceIPhone::RecordingError() const
{
    return (_recError > 0);
}

void AudioDeviceIPhone::ClearPlayoutWarning()
{
    _playWarning = 0;
}

void AudioDeviceIPhone::ClearPlayoutError()
{
    _playError = 0;
}
  
void AudioDeviceIPhone::ClearPlayoutRouteChanged()
{
    _playoutRouteChanged = false;
}

void AudioDeviceIPhone::ClearRecordingWarning()
{
    _recWarning = 0;
}

void AudioDeviceIPhone::ClearRecordingError()
{
    _recError = 0;
}

// ============================================================================
//                                 Private Methods
// ============================================================================

WebRtc_Word32
AudioDeviceIPhone::GetNumberDevices()
{
    return 1;
}

WebRtc_Word32
AudioDeviceIPhone::GetDeviceName(char* name)
{
    sprintf(name, "iPhone device");

    return 0;
}
    
void AudioDeviceIPhone::interruptionListenerCallback(void *inUserData, UInt32 interruptionState)
{
}
    
void AudioDeviceIPhone::propertyListenerCallback(void *inClientData, AudioSessionPropertyID	inID,
            UInt32 inDataSize, const void* inData)
{
    AudioDeviceIPhone* ptrThis = (AudioDeviceIPhone*) inClientData;
  
    if (inID == kAudioSessionProperty_AudioRouteChange)
    {
        char osVersion[30];
        [[ptrThis->_audioDeviceInfo getOSVersion] getCString:osVersion maxLength:30 encoding:NSUTF8StringEncoding];
      
        if (strncmp(osVersion, "5.0", 3) >= 0)
        {
            CFDictionaryRef routeChangeDictionary = (CFDictionaryRef) inData;
            CFDictionaryRef currentRouteDescription =
                (CFDictionaryRef) CFDictionaryGetValue(
                                                       routeChangeDictionary,
                                                       CFSTR("ActiveAudioRouteDidChange_NewDetailedRoute"));
            CFArrayRef audioRuteOutputs = (CFArrayRef)CFDictionaryGetValue(currentRouteDescription,  CFSTR("RouteDetailedDescription_Outputs"));
            for (int i = 0; i < CFArrayGetCount(audioRuteOutputs); i++)
            {
                CFDictionaryRef audioRouteOutput = (CFDictionaryRef)CFArrayGetValueAtIndex(audioRuteOutputs, i);
                CFStringRef audioRoute = (CFStringRef)CFDictionaryGetValue(audioRouteOutput, CFSTR("RouteDetailedDescription_PortType"));
                if (CFStringCompare(audioRoute, CFSTR("Headphones"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
                {
                    ptrThis->_mixerManager.SetOutputAudioRoute(kOutputAudioRouteHeadphone);
                    ptrThis->_playoutRouteChanged = true;
                    break;
                }
                else if (CFStringCompare(audioRoute, CFSTR("Receiver"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
                {
                    ptrThis->_mixerManager.SetOutputAudioRoute(kOutputAudioRouteBuiltInReceiver);
                    ptrThis->_playoutRouteChanged = true;
                    break;
                }
                else if (CFStringCompare(audioRoute, CFSTR("Speaker"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
                {
                    ptrThis->_mixerManager.SetOutputAudioRoute(kOutputAudioRouteBuiltInSpeaker);
                    ptrThis->_playoutRouteChanged = true;
                    break;
                }
            }
        }
        else
        {
            CFStringRef audioRoute;
            UInt32 size = sizeof(audioRoute);
            OSStatus err = AudioSessionGetProperty(kAudioSessionProperty_AudioRoute,
                                                   &size, &audioRoute);
            if (err != noErr)
              return;
            
            if (CFStringCompare(audioRoute, CFSTR("HeadphonesAndMicrophone"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            {
                ptrThis->_mixerManager.SetOutputAudioRoute(kOutputAudioRouteHeadphone);
                ptrThis->_playoutRouteChanged = true;
            }
            else if (CFStringCompare(audioRoute, CFSTR("ReceiverAndMicrophone"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            {
                ptrThis->_mixerManager.SetOutputAudioRoute(kOutputAudioRouteBuiltInReceiver);
                ptrThis->_playoutRouteChanged = true;
            }
            else if (CFStringCompare(audioRoute, CFSTR("SpeakerAndMicrophone"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
            {
                ptrThis->_mixerManager.SetOutputAudioRoute(kOutputAudioRouteBuiltInSpeaker);
                ptrThis->_playoutRouteChanged = true;
            }
        }
    }
}
    
OSStatus AudioDeviceIPhone::captureCallback(void *inRefCon,
                                            AudioUnitRenderActionFlags *ioActionFlags,
                                            const AudioTimeStamp *inTimeStamp,
                                            UInt32 inBusNumber,
                                            UInt32 inNumberFrames,
                                            AudioBufferList *ioData)
{
    AudioDeviceIPhone *ptrThis = (AudioDeviceIPhone*)inRefCon;
    assert(ptrThis != NULL);
    
    return ptrThis->implCaptureCallback(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames);
}
    
OSStatus AudioDeviceIPhone::renderCallback(void *inRefCon,
                                           AudioUnitRenderActionFlags *ioActionFlags,
                                           const AudioTimeStamp *inTimeStamp,
                                           UInt32 inBusNumber,
                                           UInt32 inNumberFrames,
                                           AudioBufferList *ioData)
{
    AudioDeviceIPhone *ptrThis = (AudioDeviceIPhone*)inRefCon;
    assert(ptrThis != NULL);
    
    return ptrThis->implRenderCallback(inNumberFrames, ioData);
}

OSStatus AudioDeviceIPhone::implCaptureCallback(AudioUnitRenderActionFlags *ioActionFlags,
                                                const AudioTimeStamp *inTimeStamp,
                                                UInt32 inBusNumber,
                                                UInt32 inNumberFrames)
{
    OSStatus err = noErr;
    const UInt32 size = 440; // 10ms
    WebRtc_Word8 recordBuffer[2 * size];

    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "Capture Callback");

    AudioBufferList engineBuffer;
    engineBuffer.mNumberBuffers = 1; // Interleaved channels.
    engineBuffer.mBuffers->mNumberChannels = _inStreamFormat.mChannelsPerFrame;
    engineBuffer.mBuffers->mDataByteSize = _inStreamFormat.mBytesPerFrame * inNumberFrames;
    engineBuffer.mBuffers->mData = NULL;
    
    WEBRTC_CA_RETURN_ON_ERR(AudioUnitRender(_audioUnitComponent,ioActionFlags,inTimeStamp, 
            inBusNumber, inNumberFrames, &engineBuffer));
    
    WebRtc_Word32 msecOnPlaySide;
    WebRtc_Word32 msecOnRecordSide;
    
    int32_t captureDelayUs = AtomicGet32(&_captureDelayUs);
    int32_t renderDelayUs = AtomicGet32(&_renderDelayUs);
    
    msecOnPlaySide = static_cast<WebRtc_Word32> (1e-3 * (renderDelayUs
                                                         + _renderLatencyUs) + 0.5);
    msecOnRecordSide = static_cast<WebRtc_Word32> (1e-3 * (captureDelayUs
                                                           + _captureLatencyUs) + 0.5);
    
    if (!_ptrAudioBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  capture AudioBuffer is invalid");
        return -1;
    }    
    
    UInt32 leftSamples = inNumberFrames;
    UInt32 offset = 0;
    
    if (_remainingRecordedSamplesLength > 0)
    {
        memcpy(recordBuffer, _recordBufferRemainingSamples, _remainingRecordedSamplesLength * 2);

        memcpy(recordBuffer + _remainingRecordedSamplesLength * 2, (WebRtc_Word8*)engineBuffer.mBuffers->mData, 
               (size - _remainingRecordedSamplesLength) * 2);

        _ptrAudioBuffer->SetRecordedBuffer(recordBuffer, size);
        
        _ptrAudioBuffer->SetVQEData(msecOnPlaySide, msecOnRecordSide, 0);
        _ptrAudioBuffer->DeliverRecordedData();
        
        leftSamples -= size - _remainingRecordedSamplesLength;
        offset += 2 * (size - _remainingRecordedSamplesLength);
        _remainingRecordedSamplesLength = 0;
    }

    while (leftSamples >= size)
    {
        // store the recorded buffer (no action will be taken if the
        // #recorded samples is not a full buffer)
        _ptrAudioBuffer->SetRecordedBuffer(((WebRtc_Word8*)engineBuffer.mBuffers->mData) + offset,
                                           size);
        
        _ptrAudioBuffer->SetVQEData(msecOnPlaySide, msecOnRecordSide, 0);
        _ptrAudioBuffer->DeliverRecordedData();
        
        leftSamples -= size;
        offset += 2 * size;
    }

    if (leftSamples > 0)
    {
        memcpy(_recordBufferRemainingSamples, ((WebRtc_Word8*)engineBuffer.mBuffers->mData) + offset,
               leftSamples * 2);
        _remainingRecordedSamplesLength = leftSamples;
    }

    return 0;
}
    
OSStatus AudioDeviceIPhone::implRenderCallback(UInt32 inNumberFrames, AudioBufferList *ioData)
{
    const UInt32 size = 440; // 10ms
    WebRtc_Word8 playBuffer[2 * size];
    
    WEBRTC_TRACE(kTraceDebug, kTraceAudioDevice, _id,
                 "Render Callback");

    if (!_ptrAudioBuffer)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  capture AudioBuffer is invalid");
        return -1;
    }
    
    UInt32 leftSamples = inNumberFrames;
    UInt32 offset = 0;
    
    AudioBuffer* ioBuffer = &(ioData->mBuffers[0]);
    
    if (_remainingPlayoutSamplesLength > 0)
    {
        memcpy((WebRtc_Word8*)ioBuffer->mData, _playBufferRemainingSamples, _remainingPlayoutSamplesLength * 2);

        leftSamples -= _remainingPlayoutSamplesLength;
        offset += 2 * _remainingPlayoutSamplesLength;
        _remainingPlayoutSamplesLength = 0;
    }
    
    while (leftSamples >= size)
    {
        // Ask for new PCM data to be played out using the AudioDeviceBuffer.
        WebRtc_UWord32 nSamples =
            _ptrAudioBuffer->RequestPlayoutData(size);
        
        nSamples = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        
//        for (int i = 0; i < nSamples; i++)
//        {
//            SInt16 val = ((SInt16*)playBuffer)[i];
//            ((SInt16*)playBuffer)[i] = val;
//        }
        
        if (nSamples != size)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  invalid number of output samples(%d)", nSamples);
        }
        
        WebRtc_UWord32 nOutSamples = nSamples * _outStreamFormat.mChannelsPerFrame;

        memcpy(((WebRtc_Word8*)ioBuffer->mData) + offset, playBuffer, nOutSamples * 2);
        
        leftSamples -= size;
        offset += 2 * size;
    }

    if (leftSamples > 0)
    {
        WebRtc_UWord32 nSamples =
            _ptrAudioBuffer->RequestPlayoutData(size);
        
        nSamples = _ptrAudioBuffer->GetPlayoutData(playBuffer);

//        for (int i = 0; i < nSamples; i++)
//        {
//            SInt16 val = ((SInt16*)playBuffer)[i];
//            ((SInt16*)playBuffer)[i] = val;
//        }

        if (nSamples != size)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  invalid number of output samples(%d)", nSamples);
        }
        
        WebRtc_UWord32 nOutSamples = nSamples * _outStreamFormat.mChannelsPerFrame;
        
        memcpy(((WebRtc_Word8*)ioBuffer->mData) + offset, playBuffer, leftSamples * 2);
        
        _remainingPlayoutSamplesLength = nOutSamples - leftSamples;
        
        memcpy(_playBufferRemainingSamples, playBuffer + (leftSamples * 2), _remainingPlayoutSamplesLength * 2);
    }

    return 0;
}
    
} //  namespace webrtc
