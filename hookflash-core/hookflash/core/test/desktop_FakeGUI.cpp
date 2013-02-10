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

#include "config.h"
#include "desktop_FakeGUI.h"
#include "boost_replacement.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif //__APPLE__

#include <zsLib/Log.h>

namespace hookflash { namespace core { namespace test { ZS_DECLARE_SUBSYSTEM(hfcoretest) } } }

namespace hookflash
{
  namespace core
  {
    namespace test
    {
      FakeGUI::FakeGUI()
      {
      }

      void FakeGUI::init()
      {
        IStack::singleton()->setup(mThisWeak.lock(), mThisWeak.lock(), "123456", "hookflash/1.0.1001a (iOS/iPad)", "iOS 5.0.3", "iPad 2");
      }

      FakeGUI::~FakeGUI()
      {
        mThisWeak.reset();
      }

      FakeGUIPtr FakeGUI::create()
      {
        FakeGUIPtr pThis(new FakeGUI);
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      void FakeGUI::onStackShutdown(IStackAutoCleanupPtr)
      {
#ifdef __APPLE__
        CFRunLoopStop(CFRunLoopGetCurrent());
#endif // __APPLE__
      }

      void FakeGUI::onMediaEngineAudioRouteChanged(OutputAudioRoutes audioRoute)
      {
      }

    }
  }
}

using hookflash::core::test::FakeGUI;
using hookflash::core::test::FakeGUIPtr;

void doFakeGUITest()
{
  if (!HOOKFLASH_CORE_TEST_DO_FAKE_GUI_TEST) return;

  BOOST_INSTALL_LOGGER();

  FakeGUIPtr fakeGUI = FakeGUI::create();

//  IStack::setup(threadDelegate, threadStack, threadServices, "123456", "hookflash/1.0.1001a (iOS/iPad)", "iOS 5.0.3", "iPad 2");

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

}

void foo()
{
//  if (!HOOKFLASH_SERVICE_TEST_DO_STACK_TEST) return;
//
//  BOOST_INSTALL_LOGGER();
//
//  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());
//  zsLib::MessageQueueThreadPtr threadDelegate(zsLib::MessageQueueThread::createBasic());
//  zsLib::MessageQueueThreadPtr threadStack(zsLib::MessageQueueThread::createBasic());
//  zsLib::MessageQueueThreadPtr threadServices(zsLib::MessageQueueThread::createBasic());
//
//  IStack::setup(threadDelegate, threadStack, threadServices, "123456", "hookflash/1.0.1001a (iOS/iPad)", "iOS 5.0.3", "iPad 2");
//
//  TestStackCallbackPtr testObject = TestStackCallback::create(thread);
//
//  std::cout << "WAITING:      Waiting for stack test to complete (max wait is 60 seconds).\n";
//
//  ULONG expectingProcessed = 0;
//
//  // count from each object
//  expectingProcessed += (testObject ? 1 : 0);
//
//  // check to see if all test routines have completed
//  {
//    ULONG lastProcessed = 0;
//    ULONG totalWait = 0;
//    do
//    {
//      ULONG totalProcessed = 0;
//      if (totalProcessed != lastProcessed) {
//        lastProcessed = totalProcessed;
//        std::cout << "WAITING:      [" << totalProcessed << "\n";
//      }
//
//      // tally up count from each object
//      totalProcessed += testObject->mCount;
//
//      if (totalProcessed < expectingProcessed) {
//        ++totalWait;
//        boost::this_thread::sleep(zsLib::Seconds(1));
//      }
//      else
//        break;
//    } while (totalWait < (60)); // max three minutes
//    BOOST_CHECK(totalWait < (60));
//  }
//
//  std::cout << "\n\nWAITING:      All tests have finished. Waiting for 'bogus' events to process (10 second wait).\n";
//  boost::this_thread::sleep(zsLib::Seconds(10));
//
//  BOOST_CHECK(testObject->mNetworkDone)
//  BOOST_CHECK(testObject->mNetwork->isPreparationComplete())
//  BOOST_CHECK(testObject->mNetwork->wasSuccessful())
//
//  // wait for shutdown
//  {
//    ULONG count = 0;
//    do
//    {
//      count = 0;
//
//      count += thread->getTotalUnprocessedMessages();
//      count += threadDelegate->getTotalUnprocessedMessages();
//      count += threadStack->getTotalUnprocessedMessages();
//      count += threadServices->getTotalUnprocessedMessages();
//      if (0 != count)
//        boost::this_thread::yield();
//    } while (count > 0);
//    
//    thread->waitForShutdown();
//  }
//  BOOST_UNINSTALL_LOGGER()
//  zsLib::proxyDump();
//  BOOST_EQUAL(zsLib::proxyGetTotalConstructed(), 0);
}
