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

#ifndef WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_BB_H
#define WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_BB_H

#include <stdio.h>

#include "audio_device_defines.h"
#include "audio_device_generic.h"
#include "critical_section_wrapper.h"

//Blackbery includes
#include <sys/asoundlib.h>
#include <audio/audio_manager_routing.h>

namespace webrtc {
class ThreadWrapper;

class AudioDeviceBB : public AudioDeviceGeneric
{
public:
    AudioDeviceBB(const WebRtc_Word32 id);
    ~AudioDeviceBB();

    // Retrieve the currently utilized audio layer
    virtual WebRtc_Word32 ActiveAudioLayer(AudioDeviceModule::AudioLayer& audioLayer) const;

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
    virtual WebRtc_Word32 SetWaveOutVolume(
        WebRtc_UWord16 volumeLeft, WebRtc_UWord16 volumeRight);
    virtual WebRtc_Word32 WaveOutVolume(
        WebRtc_UWord16& volumeLeft, WebRtc_UWord16& volumeRight) const;

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
    virtual WebRtc_Word32 MicrophoneVolumeStepSize(
        WebRtc_UWord16& stepSize) const;

    // Speaker mute control
    virtual WebRtc_Word32 SpeakerMuteIsAvailable(bool& available);
    virtual WebRtc_Word32 SetSpeakerMute(bool enable);
    virtual WebRtc_Word32 SpeakerMute(bool& enabled) const;

    // Microphone mute control
    virtual WebRtc_Word32 MicrophoneMuteIsAvailable(bool& available);
    virtual WebRtc_Word32 SetMicrophoneMute(bool enable);
    virtual WebRtc_Word32 MicrophoneMute(bool& enabled) const;

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
    virtual WebRtc_Word32 SetPlayoutBuffer(
        const AudioDeviceModule::BufferType type, WebRtc_UWord16 sizeMS);
    virtual WebRtc_Word32 PlayoutBuffer(
        AudioDeviceModule::BufferType& type, WebRtc_UWord16& sizeMS) const;
    virtual WebRtc_Word32 PlayoutDelay(WebRtc_UWord16& delayMS) const;
    virtual WebRtc_Word32 RecordingDelay(WebRtc_UWord16& delayMS) const;

    // CPU load
    virtual WebRtc_Word32 CPULoad(WebRtc_UWord16& load) const;

    virtual bool PlayoutWarning() const;
    virtual bool PlayoutError() const;
    virtual bool RecordingWarning() const;
    virtual bool RecordingError() const;
    virtual void ClearPlayoutWarning();
    virtual void ClearPlayoutError();
    virtual void ClearRecordingWarning();
    virtual void ClearRecordingError();

    virtual void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer);

private:
    void Lock() { _critSect.Enter(); };
    void UnLock() { _critSect.Leave(); };

    static bool RecThreadFunc(void*);
    static bool PlayThreadFunc(void*);
    bool RecThreadProcess();
    bool PlayThreadProcess();

    AudioDeviceBuffer* _ptrAudioBuffer;
    CriticalSectionWrapper&	_critSect;
    WebRtc_Word32 _id;

    ThreadWrapper* _ptrThreadRec;
    ThreadWrapper* _ptrThreadPlay;
    WebRtc_UWord32 _recThreadID;
    WebRtc_UWord32 _playThreadID;

    bool _inputDeviceIsSpecified;
    bool _outputDeviceIsSpecified;

    snd_pcm_t* _handleRecord;
    snd_pcm_t* _handlePlayout;
    unsigned int _handleAudioManagerRecord;
    unsigned int _handleAudioManagerPlayout;

    ssize_t _recordingBufferSizeIn10MS;
    ssize_t _playoutBufferSizeIn10MS;
    WebRtc_UWord32 _recordingFramesIn10MS;
    WebRtc_UWord32 _playoutFramesIn10MS;

    WebRtc_UWord32 _recordingFreq;
    WebRtc_UWord32 _playoutFreq;
    WebRtc_UWord8 _recChannels;
    WebRtc_UWord8 _playChannels;

    WebRtc_UWord32 _recFrameSize;
    WebRtc_UWord32 _playFrameSize;

    WebRtc_Word8* _recordingBuffer; // in byte
    WebRtc_Word8* _playoutBuffer; // in byte
    WebRtc_UWord32 _recordingFramesLeft;
    WebRtc_UWord32 _playoutFramesLeft;

    AudioDeviceModule::BufferType _playBufType;

private:
    bool _initialized;
    bool _recording;
    bool _playing;
    bool _recIsInitialized;
    bool _playIsInitialized;
    bool _micIsInitialized;
    bool _speakerIsInitialized;

    WebRtc_UWord16 _playBufDelayFixed;            // fixed playback delay

    bool _AGC;

    // Errors and warnings count
	WebRtc_UWord16 _playWarning;
	WebRtc_UWord16 _playError;
	WebRtc_UWord16 _recWarning;
	WebRtc_UWord16 _recError;

	WebRtc_UWord16 _writeErrors;
};

}  // namespace webrtc

#endif  // WEBRTC_AUDIO_DEVICE_AUDIO_DEVICE_BB_H
