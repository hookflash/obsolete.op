#include <stdio.h>
#include <string.h>

#include "func_test_manager.h"
#include "audio_device_test_defines.h"

using namespace webrtc;


int hookflash_test_func(const char* fileName, int testID)
{
    Trace::CreateTrace();
    Trace::SetTraceFile(fileName);
    Trace::SetLevelFilter(kTraceAll);
    
    TEST_LOG("=========================================\n");
    TEST_LOG("Func Test of the WebRtcAudioDevice Module\n");
    TEST_LOG("=========================================\n\n");
        
    FuncTestManager funcMgr;
    
    funcMgr.Init();
    enum TestType testType(TTInvalid);
    
    if (testID == 0)
        testType = TTAudioTransport;
    else if (testID == 1)
        testType = TTLoopback;
    
//    testType = TTAll;
//    testType = TTAudioLayerSelection;
//    testType = TTDeviceEnumeration;
//    testType = TTDeviceSelection;
//    testType = TTAudioTransport;
//    testType = TTSpeakerVolume;
//    testType = TTMicrophoneVolume;
//    testType = TTSpeakerMute;
//    testType = TTMicrophoneBoost;
//    testType = TTMicrophoneAGC;
//    testType = TTLoopback;
//    testType = TTDeviceRemoval;
//    testType = TTMobileAPI;
//    testType = TTTest;
 
    funcMgr.DoTest(testType);
    
    funcMgr.Close();
    
    return 0;
}