/*
 
 Copyright (c) 2013, SMB Phone Inc.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */

#import "ProvisioningManager.h"
#import <openpeer/stack/IStack.h>
#import <openpeer/stack/IServiceIdentity.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>
#include <zsLib/Proxy.h>

@interface ProvisioningManager()
- (void) initSingleton;
@end

using namespace openpeer::stack;

static ProvisioningManager *sProvisioningManager = nil;

@implementation ProvisioningManager

+(ProvisioningManager*)getInstance
{
  @synchronized(self)
  {
    if(sProvisioningManager == nil)
    {
      sProvisioningManager = [[super allocWithZone:NULL] init];
      
      if (sProvisioningManager)
        [sProvisioningManager initSingleton];
    }
  }
  return sProvisioningManager;
}
+ (id)allocWithZone:(NSZone *)zone {
  return [[self getInstance] retain];
}

- (id)retain
{
  return self;
}
- (id)init
{
  self = [super init];
  
  return self;
}

-(void) initSingleton
{
  [self initDelegates];
  
  [self initializeStack];
}

- (void) initializeStack
{
  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadDelegate(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadStack(zsLib::MessageQueueThread::createBasic());
  zsLib::MessageQueueThreadPtr threadServices(zsLib::MessageQueueThread::createBasic());
  
  //prepare stack
  IStack::setup(
                threadDelegate,
                threadStack,
                threadServices,
                "com.xyz123.app1",
                "Bojan's Test App",
                "https://testapp.com/image.png",
                "https://testapp.com/app/",
                "hookflash/1.0.1001a (iOS/iPad)",
                "123456",
                "iOS 5.0.3",
                "iPad 2"
                );
  
  //initialize bootstrapped network
  mNetwork = IBootstrappedNetwork::prepare("unstable.hookflash.me", mBootstrappedNetworkDelegate);
  
}

- (void) initDelegates
{
  if(!mBootstrappedNetworkDelegate)
  {
    mBootstrappedNetworkDelegate = BootstrappedNetworkDelegateWrapper::create();
  }
  
  if(!mServiceIdentitySessionDelegate)
  {
    mServiceIdentitySessionDelegate = ServiceIdentitySessionDelegateWrapper::create();
  }
}

- (void) onBootstrappedNetworkPreparationCompleted: (IBootstrappedNetworkPtr) bootstrappedNetwork
{
  mNetwork = bootstrappedNetwork;
  
  //create identity
  mServiceIdentity = IServiceIdentity::createServiceIdentityFrom(bootstrappedNetwork);
  
  //create identity session
//  mServiceIdentitySession = IServiceIdentitySession::loginWithIdentityTBD(mServiceIdentitySessionDelegate, "provisioning-unstable-dev.hookflash.me", mServiceIdentity);
}

- (void) onServiceIdentitySessionStateChanged: (IServiceIdentitySessionPtr)session identitySessionState:(IServiceIdentitySession::SessionStates) state
{
  // TODO: handle state change properly
  mServiceIdentitySession = session;
}

- (void) onServiceIdentitySessionPendingMessageForInnerBrowserWindowFrame: (IServiceIdentitySessionPtr) session
{
  // TODO: handle message for inner frame properly
  mServiceIdentitySession = session;
}

- (void)dealloc
{
  //dispatch_release(mEventQueue);
  [super dealloc];
}

@end
