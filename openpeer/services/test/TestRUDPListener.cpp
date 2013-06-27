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


#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>
#include <zsLib/Socket.h>
#include <zsLib/Timer.h>
#include <openpeer/services/IRUDPListener.h>
#include <openpeer/services/IRUDPMessaging.h>
#include <openpeer/services/IHelper.h>
#include <openpeer/services/IDNS.h>


#include "config.h"
#include "boost_replacement.h"

namespace hookflash { namespace services { namespace test { ZS_DECLARE_SUBSYSTEM(hookflash_services_test) } } }

using zsLib::BYTE;
using zsLib::WORD;
using zsLib::ULONG;
using zsLib::CSTR;
using zsLib::Socket;
using zsLib::SocketPtr;
using zsLib::ISocketPtr;
using zsLib::IPAddress;
using zsLib::AutoRecursiveLock;
using hookflash::services::IRUDPListener;
using hookflash::services::IRUDPListenerPtr;
using hookflash::services::IRUDPListenerDelegate;
using hookflash::services::IRUDPMessaging;
using hookflash::services::IRUDPMessagingPtr;
using hookflash::services::IRUDPMessagingDelegate;
using hookflash::services::IHelper;
using hookflash::services::IDNS;

namespace hookflash
{
  namespace services
  {
    namespace test
    {
      class TestRUDPListenerCallback;
      typedef boost::shared_ptr<TestRUDPListenerCallback> TestRUDPListenerCallbackPtr;
      typedef boost::weak_ptr<TestRUDPListenerCallback> TestRUDPListenerCallbackWeakPtr;

      class TestRUDPListenerCallback : public zsLib::MessageQueueAssociator,
                                       public IRUDPListenerDelegate,
                                       public IRUDPMessagingDelegate
      {
      private:
        TestRUDPListenerCallback(zsLib::IMessageQueuePtr queue) :
          zsLib::MessageQueueAssociator(queue)
        {
        }

        void init(WORD port)
        {
          AutoRecursiveLock lock(mLock);
          mListener = IRUDPListener::create(getAssociatedMessageQueue(), mThisWeak.lock(), port);
        }

      public:
        static TestRUDPListenerCallbackPtr create(
                                                  zsLib::IMessageQueuePtr queue,
                                                  WORD port
                                                  )
        {
          TestRUDPListenerCallbackPtr pThis(new TestRUDPListenerCallback(queue));
          pThis->mThisWeak = pThis;
          pThis->init(port);
          return pThis;
        }

        ~TestRUDPListenerCallback()
        {
        }

        virtual void onRUDPListenerStateChanged(
                                                IRUDPListenerPtr listener,
                                                RUDPListenerStates state
                                                )
        {
          AutoRecursiveLock lock(mLock);
        }

        virtual void onRUDPListenerChannelWaiting(IRUDPListenerPtr listener)
        {
          zsLib::AutoRecursiveLock lock(mLock);
          mMessaging = IRUDPMessaging::acceptChannel(
                                                     getAssociatedMessageQueue(),
                                                     mListener,
                                                     mThisWeak.lock()
                                                     );

        }

        virtual void onRUDPMessagingStateChanged(
                                                 IRUDPMessagingPtr session,
                                                 RUDPMessagingStates state
                                                 )
        {
        }

        virtual void onRUDPMessagingReadReady(IRUDPMessagingPtr session)
        {
          AutoRecursiveLock lock(mLock);
          if (session != mMessaging) return;
          while (true) {
            zsLib::ULONG messageSize = mMessaging->getNextReceivedMessageSizeInBytes();
            boost::shared_array<BYTE> buffer = mMessaging->getBufferLargeEnoughForNextMessage();
            if (!buffer) return;

            mMessaging->receive(buffer.get());

            zsLib::String str = (CSTR)(buffer.get());
            ZS_LOG_BASIC("-------------------------------------------------------------------------------")
            ZS_LOG_BASIC("-------------------------------------------------------------------------------")
            ZS_LOG_BASIC("-------------------------------------------------------------------------------")
            ZS_LOG_BASIC(zsLib::String("RECEIVED: \"") + str + "\"")
            ZS_LOG_BASIC("-------------------------------------------------------------------------------")
            ZS_LOG_BASIC("-------------------------------------------------------------------------------")
            ZS_LOG_BASIC("-------------------------------------------------------------------------------")

            zsLib::String add = "(SERVER->" + IHelper::randomString(10) + ")";

            zsLib::ULONG newMessageSize = messageSize + add.length();
            boost::shared_array<BYTE> newBuffer(new BYTE[newMessageSize]);

            memcpy(newBuffer.get(), buffer.get(), messageSize);
            memcpy(newBuffer.get() + messageSize, (const zsLib::BYTE *)(add.c_str()), add.length());

            mMessaging->send(newBuffer.get(), newMessageSize);
          }
        }

        virtual void onRUDPMessagingWriteReady(IRUDPMessagingPtr session)
        {
        }

      private:
        mutable zsLib::RecursiveLock mLock;
        TestRUDPListenerCallbackWeakPtr mThisWeak;

        IRUDPMessagingPtr mMessaging;
        IRUDPListenerPtr mListener;
      };
    }
  }
}

using namespace hookflash::services::test;

using hookflash::services::test::TestRUDPListenerCallback;
using hookflash::services::test::TestRUDPListenerCallbackPtr;

void doTestRUDPListener()
{
  if (!HOOKFLASH_SERVICE_TEST_DO_RUDPICESOCKET_CLIENT_TO_SERVER_TEST) return;
  if (HOOKFLASH_SERVICE_TEST_RUNNING_AS_CLIENT) return;

  BOOST_INSTALL_LOGGER();

  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());

  TestRUDPListenerCallbackPtr testObject1 = TestRUDPListenerCallback::create(thread, HOOKFLASH_SERVICE_TEST_RUDP_SERVER_PORT);

  ZS_LOG_BASIC("WAITING:      Waiting for RUDP Listener testing to complete (max wait is 60 minutes).");

  {
    ULONG totalWait = 0;
    do
    {
      boost::this_thread::sleep(zsLib::Seconds(1));
      ++totalWait;
      if (totalWait >= (60*60))
        break;
    } while(true);
  }

  testObject1.reset();

  ZS_LOG_BASIC("WAITING:      All RUDP listeners have finished. Waiting for 'bogus' events to process (10 second wait).");

  boost::this_thread::sleep(zsLib::Seconds(10));

  // wait for shutdown
  {
    ULONG count = 0;
    do
    {
      count = thread->getTotalUnprocessedMessages();
      //    count += mThreadNeverCalled->getTotalUnprocessedMessages();
      if (0 != count)
        boost::this_thread::yield();
    } while (count > 0);

    thread->waitForShutdown();
  }
  BOOST_UNINSTALL_LOGGER();
  zsLib::proxyDump();
  BOOST_EQUAL(zsLib::proxyGetTotalConstructed(), 0);
}
