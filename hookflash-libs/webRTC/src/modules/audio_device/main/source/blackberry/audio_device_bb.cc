/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cassert>
#include <string.h>

#include "audio_device_utility.h"
#include "audio_device_bb.h"
#include "audio_device_config.h"

#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"

// needed for Blackberry
//#include <sys/asoundlib.h>
//#include <audio/audio_manager_routing.h>
#include <stdio.h> // fprintf
#include <stdlib.h> //malloc

// snd_lib_error_handler_t
void WebrtcAlsaErrorHandler(const char *file,
                          int line,
                          const char *function,
                          int err,
                          const char *fmt,...){};

namespace webrtc
{
static const unsigned int ALSA_PLAYOUT_FREQ = 48000;
static const unsigned int ALSA_PLAYOUT_CH = 1;
static const unsigned int ALSA_PLAYOUT_LATENCY = 40*1000; // in us
static const unsigned int ALSA_CAPTURE_FREQ = 48000;
static const unsigned int ALSA_CAPTURE_CH = 2;
static const unsigned int ALSA_CAPTURE_LATENCY = 40*1000; // in us
static const unsigned int ALSA_PLAYOUT_WAIT_TIMEOUT = 5; // in ms
static const unsigned int ALSA_CAPTURE_WAIT_TIMEOUT = 5; // in ms

#define FUNC_GET_NUM_OF_DEVICE 0
#define FUNC_GET_DEVICE_NAME 1
#define FUNC_GET_DEVICE_NAME_FOR_AN_ENUM 2

//Blackberry defines
#define JITTER_BUFFER_NUMBER_FRAMES 20;
// Standard VoIP
#define PREFERRED_FRAME_SIZE 320;//640; 320 = 10ms
#define VOIP_SAMPLE_RATE 16000;
// ulaw silence is FF
#define SILENCE 0xFF;


AudioDeviceBB::AudioDeviceBB(const WebRtc_Word32 id) :
    _ptrAudioBuffer(NULL),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _ptrThreadRec(NULL),
    _ptrThreadPlay(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _id(id),
    _inputDeviceIndex(0),
    _outputDeviceIndex(0),
    _inputDeviceIsSpecified(false),
    _outputDeviceIsSpecified(false),
//    _handleRecord(NULL),
//    _handlePlayout(NULL),
//    _recordingBuffersizeInFrame(0),
//    _recordingPeriodSizeInFrame(0),
//    _playoutBufferSizeInFrame(0),
//    _playoutPeriodSizeInFrame(0),
//    _recordingBufferSizeIn10MS(0),
//    _playoutBufferSizeIn10MS(0),
    _recordingFramesIn10MS(0),
    _playoutFramesIn10MS(0),
    _recordingFreq(ALSA_CAPTURE_FREQ),
    _playoutFreq(ALSA_PLAYOUT_FREQ),
    _recChannels(ALSA_CAPTURE_CH),
    _playChannels(ALSA_PLAYOUT_CH),
    _recordingBuffer(NULL),
    _playoutBuffer(NULL),
    _recordingFramesLeft(0),
    _playoutFramesLeft(0),
    _playbackBufferSize(0),
    _playBufType(AudioDeviceModule::kFixedBufferSize),
    _initialized(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
    _recordingDeviceIsSpecified(false),
    _playoutDeviceIsSpecified(false),
    _micIsInitialized(false),
    _speakerIsInitialized(false),
    _AGC(false),
//    _recordingDelay(0),
//    _playoutDelay(0),
    _writeErrors(0),
    _playWarning(0),
    _playError(0),
    _recWarning(0),
    _recError(0),
    _playBufDelay(80),
    _playBufDelayFixed(80),

    g_pcm_handle_c(NULL),
    g_pcm_handle_p(NULL),
    g_audio_manager_handle_c(NULL),
    g_audio_manager_handle_p(0),
    g_frame_size_c(0),
    g_frame_size_p(0),
    capture_ready(false),
    g_execute_audio(true)
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

    if (!_playoutDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Playout device is not specified");
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

    if (!_recordingDeviceIsSpecified) {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice,
                     _id, "  Recording device is not specified");
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
    _playoutDeviceIsSpecified = true;

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

    _recordingDeviceIsSpecified = true;

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
    StopPlayout();

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
    StopRecording();

    if (res != -1) {
        available = true;
    }

    return 0;
}

WebRtc_Word32 AudioDeviceBB::InitPlayout()
{
#if 0
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
        LATE(snd_pcm_close)(_handlePlayout);
        _handlePlayout = NULL;
        _playIsInitialized = false;
        if (errVal < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "  Error closing current playout sound device, error:"
                         " %s", LATE(snd_strerror)(errVal));
        }
    }

    // Open PCM device for playout
    char deviceName[kAdmMaxDeviceNameSize] = {0};
    GetDevicesInfo(2, true, _outputDeviceIndex, deviceName,
                   kAdmMaxDeviceNameSize);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "  InitPlayout open (%s)", deviceName);

    errVal = LATE(snd_pcm_open)
                 (&_handlePlayout,
                  deviceName,
                  SND_PCM_STREAM_PLAYBACK,
                  SND_PCM_NONBLOCK);

    if (errVal == -EBUSY) // Device busy - try some more!
    {
        for (int i=0; i < 5; i++)
        {
            sleep(1);
            errVal = LATE(snd_pcm_open)
                         (&_handlePlayout,
                          deviceName,
                          SND_PCM_STREAM_PLAYBACK,
                          SND_PCM_NONBLOCK);
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
                     LATE(snd_strerror)(errVal),
                     errVal);
        _handlePlayout = NULL;
        return -1;
    }

    _playoutFramesIn10MS = _playoutFreq/100;
    if ((errVal = LATE(snd_pcm_set_params)( _handlePlayout,
#if defined(WEBRTC_BIG_ENDIAN)
        SND_PCM_FORMAT_S16_BE,
#else
        SND_PCM_FORMAT_S16_LE, //format
#endif
        SND_PCM_ACCESS_RW_INTERLEAVED, //access
        _playChannels, //channels
        _playoutFreq, //rate
        1, //soft_resample
        ALSA_PLAYOUT_LATENCY //40*1000 //latency required overall latency in us
    )) < 0)
    {   /* 0.5sec */
        _playoutFramesIn10MS = 0;
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     unable to set playback device: %s (%d)",
                     LATE(snd_strerror)(errVal),
                     errVal);
        ErrorRecovery(errVal, _handlePlayout);
        errVal = LATE(snd_pcm_close)(_handlePlayout);
        _handlePlayout = NULL;
        return -1;
    }

    errVal = LATE(snd_pcm_get_params)(_handlePlayout,
        &_playoutBufferSizeInFrame, &_playoutPeriodSizeInFrame);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    snd_pcm_get_params %s",
                     LATE(snd_strerror)(errVal),
                     errVal);
        _playoutBufferSizeInFrame = 0;
        _playoutPeriodSizeInFrame = 0;
    }
    else {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "    playout snd_pcm_get_params "
                     "buffer_size:%d period_size :%d",
                     _playoutBufferSizeInFrame, _playoutPeriodSizeInFrame);
    }

    if (_ptrAudioBuffer)
    {
        // Update webrtc audio buffer with the selected parameters
        _ptrAudioBuffer->SetPlayoutSampleRate(_playoutFreq);
        _ptrAudioBuffer->SetPlayoutChannels(_playChannels);
    }

    // Set play buffer size
    _playoutBufferSizeIn10MS = LATE(snd_pcm_frames_to_bytes)(
        _handlePlayout, _playoutFramesIn10MS);

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
#endif
    return 0;
}

WebRtc_Word32 AudioDeviceBB::InitRecording()
{
	snd_pcm_channel_setup_t setup;
	int ret;
	snd_pcm_channel_info_t pi;
	snd_mixer_group_t group;
	snd_pcm_channel_params_t pp;
	int card = setup.mixer_card;



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




	audio_manager_snd_pcm_open_name(AUDIO_TYPE_VIDEO_CHAT, &g_pcm_handle_c,
			&g_audio_manager_handle_c, (char*) "/dev/snd/defaultc", SND_PCM_OPEN_CAPTURE);

	if ((ret = snd_pcm_plugin_set_disable(g_pcm_handle_c, PLUGIN_DISABLE_MMAP))
			< 0) {
		fprintf(stderr, "snd_pcm_plugin_set_disable failed: %s\n", snd_strerror (ret));
		return -1;
	}

	if ((ret = snd_pcm_plugin_set_enable(g_pcm_handle_c, PLUGIN_ROUTING)) < 0) {
		fprintf(stderr, "snd_pcm_plugin_set_enable: %s\n", snd_strerror (ret));
		return -1;
	}

	// sample reads the capabilities of the capture
	memset(&pi, 0, sizeof(pi));
	pi.channel = SND_PCM_CHANNEL_CAPTURE;
	if ((ret = snd_pcm_plugin_info(g_pcm_handle_c, &pi)) < 0) {
		fprintf(stderr, "snd_pcm_plugin_info failed: %s\n", snd_strerror (ret));
		return -1;
	}

	fprintf(stderr,"CAPTURE Minimum Rate = %d\n",pi.min_rate);

	// Request the VoIP parameters
	// These parameters are different to waverec sample
	memset(&pp, 0, sizeof(pp));
	fprintf(stderr,"CAPTURE Minimum fragment size = %d\n",pi.min_fragment_size);
	// Blocking read
	pp.mode = SND_PCM_MODE_BLOCK;
	pp.channel = SND_PCM_CHANNEL_CAPTURE;
	pp.start_mode = SND_PCM_START_DATA;
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
	if ((ret = snd_pcm_plugin_params(g_pcm_handle_c, &pp)) < 0) {
		fprintf(stderr, "snd_pcm_plugin_params failed: %s\n", snd_strerror (ret));
		return -1;
	}

	// Again based on the sample
	memset(&setup, 0, sizeof(setup));
	memset(&group, 0, sizeof(group));
	setup.channel = SND_PCM_CHANNEL_CAPTURE;
	setup.mixer_gid = &group.gid;
	if ((ret = snd_pcm_plugin_setup(g_pcm_handle_c, &setup)) < 0) {
		fprintf(stderr, "snd_pcm_plugin_setup failed: %s\n", snd_strerror (ret));
		return -1;
	}
	// On the simulator at least, our requested capabilities are accepted.
	fprintf(stderr,"CAPTURE Format %s card = %d\n", snd_pcm_get_format_name (setup.format.format),card);
	fprintf(stderr,"CAPTURE Rate %d \n", setup.format.rate);
	g_frame_size_c = setup.buf.block.frag_size;

	if (group.gid.name[0] == 0) {
		printf("Mixer Pcm Group [%s] Not Set \n", group.gid.name);
		printf("***>>>> Input Gain Controls Disabled <<<<*** \n");
	} else {
		printf("Mixer Pcm Group [%s]\n", group.gid.name);
	}

	// frag_size should be 160
	g_frame_size_c = setup.buf.block.frag_size;
	fprintf(stderr, "CAPTURE frame_size = %d\n", g_frame_size_c);

	// Sample calls prepare()

	//moved to start method to be compliant to alsa example
	if ((ret = snd_pcm_plugin_prepare(g_pcm_handle_c, SND_PCM_CHANNEL_CAPTURE))
			< 0) {
		fprintf(stderr, "snd_pcm_plugin_prepare failed: %s\n", snd_strerror (ret));
	}


#if 0
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
        int errVal = LATE(snd_pcm_close)(_handleRecord);
        _handleRecord = NULL;
        _recIsInitialized = false;
        if (errVal < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "     Error closing current recording sound device,"
                         " error: %s",
                         LATE(snd_strerror)(errVal));
        }
    }

    // Open PCM device for recording
    // The corresponding settings for playout are made after the record settings
    char deviceName[kAdmMaxDeviceNameSize] = {0};
    GetDevicesInfo(2, false, _inputDeviceIndex, deviceName,
                   kAdmMaxDeviceNameSize);

    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                 "InitRecording open (%s)", deviceName);
    errVal = LATE(snd_pcm_open)
                 (&_handleRecord,
                  deviceName,
                  SND_PCM_STREAM_CAPTURE,
                  SND_PCM_NONBLOCK);

    // Available modes: 0 = blocking, SND_PCM_NONBLOCK, SND_PCM_ASYNC
    if (errVal == -EBUSY) // Device busy - try some more!
    {
        for (int i=0; i < 5; i++)
        {
            sleep(1);
            errVal = LATE(snd_pcm_open)
                         (&_handleRecord,
                          deviceName,
                          SND_PCM_STREAM_CAPTURE,
                          SND_PCM_NONBLOCK);
            if (errVal == 0)
            {
                break;
            }
        }
    }
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    unable to open record device: %s",
                     LATE(snd_strerror)(errVal));
        _handleRecord = NULL;
        return -1;
    }

    _recordingFramesIn10MS = _recordingFreq/100;
    if ((errVal = LATE(snd_pcm_set_params)(_handleRecord,
#if defined(WEBRTC_BIG_ENDIAN)
        SND_PCM_FORMAT_S16_BE, //format
#else
        SND_PCM_FORMAT_S16_LE, //format
#endif
        SND_PCM_ACCESS_RW_INTERLEAVED, //access
        _recChannels, //channels
        _recordingFreq, //rate
        1, //soft_resample
        ALSA_CAPTURE_LATENCY //latency in us
    )) < 0)
    {
         // Fall back to another mode then.
         if (_recChannels == 1)
           _recChannels = 2;
         else
           _recChannels = 1;

         if ((errVal = LATE(snd_pcm_set_params)(_handleRecord,
#if defined(WEBRTC_BIG_ENDIAN)
             SND_PCM_FORMAT_S16_BE, //format
#else
             SND_PCM_FORMAT_S16_LE, //format
#endif
             SND_PCM_ACCESS_RW_INTERLEAVED, //access
             _recChannels, //channels
             _recordingFreq, //rate
             1, //soft_resample
             ALSA_CAPTURE_LATENCY //latency in us
         )) < 0)
         {
             _recordingFramesIn10MS = 0;
             WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                          "    unable to set record settings: %s (%d)",
                          LATE(snd_strerror)(errVal), errVal);
             ErrorRecovery(errVal, _handleRecord);
             errVal = LATE(snd_pcm_close)(_handleRecord);
             _handleRecord = NULL;
             return -1;
         }
    }

    errVal = LATE(snd_pcm_get_params)(_handleRecord,
        &_recordingBuffersizeInFrame, &_recordingPeriodSizeInFrame);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    snd_pcm_get_params %s",
                     LATE(snd_strerror)(errVal), errVal);
        _recordingBuffersizeInFrame = 0;
        _recordingPeriodSizeInFrame = 0;
    }
    else {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                     "    capture snd_pcm_get_params "
                     "buffer_size:%d period_size:%d",
                     _recordingBuffersizeInFrame, _recordingPeriodSizeInFrame);
    }

    if (_ptrAudioBuffer)
    {
        // Update webrtc audio buffer with the selected parameters
        _ptrAudioBuffer->SetRecordingSampleRate(_recordingFreq);
        _ptrAudioBuffer->SetRecordingChannels(_recChannels);
    }

    // Set rec buffer size and create buffer
    _recordingBufferSizeIn10MS = LATE(snd_pcm_frames_to_bytes)(
        _handleRecord, _recordingFramesIn10MS);

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
#endif
    return 0;
}

WebRtc_Word32 AudioDeviceBB::StartRecording()
{
	// Re-usable buffer for capture
	char *record_buffer;
	record_buffer = (char*) malloc(g_frame_size_c);

	// Some diagnostic variables
	int failed = 0;
	int totalRead = 0;
	snd_pcm_channel_status_t status;
	status.channel = SND_PCM_CHANNEL_CAPTURE;

	// Loop until stopAudio() flags us
	while (g_execute_audio) {
		// This blocking read appears to take much longer than 20ms on the simulator
		// but it never fails and always returns 160 bytes
		int read = snd_pcm_plugin_read(g_pcm_handle_c, record_buffer,
				g_frame_size_c);
		if (read < 0 || read != g_frame_size_c) {
			failed++;
			fprintf(stderr,"CAPTURE FAILURE: snd_pcm_plugin_read: %d requested = %d\n",read,g_frame_size_c);
			if (snd_pcm_plugin_status(g_pcm_handle_c, &status) < 0) {
				fprintf(stderr, "Capture channel status error: %d\n",status.status);
			} else {
				if (status.status == SND_PCM_STATUS_READY
				|| status.status == SND_PCM_STATUS_OVERRUN
				|| status.status == SND_PCM_STATUS_ERROR) {
					fprintf(stderr, "CAPTURE FAILURE:snd_pcm_plugin_status: = %d \n",status.status);
					if (snd_pcm_plugin_prepare (g_pcm_handle_c, SND_PCM_CHANNEL_CAPTURE) < 0) {
						fprintf (stderr, "Capture channel prepare error %d\n",status.status);
						exit (1);
					}
				}
			}
		} else {
			totalRead += read;
		}
		capture_ready = true;
		// On simulator always room in the circular buffer
//		if (!writeToCircularBuffer(circular_buffer, record_buffer,
//				g_frame_size_c)) {
//			failed++;
//		}
	}
#if 0
    if (!_recIsInitialized)
    {
        return -1;
    }

    if (_recording)
    {
        return 0;
    }

    _recording = true;

    int errVal = 0;
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

    errVal = LATE(snd_pcm_prepare)(_handleRecord);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     capture snd_pcm_prepare failed (%s)\n",
                     LATE(snd_strerror)(errVal));
        // just log error
        // if snd_pcm_open fails will return -1
    }

    errVal = LATE(snd_pcm_start)(_handleRecord);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     capture snd_pcm_start err: %s",
                     LATE(snd_strerror)(errVal));
        errVal = LATE(snd_pcm_start)(_handleRecord);
        if (errVal < 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "     capture snd_pcm_start 2nd try err: %s",
                         LATE(snd_strerror)(errVal));
            StopRecording();
            return -1;
        }
    }
#endif
    return 0;
}

WebRtc_Word32 AudioDeviceBB::StopRecording()
{
	{
	  CriticalSectionScoped lock(&_critSect);

	  if (!_recIsInitialized)
	  {
		  return 0;
	  }

//	  if (_handleRecord == NULL)
//	  {
//		  return -1;
//	  }

	  // Make sure we don't start recording (it's asynchronous).
	  _recIsInitialized = false;
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

	// Threads will see this flag every 20ms in their loop
	fprintf(stderr,"\nStopPCMAudio ****************: ENTER \n");
	g_execute_audio = false;


	fprintf(stderr,"CAPTURE EXIT BEGIN\n");
	(void) snd_pcm_plugin_flush(g_pcm_handle_c, SND_PCM_CHANNEL_CAPTURE);
	//(void)snd_mixer_close (mixer_handle);
	(void) snd_pcm_close(g_pcm_handle_c);
	audio_manager_free_handle(g_audio_manager_handle_c);
	// IMPORTANT NB: You only get failed on capture if the play loop has exited hence the circular buffer fills. This is with the simulator

#if 0
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

    // Stop and close pcm recording device.
    int errVal = LATE(snd_pcm_drop)(_handleRecord);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     Error stop recording: %s",
                     LATE(snd_strerror)(errVal));
        return -1;
    }

    errVal = LATE(snd_pcm_close)(_handleRecord);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "     Error closing record sound device, error: %s",
                     LATE(snd_strerror)(errVal));
        return -1;
    }

    // Check if we have muted and unmute if so.
    bool muteEnabled = false;
    MicrophoneMute(muteEnabled);
    if (muteEnabled)
    {
        SetMicrophoneMute(false);
    }

    // set the pcm input handle to NULL
    _handleRecord = NULL;
#endif
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
#if 0
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
        _playoutBuffer = new WebRtc_Word8[_playoutBufferSizeIn10MS];
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

    int errVal = LATE(snd_pcm_prepare)(_handlePlayout);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "     playout snd_pcm_prepare failed (%s)\n",
                     LATE(snd_strerror)(errVal));
        // just log error
        // if snd_pcm_open fails will return -1
    }
#endif
    return 0;
}

WebRtc_Word32 AudioDeviceBB::StopPlayout()
{
#if 0
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

    // stop and close pcm playout device
    int errVal = LATE(snd_pcm_drop)(_handlePlayout);
    if (errVal < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "    Error stop playing: %s",
                     LATE(snd_strerror)(errVal));
    }

    errVal = LATE(snd_pcm_close)(_handlePlayout);
     if (errVal < 0)
         WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                      "    Error closing playout sound device, error: %s",
                      LATE(snd_strerror)(errVal));

     // set the pcm input handle to NULL
     _playIsInitialized = false;
     _handlePlayout = NULL;
     WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                  "  handle_playout is now set to NULL");
#endif
     return 0;
}

WebRtc_Word32 AudioDeviceBB::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    delayMS = 0; /*(WebRtc_UWord16)_playoutDelay * 1000 / _playoutFreq;*/
    return 0;
}

WebRtc_Word32 AudioDeviceBB::RecordingDelay(WebRtc_UWord16& delayMS) const
{
    // Adding 10ms adjusted value to the record delay due to 10ms buffering.
    delayMS = 0; /*(WebRtc_UWord16)(10 + _recordingDelay * 1000 / _recordingFreq);*/
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
    else
    {
        sizeMS = _playBufDelay; 
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

void AudioDeviceBB::ClearRecordingWarning()
{
    _recWarning = 0;
}

void AudioDeviceBB::ClearRecordingError()
{
    _recError = 0;
}

// ============================================================================
//                                 Private Methods
// ============================================================================
#if 0
WebRtc_Word32 AudioDeviceBB::GetDevicesInfo(
    const WebRtc_Word32 function,
    const bool playback,
    const WebRtc_Word32 enumDeviceNo,
    char* enumDeviceName,
    const WebRtc_Word32 ednLen) const
{
    
    // Device enumeration based on libjingle implementation
    // by Tristan Schmelcher at Google Inc.

    const char *type = playback ? "Output" : "Input";
    // dmix and dsnoop are only for playback and capture, respectively, but ALSA
    // stupidly includes them in both lists.
    const char *ignorePrefix = playback ? "dsnoop:" : "dmix:" ;
    // (ALSA lists many more "devices" of questionable interest, but we show them
    // just in case the weird devices may actually be desirable for some
    // users/systems.)

    int err;
    int enumCount(0);
    bool keepSearching(true);

    void **hints;
    err = LATE(snd_device_name_hint)(-1,     // All cards
                                     "pcm",  // Only PCM devices
                                     &hints);
    if (err != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "GetDevicesInfo - device name hint error: %s",
                     LATE(snd_strerror)(err));
        return -1;
    }

    enumCount++; // default is 0
    if (function == FUNC_GET_DEVICE_NAME && enumDeviceNo == 0)
    {
        strcpy(enumDeviceName, "default");
        return 0;
    }
    if (function == FUNC_GET_DEVICE_NAME_FOR_AN_ENUM && enumDeviceNo == 0)
    {
        strcpy(enumDeviceName, "default");
        return 0;
    }

    for (void **list = hints; *list != NULL; ++list)
    {
        char *actualType = LATE(snd_device_name_get_hint)(*list, "IOID");
        if (actualType)
        {   // NULL means it's both.
            bool wrongType = (strcmp(actualType, type) != 0);
            free(actualType);
            if (wrongType)
            {
                // Wrong type of device (i.e., input vs. output).
                continue;
            }
        }

        char *name = LATE(snd_device_name_get_hint)(*list, "NAME");
        if (!name)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                         "Device has no name");
            // Skip it.
            continue;
        }

        // Now check if we actually want to show this device.
        if (strcmp(name, "default") != 0 &&
            strcmp(name, "null") != 0 &&
            strcmp(name, "pulse") != 0 &&
            strncmp(name, ignorePrefix, strlen(ignorePrefix)) != 0)
        {
            // Yes, we do.
            char *desc = LATE(snd_device_name_get_hint)(*list, "DESC");
            if (!desc)
            {
                // Virtual devices don't necessarily have descriptions.
                // Use their names instead
                desc = name;
            }

            if (FUNC_GET_NUM_OF_DEVICE == function)
            {
                WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                             "    Enum device %d - %s", enumCount, name);

            }
            if ((FUNC_GET_DEVICE_NAME == function) &&
                (enumDeviceNo == enumCount))
            {

                // We have found the enum device, copy the name to buffer
                strncpy(enumDeviceName, desc, ednLen);
                enumDeviceName[ednLen-1] = '\0';
                keepSearching = false;
                // replace '\n' with '-'
                char * pret = strchr(enumDeviceName, '\n'/*0xa*/); //LF
                if (pret)
                    *pret = '-';
            }
            if ((FUNC_GET_DEVICE_NAME_FOR_AN_ENUM == function) &&
                (enumDeviceNo == enumCount))
            {
                // We have found the enum device, copy the name to buffer
                strncpy(enumDeviceName, name, ednLen);
                enumDeviceName[ednLen-1] = '\0';
                keepSearching = false;
            }
            if (keepSearching)
            {
                ++enumCount;
            }

            if (desc != name)
            {
                free(desc);
            }
        }

        free(name);

        if (!keepSearching)
        {
            break;
        }
    }

    err = LATE(snd_device_name_free_hint)(hints);
    if (err != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "GetDevicesInfo - device name free hint error: %s",
                     LATE(snd_strerror)(err));
        // Continue and return true anyways, since we did get the whole list.
    }

    if (FUNC_GET_NUM_OF_DEVICE == function)
    {
        if (enumCount == 1) // only default?
            enumCount = 0;
        return enumCount; // Normal return point for function 0
    }

    if (keepSearching)
    {
        // If we get here for function 1 and 2, we didn't find the specified
        // enum device
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "GetDevicesInfo - Could not find device name or numbers");
        return -1;
    }

    return 0;
}
#endif
#if 0
WebRtc_Word32 AudioDeviceBB::InputSanityCheckAfterUnlockedPeriod() const
{
    if (_handleRecord == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  input state has been modified during unlocked period");
        return -1;
    }
    return 0;
}
#endif
#if 0
WebRtc_Word32 AudioDeviceBB::OutputSanityCheckAfterUnlockedPeriod() const
{
    if (_handlePlayout == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  output state has been modified during unlocked period");
        return -1;
    }
    return 0;
}
#endif
#if 0
WebRtc_Word32 AudioDeviceBB::ErrorRecovery(WebRtc_Word32 error,
                                                  snd_pcm_t* deviceHandle)
{
    int st = LATE(snd_pcm_state)(deviceHandle);
    WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
               "Trying to recover from error: %s (%d) (state %d)",
               (LATE(snd_pcm_stream)(deviceHandle) == SND_PCM_STREAM_CAPTURE) ?
                   "capture" : "playout", LATE(snd_strerror)(error), error, st);

    // It is recommended to use snd_pcm_recover for all errors. If that function
    // cannot handle the error, the input error code will be returned, otherwise
    // 0 is returned. From snd_pcm_recover API doc: "This functions handles
    // -EINTR (4) (interrupted system call), -EPIPE (32) (playout overrun or
    // capture underrun) and -ESTRPIPE (86) (stream is suspended) error codes
    // trying to prepare given stream for next I/O."

    /** Open */
    //    SND_PCM_STATE_OPEN = 0,
    /** Setup installed */
    //    SND_PCM_STATE_SETUP,
    /** Ready to start */
    //    SND_PCM_STATE_PREPARED,
    /** Running */
    //    SND_PCM_STATE_RUNNING,
    /** Stopped: underrun (playback) or overrun (capture) detected */
    //    SND_PCM_STATE_XRUN,= 4
    /** Draining: running (playback) or stopped (capture) */
    //    SND_PCM_STATE_DRAINING,
    /** Paused */
    //    SND_PCM_STATE_PAUSED,
    /** Hardware is suspended */
    //    SND_PCM_STATE_SUSPENDED,
    //  ** Hardware is disconnected */
    //    SND_PCM_STATE_DISCONNECTED,
    //    SND_PCM_STATE_LAST = SND_PCM_STATE_DISCONNECTED

    // snd_pcm_recover isn't available in older alsa, e.g. on the FC4 machine
    // in Sthlm lab.

    int res = LATE(snd_pcm_recover)(deviceHandle, error, 1);
    if (0 == res)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceAudioDevice, _id,
                   "    Recovery - snd_pcm_recover OK");

        if ((error == -EPIPE || error == -ESTRPIPE) && // Buf underrun/overrun.
            _recording &&
            LATE(snd_pcm_stream)(deviceHandle) == SND_PCM_STREAM_CAPTURE)
        {
            // For capture streams we also have to repeat the explicit start()
            // to get data flowing again.
            int err = LATE(snd_pcm_start)(deviceHandle);
            if (err != 0)
            {
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                             "  Recovery - snd_pcm_start error: %u", err);
                return -1;
            }
        }

        if ((error == -EPIPE || error == -ESTRPIPE) &&  // Buf underrun/overrun.
            _playing &&
            LATE(snd_pcm_stream)(deviceHandle) == SND_PCM_STREAM_PLAYBACK)
        {
            // For capture streams we also have to repeat the explicit start() to get
            // data flowing again.
            int err = LATE(snd_pcm_start)(deviceHandle);
            if (err != 0)
            {
              WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                       "    Recovery - snd_pcm_start error: %s",
                       LATE(snd_strerror)(err));
              return -1;
            }
        }

        return -EPIPE == error ? 1 : 0;
    }
    else {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "  Terriable, it shouldn't happen");
    }

    return res;
}
#endif
// ============================================================================
//                                  Thread Methods
// ============================================================================

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
#if 0
    if(!_playing)
        return false;

    int err;
    snd_pcm_sframes_t frames;
    snd_pcm_sframes_t avail_frames;

    Lock();
    //return a positive number of frames ready otherwise a negative error code
    avail_frames = LATE(snd_pcm_avail_update)(_handlePlayout);
    if (avail_frames < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                   "playout snd_pcm_avail_update error: %s",
                   LATE(snd_strerror)(avail_frames));
        ErrorRecovery(avail_frames, _handlePlayout);
        UnLock();
        return true;
    }
    else if (avail_frames == 0)
    {
        UnLock();

        //maximum tixe in milliseconds to wait, a negative value means infinity
        err = LATE(snd_pcm_wait)(_handlePlayout, 2);
        if (err == 0)
        { //timeout occured
            WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                         "playout snd_pcm_wait timeout");
        }

        return true;
    }

    if (_playoutFramesLeft <= 0)
    {
        UnLock();
        _ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
        Lock();

        _playoutFramesLeft = _ptrAudioBuffer->GetPlayoutData(_playoutBuffer);
        assert(_playoutFramesLeft == _playoutFramesIn10MS);
    }

    if (static_cast<WebRtc_UWord32>(avail_frames) > _playoutFramesLeft)
        avail_frames = _playoutFramesLeft;

    int size = LATE(snd_pcm_frames_to_bytes)(_handlePlayout,
        _playoutFramesLeft);
    frames = LATE(snd_pcm_writei)(
        _handlePlayout,
        &_playoutBuffer[_playoutBufferSizeIn10MS - size],
        avail_frames);

    if (frames < 0)
    {
        WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                     "playout snd_pcm_avail_update error: %s",
                     LATE(snd_strerror)(frames));
        _playoutFramesLeft = 0;
        ErrorRecovery(frames, _handlePlayout);
        UnLock();
        return true;
    }
    else {
        assert(frames==avail_frames);
        _playoutFramesLeft -= frames;
    }

    UnLock();
#endif
    return true;
}

bool AudioDeviceBB::RecThreadProcess()
{
#if 0
    if (!_recording)
        return false;

    int err;
    snd_pcm_sframes_t frames;
    snd_pcm_sframes_t avail_frames;
    WebRtc_Word8 buffer[_recordingBufferSizeIn10MS];

    Lock();

    //return a positive number of frames ready otherwise a negative error code
    avail_frames = LATE(snd_pcm_avail_update)(_handleRecord);
    if (avail_frames < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "capture snd_pcm_avail_update error: %s",
                     LATE(snd_strerror)(avail_frames));
        ErrorRecovery(avail_frames, _handleRecord);
        UnLock();
        return true;
    }
    else if (avail_frames == 0)
    { // no frame is available now
        UnLock();

        //maximum time in milliseconds to wait, a negative value means infinity
        err = LATE(snd_pcm_wait)(_handleRecord,
            ALSA_CAPTURE_WAIT_TIMEOUT);
        if (err == 0) //timeout occured
            WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                         "caputre snd_pcm_wait timeout");

        return true;
    }

    if (static_cast<WebRtc_UWord32>(avail_frames) > _recordingFramesLeft)
        avail_frames = _recordingFramesLeft;

    frames = LATE(snd_pcm_readi)(_handleRecord,
        buffer, avail_frames); // frames to be written
    if (frames < 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                     "caputre snd_pcm_readi error: %s",
                     LATE(snd_strerror)(frames));
        ErrorRecovery(frames, _handleRecord);
        UnLock();
        return true;
    }
    else if (frames > 0)
    {
        assert(frames == avail_frames);

        int left_size = LATE(snd_pcm_frames_to_bytes)(_handleRecord,
            _recordingFramesLeft);
        int size = LATE(snd_pcm_frames_to_bytes)(_handleRecord, frames);

        memcpy(&_recordingBuffer[_recordingBufferSizeIn10MS - left_size],
               buffer, size);
        _recordingFramesLeft -= frames;

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

            // calculate delay
            _playoutDelay = 0;
            _recordingDelay = 0;
            if (_handlePlayout)
            {
                err = LATE(snd_pcm_delay)(_handlePlayout,
                    &_playoutDelay); // returned delay in frames
                if (err < 0)
                {
                    // TODO(xians): Shall we call ErrorRecovery() here?
                    _playoutDelay = 0;
                    WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                                 "playout snd_pcm_delay: %s",
                                 LATE(snd_strerror)(err));
                }
            }

            err = LATE(snd_pcm_delay)(_handleRecord,
                &_recordingDelay); // returned delay in frames
            if (err < 0)
            {
                // TODO(xians): Shall we call ErrorRecovery() here?
                _recordingDelay = 0;
                WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                             "caputre snd_pcm_delay: %s",
                             LATE(snd_strerror)(err));
            }

           // TODO(xians): Shall we add 10ms buffer delay to the record delay?
            _ptrAudioBuffer->SetVQEData(
                _playoutDelay * 1000 / _playoutFreq,
                _recordingDelay * 1000 / _recordingFreq, 0);

            // Deliver recorded samples at specified sample rate, mic level etc.
            // to the observer using callback.
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

    UnLock();
#endif
    return true;
}

}  // namespace webrtc

