#include <stdio.h>
#include <string.h>
#include "voe_standard_test.h"
#include "voe_extended_test.h"
#include "thread_wrapper.h"

using namespace webrtc;

namespace voetest{
    
bool TestThread(ThreadObj obj);
const char* storeDirectory = new char[256];
char filenameStr[2][256];
int currentStr = 0;
const char* sendIpAddress = new char[256];
int sendPort;
int receivePort;
VoETestManager testManager;
  
int GetDocumentsDir(char* buf, int bufLen)
{
    currentStr = !currentStr;
    sprintf(filenameStr[currentStr], "%s", storeDirectory);
    strncpy(buf, filenameStr[currentStr], bufLen-1);
    return 0;
}

char* GetFilename(char* filename)
{
    currentStr = !currentStr;
    sprintf(filenameStr[currentStr], "%s/%s", storeDirectory, filename); 
    return filenameStr[currentStr];
}
    
const char* GetFilename(const char* filename)
{
    currentStr = !currentStr;
    sprintf(filenameStr[currentStr], "%s/%s", storeDirectory, filename); 
    return filenameStr[currentStr];
}
    
int GetResource(char* resource, char* dest, int destLen)
{
    currentStr = !currentStr;
    sprintf(filenameStr[currentStr], "%s/%s", storeDirectory, resource);
    strncpy(dest, filenameStr[currentStr], destLen-1);
    return 0;
}
    
char* GetResource(char* resource)
{
    currentStr = !currentStr;
    sprintf(filenameStr[currentStr], "%s/%s", storeDirectory, resource); 
    return filenameStr[currentStr];
}
    
const char* GetResource(const char* resource)
{ 
    currentStr = !currentStr;
    sprintf(filenameStr[currentStr], "%s/%s", storeDirectory, resource); 
    return filenameStr[currentStr];
}

int hookflash_test(const char* directory, int testID, const char* sndIpAddress, int sndPort, int rcvPort)
{
  strcpy((char*)storeDirectory, directory);
  strcpy((char*)sendIpAddress, sndIpAddress);
  sendPort = sndPort;
  receivePort = rcvPort;
  
  testManager.Init();
  testManager.GetInterfaces();
  testManager.BasePtr()->Init();

  ThreadWrapper* _messageThread = ThreadWrapper::CreateThread(TestThread, NULL,
                                                              kLowPriority, "TestThread");
  unsigned int threadId;
  _messageThread->Start(threadId);
  
  return 0;
}
  
bool TestThread(ThreadObj obj)
{    
    VoEExtendedTest extendedTest(testManager);
    extendedTest.TestCodec(sendIpAddress, sendPort, receivePort);
    
    int releaseOK = testManager.ReleaseInterfaces();
    
    return 0;
}

}  // namespace voetest