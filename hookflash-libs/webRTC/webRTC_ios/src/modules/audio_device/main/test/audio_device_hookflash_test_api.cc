
#include <stdio.h>
#include <string.h>

#include "audio_device_test_defines.h"

#include "../source/audio_device_impl.h"
#include "../source/audio_device_config.h"
#include "../source/audio_device_utility.h"
#include "gtest/gtest.h"

using namespace webrtc;

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
    EXPECT_TRUE((audioDevice = AudioDeviceModuleImpl::Create(myId, AudioDeviceModule::kPlatformDefaultAudio)) != NULL);

    bool available(false);
    
    WebRtc_Word16 nDevices(0);

    // ===========
    // InitPlayout
    // ===========

    EXPECT_TRUE(audioDevice->Terminate() == 0);
    EXPECT_TRUE(audioDevice->Init() == 0);

    // check initial state
    EXPECT_TRUE(audioDevice->PlayoutIsInitialized() == false);

    EXPECT_TRUE(audioDevice->InitPlayout() == 0);
    EXPECT_TRUE(audioDevice->PlayoutIsInitialized() == true);

    // bulk tests
    EXPECT_TRUE(audioDevice->PlayoutIsAvailable(&available) == 0);
    if (available)
    {
        EXPECT_TRUE(audioDevice->InitPlayout() == 0);
        EXPECT_TRUE(audioDevice->PlayoutIsInitialized() == true);
        EXPECT_TRUE(audioDevice->InitPlayout() == 0);
        EXPECT_TRUE(audioDevice->StopPlayout() == 0);
        EXPECT_TRUE(audioDevice->PlayoutIsInitialized() == false);
    }

    EXPECT_TRUE(audioDevice->PlayoutIsAvailable(&available) == 0);
    if (available)
    {
        EXPECT_TRUE(audioDevice->InitPlayout() == 0);
        // Sleep is needed for e.g. iPhone since we after stopping then starting may
        // have a hangover time of a couple of ms before initialized.
        AudioDeviceUtility::Sleep(50);
        EXPECT_TRUE(audioDevice->PlayoutIsInitialized() == true);
    }

    nDevices = audioDevice->PlayoutDevices();
    for (i = 0; i < nDevices; i++)
    {
        EXPECT_TRUE(audioDevice->PlayoutIsAvailable(&available) == 0);
        if (available)
        {
            EXPECT_TRUE(audioDevice->StopPlayout() == 0);
            EXPECT_TRUE(audioDevice->PlayoutIsInitialized() == false);
            EXPECT_TRUE(audioDevice->SetPlayoutDevice(i) == 0);
            EXPECT_TRUE(audioDevice->PlayoutIsAvailable(&available) == 0);
            if (available)
            {
                EXPECT_TRUE(audioDevice->InitPlayout() == 0);
                EXPECT_TRUE(audioDevice->PlayoutIsInitialized() == true);
            }
        }
    }

    EXPECT_TRUE(audioDevice->StopPlayout() == 0);

    // =============
    // InitRecording
    // =============

    EXPECT_TRUE(audioDevice->Terminate() == 0);
    EXPECT_TRUE(audioDevice->Init() == 0);

    // check initial state
    EXPECT_TRUE(audioDevice->RecordingIsInitialized() == false);

    EXPECT_TRUE(audioDevice->InitRecording() == 0);
    EXPECT_TRUE(audioDevice->RecordingIsInitialized() == true);

    // bulk tests
    EXPECT_TRUE(audioDevice->RecordingIsAvailable(&available) == 0);
    if (available)
    {
        EXPECT_TRUE(audioDevice->InitRecording() == 0);
        EXPECT_TRUE(audioDevice->RecordingIsInitialized() == true);
        EXPECT_TRUE(audioDevice->InitRecording() == 0);
        EXPECT_TRUE(audioDevice->StopRecording() == 0);
        EXPECT_TRUE(audioDevice->RecordingIsInitialized() == false);
    }

    EXPECT_TRUE(audioDevice->RecordingIsAvailable(&available) == 0);
    if (available)
    {
        EXPECT_TRUE(audioDevice->InitRecording() == 0);
        AudioDeviceUtility::Sleep(50);
        EXPECT_TRUE(audioDevice->RecordingIsInitialized() == true);
    }

    nDevices = audioDevice->RecordingDevices();
    for (i = 0; i < nDevices; i++)
    {
        EXPECT_TRUE(audioDevice->RecordingIsAvailable(&available) == 0);
        if (available)
        {
            EXPECT_TRUE(audioDevice->StopRecording() == 0);
            EXPECT_TRUE(audioDevice->RecordingIsInitialized() == false);
            EXPECT_TRUE(audioDevice->SetRecordingDevice(i) == 0);
            EXPECT_TRUE(audioDevice->RecordingIsAvailable(&available) == 0);
            if (available)
            {
                EXPECT_TRUE(audioDevice->InitRecording() == 0);
                EXPECT_TRUE(audioDevice->RecordingIsInitialized() == true);
            }
        }
    }

    EXPECT_TRUE(audioDevice->StopRecording() == 0);

    // ============
    // StartPlayout
    // StopPlayout
    // ============

    EXPECT_TRUE(audioDevice->Terminate() == 0);
    EXPECT_TRUE(audioDevice->Init() == 0);

    EXPECT_TRUE(audioDevice->RegisterAudioCallback(NULL) == 0);

    // check initial states
    EXPECT_TRUE(audioDevice->Initialized() == true);
    EXPECT_TRUE(audioDevice->PlayoutIsInitialized() == false);
    EXPECT_TRUE(audioDevice->Playing() == false);

    EXPECT_TRUE(audioDevice->StartPlayout() == -1);
    EXPECT_TRUE(audioDevice->StopPlayout() == 0);

    // ==============
    // StartRecording
    // StopRecording
    // ==============

    EXPECT_TRUE(audioDevice->Terminate() == 0);
    EXPECT_TRUE(audioDevice->Init() == 0);

    EXPECT_TRUE(audioDevice->RegisterAudioCallback(NULL) == 0);

    // check initial states
    EXPECT_TRUE(audioDevice->Initialized() == true);
    EXPECT_TRUE(audioDevice->RecordingIsInitialized() == false);
    EXPECT_TRUE(audioDevice->Recording() == false);

    EXPECT_TRUE(audioDevice->StartRecording() == -1);
    EXPECT_TRUE(audioDevice->StopRecording() == 0);
    
    return 0;
}