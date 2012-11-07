
#include <stdio.h>
#include <string.h>

#include "audio_device_test_defines.h"

#include "../source/audio_device_config.h"
#include "../source/audio_device_utility.h"

using namespace webrtc;

class AudioTransportAPI: public AudioTransport
{
public:
    AudioTransportAPI(AudioDeviceModule* audioDevice) :
    _audioDevice(audioDevice), _recCount(0), _playCount(0)
    {
    }
    ;
    
    ~AudioTransportAPI()
    {
    }
    ;
    
    virtual WebRtc_Word32 RecordedDataIsAvailable(
                                                  const WebRtc_Word8* audioSamples,
                                                  const WebRtc_UWord32 nSamples,
                                                  const WebRtc_UWord8 nBytesPerSample,
                                                  const WebRtc_UWord8 nChannels,
                                                  const WebRtc_UWord32 sampleRate,
                                                  const WebRtc_UWord32 totalDelay,
                                                  const WebRtc_Word32 clockSkew,
                                                  const WebRtc_UWord32 currentMicLevel,
                                                  WebRtc_UWord32& newMicLevel)
    {
        _recCount++;
        if (_recCount % 100 == 0)
        {
            if (nChannels == 1)
            {
                // mono
                TEST_LOG("-");
            } else if ((nChannels == 2) && (nBytesPerSample == 2))
            {
                // stereo but only using one channel
                TEST_LOG("-|");
            } else
            {
                // stereo
                TEST_LOG("--");
            }
        }
        
        return 0;
    }
    
    virtual WebRtc_Word32 NeedMorePlayData(const WebRtc_UWord32 nSamples,
                                           const WebRtc_UWord8 nBytesPerSample,
                                           const WebRtc_UWord8 nChannels,
                                           const WebRtc_UWord32 sampleRate,
                                           WebRtc_Word8* audioSamples,
                                           WebRtc_UWord32& nSamplesOut)
    {
        _playCount++;
        if (_playCount % 100 == 0)
        {
            if (nChannels == 1)
            {
                TEST_LOG("+");
            } else
            {
                TEST_LOG("++");
            }
        }
        
        nSamplesOut = 480;
        
        return 0;
    }
    ;
private:
    AudioDeviceModule* _audioDevice;
    WebRtc_UWord32 _recCount;
    WebRtc_UWord32 _playCount;
};

int hookflash_test_api(const char* fileName)
{
    int i(0);
    
    Trace::CreateTrace();
    Trace::SetTraceFile(fileName);

    TEST_LOG("========================================\n");
    TEST_LOG("API Test of the WebRtcAudioDevice Module\n");
    TEST_LOG("========================================\n\n");
    
    ProcessThread* processThread = ProcessThread::CreateProcessThread();
    processThread->Start();
    
    // =======================================================
    // AudioDeviceModule::Create
    //
    // Windows:
    //      if (WEBRTC_WINDOWS_CORE_AUDIO_BUILD)
    //          user can select between default (Core) or Wave
    //      else
    //          user can select between default (Wave) or Wave
    // =======================================================
    
    const WebRtc_Word32 myId = 444;
    
    AudioDeviceModule* audioDevice(NULL);

    // Create default implementation instance
    TEST((audioDevice = AudioDeviceModule::Create(myId, AudioDeviceModule::kPlatformDefaultAudio)) != NULL);

    AudioTransportAPI* audioTransport = new AudioTransportAPI(audioDevice);
    
    bool available(false);
    
    WebRtc_Word16 nDevices(0);

    // ===========
    // InitPlayout
    // ===========

    TEST(audioDevice->Terminate() == 0);
    TEST(audioDevice->Init() == 0);

    // check initial state
    TEST(audioDevice->PlayoutIsInitialized() == false);

    TEST(audioDevice->InitPlayout() == 0);
    TEST(audioDevice->PlayoutIsInitialized() == true);

    // bulk tests
    TEST(audioDevice->PlayoutIsAvailable(&available) == 0);
    if (available)
    {
        TEST(audioDevice->InitPlayout() == 0);
        TEST(audioDevice->PlayoutIsInitialized() == true);
        TEST(audioDevice->InitPlayout() == 0);
        TEST(audioDevice->StopPlayout() == 0);
        TEST(audioDevice->PlayoutIsInitialized() == false);
    }

    TEST(audioDevice->PlayoutIsAvailable(&available) == 0);
    if (available)
    {
        TEST(audioDevice->InitPlayout() == 0);
        // Sleep is needed for e.g. iPhone since we after stopping then starting may
        // have a hangover time of a couple of ms before initialized.
        AudioDeviceUtility::Sleep(50);
        TEST(audioDevice->PlayoutIsInitialized() == true);
    }

    nDevices = audioDevice->PlayoutDevices();
    for (i = 0; i < nDevices; i++)
    {
        TEST(audioDevice->PlayoutIsAvailable(&available) == 0);
        if (available)
        {
            TEST(audioDevice->StopPlayout() == 0);
            TEST(audioDevice->PlayoutIsInitialized() == false);
            TEST(audioDevice->SetPlayoutDevice(i) == 0);
            TEST(audioDevice->PlayoutIsAvailable(&available) == 0);
            if (available)
            {
                TEST(audioDevice->InitPlayout() == 0);
                TEST(audioDevice->PlayoutIsInitialized() == true);
            }
        }
    }

    TEST(audioDevice->StopPlayout() == 0);

    // =============
    // InitRecording
    // =============

    TEST(audioDevice->Terminate() == 0);
    TEST(audioDevice->Init() == 0);

    // check initial state
    TEST(audioDevice->RecordingIsInitialized() == false);

    TEST(audioDevice->InitRecording() == 0);
    TEST(audioDevice->RecordingIsInitialized() == true);

    // bulk tests
    TEST(audioDevice->RecordingIsAvailable(&available) == 0);
    if (available)
    {
        TEST(audioDevice->InitRecording() == 0);
        TEST(audioDevice->RecordingIsInitialized() == true);
        TEST(audioDevice->InitRecording() == 0);
        TEST(audioDevice->StopRecording() == 0);
        TEST(audioDevice->RecordingIsInitialized() == false);
    }

    TEST(audioDevice->RecordingIsAvailable(&available) == 0);
    if (available)
    {
        TEST(audioDevice->InitRecording() == 0);
        AudioDeviceUtility::Sleep(50);
        TEST(audioDevice->RecordingIsInitialized() == true);
    }

    nDevices = audioDevice->RecordingDevices();
    for (i = 0; i < nDevices; i++)
    {
        TEST(audioDevice->RecordingIsAvailable(&available) == 0);
        if (available)
        {
            TEST(audioDevice->StopRecording() == 0);
            TEST(audioDevice->RecordingIsInitialized() == false);
            TEST(audioDevice->SetRecordingDevice(i) == 0);
            TEST(audioDevice->RecordingIsAvailable(&available) == 0);
            if (available)
            {
                TEST(audioDevice->InitRecording() == 0);
                TEST(audioDevice->RecordingIsInitialized() == true);
            }
        }
    }

    TEST(audioDevice->StopRecording() == 0);

    // ============
    // StartPlayout
    // StopPlayout
    // ============

    TEST(audioDevice->Terminate() == 0);
    TEST(audioDevice->Init() == 0);

    TEST(audioDevice->RegisterAudioCallback(NULL) == 0);

    // check initial states
    TEST(audioDevice->Initialized() == true);
    TEST(audioDevice->PlayoutIsInitialized() == false);
    TEST(audioDevice->Playing() == false);

    TEST(audioDevice->StartPlayout() == -1);
    TEST(audioDevice->StopPlayout() == 0);

    // ==============
    // StartRecording
    // StopRecording
    // ==============

    TEST(audioDevice->Terminate() == 0);
    TEST(audioDevice->Init() == 0);

    TEST(audioDevice->RegisterAudioCallback(NULL) == 0);

    // check initial states
    TEST(audioDevice->Initialized() == true);
    TEST(audioDevice->RecordingIsInitialized() == false);
    TEST(audioDevice->Recording() == false);

    TEST(audioDevice->StartRecording() == -1);
    TEST(audioDevice->StopRecording() == 0);
    
    return 0;
}