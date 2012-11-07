//
//  test.cpp
//  hfcoretest
//
//  Created by Vladimir Morosev on 9/10/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "test.h"
#include <hookflash/hookflashTypes.h>
#include <hookflash/internal/hookflash_Client.h>

using namespace hookflash;

extern IClientLogDelegatePtr createWrapper (id<ClientLogDelegateObjC> del);

void test_delegate ()
{
  IClientPtr client;
  try{
    client = IClient::singleton();
  } catch (...) {
   // catch
  }
  
  FakeGUIViewController *gui = [[FakeGUIViewController alloc] init];

  IClientLogDelegatePtr temp = createWrapper(gui);
  
  try{
    IClient::setup();
  } catch (...) {
    // catch
  }
  try {
    IClient::installCustomLogger(temp);
    client = IClient::singleton();
  } catch (...) {
    // catch
  }
  
  client->finalizeShutdown();
};
