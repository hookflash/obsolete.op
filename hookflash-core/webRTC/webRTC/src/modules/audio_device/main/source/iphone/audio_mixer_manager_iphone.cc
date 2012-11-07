/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio_mixer_manager_iphone.h"
#include "trace.h"

#include <unistd.h>             // getpid()

namespace webrtc {
	
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

AudioMixerManagerIPhone::AudioMixerManagerIPhone(const WebRtc_Word32 id) :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _inputInitialized(false),
    _outputInitialized(false),
    _noInputChannels(0),
    _noOutputChannels(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s constructed", __FUNCTION__);
}

AudioMixerManagerIPhone::~AudioMixerManagerIPhone()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destructed", __FUNCTION__);

    Close();

    delete &_critSect;
}

// ============================================================================
//	                                PUBLIC METHODS
// ============================================================================

WebRtc_Word32 AudioMixerManagerIPhone::Close()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    CloseSpeaker();
    CloseMicrophone();

    return 0;

}

WebRtc_Word32 AudioMixerManagerIPhone::CloseSpeaker()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    _outputInitialized = false;
    _noOutputChannels = 0;

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::CloseMicrophone()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    CriticalSectionScoped lock(_critSect);

    _outputInitialized = false;
    _noInputChannels = 0;

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::OpenSpeaker()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerIPhone::OpenSpeaker");

    CriticalSectionScoped lock(_critSect);

    OSStatus err = noErr;
    UInt32 size = 0;

    AudioSessionPropertyID audioSessionProperty;
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareOutputNumberChannels;
    UInt32 numberChannels = 0;
    size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &numberChannels));
    numberChannels = 1;
    _noOutputChannels = numberChannels;
    
    _outputInitialized = true;
    
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::OpenMicrophone()
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerIPhone::OpenMicrophone");

    CriticalSectionScoped lock(_critSect);
/*
    OSStatus err = noErr;
    UInt32 size = 0;

    AudioSessionPropertyID audioSessionProperty;
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareInputNumberChannels;
    UInt32 numberChannels = 0;
    size = sizeof(UInt32);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
                                                    &size, &numberChannels));
 */
    UInt32 numberChannels = 1;
    _noInputChannels = numberChannels;
    
    _inputInitialized = true;

    return 0;
}

bool AudioMixerManagerIPhone::SpeakerIsInitialized() const
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    return _outputInitialized;
}

bool AudioMixerManagerIPhone::MicrophoneIsInitialized() const
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s",
                 __FUNCTION__);

    return _inputInitialized;
}

WebRtc_Word32 AudioMixerManagerIPhone::SetSpeakerVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerIPhone::SetSpeakerVolume(volume=%u)", volume);
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::SpeakerVolume(WebRtc_UWord32& volume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    OSStatus err = noErr;
    UInt32 size = 0;
    unsigned int channels = 0;
    Float32 vol = 0;
    
    AudioSessionPropertyID audioSessionProperty;
    audioSessionProperty = kAudioSessionProperty_CurrentHardwareOutputVolume;
    size = sizeof(Float32);
    WEBRTC_CA_RETURN_ON_ERR(AudioSessionGetProperty(audioSessionProperty,
            &size, &vol));
    
    // vol 0.0 to 1.0 -> convert to 0 - 255
    channels = 1;
    volume = static_cast<WebRtc_UWord32> (255 * vol / channels + 0.5);

    
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerIPhone::SpeakerVolume() => vol=%i", vol);

    return 0;
}

WebRtc_Word32
AudioMixerManagerIPhone::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    // volume range is 0.0 to 1.0
    // we convert that to 0 - 255
    maxVolume = 255;

    return 0;
}

WebRtc_Word32
AudioMixerManagerIPhone::MinSpeakerVolume(WebRtc_UWord32& minVolume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    // volume range is 0.0 to 1.0
    // we convert that to 0 - 255
    minVolume = 0;

    return 0;
}

WebRtc_Word32
AudioMixerManagerIPhone::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    // volume range is 0.0 to 1.0
    // we convert that to 0 - 255
    stepSize = 1;

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::SpeakerVolumeIsAvailable(bool& available)
{
    available = false;
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::SpeakerMuteIsAvailable(bool& available)
{
    available = false;
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::SetSpeakerMute(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerIPhone::SetSpeakerMute(enable=%u)", enable);

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::SpeakerMute(bool& enabled) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerIPhone::SpeakerMute() => enabled=%d, enabled");

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::StereoPlayoutIsAvailable(bool& available)
{
    available = (_noOutputChannels == 2);
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::StereoRecordingIsAvailable(bool& available)
{
    available = (_noInputChannels == 2);
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::MicrophoneMuteIsAvailable(bool& available)
{
    available = false;
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::SetMicrophoneMute(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerIPhone::SetMicrophoneMute(enable=%u)", enable);

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::MicrophoneMute(bool& enabled) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "     AudioMixerManagerIPhone::MicrophoneMute() => enabled=%d",
                 enabled);

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::MicrophoneBoostIsAvailable(bool& available)
{
    available = false; // No AudioObjectPropertySelector value for Mic Boost

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::SetMicrophoneBoost(bool enable)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerIPhone::SetMicrophoneBoost(enable=%u)", enable);

    // It is assumed that the call above fails!
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::MicrophoneBoost(bool& enabled) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    // Microphone boost cannot be enabled on this platform!
    enabled = false;

    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::MicrophoneVolumeIsAvailable(bool& available)
{
    available = false;
    return 0;
}

WebRtc_Word32 AudioMixerManagerIPhone::SetMicrophoneVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "AudioMixerManagerIPhone::SetMicrophoneVolume(volume=%u)", volume);

    return 0;
}

WebRtc_Word32
AudioMixerManagerIPhone::MicrophoneVolume(WebRtc_UWord32& volume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    return 0;
}

WebRtc_Word32
AudioMixerManagerIPhone::MaxMicrophoneVolume(WebRtc_UWord32& maxVolume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    // volume range is 0.0 to 1.0
    // we convert that to 0 - 255
    maxVolume = 255;

    return 0;
}

WebRtc_Word32
AudioMixerManagerIPhone::MinMicrophoneVolume(WebRtc_UWord32& minVolume) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    // volume range is 0.0 to 1.0
    // we convert that to 0 - 10
    minVolume = 0;

    return 0;
}

WebRtc_Word32
AudioMixerManagerIPhone::MicrophoneVolumeStepSize(WebRtc_UWord16& stepSize) const
{
    WEBRTC_TRACE(kTraceModuleCall, kTraceAudioDevice, _id,
                 "%s", __FUNCTION__);

    // volume range is 0.0 to 1.0
    // we convert that to 0 - 10
    stepSize = 1;

    return 0;
}

// ============================================================================
//                                 Private Methods
// ============================================================================

// CoreAudio errors are best interpreted as four character strings.
void AudioMixerManagerIPhone::logCAMsg(const TraceLevel level,
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

} // namespace webrtc
// EOF
