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

#pragma once

#include <hookflash/core/types.h>
#include <hookflash/core/IStack.h>
#include <hookflash/core/IMediaEngine.h>
#include <hookflash/core/ILogger.h>
#include <hookflash/core/IConversationThread.h>
#include <hookflash/core/ICall.h>
#include <hookflash/core/IContact.h>
#include <hookflash/core/IAccount.h>
#include <hookflash/core/provisioning/IAccount.h>


#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/Timer.h>

#include <map>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif //__APPLE__

#define PROVISIONING_TEST
//#define OPENPEER_TEST

#define PEER_CONTACT_ID "" // "049F06B94D68668AAF331D979D204F1427C1C6120D59144B16FD680D0C1731F4"

namespace hfcoretest
{
  class FakeGUI;
  typedef boost::shared_ptr<FakeGUI> FakeGUIPtr;
  typedef boost::weak_ptr<FakeGUI> FakeGUIWeakPtr;
  
  class FakeGUITimer;
  typedef boost::shared_ptr<FakeGUITimer> FakeGUITimerPtr;
  typedef boost::weak_ptr<FakeGUITimer> FakeGUITimerWeakPtr;
  
  class FakeGUITimer : public zsLib::MessageQueueAssociator,
  public zsLib::ITimerDelegate
  {
    FakeGUITimer(
                 zsLib::IMessageQueuePtr queue,
                 FakeGUIPtr outer
                 );
    
    void init();
    
  public:
    static FakeGUITimerPtr create(
                                  zsLib::IMessageQueuePtr queue,
                                  FakeGUIPtr outer
                                  );
    
    ~FakeGUITimer();
    
    // TimerDelegate
    virtual void onTimer(zsLib::TimerPtr timer);
    
  private:
    FakeGUITimerWeakPtr mThisWeak;
    FakeGUIWeakPtr mOuter;
    
    zsLib::TimerPtr mTimer;
  };
  
  // notice that this object is intentionally NOT inheritenting from zsLib::MessageQueueAssociator? That's because
  // we want to make sure that the stack will automatically assign our object to the message queue of the main
  // GUI thread.
  class FakeGUI : public hookflash::core::IStackDelegate,
                  public hookflash::core::IConversationThreadDelegate,
                  public hookflash::core::ICallDelegate,
                  public hookflash::core::IMediaEngineDelegate,
                  public hookflash::core::ILoggerDelegate,
                  public hookflash::core::IAccountDelegate,
                  public hookflash::core::provisioning::IAccountDelegate
  {
  public:
    typedef zsLib::String String;
    typedef zsLib::ULONG ULONG;
    typedef zsLib::RecursiveLock RecursiveLock;
    typedef hookflash::core::IStackPtr IStackPtr;
    typedef hookflash::core::ICallPtr ICallPtr;
    typedef hookflash::core::IContactPtr IContactPtr;
    typedef hookflash::core::IConversationThreadPtr IConversationThreadPtr;
    typedef hookflash::core::ILogger::SubsystemID SubsystemID;
    
  protected:
    FakeGUI();
    
    void init();
    
  public:
    static FakeGUIPtr create();
    
    void go(
            zsLib::IMessageQueuePtr queue,
            hookflash::core::IStackPtr stack
            );
    void tick();
    
    // IStackDelegate
    virtual void onStackShutdown(hookflash::core::IStackAutoCleanupPtr ignoreThisArgument);
    
    // IConversationThreadDelegate
    virtual void onConversationThreadNew(IConversationThreadPtr conversationThread) {}
    
    virtual void onConversationThreadContactsChanged(IConversationThreadPtr conversationThread) {}
    
    virtual void onConversationThreadContactStateChanged(
                                                     IConversationThreadPtr conversationThread,
                                                     IContactPtr contact,
                                                     ContactStates state) {}
    
    virtual void onConversationThreadMessage(
                                             IConversationThreadPtr conversationThread,
                                             const char *messageID
                                             ) {}
    
    virtual void onConversationThreadMessageDeliveryStateChanged(
                                                                 IConversationThreadPtr conversationThread,
                                                                 const char *messageID,
                                                                 MessageDeliveryStates state
                                                                 ) {}
    
    virtual void onConversationThreadPushMessage(
                                                 IConversationThreadPtr conversationThread,
                                                 const char *messageID,
                                                 IContactPtr contact
                                                 ) {}

    // ICallDelegate
    virtual void onCallStateChanged(
                                    hookflash::core::ICallPtr call,
                                    hookflash::core::ICall::CallStates state
                                    ) {}
    
    //virtual void onCallRequesting(hookflash::ICallPtr call) {}
    
    //virtual void onCallStateChanged(hookflash::ICallPtr call);
    
    //IMediaEngineDelegate
    virtual void onMediaEngineAudioRouteChanged(OutputAudioRoutes audioRoute) {}
    
    
    // ILoggerDelegate
    virtual void onNewSubsystem(
                                SubsystemID subsystemUniqueID,
                                const char *subsystemName
                                );
    
    virtual void onLog(
                       SubsystemID subsystemUniqueID,
                       const char *subsystemName,
                       hookflash::core::ILogger::Severity severity,
                       hookflash::core::ILogger::Level level,
                       const char *message,
                       const char *function,
                       const char *filePath,
                       ULONG lineNumber
                       );
    
    // hookflash::IAccountDelegate
    virtual void onAccountStateChanged(
                                       hookflash::core::IAccountPtr account,
                                       hookflash::core::IAccount::AccountStates state
                                       );
    
    // provisioning::IAccountDelegate
    virtual void onProvisioningAccountStateChanged(
                                                   hookflash::core::provisioning::IAccountPtr account,
                                                   hookflash::core::provisioning::IAccount::AccountStates state
                                                   ) {}
    
    virtual void onProvisioningAccountError(
                                            hookflash::core::provisioning::IAccountPtr account,
                                            hookflash::core::provisioning::IAccount::AccountErrorCodes error
                                            ) {}
    
    //virtual void onProvisioningAccountProfileChanged(hookflash::core::provisioning::IAccountPtr account) {}
    
    virtual void onProvisioningAccountPrivatePeerFileChanged(hookflash::core::provisioning::IAccountPtr account) {}
    
    virtual void onProvisioningAccountIdentitiesChanged(hookflash::core::provisioning::IAccountPtr account) {}
    
//    virtual void onProvisioningAccountIdentityValidationResult(
//                                                               hookflash::core::provisioning::IAccountPtr account,
//                                                               hookflash::core::provisioning::IAccount::IdentityID identity,
//                                                               hookflash::core::provisioning::IAccount::IdentityValidationResultCode result
//                                                               ) {}

  protected:
    typedef std::map<zsLib::PTRNUMBER, const char *> SubsystemMap;

    RecursiveLock mLock;
    FakeGUIWeakPtr mThisWeak;

    bool mShouldShutdown;
    ULONG mRunLoop;

    //-------------------------------------------------------------------------
    // NOTE:  We are using the FakeGUITimer rather than implementing
    //        zsLib::ITimerDelegate on this class to make this fake GUI more
    //        realistic to the real GUI since the timer delegate requires an
    //        object derived from the MessageQueueAssociator and the real GUI
    //        would never derive from this object. Thus we use this
    //        intermediate object which can derive from MessageQueueAssociator
    //        and forward the event into the FakeGUI.
    FakeGUITimerPtr mTimer;
    
#ifdef _WIN32
    DWORD mThreadID;
#endif //_WIN32
#ifdef __APPLE__
    CFRunLoopRef mRunLoopRef;
#endif //__APPLE__
    
    IStackPtr mStack;
    hookflash::core::IAccountPtr mAccount;
    hookflash::core::provisioning::IAccountPtr mProvisioningAccount;
    
    IConversationThreadPtr mConversationThread;
    ICallPtr mCall;
    ULONG mHangupAtLoop;

    String mPrivatePeerFile;

    SubsystemMap mSubsystemMap;
  };
}

