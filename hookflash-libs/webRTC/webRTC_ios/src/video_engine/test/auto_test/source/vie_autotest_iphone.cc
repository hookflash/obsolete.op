#include "vie_autotest_iphone.h"

#include "iphone_render_view.h"
#include "trace.h"
#include "thread_wrapper.h"

#include	"vie_autotest_iphone.h"
#include	"vie_autotest_defines.h"
#include	"vie_autotest.h"

using namespace std;
using namespace webrtc;

#include <stdio.h>
#include <string.h>

bool TestThread(ThreadObj obj);
const char* storeDirectory = new char[256];
int testID; 
const char* sendIpAddress = new char[256];
int sendPort;
int receivePort;
IPhoneRenderView* renderView1;
IPhoneRenderView* renderView2;
int captureIndex;
ViEAutoTest* vieAutoTest = NULL;

ViEAutoTestWindowManager::ViEAutoTestWindowManager(IPhoneRenderView* renderView1, IPhoneRenderView* renderView2) :
_iPhoneRenderView1(renderView1), _iPhoneRenderView2(renderView2)
{
    
}

ViEAutoTestWindowManager::~ViEAutoTestWindowManager()
{
}

int ViEAutoTestWindowManager::CreateWindows(AutoTestRect window1Size,
                                            AutoTestRect window2Size,
                                            void* window1Title,
                                            void* window2Title)
{
    return 0;
}

int ViEAutoTestWindowManager::TerminateWindows()
{
    return 0;
}

void* ViEAutoTestWindowManager::GetWindow1()
{
    return _iPhoneRenderView1;
}

void* ViEAutoTestWindowManager::GetWindow2()
{
    return _iPhoneRenderView2;
}

bool ViEAutoTestWindowManager::SetTopmostWindow()
{
    return true;
}

int hookflash_test(const char* directory, int testID, int cameraIndex,
                   const char* sndIpAddress, int sndPort, int rcvPort,
                   IPhoneRenderView* view1, IPhoneRenderView* view2)
{
    strcpy((char*)storeDirectory, directory);
    strcpy((char*)sendIpAddress, sndIpAddress);
    sendPort = sndPort;
    receivePort = rcvPort;
    renderView1 = view1;
    renderView2 = view2;
    captureIndex = cameraIndex;
    
    ThreadWrapper* _messageThread = ThreadWrapper::CreateThread(TestThread, NULL,
                                                                kLowPriority, "TestThread");
    unsigned int threadId;
    _messageThread->Start(threadId);
    
    return 0;
}

int hookflash_set_codec_size(int width, int height)
{
    if (vieAutoTest != NULL)
        return vieAutoTest->ViECodecSetCodecSize(width, height);
    else 
        return -1;
}

int hookflash_set_capture_rotation()
{
    if (vieAutoTest != NULL)
        return vieAutoTest->ViECodecSetCaptureRotation();
    else 
        return -1;
}

int hookflash_set_event_callback(void (*eventCallback)(const char*, void*))
{
    if (vieAutoTest != NULL)
        return vieAutoTest->ViECodecSetEventCallback(eventCallback);
    else 
        return -1;
}

int hookflash_get_average_system_cpu(int& systemCPU)
{
    if (vieAutoTest != NULL)
        return vieAutoTest->GetAverageSystemCPU(systemCPU);
    else 
        return -1;
}

int hookflash_get_send_codec_statistics(unsigned int& keyFrames, unsigned int& deltaFrames)
{
    if (vieAutoTest != NULL)
        return vieAutoTest->GetSendCodecStastistics(keyFrames, deltaFrames);
    else 
        return -1;
}

int hookflash_get_receive_codec_statistics(unsigned int& keyFrames, unsigned int& deltaFrames)
{
    if (vieAutoTest != NULL)
        return vieAutoTest->GetReceiveCodecStastistics(keyFrames, deltaFrames);
    else 
        return -1;
}

int hookflash_get_received_rtcp_statistics(unsigned short& fractionLost,
                                           unsigned int& cumulativeLost, unsigned int& extendedMax,
                                           unsigned int& jitter, int& rttMs)
{
    if (vieAutoTest != NULL)
        return vieAutoTest->GetReceivedRTCPStatistics(fractionLost, cumulativeLost, extendedMax, jitter, rttMs);
    else 
        return -1;
}

int hookflash_get_sent_rtcp_statistics(unsigned short& fractionLost,
                                       unsigned int& cumulativeLost,
                                       unsigned int& extendedMax,
                                       unsigned int& jitter,
                                       int& rttMs)
{
    if (vieAutoTest != NULL)
        return vieAutoTest->GetSentRTCPStatistics(fractionLost, cumulativeLost, extendedMax, jitter, rttMs);
    else 
        return -1;
}

int hookflash_get_rtp_statistics(unsigned int& bytesSent,
                                 unsigned int& packetsSent,
                                 unsigned int& bytesReceived,
                                 unsigned int& packetsReceived)
{
    if (vieAutoTest != NULL)
        return vieAutoTest->GetRTPStatistics(bytesSent, packetsSent, bytesReceived, packetsReceived);
    else 
        return -1;
}

bool TestThread(ThreadObj obj)
{
    ViEAutoTestWindowManagerInterface* windowManager =
        new ViEAutoTestWindowManager(renderView1, renderView2);
    vieAutoTest = new ViEAutoTest(windowManager->GetWindow1(), windowManager->GetWindow2());
    
    ViETest::Init();
    ViETest::Log(" ============================== ");
    ViETest::Log("    WebRTC ViE 3.x Autotest     ");
    ViETest::Log(" ============================== \n");
    
    int testErrors = 0;
    
    vieAutoTest->ViECodecStandardTest(captureIndex,
                                      sendIpAddress, sendPort, receivePort);
    
    windowManager->TerminateWindows();
    
    if (testErrors)
    {
        ViETest::Log("Test done with errors, see ViEAutotestLog.txt for test "
                     "result.\n");
    }
    else
    {
        ViETest::Log("Test done without errors, see ViEAutotestLog.txt for "
                     "test result.\n");
    }
    
    delete windowManager;

    return 0;
}


