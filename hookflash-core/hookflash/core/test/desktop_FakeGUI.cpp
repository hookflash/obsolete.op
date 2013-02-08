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

#include "desktop_FakeGUI.h"
#include "boost_replacement.h"

#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/MessageQueueThread.h>
#include <boost/thread.hpp>

#include <zsLib/Log.h>

// NORMALLY we would not include private header but it's the only way to create an account bypassing the provisioning service for the sake of the unit test
#include <hookflash/core/internal/core_Account.h>

namespace hfcoretest { ZS_DECLARE_SUBSYSTEM(hfcoretest) }

typedef zsLib::MessageQueueThreadPtr MessageQueueThreadPtr;

typedef hookflash::core::IStack IStack;
typedef hookflash::core::IStackPtr IStackPtr;
typedef hookflash::core::ILogger ILogger;

namespace hfcoretest
{
  FakeGUITimer::FakeGUITimer(
                             zsLib::IMessageQueuePtr queue,
                             FakeGUIPtr outer
                             ) :
    MessageQueueAssociator(queue)
  {
    mOuter = outer;
  }
  
  void FakeGUITimer::init()
  {
    mTimer = zsLib::Timer::create(mThisWeak.lock(), zsLib::Seconds(1));
  }
  
  FakeGUITimerPtr FakeGUITimer::create(
                                       zsLib::IMessageQueuePtr queue,
                                       FakeGUIPtr outer
                                       )
  {
    FakeGUITimerPtr pThis(new FakeGUITimer(queue, outer));
    pThis->mThisWeak = pThis;
    pThis->init();
    return pThis;
  }
  
  FakeGUITimer::~FakeGUITimer()
  {
  }
  
  // TimerDelegate
  void FakeGUITimer::onTimer(zsLib::TimerPtr timer)
  {
    FakeGUIPtr gui = mOuter.lock();
    if (!gui) return;
    gui->tick();
  }
  
  FakeGUI::FakeGUI() :
    mShouldShutdown(false),
    mRunLoop(0),
    mHangupAtLoop(0)
  {}

  FakeGUIPtr FakeGUI::create()
  {
    FakeGUIPtr pThis = FakeGUIPtr(new FakeGUI());
    pThis->mThisWeak = pThis;
    pThis->init();
    return pThis;
  }
  
  void FakeGUI::init()
  {
  }
  
  void FakeGUI::go(
                   zsLib::IMessageQueuePtr queue,
                   hookflash::core::IStackPtr stack
                   )
  {
    std::cout << "WAITING:      Waiting for core test to complete...\n";
    mStack = stack;
    mTimer = FakeGUITimer::create(queue, mThisWeak.lock());
  }
  
  void FakeGUI::tick()
  {
    ++mRunLoop;

    if ((mHangupAtLoop == mRunLoop) &&
        (0 != mHangupAtLoop)) {
      mHangupAtLoop = 0;
      if (mCall) {
        mCall->hangup();
        mCall.reset();
        mHangupAtLoop = mRunLoop + 30;
      } else {
        mConversationThread.reset();
      }
    }
    
    switch (mRunLoop) {
#ifdef OPENPEER_TEST
      case 1:
      {
        // attempt to load the private peer file...
        FILE *file = fopen("/tmp/peer_load.txt", "r");
        if (file) {
          char buffer[1024*64+1];
          memset(&(buffer[0]), 0, sizeof(buffer));
          fread(buffer, sizeof(char), sizeof(buffer)-sizeof(char), file);
          fclose(file);
          
          mPrivatePeerFile = (zsLib::CSTR)buffer;
          
          if (OPENPEER_TEST_GENERATE_NEW_PRIVATE_PEER) {
            mPrivatePeerFile.clear();
          }
        }
        break;
      }
      case 2:
      {
        if (mPrivatePeerFile.isEmpty()) break;
        
        ZS_LOG_BASIC("--------------------------------------------------------------------------------")
        ZS_LOG_BASIC("LOADING:      Loading previous peer file instead of generating new peer file.")
        ZS_LOG_BASIC("--------------------------------------------------------------------------------")
        mAccount = hookflash::core::internal::Account::loadExisting(
                                                                    mStack,
                                                                    mThisWeak.lock(),
                                                                    mThisWeak.lock(),
                                                                    mThisWeak.lock(),
                                                                    mThisWeak.lock(),
                                                                    mThisWeak.lock(),
                                                                    mPrivatePeerFile,
                                                                    "123456",
                                                                    "192.168.2.30:5446",
                                                                    "siptest.yakolako.com",//URL_TURNSERVER
                                                                    "toto",
                                                                    "toto",
                                                                    "siptest.yakolako.com"//URL_STUNSERVER
                                                                    );
      }
      case 3:
      {
        if (!mPrivatePeerFile.isEmpty()) break;
        
        ZS_LOG_BASIC("--------------------------------------------------------------------------------")
        ZS_LOG_BASIC("GENERATING:   Account generation for \"John Doe\" in progress.")
        ZS_LOG_BASIC("--------------------------------------------------------------------------------")
        mAccount = hookflash::core::internal::Account::generate(
                                                                mStack,
                                                                mThisWeak.lock(),
                                                                mThisWeak.lock(),
                                                                mThisWeak.lock(),
                                                                mThisWeak.lock(),
                                                                mThisWeak.lock(),
                                                                "<whatever></whatever>",
                                                                NULL,
                                                                "123456",
                                                                "192.168.2.30:5446",
                                                                "siptest.yakolako.com", // URL_TURNSERVER
                                                                "toto",
                                                                "toto",
                                                                "turn.hookflash.me" // URL_STUNSERVER
                                                                );
        
        break;
      }
#endif //OPENPEER_TEST
        
#ifdef PROVISIONING_TEST
        
        
      case 5:
      {
        break;
      }
        
      case 9:
      {
        ZS_LOG_BASIC("-----------------------------------------------------------------------------------------------")
        ZS_LOG_BASIC("FIRST TIME OAUTH LOGIN:")
        ZS_LOG_BASIC("-----------------------------------------------------------------------------------------------")
        
        mProvisioningAccount = hookflash::core::provisioning::IAccount::create(mThisWeak.lock(), mThisWeak.lock(), "hfapi.hookflash.me");
        break;
      }
        
        
#endif // PROVISIONING_TEST
        
#ifdef OPENPEER_TEST
      case 600:
      {
        ZS_LOG_BASIC("--------------------------------------------------------------------------------")
        ZS_LOG_BASIC("SHUTDOWN:     Account shutdown in progress.")
        ZS_LOG_BASIC("--------------------------------------------------------------------------------")
        mAccount->shutdown();
        break;
      }
#endif // OPENPEER_TEST
        
      case 650: 
      {
        mTimer.reset();
        
        ZS_LOG_BASIC("--------------------------------------------------------------------------------")
        ZS_LOG_BASIC("SHUTDOWN:     Stack shutdown in progress.")
        ZS_LOG_BASIC("--------------------------------------------------------------------------------")
        mStack->shutdown();
        break;
      }
    }
  }
  
  void FakeGUI::onStackShutdown(hookflash::core::IStackAutoCleanupPtr ignoreThisArgument)
  {
#ifdef _WIN32
    ::PostThreadMessage(
                        mThreadID,  // this won't change after spawned so it is thread safe
                        WM_QUIT,
                        0,
                        0
                        );
#endif //_WIN32
#ifdef __APPLE__
    CFRunLoopStop(mRunLoopRef);
#endif //__APPLE__
  }
  
  void FakeGUI::onNewSubsystem(
                               zsLib::PTRNUMBER subsystemID,
                               const char *subsystemName
                               )
  {
    zsLib::AutoRecursiveLock lock(mLock);
    mSubsystemMap[subsystemID] = subsystemName;
  }
  
  void FakeGUI::onLog(
                      zsLib::PTRNUMBER subsystemID,
                      const char *subsystemName,
                      ILogger::Severity inSeverity,
                      ILogger::Level inLevel,
                      const char *inMessage,
                      const char *inFunction,
                      const char *inFilePath,
                      zsLib::ULONG inLineNumber
                      )
  {
  }
  

  void FakeGUI::onAccountStateChanged(
                                      hookflash::core::IAccountPtr account,
                                      hookflash::core::IAccount::AccountStates state
                                      )
  {}
  
  

  
}

using namespace hfcoretest;

void doFakeGUITest()
{
  bool skip = false;
  if (skip) return;
  
  BOOST_INSTALL_LOGGER();
  
  IStackPtr stack;
  bool thrown = false;
  try {
    stack = IStack::singleton();
  } catch(...) {
    thrown = true;
  }
  BOOST_CHECK(thrown) // this should throw!
  thrown = false;
  
  //IStack::setup();
  
  try {
    stack = IStack::singleton();
  } catch(...) {
    thrown = true;
  }
  BOOST_CHECK(!thrown)  // this shoudl not throw because IClient::setup() was called
  thrown = false;
  
  MessageQueueThreadPtr mainThreadQueue = zsLib::MessageQueueThread::singletonUsingCurrentGUIThreadsMessageQueue();
  MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());

  FakeGUIPtr testGUI = FakeGUI::create();
  IStack::singleton()->setup(testGUI, testGUI, testGUI, testGUI, "uuid", "hookflash/1.0.FakeGUI (iOS/iPad)", "iOS 5.0.3", "iPad 2");

  ILogger::setLogLevel(ILogger::Trace);
  // IClient::setLogLevel("hookflash_provisioning_message", IClient::Log::Trace);

  ILogger::installStdOutLogger(false);
  // ILogger::installFileLogger("/tmp/hookflash.fifo", true);
  // ILogger::installTelnetLogger(59999, 60, true);
  // ILogger::installWindowsDebuggerLogger();
  ILogger::installCustomLogger(testGUI);

  testGUI->go(
              mainThreadQueue,
              stack
              );
  
#ifdef _WIN32
  MSG msg;
  ZeroMemory(&msg, sizeof(msg));
  while (::GetMessage(&msg, NULL, 0, 0))
  {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
#endif //_WIN32
#ifdef __APPLE__
  CFRunLoopRun();
#endif //__APPLE__

  stack->shutdown();
}
