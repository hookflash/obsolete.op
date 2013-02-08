//
//  main.cpp
//  hfcoretest
//
//  Created by Robin Raymond on 11-08-18.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <vie_capture.h>

//hf API includes
#include <hookflash/hookflashTypes.h>
#include <hookflash/internal/Client.h>
#include <hookflash/internal/Stack.h>

using namespace webrtc;
using namespace hookflash;

int main (int argc, const char * argv[])
{

  // insert code here...
    IClient::setup(IClientDelegatePtr());
  IClientPtr client = IClient::singleton();
  //webrtc structure in order tocheck the linkage of webrtc
  CaptureCapability temp;
  
  std::cout << "Hello, World!\n";
    return 0;
}

