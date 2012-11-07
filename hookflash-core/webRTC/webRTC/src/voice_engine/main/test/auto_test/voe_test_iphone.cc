#include <stdio.h>
#include <string.h>
#include "voe_standard_test.h"
#include "voe_extended_test.h"

using namespace webrtc;

namespace voetest{
    
const char* storeDirectory = new char[256];
char filenameStr[2][256];
int currentStr = 0;

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
    
    VoETestManager testManager;
    testManager.GetInterfaces();
//    testManager.DoStandardTest();
    
    VoEExtendedTest extendedTest(testManager);
    extendedTest.TestCodec(sndIpAddress, sndPort, rcvPort);
    
    int releaseOK = testManager.ReleaseInterfaces();

    return 0;
}

}  // namespace voetest