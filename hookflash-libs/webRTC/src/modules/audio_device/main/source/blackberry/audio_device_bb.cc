/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio_device_bb.h"

#include <string.h>

#include "trace.h"
#include "thread_wrapper.h"
#include "event_wrapper.h"

#include <stdio.h> // fprintf
#include <stdlib.h> //malloc

// Enable to record playout data
//#define RECORD_PLAYOUT 1

namespace webrtc {

const WebRtc_UWord32 REC_TIMER_PERIOD_MS = 10;
const WebRtc_UWord32 PLAY_TIMER_PERIOD_MS = 10;

// ============================================================================
//                            Construction & Destruction
// ============================================================================

// ----------------------------------------------------------------------------
//  AudioDeviceBB() - ctor
// ----------------------------------------------------------------------------

// needed for Blackberry
//#include <sys/asoundlib.h>
//#include <audio/audio_manager_routing.h>


// snd_lib_error_handler_t
void WebrtcAlsaErrorHandler(const char *file,
                          int line,
                          const char *function,
                          int err,
                          const char *fmt,...){};

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
    _id(id),
    _timeEventRec(*EventWrapper::Create()),
    _timeEventPlay(*EventWrapper::Create()),
    _recStartEvent(*EventWrapper::Create()),
    _playStartEvent(*EventWrapper::Create()),
    _ptrThreadRec(NULL),
    _ptrThreadPlay(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _initialized(false),
    _recording(false),
    _playing(false),
    _recIsInitialized(false),
    _playIsInitialized(false),
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
    g_execute_audio(true),
    _microphoneIsInitialized(false),
    _playDataFile(NULL)
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, id, "%s created", __FUNCTION__);

    memset(_recBuffer, 0, sizeof(_recBuffer));
    WebRtc_Word16* tmp = (WebRtc_Word16*)_recBuffer;

    // Saw tooth -16000 to 16000, 100 Hz @ fs = 16 kHz
//    for(int i=0; i<160; ++i)
//    {
//        tmp[i] = i*200-16000;
//    }

    // Rough sinus 2 kHz @ fs = 16 kHz
    for(int i=0; i<20; ++i)
    {
      tmp[i*8] = 0;
      tmp[i*8+1] = -5000;
      tmp[i*8+2] = -16000;
      tmp[i*8+3] = -5000;
      tmp[i*8+4] = 0;
      tmp[i*8+5] = 5000;
      tmp[i*8+6] = 16000;
      tmp[i*8+7] = 5000;
    }
  
#ifdef RECORD_PLAYOUT
    _playDataFile = fopen("webrtc_VoiceEngine_playout.pcm", "wb");
    if (!_playDataFile)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "  Could not open file for writing playout data");
    }
#endif
}

// ----------------------------------------------------------------------------
//  AudioDeviceBB() - dtor
// ----------------------------------------------------------------------------

AudioDeviceBB::~AudioDeviceBB()
{
    WEBRTC_TRACE(kTraceMemory, kTraceAudioDevice, _id, "%s destroyed", __FUNCTION__);

    Terminate();

    _ptrAudioBuffer = NULL;

    delete &_recStartEvent;
    delete &_playStartEvent;
    delete &_timeEventRec;
    delete &_timeEventPlay;
    delete &_critSect;

    if (_playDataFile)
    {
        fclose(_playDataFile);
    }
}

// ============================================================================
//                                     API
// ============================================================================

// ----------------------------------------------------------------------------
//  AttachAudioBuffer
// ----------------------------------------------------------------------------

void AudioDeviceBB::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer)
{

    _ptrAudioBuffer = audioBuffer;

    // Inform the AudioBuffer about default settings for this implementation.
    _ptrAudioBuffer->SetRecordingSampleRate(16000);
    _ptrAudioBuffer->SetPlayoutSampleRate(16000);
    _ptrAudioBuffer->SetRecordingChannels(1);
    _ptrAudioBuffer->SetPlayoutChannels(1);
}

// ----------------------------------------------------------------------------
//  ActiveAudioLayer
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const
{
    audioLayer = AudioDeviceModule::kDummyAudio;
    return 0;
}

// ----------------------------------------------------------------------------
//  Init
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::Init()
{

    CriticalSectionScoped lock(&_critSect);

    if (_initialized)
    {
        return 0;
    }

    const bool periodic(true);
    unsigned int threadID(0);
    char threadName[64] = {0};

    // RECORDING
    strncpy(threadName, "webrtc_audio_module_rec_thread", 63);
    _ptrThreadRec = ThreadWrapper::CreateThread(
        RecThreadFunc, this, kRealtimePriority, threadName);
    if (_ptrThreadRec == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the rec audio thread");
        return -1;
    }

    if (!_ptrThreadRec->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the rec audio thread");
        delete _ptrThreadRec;
        _ptrThreadRec = NULL;
        return -1;
    }
    _recThreadID = threadID;

    if (!_timeEventRec.StartTimer(periodic, REC_TIMER_PERIOD_MS))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the rec timer event");
        if (_ptrThreadRec->Stop())
        {
            delete _ptrThreadRec;
            _ptrThreadRec = NULL;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  unable to stop the activated rec thread");
        }
        return -1;
    }

    // PLAYOUT
    strncpy(threadName, "webrtc_audio_module_play_thread", 63);
    _ptrThreadPlay = ThreadWrapper::CreateThread(
        PlayThreadFunc, this, kRealtimePriority, threadName);
    if (_ptrThreadPlay == NULL)
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to create the play audio thread");
        return -1;
    }

    threadID = 0;
    if (!_ptrThreadPlay->Start(threadID))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the play audio thread");
        delete _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        return -1;
    }
    _playThreadID = threadID;

    if (!_timeEventPlay.StartTimer(periodic, PLAY_TIMER_PERIOD_MS))
    {
        WEBRTC_TRACE(kTraceCritical, kTraceAudioDevice, _id,
                     "  failed to start the play timer event");
        if (_ptrThreadPlay->Stop())
        {
            delete _ptrThreadPlay;
            _ptrThreadPlay = NULL;
        }
        else
        {
            WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                         "  unable to stop the activated play thread");
        }
        return -1;
    }

    _initialized = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  Terminate
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::Terminate()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_initialized)
    {
        return 0;
    }

    // RECORDING
    if (_ptrThreadRec)
    {
        ThreadWrapper* tmpThread = _ptrThreadRec;
        _ptrThreadRec = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();
        _timeEventRec.Set();

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

    _timeEventRec.StopTimer();

    // PLAYOUT
    if (_ptrThreadPlay)
    {
        ThreadWrapper* tmpThread = _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();
        _timeEventPlay.Set();

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

    _timeEventPlay.StopTimer();

    _initialized = false;

    return 0;
}

// ----------------------------------------------------------------------------
//  Initialized
// ----------------------------------------------------------------------------

bool AudioDeviceBB::Initialized() const
{
    return (_initialized);
}

// ----------------------------------------------------------------------------
//  SpeakerIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SpeakerIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  InitSpeaker
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::InitSpeaker()
{

    CriticalSectionScoped lock(&_critSect);

    if (_playing)
    {
        return -1;
    }

	_speakerIsInitialized = true;

	return 0;
}

// ----------------------------------------------------------------------------
//  MicrophoneIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  InitMicrophone
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::InitMicrophone()
{

    CriticalSectionScoped lock(&_critSect);

    if (_recording)
    {
        return -1;
    }

    _microphoneIsInitialized = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  SpeakerIsInitialized
// ----------------------------------------------------------------------------

bool AudioDeviceBB::SpeakerIsInitialized() const
{

    return (_speakerIsInitialized);
}

// ----------------------------------------------------------------------------
//  MicrophoneIsInitialized
// ----------------------------------------------------------------------------

bool AudioDeviceBB::MicrophoneIsInitialized() const
{

    return (_microphoneIsInitialized);
}

// ----------------------------------------------------------------------------
//  SpeakerVolumeIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SpeakerVolumeIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = false;

    return 0;
}

// ----------------------------------------------------------------------------
//  SetSpeakerVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetSpeakerVolume(WebRtc_UWord32 volume)
{

	return -1;
}

// ----------------------------------------------------------------------------
//  SpeakerVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SpeakerVolume(WebRtc_UWord32& volume) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  SetWaveOutVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetWaveOutVolume(WebRtc_UWord16 volumeLeft, WebRtc_UWord16 volumeRight)
{

    return -1;
}

// ----------------------------------------------------------------------------
//  WaveOutVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::WaveOutVolume(WebRtc_UWord16& volumeLeft, WebRtc_UWord16& volumeRight) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  MaxSpeakerVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MaxSpeakerVolume(WebRtc_UWord32& maxVolume) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  MinSpeakerVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MinSpeakerVolume(WebRtc_UWord32& minVolume) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  SpeakerVolumeStepSize
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SpeakerVolumeStepSize(WebRtc_UWord16& stepSize) const
{
	
    return -1;
}

// ----------------------------------------------------------------------------
//  SpeakerMuteIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SpeakerMuteIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = false;

    return 0;
}

// ----------------------------------------------------------------------------
//  SetSpeakerMute
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetSpeakerMute(bool enable)
{

    return -1;
}

// ----------------------------------------------------------------------------
//  SpeakerMute
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SpeakerMute(bool& enabled) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  MicrophoneMuteIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneMuteIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = false;

    return 0;
}

// ----------------------------------------------------------------------------
//  SetMicrophoneMute
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetMicrophoneMute(bool enable)
{

    return -1;
}

// ----------------------------------------------------------------------------
//  MicrophoneMute
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneMute(bool& enabled) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  MicrophoneBoostIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneBoostIsAvailable(bool& available)
{

    available = false;
    return 0;
}

// ----------------------------------------------------------------------------
//  SetMicrophoneBoost
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetMicrophoneBoost(bool enable)
{

    return -1;
}

// ----------------------------------------------------------------------------
//  MicrophoneBoost
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneBoost(bool& enabled) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  StereoRecordingIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::StereoRecordingIsAvailable(bool& available)
{

    available = false;
    return 0;
}

// ----------------------------------------------------------------------------
//  SetStereoRecording
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetStereoRecording(bool enable)
{

    CriticalSectionScoped lock(&_critSect);

    if (enable)
    {
        return -1;
    }

    return 0;
}

// ----------------------------------------------------------------------------
//  StereoRecording
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::StereoRecording(bool& enabled) const
{

    enabled = false;

    return 0;
}

// ----------------------------------------------------------------------------
//  StereoPlayoutIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::StereoPlayoutIsAvailable(bool& available)
{

    available = false;
    return 0;
}

// ----------------------------------------------------------------------------
//  SetStereoPlayout
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetStereoPlayout(bool enable)
{

    CriticalSectionScoped lock(&_critSect);

    if (enable)
    {
        return -1;
    }

    return 0;
}

// ----------------------------------------------------------------------------
//  StereoPlayout
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::StereoPlayout(bool& enabled) const
{

    enabled = false;

    return 0;
}

// ----------------------------------------------------------------------------
//  SetAGC
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetAGC(bool enable)
{

    return -1;
}

// ----------------------------------------------------------------------------
//  AGC
// ----------------------------------------------------------------------------

bool AudioDeviceBB::AGC() const
{
    // WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "%s", __FUNCTION__);
    return false;
}

// ----------------------------------------------------------------------------
//  MicrophoneVolumeIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneVolumeIsAvailable(bool& available)
{

    CriticalSectionScoped lock(&_critSect);

    available = false;

    return 0;
}

// ----------------------------------------------------------------------------
//  SetMicrophoneVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetMicrophoneVolume(WebRtc_UWord32 volume)
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id,
                 "AudioDeviceBB::SetMicrophoneVolume(volume=%u)", volume);

    CriticalSectionScoped lock(&_critSect);

    return -1;
}

// ----------------------------------------------------------------------------
//  MicrophoneVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneVolume(WebRtc_UWord32& volume) const
{
    // WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    CriticalSectionScoped lock(&_critSect);

    return -1;
}

// ----------------------------------------------------------------------------
//  MaxMicrophoneVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MaxMicrophoneVolume(
    WebRtc_UWord32& maxVolume) const
{
    WEBRTC_TRACE(kTraceStream, kTraceAudioDevice, _id, "%s", __FUNCTION__);

    return -1;
}

// ----------------------------------------------------------------------------
//  MinMicrophoneVolume
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MinMicrophoneVolume(
    WebRtc_UWord32& minVolume) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  MicrophoneVolumeStepSize
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::MicrophoneVolumeStepSize(
    WebRtc_UWord16& stepSize) const
{

    return -1;
}

// ----------------------------------------------------------------------------
//  PlayoutDevices
// ----------------------------------------------------------------------------

WebRtc_Word16 AudioDeviceBB::PlayoutDevices()
{

    CriticalSectionScoped lock(&_critSect);

    return 1;
}

// ----------------------------------------------------------------------------
//  SetPlayoutDevice I (II)
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetPlayoutDevice(WebRtc_UWord16 index)
{

    if (_playIsInitialized)
    {
        return -1;
    }

    if (index != 0)
    {
      return -1;
    }

    return 0;
}

// ----------------------------------------------------------------------------
//  SetPlayoutDevice II (II)
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType device)
{
	return -1;
}

// ----------------------------------------------------------------------------
//  PlayoutDeviceName
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::PlayoutDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    if (index != 0)
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
      memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}

// ----------------------------------------------------------------------------
//  RecordingDeviceName
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::RecordingDeviceName(
    WebRtc_UWord16 index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize])
{

    if (index != 0)
    {
        return -1;
    }

    memset(name, 0, kAdmMaxDeviceNameSize);

    if (guid != NULL)
    {
        memset(guid, 0, kAdmMaxGuidSize);
    }

    return 0;
}

// ----------------------------------------------------------------------------
//  RecordingDevices
// ----------------------------------------------------------------------------

WebRtc_Word16 AudioDeviceBB::RecordingDevices()
{

    CriticalSectionScoped lock(&_critSect);

    return 1;
}

// ----------------------------------------------------------------------------
//  SetRecordingDevice I (II)
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetRecordingDevice(WebRtc_UWord16 index)
{

    if (_recIsInitialized)
    {
        return -1;
    }

    if (index != 0 )
    {
        return -1;
    }

    return 0;
}

// ----------------------------------------------------------------------------
//  SetRecordingDevice II (II)
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType device)
{
    return -1;
}

// ----------------------------------------------------------------------------
//  PlayoutIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::PlayoutIsAvailable(bool& available)
{

    available = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  RecordingIsAvailable
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::RecordingIsAvailable(bool& available)
{

    available = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  InitPlayout
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::InitPlayout()
{

    CriticalSectionScoped lock(&_critSect);

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

    _playIsInitialized = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  InitRecording
// ----------------------------------------------------------------------------

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

//	if (!_inputDeviceIsSpecified)
//	{
//		return -1;
//	}

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

    _recIsInitialized = true;
#endif

    return 0;

}

// ----------------------------------------------------------------------------
//  StartRecording
// ----------------------------------------------------------------------------

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

    return 0;
}

// ----------------------------------------------------------------------------
//  StopRecording
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::StopRecording()
{
	{
		CriticalSectionScoped lock(&_critSect);

		if (!_recIsInitialized)
		{
			return 0;
		}

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

//	CriticalSectionScoped lock(&_critSect);
//	_recordingFramesLeft = 0;
//	if (_recordingBuffer)
//	{
//		delete [] _recordingBuffer;
//		_recordingBuffer = NULL;
//	}

	// Threads will see this flag every 20ms in their loop
	fprintf(stderr,"\nStopPCMAudio ****************: ENTER \n");
	g_execute_audio = false;


	fprintf(stderr,"CAPTURE EXIT BEGIN\n");
	(void) snd_pcm_plugin_flush(g_pcm_handle_c, SND_PCM_CHANNEL_CAPTURE);
	//(void)snd_mixer_close (mixer_handle);
	(void) snd_pcm_close(g_pcm_handle_c);
	audio_manager_free_handle(g_audio_manager_handle_c);
	// IMPORTANT NB: You only get failed on capture if the play loop has exited hence the circular buffer fills. This is with the simulator

    return 0;
}

// ----------------------------------------------------------------------------
//  RecordingIsInitialized
// ----------------------------------------------------------------------------

bool AudioDeviceBB::RecordingIsInitialized() const
{
    return (_recIsInitialized);
}

// ----------------------------------------------------------------------------
//  Recording
// ----------------------------------------------------------------------------

bool AudioDeviceBB::Recording() const
{
    return (_recording);
}

// ----------------------------------------------------------------------------
//  PlayoutIsInitialized
// ----------------------------------------------------------------------------

bool AudioDeviceBB::PlayoutIsInitialized() const
{

    return (_playIsInitialized);
}

// ----------------------------------------------------------------------------
//  StartPlayout
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::StartPlayout()
{

    CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized)
    {
        return -1;
    }

    if (_playing)
    {
        return 0;
    }

    _playing = true;

    return 0;
}

// ----------------------------------------------------------------------------
//  StopPlayout
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::StopPlayout()
{

    if (!_playIsInitialized)
    {
        return 0;
    }

    _playIsInitialized = false;
    _playing = false;

    return 0;
}

// ----------------------------------------------------------------------------
//  PlayoutDelay
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::PlayoutDelay(WebRtc_UWord16& delayMS) const
{
    CriticalSectionScoped lock(&_critSect);
    delayMS = 0;
    return 0;
}

// ----------------------------------------------------------------------------
//  RecordingDelay
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::RecordingDelay(WebRtc_UWord16& delayMS) const
{
    CriticalSectionScoped lock(&_critSect);
    delayMS = 0;
    return 0;
}

// ----------------------------------------------------------------------------
//  Playing
// ----------------------------------------------------------------------------

bool AudioDeviceBB::Playing() const
{
    return (_playing);
}
// ----------------------------------------------------------------------------
//  SetPlayoutBuffer
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::SetPlayoutBuffer(
    const AudioDeviceModule::BufferType type, WebRtc_UWord16 sizeMS)
{

    CriticalSectionScoped lock(&_critSect);

    // Just ignore

    return 0;
}

// ----------------------------------------------------------------------------
//  PlayoutBuffer
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::PlayoutBuffer(
    AudioDeviceModule::BufferType& type, WebRtc_UWord16& sizeMS) const
{
    CriticalSectionScoped lock(&_critSect);

    type = AudioDeviceModule::kAdaptiveBufferSize;
    sizeMS = 0;

    return 0;
}

// ----------------------------------------------------------------------------
//  CPULoad
// ----------------------------------------------------------------------------

WebRtc_Word32 AudioDeviceBB::CPULoad(WebRtc_UWord16& load) const
{

    load = 0;

    return 0;
}

// ----------------------------------------------------------------------------
//  PlayoutWarning
// ----------------------------------------------------------------------------

bool AudioDeviceBB::PlayoutWarning() const
{
    return false;
}

// ----------------------------------------------------------------------------
//  PlayoutError
// ----------------------------------------------------------------------------

bool AudioDeviceBB::PlayoutError() const
{
    return false;
}

// ----------------------------------------------------------------------------
//  RecordingWarning
// ----------------------------------------------------------------------------

bool AudioDeviceBB::RecordingWarning() const
{
    return false;
}

// ----------------------------------------------------------------------------
//  RecordingError
// ----------------------------------------------------------------------------

bool AudioDeviceBB::RecordingError() const
{
    return false;
}

// ----------------------------------------------------------------------------
//  ClearPlayoutWarning
// ----------------------------------------------------------------------------

void AudioDeviceBB::ClearPlayoutWarning()
{
}

// ----------------------------------------------------------------------------
//  ClearPlayoutError
// ----------------------------------------------------------------------------

void AudioDeviceBB::ClearPlayoutError()
{
}

// ----------------------------------------------------------------------------
//  ClearRecordingWarning
// ----------------------------------------------------------------------------

void AudioDeviceBB::ClearRecordingWarning()
{
}

// ----------------------------------------------------------------------------
//  ClearRecordingError
// ----------------------------------------------------------------------------

void AudioDeviceBB::ClearRecordingError()
{
}

// ============================================================================
//                                  Thread Methods
// ============================================================================

// ----------------------------------------------------------------------------
//  PlayThreadFunc
// ----------------------------------------------------------------------------

bool AudioDeviceBB::PlayThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceBB*>(pThis)->PlayThreadProcess());
}

// ----------------------------------------------------------------------------
//  RecThreadFunc
// ----------------------------------------------------------------------------

bool AudioDeviceBB::RecThreadFunc(void* pThis)
{
    return (static_cast<AudioDeviceBB*>(pThis)->RecThreadProcess());
}

// ----------------------------------------------------------------------------
//  PlayThreadProcess
// ----------------------------------------------------------------------------

bool AudioDeviceBB::PlayThreadProcess()
{
    switch (_timeEventPlay.Wait(1000))
    {
    case kEventSignaled:
        break;
    case kEventError:
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "EventWrapper::Wait() failed => restarting timer");
        _timeEventPlay.StopTimer();
        _timeEventPlay.StartTimer(true, PLAY_TIMER_PERIOD_MS);
        return true;
    case kEventTimeout:
        return true;
    }

    Lock();

    if(_playing)
    {
        WebRtc_Word8 playBuffer[2*160];

        UnLock();
        WebRtc_Word32 nSamples = (WebRtc_Word32)_ptrAudioBuffer->RequestPlayoutData(160);
        Lock();

        if (!_playing)
        {
            UnLock();
            return true;
        }

        nSamples = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        if (nSamples != 160)
        {
            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
                "  invalid number of output samples(%d)", nSamples);
        }

        if (_playDataFile)
        {
            int wr = fwrite(playBuffer, 2, 160, _playDataFile);
            if (wr != 160)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                           "  Could not write playout data to file (%d) ferror = %d",
                           wr, ferror(_playDataFile));
            }
        }
    }

    UnLock();
    return true;
}

// ----------------------------------------------------------------------------
//  RecThreadProcess
// ----------------------------------------------------------------------------

bool AudioDeviceBB::RecThreadProcess()
{
    switch (_timeEventRec.Wait(1000))
    {
    case kEventSignaled:
        break;
    case kEventError:
        WEBRTC_TRACE(kTraceWarning, kTraceAudioDevice, _id,
                   "EventWrapper::Wait() failed => restarting timer");
        _timeEventRec.StopTimer();
        _timeEventRec.StartTimer(true, REC_TIMER_PERIOD_MS);
        return true;
    case kEventTimeout:
        return true;
    }

    Lock();

    if (_recording)
    {
        // store the recorded buffer
        _ptrAudioBuffer->SetRecordedBuffer(_recBuffer, 160);

        // store vqe delay values
        _ptrAudioBuffer->SetVQEData(0, 0, 0);

        // deliver recorded samples at specified sample rate, mic level etc. to the observer using callback
        UnLock();
        _ptrAudioBuffer->DeliverRecordedData();
    }
    else
    {
        UnLock();
    }

    return true;
}

}  // namespace webrtc
