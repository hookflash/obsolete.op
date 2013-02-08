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
#include <hookflash/services/IRUDPICESocket.h>
#include <hookflash/services/IRUDPICESocketSession.h>
#include <hookflash/services/IRUDPMessaging.h>
#include <hookflash/services/IHelper.h>


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
using hookflash::services::IRUDPICESocket;
using hookflash::services::IRUDPICESocketPtr;
using hookflash::services::IRUDPICESocketDelegate;
using hookflash::services::IRUDPICESocketSession;
using hookflash::services::IRUDPICESocketSessionPtr;
using hookflash::services::IRUDPICESocketSessionDelegate;
using hookflash::services::IRUDPMessaging;
using hookflash::services::IRUDPMessagingPtr;
using hookflash::services::IRUDPMessagingDelegate;
using hookflash::services::IHelper;
using hookflash::services::IICESocket;
using hookflash::services::IDNS;

static const char *gUsername = HOOKFLASH_SERVICE_TEST_TURN_USERNAME;
static const char *gPassword = HOOKFLASH_SERVICE_TEST_TURN_PASSWORD;

namespace hookflash
{
  namespace services
  {
    namespace test
    {
      class TestRUDPICESocketCallback;
      typedef boost::shared_ptr<TestRUDPICESocketCallback> TestRUDPICESocketCallbackPtr;
      typedef boost::weak_ptr<TestRUDPICESocketCallback> TestRUDPICESocketCallbackWeakPtr;

      class TestRUDPICESocketCallback : public zsLib::MessageQueueAssociator,
                                        public IRUDPICESocketDelegate,
                                        public IRUDPICESocketSessionDelegate,
                                        public IRUDPMessagingDelegate
      {
      private:
        TestRUDPICESocketCallback(
                                  zsLib::IMessageQueuePtr queue,
                                  const zsLib::IPAddress &serverIP
                                  ) :
          zsLib::MessageQueueAssociator(queue),
          mServerIP(serverIP),
          mSocketShutdown(false),
          mSessionShutdown(false),
          mMessagingShutdown(false)
        {
        }

        void init()
        {
          zsLib::AutoRecursiveLock lock(mLock);
          mSocket = IRUDPICESocket::create(
                                             getAssociatedMessageQueue(),
                                             mThisWeak.lock(),
                                             HOOKFLASH_SERVICE_TEST_TURN_SERVER_DOMAIN,
                                             gUsername,
                                             gPassword,
                                             HOOKFLASH_SERVICE_TEST_STUN_SERVER
                                             );
        }

      public:
        static TestRUDPICESocketCallbackPtr create(
                                                   zsLib::IMessageQueuePtr queue,
                                                   zsLib::IPAddress serverIP
                                                   )
        {
          TestRUDPICESocketCallbackPtr pThis(new TestRUDPICESocketCallback(queue, serverIP));
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        ~TestRUDPICESocketCallback()
        {
        }

        void shutdown()
        {
          zsLib::AutoRecursiveLock lock(mLock);
          mSocket->shutdown();
        }

        bool isShutdown()
        {
          zsLib::AutoRecursiveLock lock(mLock);
          return mSocketShutdown && mSessionShutdown && mMessagingShutdown;
        }

        virtual void onRUDPICESocketStateChanged(
                                                 IRUDPICESocketPtr socket,
                                                 RUDPICESocketStates state
                                                 )
        {
          zsLib::AutoRecursiveLock lock(mLock);
          if (socket != mSocket) return;

          switch (state) {
            case IRUDPICESocket::RUDPICESocketState_Ready:
            {
              IRUDPICESocket::CandidateList candidates;
              IRUDPICESocket::Candidate candidate;
              candidate.mType = IICESocket::Type_Local;
              candidate.mIPAddress = mServerIP;
              candidate.mPriority = 0;
              candidate.mLocalPreference = 0;

              candidates.push_back(candidate);

              mSocketSession = mSocket->createSessionFromRemoteCandidates(
                                                                          mThisWeak.lock(),
                                                                          candidates,
                                                                          IICESocket::ICEControl_Controlling
                                                                          );
              break;
            }
            case IRUDPICESocket::RUDPICESocketState_Shutdown:
            {
              mSocketShutdown = true;
              break;
            }
            default: break;
          }
        }

        virtual void onRUDPICESocketSessionStateChanged(
                                                        IRUDPICESocketSessionPtr session,
                                                        RUDPICESocketSessionStates state
                                                        )
        {
          zsLib::AutoRecursiveLock lock(mLock);
          if (IRUDPICESocketSession::RUDPICESocketSessionState_Ready == state) {
            mMessaging = IRUDPMessaging::openChannel(
                                                     getAssociatedMessageQueue(),
                                                     mSocketSession,
                                                     mThisWeak.lock(),
                                                     "bogus/text-bogus"
                                                     );
          }
          if (IRUDPICESocketSession::RUDPICESocketSessionState_Shutdown == state) {
            mSessionShutdown = true;
          }
        }

        virtual void onRUDPICESocketSessionChannelWaiting(IRUDPICESocketSessionPtr session)
        {
        }

        virtual void onRUDPMessagingStateChanged(
                                                 IRUDPMessagingPtr session,
                                                 RUDPMessagingStates state
                                                 )
        {
          zsLib::AutoRecursiveLock lock(mLock);
          if (IRUDPMessaging::RUDPMessagingState_Connected == state) {
            mMessaging->send((const BYTE *)"*HELLO*", strlen("*HELLO*"));
          }
          if (IRUDPMessaging::RUDPMessagingState_Shutdown == state) {
            mMessagingShutdown = true;
          }
        }

        virtual void onRUDPMessagingReadReady(IRUDPMessagingPtr session)
        {
          zsLib::AutoRecursiveLock lock(mLock);
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

            zsLib::String add = "<SOCKET->" + IHelper::randomString(1000) + ">";

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
        TestRUDPICESocketCallbackWeakPtr mThisWeak;

        zsLib::IPAddress mServerIP;

        bool mSocketShutdown;
        bool mSessionShutdown;
        bool mMessagingShutdown;

        IRUDPMessagingPtr mMessaging;
        IRUDPICESocketPtr mSocket;
        IRUDPICESocketSessionPtr mSocketSession;
      };
    }
  }
}

using namespace hookflash::services::test;
using hookflash::services::test::TestRUDPICESocketCallback;
using hookflash::services::test::TestRUDPICESocketCallbackPtr;

void doTestRUDPICESocket()
{
  if (!HOOKFLASH_SERVICE_TEST_DO_RUDPICESOCKET_CLIENT_TO_SERVER_TEST) return;
  if (!HOOKFLASH_SERVICE_TEST_RUNNING_AS_CLIENT) return;

  BOOST_INSTALL_LOGGER();

  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());

  TestRUDPICESocketCallbackPtr testObject1 = TestRUDPICESocketCallback::create(thread, IPAddress(HOOKFLASH_SERVICE_TEST_RUDP_SERVER_IP, HOOKFLASH_SERVICE_TEST_RUDP_SERVER_PORT));

  ZS_LOG_BASIC("WAITING:      Waiting for RUDP ICE socket testing to complete (max wait is 60 minutes).");

  {
    int expecting = 1;
    int found = 0;

    ULONG totalWait = 0;
    do
    {
      boost::this_thread::sleep(zsLib::Seconds(1));
      ++totalWait;
      if (totalWait >= (10*60))
        break;

      if ((5*60) == totalWait) {
        testObject1->shutdown();
      }

      found = 0;
      if (testObject1->isShutdown()) ++found;

      if (found == expecting)
        break;

    } while(true);
    BOOST_CHECK(found == expecting)
  }

  testObject1.reset();

  ZS_LOG_BASIC("WAITING:      All RUDP sockets have finished. Waiting for 'bogus' events to process (10 second wait).");

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
