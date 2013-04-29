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

#include <cassert>
#include <string.h>
#include <errno.h>
#include <stdlib.h> //malloc

#include "audio_device_utility.h"
#include "audio_device_bb.h"
#include "audio_device_config.h"

#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"

#include <audio/audio_manager_device.h>
#include <audio/audio_manager_volume.h>

namespace webrtc
{
static const unsigned int ALSA_PLAYOUT_CH = 1;
static const unsigned int ALSA_CAPTURE_CH = 1;

//Blackberry defines
// Standard VoIP
static const unsigned int PREFERRED_FRAME_SIZE = 640;// 320 = 10ms
static const unsigned int VOIP_SAMPLE_RATE = 16000;

AudioDeviceBB::AudioDeviceBB(const WebRtc_Word32 id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _id(id),
    _ptrThreadRec(NULL),
    _ptrThreadPlay(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _inputDeviceIsSpecified(false),
    _outputDeviceIsSpecified(false),
    _handleRecord(NULL),
    _handlePlayout(NULL),
    _handleAudioManagerRecord(-1),
	_handleAudioManagerPlayout(-1),
    _recordingBufferSizeIn10MS(0),
    _playoutBufferSizeIn10MS(0),
    _recordingFramesIn10MS(0),
    _playoutFramesIn10MS(0),
    _recordingFreq(VOIP_SAMPLE_RATE),
    _playoutFreq(VOIP_SAMPLE_RATE),
    _recChannels(ALSA_CAPTURE_CH),
    _playChannels(ALSA_PLAYOUT_CH),
    _recFrameSize(0),
    _playFrameSize(0),
    _recordingBuffer(NULL),
    _playoutBuffer(NULL),
    _recordingFramesLeft(0),
    _playoutFramesLeft(0),
    _playBufType(AudioDeviceModule::kFixedBufferSize),
    _initialized(false),
    _recording(false),
    _playing(false),
    _wantRecording(false),
    _wantPlaying(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _micIsInitialized(false),
    _speakerIsInitialized(false),
    _playBufDelayFixed(80),
    _AGC(false),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _writeErrors(0)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id,
                 "%s created", __FUNCTION__);
}

// ----------------------------------------------------------------------------
//  AudioDeviceBB - dtor
// ----------------------------------------------------------------------------

AudioDeviceBB::~AudioDeviceBB()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id,
                 "%s destroyed", __FUNCTION__);
    
    Terminate();

    // Clean up the recording buffer and playout buffer.
    if (_recordingBuffer)
    {
        delete [] _recordingBuffer;
        _recordingBuffer = NULL;
    }
    if (_playoutBuffer)
    {
        delete [] _playoutBuffer;
        _playoutBuffer = NULL;
    }
    delete &_critSect;
}

void AudioDeviceBB::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    CriticalSectionScoped lock(&_critSect);

    _ptrAudioBuffer = audioBuffer;

    // Inform the AudioBuffer about default settings for this implementation.
    // Set all values to zero here since the actual settings will be done by
    // InitPlayout and InitRecording later.
    _ptrAudioBuffer->SetRecordingSampleRate(0);
    _ptrAudioBuffer->SetPlayoutSampleRate(0);
    _ptrAudioBuffer->SetRecordingChannels(0);
    _ptrAudioBuffer->SetPlayoutChannels(0);
}

WebRtc_Word32 AudioDeviceBB::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kPlatformDefaultAudio;
    return 0;
}

WebRtc_Word32 AudioDeviceBB::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    _playWarning = 0;
    _playError = 0;
    _recWarning = 0;
    _recError = 0;

    _initialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::Terminate()
{

    if (!_initialized)
    {
        return 0;
    }

    CriticalSectionScoped lock(&_critSect);

    // RECORDING
    if (_ptrThreadRec)
    {
        ThreadWrapper* tmpThread = _ptrThreadRec;
        _ptrThreadRec = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();

        if (tmpThread->Stop())
        {
            delete tmpThread;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the rec audio thread");
        }

        _critSect.Enter();
    }

    // PLAYOUT
    if (_ptrThreadPlay)
    {
        ThreadWrapper* tmpThread = _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();

        if (tmpThread->Stop())
        {
            delete tmpThread;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  failed to close down the play audio thread");
        }

        _critSect.Enter();
    }

    _initialized = false;
    _speakerIsInitialized = false;
    _micIsInitialized = false;
    _outputDeviceIsSpecified = false;
    _inputDeviceIsSpecified = false;

    return 0;
}

bool AudioDeviceBB::Initialized() const
{
    return (_initialized);
}

WebRtc_Word32 AudioDeviceBB::SpeakerIsAvailable(bool& available)
{

    available = true;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Not initialized");
        return -1;
    }

    if (_playing) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Cannot init speaker when playing");
        return -1;
    }

    if (!_outputDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Output device is not specified");
        return -1;
    }

    _speakerIsInitialized = true;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::MicrophoneIsAvailable(bool& available)
{

    available = true;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::InitMicrophone()
{

    CriticalSectionScoped lock(_critSect);

    if (!_initialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Not initialized");
        return -1;
    }

    if (_recording) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Cannot init mic when recording");
        return -1;
    }

    if (!_inputDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Input device is not specified");
        return -1;
    }

    _micIsInitialized = true;

    return 0;
}

bool AudioDeviceBB::SpeakerIsInitialized() const
{
    return _speakerIsInitialized;
}

bool AudioDeviceBB::MicrophoneIsInitialized() const
{
    return _micIsInitialized;
}

WebRtc_Word32 AudioDeviceBB::SpeakerVolumeIsAvailable(bool& available)
{

    available = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetSpeakerVolume(WebRtc_UWord32 volume)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::SpeakerVolume(WebRtc_UWord32& volume) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}


WebRtc_Word32 AudioDeviceBB::SetWaveOutVolume(WebRtc_UWord16 volumeLeft,
                                                     WebRtc_UWord16 volumeRight)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::WaveOutVolume(
    WebRtc_UWord16& /*volumeLeft*/,
    WebRtc_UWord16& /*volumeRight*/) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::MaxSpeakerVolume(
    WebRtc_UWord32& maxVolume) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::MinSpeakerVolume(
    WebRtc_UWord32& minVolume) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::SpeakerVolumeStepSize(
    WebRtc_UWord16& stepSize) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::SpeakerMuteIsAvailable(bool& available)
{

    available = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetSpeakerMute(bool enable)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::SpeakerMute(bool& enabled) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::MicrophoneMuteIsAvailable(bool& available)
{

    available = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetMicrophoneMute(bool enable)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

// ----------------------------------------------------------------------------
//  MicrophoneMute
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneMute(bool& enabled) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::MicrophoneBoostIsAvailable(bool& available)
{

    available = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetMicrophoneBoost(bool enable)
{

    if (!_micIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Microphone not initialized");
        return -1;
    }

    if (enable) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  SetMicrophoneBoost cannot be enabled on this platform");
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceBB::MicrophoneBoost(bool& enabled) const
{
    if (!_micIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Microphone not initialized");
        return -1;
    }

    enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::StereoRecordingIsAvailable(bool& available)
{

    available = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetStereoRecording(bool enable)
{

    if (enable) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " Stereo recording is not supported on this platform");
        return -1;
    }
    return 0;
}

WebRtc_Word32 AudioDeviceBB::StereoRecording(bool& enabled) const
{

    enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::StereoPlayoutIsAvailable(bool& available)
{

    available = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetStereoPlayout(bool enable)
{

    if (enable) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     " Stereo playout is not supported on this platform");
        return -1;
    }
    return 0;
}

WebRtc_Word32 AudioDeviceBB::StereoPlayout(bool& enabled) const
{

    enabled = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetAGC(bool enable)
{

    _AGC = enable;

    return 0;
}

bool AudioDeviceBB::AGC() const
{

    return _AGC;
}

WebRtc_Word32 AudioDeviceBB::MicrophoneVolumeIsAvailable(bool& available)
{

    available = false;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetMicrophoneVolume(WebRtc_UWord32 volume)
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::MicrophoneVolume(WebRtc_UWord32& volume) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::MaxMicrophoneVolume(
    WebRtc_UWord32& maxVolume) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::MinMicrophoneVolume(
    WebRtc_UWord32& minVolume) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::MicrophoneVolumeStepSize(
    WebRtc_UWord16& stepSize) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                 "  API call not supported on this platform");
    return -1;
}

WebRtc_Word16 AudioDeviceBB::PlayoutDevices()
{
    return (WebRtc_Word16)1;
}

WebRtc_Word32 AudioDeviceBB::SetPlayoutDevice(WebRtc_UWord16 index)
{

    if (_playIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Playout already initialized");
        return -1;
    }

    if (index !=0) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  SetPlayoutDevice invalid index");
        return -1;
    }
    _outputDeviceIsSpecified = true;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType /*device*/)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::PlayoutDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    if (index != 0) {
        return -1;
    }
    // return empty strings
    memset(name, 0, kAdmMaxDeviceNameSize);
    if (guid != NULL) {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}

WebRtc_Word32 AudioDeviceBB::RecordingDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    if (index != 0) {
        return -1;
    }
    // return empty strings
    memset(name, 0, kAdmMaxDeviceNameSize);
    if (guid != NULL) {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}

WebRtc_Word16 AudioDeviceBB::RecordingDevices()
{
    return (WebRtc_Word16)1;
}

WebRtc_Word32 AudioDeviceBB::SetRecordingDevice(WebRtc_UWord16 index)
{

    if (_recIsInitialized) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Recording already initialized");
        return -1;
    }

    if (index !=0) {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                     "  SetRecordingDevice invalid index");
        return -1;
    }

    _inputDeviceIsSpecified = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  SetRecordingDevice II (II)
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType /*device*/)
{
    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                 "WindowsDeviceType not supported");
    return -1;
}

WebRtc_Word32 AudioDeviceBB::PlayoutIsAvailable(bool& available)
{

    available = false;

    // Try to initialize the playout side
    WebRtc_Word32 res = InitPlayout();

    // Cancel effect of initialization
    StopPlayoutThread();

    if (res != -1) {
        available = true;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceBB::RecordingIsAvailable(bool& available)
{

    available = false;

    // Try to initialize the recording side
    WebRtc_Word32 res = InitRecording();

    // Cancel effect of initialization
    StopRecordingThread();

    if (res != -1) {
        available = true;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceBB::InitPlayout()
{
    int errVal = 0;

    CriticalSectionScoped lock(&_critSect);
    if (_playing)
    {
        return -1;
    }

    if (!_outputDeviceIsSpecified)
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

    // Start by closing any existing wave-output devices
    //
    if (_handlePlayout != NULL)
    {
        snd_pcm_close(_handlePlayout);
        _handlePlayout = NULL;
        _playIsInitialized = false;
        if (errVal < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Error closing current playout sound device, error:"
                         " %s", snd_strerror(errVal));
        }
    }

    // Open PCM device for playout
    errVal = audio_manager_snd_pcm_open_name(AUDIO_TYPE_VIDEO_CHAT,
    			&_handlePlayout, &_handleAudioManagerPlayout, (char*) "voice",//"/dev/snd/defaultp",
    			SND_PCM_OPEN_NONBLOCK | SND_PCM_OPEN_PLAYBACK);

    if (errVal == -EBUSY) // Device busy - try some more!
    {
        for (int i=0; i < 5; i++)
        {
            sleep(1);
            errVal = audio_manager_snd_pcm_open_name(AUDIO_TYPE_VIDEO_CHAT,
            			&_handlePlayout, &_handleAudioManagerPlayout, (char*) "voice", //"/dev/snd/defaultp",
            			SND_PCM_OPEN_NONBLOCK | SND_PCM_OPEN_PLAYBACK);
            if (errVal == 0)
            {
                break;
            }
        }
    }
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     unable to open playback device: %s (%d)",
                     snd_strerror(errVal),
                     errVal);
        _handlePlayout = NULL;
        return -1;
    }

//    audio_manager_device_t dev;
//    errVal = audio_manager_get_default_device(&dev);
//    errVal = audio_manager_set_output_level(dev, 100.0);

    errVal = snd_pcm_plugin_set_disable(_handlePlayout, PLUGIN_DISABLE_MMAP);
    if (errVal < 0)
    {
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"    snd_pcm_get_params %s",
    				snd_strerror(errVal),
    				errVal);
    	_handlePlayout = NULL;
    	return -1;
    }

	snd_pcm_channel_setup_t setup;
	snd_pcm_channel_info_t pi;
	snd_mixer_group_t group;
	snd_pcm_channel_params_t pp;

	memset(&pi, 0, sizeof(pi));
	pi.channel = SND_PCM_CHANNEL_PLAYBACK;
	errVal = snd_pcm_plugin_info(_handlePlayout, &pi);
    if (errVal < 0)
    {
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"    snd_pcm_plugin_info %s",
    				snd_strerror(errVal),
    				errVal);
    	_handlePlayout = NULL;
		return -1;
	}

	memset(&pp, 0, sizeof(pp));
	pp.mode = SND_PCM_MODE_BLOCK;
	pp.channel = SND_PCM_CHANNEL_PLAYBACK;
	pp.start_mode = SND_PCM_START_FULL;
	pp.stop_mode = SND_PCM_STOP_ROLLOVER;
	pp.buf.block.frag_size = PREFERRED_FRAME_SIZE;
	pp.buf.block.frags_max = 3;
	pp.buf.block.frags_min = 1;
	pp.format.interleave = 1;
	pp.format.rate = VOIP_SAMPLE_RATE;
	pp.format.voices = 1;
	pp.format.format = SND_PCM_SFMT_S16_LE;
	errVal = snd_pcm_plugin_params(_handlePlayout, &pp);
    if (errVal < 0)
    {
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"    snd_pcm_plugin_params %s",
    				snd_strerror(errVal),
    				errVal);
    	_handlePlayout = NULL;
		return -1;
	}

	memset(&setup, 0, sizeof(setup));
	memset(&group, 0, sizeof(group));
	setup.channel = SND_PCM_CHANNEL_PLAYBACK;
	setup.mixer_gid = &group.gid;
	errVal = snd_pcm_plugin_setup(_handlePlayout, &setup);
	if (errVal < 0)
	{
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"    snd_pcm_plugin_setup %s",
    				snd_strerror(errVal),
    				errVal);
    	_handlePlayout = NULL;
		return -1;
	}

    _playFrameSize = setup.buf.block.frag_size;

	if (group.gid.name[0] == 0)
	{
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"Mixer Pcm Group [%s] Not Set",
    				group.gid.name,
    				errVal);
    	_handlePlayout = NULL;
		return -1;
	}

    _playoutFramesIn10MS = _playoutFreq/100;

    if (_ptrAudioBuffer)
    {
        // Update webrtc audio buffer with the selected parameters
        _ptrAudioBuffer->SetPlayoutSampleRate(_playoutFreq);
        _ptrAudioBuffer->SetPlayoutChannels(_playChannels);
    }

    // Set play buffer size
    _playoutBufferSizeIn10MS = _playoutFramesIn10MS << 1;

    // Init varaibles used for play
    _playWarning = 0;
    _playError = 0;

    if (_handlePlayout != NULL)
    {
        _playIsInitialized = true;
        return 0;
    }
    else
    {
        return -1;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceBB::InitRecording()
{
	int errVal = 0;

	CriticalSectionScoped lock(&_critSect);

	if (_recording)
	{
		return -1;
	}

	if (!_inputDeviceIsSpecified)
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

	// Start by closing any existing pcm-input devices
	//
	if (_handleRecord != NULL)
	{
		errVal = snd_pcm_close(_handleRecord);
		_handleRecord = NULL;
		_recIsInitialized = false;
		if (errVal < 0)
		{
			WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
						 "     Error closing current recording sound device,"
						 " error: %s",
						 snd_strerror(errVal));
		}
	}

	errVal = audio_manager_snd_pcm_open_name(AUDIO_TYPE_VIDEO_CHAT,
					&_handleRecord, &_handleAudioManagerRecord, (char*) "voice" ,//"/dev/snd/defaultc",
					SND_PCM_OPEN_NONBLOCK | SND_PCM_OPEN_CAPTURE);

	if (errVal == -EBUSY) // Device busy - try some more!
	{
		for (int i=0; i < 5; i++)
		{
			sleep(1);
			errVal = audio_manager_snd_pcm_open_name(AUDIO_TYPE_VIDEO_CHAT,
						&_handleRecord, &_handleAudioManagerRecord, (char*) "voice", //"/dev/snd/defaultc",
						SND_PCM_OPEN_NONBLOCK | SND_PCM_OPEN_CAPTURE);
			if (errVal == 0)
			{
				break;
			}
		}
	}
	if (errVal < 0)
	{
		WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
					 "     unable to open capture device: %s (%d)",
					 snd_strerror(errVal),
					 errVal);
		_handleRecord = NULL;
		return -1;
	}

	errVal = snd_pcm_plugin_set_disable(_handleRecord, PLUGIN_DISABLE_MMAP);
	if (errVal < 0)
	{
		WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
					"    snd_pcm_get_params %s",
					snd_strerror(errVal),
					errVal);
		_handleRecord = NULL;
		return -1;
	}

	_recordingFramesIn10MS = _recordingFreq/100;

	snd_pcm_channel_setup_t setup;
	int ret;
	snd_pcm_channel_info_t pi;
	snd_mixer_group_t group;
	snd_pcm_channel_params_t pp;
	int card = setup.mixer_card;

	// sample reads the capabilities of the capture
	memset(&pi, 0, sizeof(pi));
	pi.channel = SND_PCM_CHANNEL_CAPTURE;
	if ((ret = snd_pcm_plugin_info(_handleRecord, &pi)) < 0) {
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"    snd_pcm_plugin_info %s",
    				snd_strerror(errVal),
    				errVal);
		return -1;
	}

	// Request the VoIP parameters
	// These parameters are different to waverec sample
	memset(&pp, 0, sizeof(pp));
	pp.mode = SND_PCM_MODE_BLOCK;
	pp.channel = SND_PCM_CHANNEL_CAPTURE;
	pp.start_mode = SND_PCM_START_FULL;
	// Auto-recover from errors
	pp.stop_mode = SND_PCM_STOP_ROLLOVER;
	pp.buf.block.frag_size = PREFERRED_FRAME_SIZE;
	pp.buf.block.frags_max = 1;
	pp.buf.block.frags_min = 1;
	pp.format.interleave = 1;
	pp.format.rate = VOIP_SAMPLE_RATE;
	pp.format.voices = 1;
	pp.format.format = SND_PCM_SFMT_S16_LE;
	// make the request
	if ((ret = snd_pcm_plugin_params(_handleRecord, &pp)) < 0) {
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"    snd_pcm_plugin_params %s",
    				snd_strerror(errVal),
    				errVal);
		return -1;
	}

	// Again based on the sample
	memset(&setup, 0, sizeof(setup));
	memset(&group, 0, sizeof(group));
	setup.channel = SND_PCM_CHANNEL_CAPTURE;
	setup.mixer_gid = &group.gid;
	if ((ret = snd_pcm_plugin_setup(_handleRecord, &setup)) < 0) {
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"    snd_pcm_plugin_setup %s",
    				snd_strerror(errVal),
    				errVal);
		return -1;
	}

	if (group.gid.name[0] == 0) {
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"Mixer Pcm Group [%s] Not Set",
    				group.gid.name,
    				errVal);
	}

	// frag_size should be 160
	_recFrameSize = setup.buf.block.frag_size;

	if (_ptrAudioBuffer)
	{
		// Update webrtc audio buffer with the selected parameters
		_ptrAudioBuffer->SetRecordingSampleRate(_recordingFreq);
		_ptrAudioBuffer->SetRecordingChannels(_recChannels);
	}

	// Set rec buffer size and create buffer
	_recordingBufferSizeIn10MS = _recordingFramesIn10MS << 1;

	if (_handleRecord != NULL)
	{
		// Mark recording side as initialized
		_recIsInitialized = true;
		return 0;
	}
	else
	{
		return -1;
	}
}

WebRtc_Word32 AudioDeviceBB::StartRecording()
{
	CriticalSectionScoped lock(&_critSect);

	bool startRecordPlayout = false;

	if (_wantRecording)
		return 0;
	_wantRecording = true;
	if (_wantPlaying)
		startRecordPlayout = true;

	if (startRecordPlayout)
	{
		WebRtc_Word32 res;
		res = StartPlayoutThread();
		if (res < 0)
			return res;
		res = StartRecordingThread();
		if (res < 0)
			return res;
	}

	return 0;
}

WebRtc_Word32 AudioDeviceBB::StopRecording()
{
	bool stopRecordPlayout = false;

	{
		CriticalSectionScoped lock(&_critSect);
		if (!_wantRecording)
			return 0;
		_wantRecording = false;
		if (!_wantPlaying)
			stopRecordPlayout = true;
	}

	if (stopRecordPlayout)
	{
		WebRtc_Word32 res;
		res = StopPlayoutThread();
		if (res < 0)
			return res;
		res = StopRecordingThread();
		if (res < 0)
			return res;
	}

	return 0;
}

bool AudioDeviceBB::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}

bool AudioDeviceBB::Recording() const
{
    return (_recording);
}

bool AudioDeviceBB::PlayoutIsInitialized() const
{
    return (_playIsInitialized);
}

WebRtc_Word32 AudioDeviceBB::StartPlayout()
{
	CriticalSectionScoped lock(&_critSect);

	bool startRecordPlayout = false;

	if (_wantPlaying)
		return 0;
	_wantPlaying = true;
	if (_wantRecording)
		startRecordPlayout = true;

	if (startRecordPlayout)
	{
		WebRtc_Word32 res;
		res = StartRecordingThread();
		if (res < 0)
			return res;
		res = StartPlayoutThread();
		if (res < 0)
			return res;
	}

	return 0;
}

WebRtc_Word32 AudioDeviceBB::StopPlayout()
{
	bool stopRecordPlayout = false;

	{
		CriticalSectionScoped lock(&_critSect);
		if (!_wantPlaying)
			return 0;
		_wantPlaying = false;
		if (!_wantRecording)
			stopRecordPlayout = true;
	}

	if (stopRecordPlayout)
	{
		WebRtc_Word32 res;
		res = StopRecordingThread();
		if (res < 0)
			return res;
		res = StopPlayoutThread();
		if (res < 0)
			return res;
	}

	return 0;
}

WebRtc_Word32 AudioDeviceBB::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    delayMS = 20;
    return 0;
}

WebRtc_Word32 AudioDeviceBB::RecordingDelay(WebRtc_UWord16& delayMS) const
{
    // Adding 20ms adjusted value to the record delay due to 20ms buffering.
    delayMS = 20;
    return 0;
}

bool AudioDeviceBB::Playing() const
{
    return (_playing);
}
// ----------------------------------------------------------------------------
//  SetPlayoutBuffer
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type,
    WebRtc_UWord16 sizeMS)
{
    _playBufType = type;
    if (type == AudioDeviceModule::kFixedBufferSize)
    {
        _playBufDelayFixed = sizeMS;
    }
    return 0;
}

WebRtc_Word32 AudioDeviceBB::PlayoutBuffer(
    AudioDeviceModule::BufferType& type,
    WebRtc_UWord16& sizeMS) const
{
    type = _playBufType;
    if (type == AudioDeviceModule::kFixedBufferSize)
    {
        sizeMS = _playBufDelayFixed;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceBB::CPULoad(WebRtc_UWord16& load) const
{

    WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
               "  API call not supported on this platform");
    return -1;
}

bool AudioDeviceBB::PlayoutWarning() const
{
    return (_playWarning > 0);
}

bool AudioDeviceBB::PlayoutError() const
{
    return (_playError > 0);
}

bool AudioDeviceBB::PlayoutRouteChanged() const
{
    return false;
}

bool AudioDeviceBB::RecordingWarning() const
{
    return (_recWarning > 0);
}

bool AudioDeviceBB::RecordingError() const
{
    return (_recError > 0);
}

void AudioDeviceBB::ClearPlayoutWarning()
{
    _playWarning = 0;
}

void AudioDeviceBB::ClearPlayoutError()
{
    _playError = 0;
}

void AudioDeviceBB::ClearPlayoutRouteChanged()
{
}

void AudioDeviceBB::ClearRecordingWarning()
{
    _recWarning = 0;
}

void AudioDeviceBB::ClearRecordingError()
{
    _recError = 0;
}

// ============================================================================
//                                  Thread Methods
// ============================================================================

WebRtc_Word32 AudioDeviceBB::StartRecordingThread()
{
    int errVal = 0;

	if (!_recIsInitialized)
	{
		return -1;
	}

	if (_recording)
	{
		return 0;
	}

	_recording = true;

	_recordingFramesLeft = _recordingFramesIn10MS;

	// Make sure we only create the buffer once.
	if (!_recordingBuffer)
		_recordingBuffer = new WebRtc_Word8[_recordingBufferSizeIn10MS];
	if (!_recordingBuffer)
	{
		WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
					 "   failed to alloc recording buffer");
		_recording = false;
		return -1;
	}

    // RECORDING
    const char* threadName = "webrtc_audio_module_capture_thread";
    _ptrThreadRec = ThreadWrapper::CreateThread(RecThreadFunc,
                                                this,
                                                kRealtimePriority,
                                                threadName);
    if (_ptrThreadRec == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the rec audio thread");
        _recording = false;
        delete [] _recordingBuffer;
        _recordingBuffer = NULL;
        return -1;
    }

    unsigned int threadID(0);
    if (!_ptrThreadRec->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the rec audio thread");
        _recording = false;
        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
        delete [] _recordingBuffer;
        _recordingBuffer = NULL;
        return -1;
    }
    _recThreadID = threadID;

	CriticalSectionScoped lock(&_critSect);

    errVal = snd_pcm_plugin_prepare(_handleRecord, SND_PCM_CHANNEL_CAPTURE);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     capture snd_pcm_prepare failed (%s)\n",
                     snd_strerror(errVal));
        // just log error
    }

    return 0;
}

WebRtc_Word32 AudioDeviceBB::StopRecordingThread()
{
    {
		CriticalSectionScoped lock(&_critSect);

		if (!_recIsInitialized)
		{
		  return 0;
		}

		if (_handleRecord == NULL)
		{
		  return -1;
		}

		// Make sure we don't start recording (it's asynchronous).
		_recIsInitialized = false;
		_micIsInitialized = false;
		_recording = false;
    }

    if (_ptrThreadRec && !_ptrThreadRec->Stop())
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    failed to stop the rec audio thread");
        return -1;
    }
    else {
        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
    }

    CriticalSectionScoped lock(&_critSect);
    _recordingFramesLeft = 0;
    if (_recordingBuffer)
    {
        delete [] _recordingBuffer;
        _recordingBuffer = NULL;
    }

    int errVal = snd_pcm_plugin_flush(_handleRecord, SND_PCM_CHANNEL_CAPTURE);
	 if (errVal < 0)
		 WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
					  "    Cannot flush pcm plugin, error: %s",
					  snd_strerror(errVal));

	 errVal = snd_pcm_close(_handleRecord);
	 if (errVal < 0)
		 WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
					  "    Error closing capture sound device, error: %s",
					  snd_strerror(errVal));

	 errVal = audio_manager_free_handle(_handleAudioManagerRecord);
	 if (errVal < 0)
		 WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
					  "    Cannot free audio manager handle, error: %s",
					  snd_strerror(errVal));

	 // set the pcm input handle to NULL
	 _recIsInitialized = false;
	 _micIsInitialized = false;
	 _handleRecord = NULL;
	 WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
				  "  handle_record is now set to NULL");

    // set the pcm input handle to NULL
    _handleRecord = NULL;

    return 0;
}

WebRtc_Word32 AudioDeviceBB::StartPlayoutThread()
{
	if (!_playIsInitialized)
	{
		return -1;
	}

	if (_playing)
	{
		return 0;
	}

	_playing = true;

	_playoutFramesLeft = 0;
	if (!_playoutBuffer)
		_playoutBuffer = new WebRtc_Word8[_playoutBufferSizeIn10MS << 1];
	if (!_playoutBuffer)
	{
	  WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
				   "    failed to alloc playout buf");
	  _playing = false;
	  return -1;
	}

    // PLAYOUT
    const char* threadName = "webrtc_audio_module_play_thread";
    _ptrThreadPlay =  ThreadWrapper::CreateThread(PlayThreadFunc,
                                                  this,
                                                  kRealtimePriority,
                                                  threadName);
    if (_ptrThreadPlay == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "    failed to create the play audio thread");
        _playing = false;
        delete [] _playoutBuffer;
        _playoutBuffer = NULL;
        return -1;
    }

    unsigned int threadID(0);
    if (!_ptrThreadPlay->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the play audio thread");
        _playing = false;
        delete _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        delete [] _playoutBuffer;
        _playoutBuffer = NULL;
        return -1;
    }
    _playThreadID = threadID;

	int errVal = snd_pcm_plugin_prepare(_handlePlayout, SND_PCM_CHANNEL_PLAYBACK);
	if (errVal < 0)
	{
    	WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
    				"    snd_pcm_plugin_prepare %s",
    				snd_strerror(errVal),
    				errVal);
		return -1;
	}

    return 0;
}

WebRtc_Word32 AudioDeviceBB::StopPlayoutThread()
{
    {
        CriticalSectionScoped lock(&_critSect);

        if (!_playIsInitialized)
        {
            return 0;
        }

        if (_handlePlayout == NULL)
        {
            return -1;
        }

        _playing = false;
    }

    // stop playout thread first
    if (_ptrThreadPlay && !_ptrThreadPlay->Stop())
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  failed to stop the play audio thread");
        return -1;
    }
    else {
        delete _ptrThreadPlay;
        _ptrThreadPlay = NULL;
    }

    CriticalSectionScoped lock(&_critSect);

    _playoutFramesLeft = 0;
    delete [] _playoutBuffer;
    _playoutBuffer = NULL;

     int errVal = snd_pcm_plugin_flush(_handlePlayout, SND_PCM_CHANNEL_PLAYBACK);
     if (errVal < 0)
         WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                      "    Cannot flush pcm plugin, error: %s",
                      snd_strerror(errVal));

     errVal = snd_pcm_close(_handlePlayout);
     if (errVal < 0)
         WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                      "    Error closing playout sound device, error: %s",
                      snd_strerror(errVal));

     errVal = audio_manager_free_handle(_handleAudioManagerPlayout);
     if (errVal < 0)
         WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                      "    Cannot free audio manager handle, error: %s",
                      snd_strerror(errVal));

     // set the pcm input handle to NULL
     _playIsInitialized = false;
     _speakerIsInitialized = false;
     _handlePlayout = NULL;
     WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                  "  handle_playout is now set to NULL");

     return 0;
}

bool AudioDeviceBB::PlayThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceBB*>(pThis)->PlayThreadProcess());
}

bool AudioDeviceBB::RecThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceBB*>(pThis)->RecThreadProcess());
}

bool AudioDeviceBB::PlayThreadProcess()
{
    if(!_playing)
        return false;

    int err;

    Lock();

    if (_playoutFramesLeft <= 0)
    {
    	_playoutFramesLeft = 0;
        for (int i = 0; i < 2; i++)
        {
			UnLock();
			_ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
			Lock();

			WebRtc_UWord32 playoutDataSize = _ptrAudioBuffer->GetPlayoutData(_playoutBuffer + i * (_playoutFramesIn10MS << 1));
			assert(playoutDataSize == _playoutFramesIn10MS);
			_playoutFramesLeft += playoutDataSize;
		}
    }

    int writeSize = _playoutFramesLeft << 1;
    int bytesWritten = snd_pcm_plugin_write(_handlePlayout,
    		&_playoutBuffer[(_playoutBufferSizeIn10MS << 1) - writeSize],
    		writeSize);
    if (bytesWritten < 0)
    {
        WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                     "       snd_pcm_plugin_write error: %s",
                     snd_strerror(bytesWritten));
        _playoutFramesLeft = 0;
        UnLock();
        return true;
    }
    else if (bytesWritten == 0)
    {
        UnLock();
    	usleep(1);
        return true;
    }
    else
    {
        _playoutFramesLeft -= bytesWritten >> 1;
        UnLock();
    }

    return true;
}

bool AudioDeviceBB::RecThreadProcess()
{
    if (!_recording)
        return false;

    int err;

    Lock();

    char *record_buffer;
    	record_buffer = (char*) malloc(_recordingBufferSizeIn10MS * 2);

    int frames = snd_pcm_plugin_read(_handleRecord,
    		record_buffer, _recordingBufferSizeIn10MS); // frames to be written
    if (frames == -EAGAIN)
    {
        free(record_buffer);
        UnLock();
    	usleep(1);
        return true;
    }
    else if (frames < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "capture snd_pcm_read error: %s",
                     snd_strerror(frames));
        free(record_buffer);
        UnLock();
        return true;
    }
    else if (frames > 0)
    {
        int left_size = _recordingFramesLeft << 1;
        int size = _recordingFramesLeft << 1;

        memcpy(&_recordingBuffer[_recordingBufferSizeIn10MS - left_size],
               record_buffer, size);
        _recordingFramesLeft -= frames >> 1;

		if (!_recordingFramesLeft)
		 { // buf is full
            _recordingFramesLeft = _recordingFramesIn10MS;

            // store the recorded buffer (no action will be taken if the
            // #recorded samples is not a full buffer)
            _ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
                                               _recordingFramesIn10MS);

            WebRtc_UWord32 currentMicLevel = 0;
            WebRtc_UWord32 newMicLevel = 0;

            if (AGC())
            {
                // store current mic level in the audio buffer if AGC is enabled
                if (MicrophoneVolume(currentMicLevel) == 0)
                {
                    if (currentMicLevel == 0xffffffff)
                        currentMicLevel = 100;
                    // this call does not affect the actual microphone volume
                    _ptrAudioBuffer->SetCurrentMicLevel(currentMicLevel);
                }
            }

            UnLock();
            _ptrAudioBuffer->DeliverRecordedData();
            Lock();

            if (AGC())
            {
                newMicLevel = _ptrAudioBuffer->NewMicLevel();
                if (newMicLevel != 0)
                {
                    // The VQE will only deliver non-zero microphone levels when a
                    // change is needed. Set this new mic level (received from the
                    // observer as return value in the callback).
                    if (SetMicrophoneVolume(newMicLevel) == -1)
                        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                                     "  the required modification of the "
                                     "microphone volume failed");
                }
            }
        }
    }
    free(record_buffer);
    UnLock();

    return true;
}

}  // namespace webrtc

