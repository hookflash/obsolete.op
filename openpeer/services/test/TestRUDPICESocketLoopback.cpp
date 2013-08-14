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
#include <openpeer/services/IRUDPICESocket.h>
#include <openpeer/services/IRUDPICESocketSession.h>
#include <openpeer/services/IRUDPMessaging.h>
#include <openpeer/services/ITransportStream.h>

#include <boost/shared_array.hpp>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>

#include "config.h"
#include "boost_replacement.h"

#include <list>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <cstring>

namespace openpeer { namespace services { namespace test { ZS_DECLARE_SUBSYSTEM(openpeer_services_test) } } }

using zsLib::BYTE;
using zsLib::WORD;
using zsLib::ULONG;
using zsLib::Socket;
using zsLib::SocketPtr;
using zsLib::ISocketPtr;
using zsLib::IPAddress;
using zsLib::String;
using zsLib::string;
using openpeer::services::IDNS;
using openpeer::services::IDNSQuery;
using openpeer::services::ITURNSocket;
using openpeer::services::ITURNSocketPtr;
using openpeer::services::ITURNSocketDelegate;
using openpeer::services::IICESocket;
using openpeer::services::IRUDPICESocket;
using openpeer::services::IRUDPICESocketPtr;
using openpeer::services::IRUDPMessaging;
using openpeer::services::IRUDPMessagingPtr;
using namespace openpeer::services::test;

namespace openpeer
{
  namespace services
  {
    namespace test
    {
      static const char *gUsername = OPENPEER_SERVICE_TEST_TURN_USERNAME;
      static const char *gPassword = OPENPEER_SERVICE_TEST_TURN_PASSWORD;

      class TestRUDPICESocketLoopback;
      typedef boost::shared_ptr<TestRUDPICESocketLoopback> TestRUDPICESocketLoopbackPtr;
      typedef boost::weak_ptr<TestRUDPICESocketLoopback> TestRUDPICESocketLoopbackWeakPtr;

      class TestRUDPICESocketLoopback : public zsLib::MessageQueueAssociator,
                                        public IRUDPICESocketDelegate,
                                        public IRUDPICESocketSessionDelegate,
                                        public IRUDPMessagingDelegate,
                                        public ITransportStreamWriterDelegate,
                                        public ITransportStreamReaderDelegate,
                                        public zsLib::ITimerDelegate
      {
      protected:
        typedef std::list<IRUDPICESocketSessionPtr> SessionList;
        typedef std::list<IRUDPMessagingPtr> MessagingList;

      private:
        //---------------------------------------------------------------------
        TestRUDPICESocketLoopback(zsLib::IMessageQueuePtr queue) :
          zsLib::MessageQueueAssociator(queue),
          mReceiveStream(ITransportStream::create()->getReader()),
          mSendStream(ITransportStream::create()->getWriter()),
          mExpectConnected(false),
          mExpectGracefulShutdown(false),
          mExpectErrorShutdown(false),
          mExpectSessionConnected(false),
          mExpectSessionClosed(false),
          mIssueConnect(false),
          mConnected(false),
          mGracefulShutdown(false),
          mErrorShutdown(false),
          mShutdownCalled(false),
          mSessionConnected(false),
          mSessionClosed(false),
          mExpectMessagingConnected(false),
          mExpectMessagingShutdown(false),
          mMessagingConnected(false),
          mMessagingShutdown(false)
        {
        }

        //---------------------------------------------------------------------
        void init(
                  WORD port,
                  const char *srvNameTURN,
                  const char *srvNameSTUN
                  )
        {
          zsLib::AutoRecursiveLock lock(getLock());

          mReceiveStreamSubscription = mReceiveStream->subscribe(mThisWeak.lock());
          mReceiveStream->notifyReaderReadyToRead();

          mRUDPSocket = IRUDPICESocket::create(
                                               getAssociatedMessageQueue(),
                                               mThisWeak.lock(),
                                               srvNameTURN,
                                               gUsername,
                                               gPassword,
                                               srvNameSTUN,
                                               port
                                               );

          mTimer = zsLib::Timer::create(mThisWeak.lock(), zsLib::Milliseconds(rand()%400+200));
        }

      public:
        //---------------------------------------------------------------------
        static TestRUDPICESocketLoopbackPtr create(
                                                   zsLib::IMessageQueuePtr queue,
                                                   WORD port,
                                                   const char *srvNameTURN,
                                                   const char *srvNameSTUN,
                                                   bool issueConnect,
                                                   bool expectConnected = true,
                                                   bool expectGracefulShutdown = true,
                                                   bool expectErrorShutdown = false,
                                                   bool expectSessionConnected = true,
                                                   bool expectSessionClosed = true,
                                                   bool expectMessagingConnected = true,
                                                   bool expectMessagingShutdown = true
                                                   )
        {
          TestRUDPICESocketLoopbackPtr pThis(new TestRUDPICESocketLoopback(queue));
          pThis->mThisWeak = pThis;
          pThis->mIssueConnect = issueConnect;
          pThis->mExpectConnected = expectConnected;
          pThis->mExpectGracefulShutdown = expectGracefulShutdown;
          pThis->mExpectErrorShutdown = expectErrorShutdown;
          pThis->mExpectSessionConnected = expectSessionConnected;
          pThis->mExpectSessionClosed = expectSessionClosed;
          pThis->mExpectMessagingConnected = expectMessagingConnected;
          pThis->mExpectMessagingShutdown = expectMessagingShutdown;
          pThis->init(port, srvNameTURN, srvNameSTUN);
          return pThis;
        }

        //---------------------------------------------------------------------
        ~TestRUDPICESocketLoopback()
        {
          if (mTimer) {
            mTimer->cancel();
            mTimer.reset();
          }
          mSessions.clear();
          mRUDPSocket.reset();
        }

        //---------------------------------------------------------------------
        virtual void onRUDPICESocketStateChanged(
                                                 IRUDPICESocketPtr socket,
                                                 RUDPICESocketStates state
                                                 )
        {
          zsLib::AutoRecursiveLock lock(getLock());
          switch (state) {
            case IRUDPICESocket::RUDPICESocketState_Ready:
            {
              BOOST_CHECK(mExpectConnected);
              mConnected = true;
              break;
            }
            case IRUDPICESocket::RUDPICESocketState_Shutdown:
            {
              if (mShutdownCalled) {
                BOOST_CHECK(mExpectGracefulShutdown);
                mGracefulShutdown = true;
              } else {
                BOOST_CHECK(mExpectErrorShutdown);
                mErrorShutdown = true;
              }
              mRUDPSocket.reset();
              break;
            }
            default:  break;
          }
        }

        //---------------------------------------------------------------------
        virtual void onRUDPICESocketCandidatesChanged(IRUDPICESocketPtr socket)
        {
          zsLib::AutoRecursiveLock lock(getLock());
          TestRUDPICESocketLoopbackPtr remote = mRemote.lock();
          if (!remote) return;

          if (!mRUDPSocket) return;

          IICESocket::CandidateList candidates;
          socket->getLocalCandidates(candidates);

          remote->updateCandidates(candidates);
        }

        //---------------------------------------------------------------------
        virtual void onRUDPICESocketSessionStateChanged(
                                                        IRUDPICESocketSessionPtr session,
                                                        RUDPICESocketSessionStates state
                                                        )
        {
          zsLib::AutoRecursiveLock lock(getLock());

          switch(state) {
            case IRUDPICESocketSession::RUDPICESocketSessionState_Ready:
            {
              BOOST_CHECK(mExpectSessionConnected);
              mSessionConnected = true;

              if (IRUDPICESocketSession::RUDPICESocketSessionState_Ready == state) {
                if (mIssueConnect) {
                  IRUDPMessagingPtr messaging = IRUDPMessaging::openChannel(
                                                                            getAssociatedMessageQueue(),
                                                                            session,
                                                                            mThisWeak.lock(),
                                                                            "bogus/text-bogus",
                                                                            mReceiveStream->getStream(),
                                                                            mSendStream->getStream()
                                                                            );
                  mMessaging.push_back(messaging);
                }
              }

              SessionList::iterator found = find(mSessions.begin(), mSessions.end(), session);
              BOOST_CHECK(found != mSessions.end())
              break;
            }
            case IRUDPICESocketSession::RUDPICESocketSessionState_Shutdown:
            {
              BOOST_CHECK(mExpectSessionClosed);
              mSessionClosed = true;

              SessionList::iterator found = find(mSessions.begin(), mSessions.end(), session);
              BOOST_CHECK(found != mSessions.end())
              mSessions.erase(found);
            }
            default: break;
          }
        }

        //---------------------------------------------------------------------
        virtual void onRUDPICESocketSessionChannelWaiting(IRUDPICESocketSessionPtr session)
        {
          zsLib::AutoRecursiveLock lock(getLock());
          BOOST_CHECK(mSessionConnected)

          IRUDPMessagingPtr messaging = IRUDPMessaging::acceptChannel(
                                                                      getAssociatedMessageQueue(),
                                                                      session,
                                                                      mThisWeak.lock(),
                                                                      mReceiveStream->getStream(),
                                                                      mSendStream->getStream()
                                                                      );
          mMessaging.push_back(messaging);

          SessionList::iterator found = find(mSessions.begin(), mSessions.end(), session);
          BOOST_CHECK(found != mSessions.end())
        }

        //---------------------------------------------------------------------
        virtual void onRUDPMessagingStateChanged(
                                                 IRUDPMessagingPtr messaging,
                                                 RUDPMessagingStates state
                                                 )
        {
          zsLib::AutoRecursiveLock lock(getLock());
          MessagingList::iterator found = find(mMessaging.begin(), mMessaging.end(), messaging);
          BOOST_CHECK(found != mMessaging.end())
          if (IRUDPMessaging::RUDPMessagingState_Connected == state)
          {
            BOOST_CHECK(mExpectMessagingConnected)
            mMessagingConnected = true;
            if (mIssueConnect) {
              static const char *message = "(*CONTROLLING**1234567890->tuTu8afutA6HatabASPeC9epHE2aHa3efew2xEc3acRANeVamUbrUsteh9C24e5h<-0987654321)";
              mSendStream->write((const BYTE *)message, strlen(message));
            } else {
              static const char *message = "(*CONTROLLED**1234567890->tuTu8afutA6HatabASPeC9epHE2aHa3efew2xEc3acRANeVamUbrUsteh9C24e5h<-0987654321)";
              mSendStream->write((const BYTE *)message, strlen(message));
            }
          }
          if (IRUDPMessaging::RUDPMessagingState_Shutdown == state)
          {
            BOOST_CHECK(mExpectMessagingShutdown)
            mMessagingShutdown = true;
            mMessaging.erase(found);
          }
        }

        //---------------------------------------------------------------------
        virtual void onTransportStreamReaderReady(ITransportStreamReaderPtr reader)
        {
          zsLib::AutoRecursiveLock lock(getLock());

          SecureByteBlockPtr buffer = mReceiveStream->read();
          if (!buffer) return;

          ULONG size = buffer->SizeInBytes() - sizeof(char);

          ZS_LOG_BASIC("---------------------------------------------------------------");
          ZS_LOG_BASIC("---------------------------------------------------------------");
          ZS_LOG_BASIC(String(mIssueConnect ? "CONTROLLING: " : "CONTROLLED: ") + ((const char *)(buffer->BytePtr())))
          ZS_LOG_BASIC("---------------------------------------------------------------");
          ZS_LOG_BASIC("---------------------------------------------------------------");

          // echo back the message to the remote party
          mSendStream->write(buffer->BytePtr(), size);
        }

        //---------------------------------------------------------------------
        virtual void onTransportStreamWriterReady(ITransportStreamWriterPtr writer)
        {
          //IGNORED
        }

        //---------------------------------------------------------------------
        virtual void onRUDPMessagingReadReady(IRUDPMessagingPtr messaging)
        {
        }

        //---------------------------------------------------------------------
        virtual void onRUDPMessagingWriteReady(IRUDPMessagingPtr messaging)
        {
          zsLib::AutoRecursiveLock lock(getLock());
        }

        //---------------------------------------------------------------------
        virtual void onTimer(zsLib::TimerPtr timer)
        {
          zsLib::AutoRecursiveLock lock(getLock());
          if (timer != mTimer) return;
        }

        //---------------------------------------------------------------------
        void shutdown()
        {
          zsLib::AutoRecursiveLock lock(getLock());

          mRemote.reset();

          if (!mRUDPSocket) return;
          if (mShutdownCalled) return;
          mShutdownCalled = true;
          for (MessagingList::iterator iter = mMessaging.begin(); iter != mMessaging.end(); ++iter) {
            IRUDPMessagingPtr &messaging = (*iter);
            messaging->shutdown();
          }
          for (SessionList::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter) {
            IRUDPICESocketSessionPtr &session = (*iter);
            session->shutdown();
          }
          mRUDPSocket->shutdown();
          if (mTimer) {
            mTimer->cancel();
            mTimer.reset();
          }
        }

        //---------------------------------------------------------------------
        bool isComplete()
        {
          zsLib::AutoRecursiveLock lock(getLock());
          return (mExpectConnected == mConnected) &&
                 (mExpectGracefulShutdown == mGracefulShutdown) &&
                 (mExpectErrorShutdown == mErrorShutdown) &&
                 (mExpectSessionConnected == mSessionConnected) &&
                 (mExpectSessionClosed == mSessionClosed) &&
                 (mExpectMessagingConnected == mMessagingConnected) &&
                 (mExpectMessagingShutdown == mMessagingShutdown);
        }

        void expectationsOkay() {
          zsLib::AutoRecursiveLock lock(getLock());
          if (mExpectConnected) {
            BOOST_CHECK(mConnected);
          } else {
            BOOST_CHECK(!mConnected);
          }

          if (mExpectGracefulShutdown) {
            BOOST_CHECK(mGracefulShutdown);
          } else {
            BOOST_CHECK(!mGracefulShutdown);
          }

          if (mExpectErrorShutdown) {
            BOOST_CHECK(mErrorShutdown);
          } else {
            BOOST_CHECK(!mErrorShutdown);
          }

          if (mExpectSessionConnected) {
            BOOST_CHECK(mSessionConnected);
          } else {
            BOOST_CHECK(!mSessionConnected);
          }

          if (mExpectSessionClosed) {
            BOOST_CHECK(mSessionClosed);
          } else {
            BOOST_CHECK(!mSessionClosed);
          }
          if (mExpectMessagingConnected) {
            BOOST_CHECK(mMessagingConnected);
          } else {
            BOOST_CHECK(!mMessagingConnected);
          }
          if (mExpectMessagingShutdown) {
            BOOST_CHECK(mMessagingShutdown);
          } else {
            BOOST_CHECK(!mMessagingShutdown);
          }
        }

        //---------------------------------------------------------------------
        void getLocalCandidates(IICESocket::CandidateList &outCandidates)
        {
          zsLib::AutoRecursiveLock lock(getLock());
          if (!mRUDPSocket) return;
          mRUDPSocket->getLocalCandidates(outCandidates);
        }

        //---------------------------------------------------------------------
        String getLocalUsernameFrag()
        {
          zsLib::AutoRecursiveLock lock(getLock());
          if (!mRUDPSocket) return String();
          return mRUDPSocket->getUsernameFrag();
        }

        //---------------------------------------------------------------------
        String getLocalPassword()
        {
          zsLib::AutoRecursiveLock lock(getLock());
          if (!mRUDPSocket) return String();
          return mRUDPSocket->getPassword();
        }
        
        //---------------------------------------------------------------------
        IRUDPICESocketSessionPtr createSessionFromRemoteCandidates(IICESocket::ICEControls control)
        {
          zsLib::AutoRecursiveLock lock(getLock());
          if (!mRUDPSocket) return IRUDPICESocketSessionPtr();

          TestRUDPICESocketLoopbackPtr remote = mRemote.lock();
          if (!remote) return IRUDPICESocketSessionPtr();

          String remoteUsernameFrag = remote->getLocalUsernameFrag();
          String remotePassword = remote->getLocalPassword();
          IICESocket::CandidateList remoteCandidates;
          remote->getLocalCandidates(remoteCandidates);

          IRUDPICESocketSessionPtr session = mRUDPSocket->createSessionFromRemoteCandidates(mThisWeak.lock(), remote->getLocalUsernameFrag(), remote->getLocalPassword(), remoteCandidates, control);
          mSessions.push_back(session);

          return session;
        }

        //---------------------------------------------------------------------
        void setRemote(TestRUDPICESocketLoopbackPtr remote)
        {
          zsLib::AutoRecursiveLock lock(getLock());
          mRemote = remote;
        }

        //---------------------------------------------------------------------
        void updateCandidates(const IICESocket::CandidateList &candidates)
        {
          zsLib::AutoRecursiveLock lock(getLock());
          for (SessionList::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter)
          {
            IRUDPICESocketSessionPtr session = (*iter);
            session->updateRemoteCandidates(candidates);
          }
        }

        //---------------------------------------------------------------------
        void notifyEndOfCandidates()
        {
          zsLib::AutoRecursiveLock lock(getLock());
          for (SessionList::iterator iter = mSessions.begin(); iter != mSessions.end(); ++iter)
          {
            IRUDPICESocketSessionPtr session = (*iter);
            session->endOfRemoteCandidates();
          }
        }
        
        //---------------------------------------------------------------------
        RecursiveLock &getLock() const
        {
          static RecursiveLock lock;
          return lock;
        }

      private:
        TestRUDPICESocketLoopbackWeakPtr mThisWeak;

        TestRUDPICESocketLoopbackWeakPtr mRemote;

        ITransportStreamReaderPtr mReceiveStream;
        ITransportStreamWriterPtr mSendStream;

        ITransportStreamReaderSubscriptionPtr mReceiveStreamSubscription;

        zsLib::TimerPtr mTimer;

        IRUDPICESocketPtr mRUDPSocket;
        SessionList mSessions;
        MessagingList mMessaging;

        bool mExpectConnected;
        bool mExpectGracefulShutdown;
        bool mExpectErrorShutdown;
        bool mExpectSessionConnected;
        bool mExpectSessionClosed;
        bool mExpectMessagingConnected;
        bool mExpectMessagingShutdown;

        bool mIssueConnect;

        bool mConnected;
        bool mGracefulShutdown;
        bool mErrorShutdown;
        bool mSessionConnected;
        bool mSessionClosed;
        bool mMessagingConnected;
        bool mMessagingShutdown;

        bool mShutdownCalled;
      };
    }
  }
}

using openpeer::services::test::TestRUDPICESocketLoopback;
using openpeer::services::test::TestRUDPICESocketLoopbackPtr;

void doTestRUDPICESocketLoopback()
{
  if (!OPENPEER_SERVICE_TEST_DO_RUDPICESOCKET_LOOPBACK_TEST) return;

  BOOST_INSTALL_LOGGER();

  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());

  TestRUDPICESocketLoopbackPtr testObject1;
  TestRUDPICESocketLoopbackPtr testObject2;
  TestRUDPICESocketLoopbackPtr testObject3;
  TestRUDPICESocketLoopbackPtr testObject4;

  IRUDPICESocket::CandidateList candidates1;
  IRUDPICESocket::CandidateList candidates2;
  IRUDPICESocket::CandidateList candidates3;
  IRUDPICESocket::CandidateList candidates4;

  ZS_LOG_BASIC("WAITING:      Waiting for ICE testing to complete (max wait is 180 seconds).");

  // check to see if all DNS routines have resolved
  {
    ULONG step = 0;

    do
    {
      ZS_LOG_BASIC(String("STEP:         ---------->>>>>>>>>> ") + string(step) + " <<<<<<<<<<----------")

      bool quit = false;
      ULONG expecting = 0;
      switch (step) {
        case 0: {
          expecting = 2;
          testObject1 = TestRUDPICESocketLoopback::create(thread, 0, OPENPEER_SERVICE_TEST_TURN_SERVER_DOMAIN, OPENPEER_SERVICE_TEST_STUN_SERVER, true);
          testObject2 = TestRUDPICESocketLoopback::create(thread, 0, OPENPEER_SERVICE_TEST_TURN_SERVER_DOMAIN, OPENPEER_SERVICE_TEST_STUN_SERVER, false);

          testObject1->setRemote(testObject2);
          testObject2->setRemote(testObject1);
          break;
        }
        case 1: {
          expecting = 2;
          testObject1 = TestRUDPICESocketLoopback::create(thread, 0, OPENPEER_SERVICE_TEST_TURN_SERVER_DOMAIN, OPENPEER_SERVICE_TEST_STUN_SERVER, true, true, false, false, true, false, true, false);
          testObject2 = TestRUDPICESocketLoopback::create(thread, 0, OPENPEER_SERVICE_TEST_TURN_SERVER_DOMAIN, OPENPEER_SERVICE_TEST_STUN_SERVER, false, true, false, false, true, false, true, false);

          testObject1->setRemote(testObject2);
          testObject2->setRemote(testObject1);
          break;
        }
        default: quit = true; break;
      }
      if (quit) break;

      ULONG found = 0;
      ULONG lastFound = 0;
      ULONG totalWait = 0;

      while (found < expecting)
      {
        boost::this_thread::sleep(zsLib::Seconds(1));
        ++totalWait;
        if (totalWait >= 70)
          break;

        found = 0;

        switch (step) {
          case 0: {
            if (1 == totalWait) {
              testObject1->createSessionFromRemoteCandidates(IICESocket::ICEControl_Controlling);
              testObject2->createSessionFromRemoteCandidates(IICESocket::ICEControl_Controlled);
            }

            if (30 == totalWait) {
              testObject1->shutdown();
              testObject2->shutdown();
            }
            break;
          }
          case 1: {
            if (10 == totalWait) {
              testObject1->createSessionFromRemoteCandidates(IICESocket::ICEControl_Controlling);
              testObject2->createSessionFromRemoteCandidates(IICESocket::ICEControl_Controlling);
            }

            break;
          }
        }

        found += (testObject1 ? (testObject1->isComplete() ? 1 : 0) : 0);
        found += (testObject2 ? (testObject2->isComplete() ? 1 : 0) : 0);
        found += (testObject3 ? (testObject3->isComplete() ? 1 : 0) : 0);
        found += (testObject4 ? (testObject4->isComplete() ? 1 : 0) : 0);

        switch (step) {
          case 0: {
            break;
          }
          case 1: {
            if (30 == totalWait) {
              found = 2;
            } else {
              // we want to prevent early auto-shutdown when the objects aren't ready
              found = 0;
            }
            break;
          }
        }

        if (lastFound != found) {
          lastFound = found;
          BOOST_STDOUT() << "FOUND:        [" << found << "].\n";
        }
      }
      BOOST_EQUAL(found, expecting);

      switch (step) {
        case 0: {
          testObject1->expectationsOkay();
          testObject2->expectationsOkay();

          break;
        }
        case 1: {
          testObject1->expectationsOkay();
          testObject2->expectationsOkay();
          break;
        }
      }
      testObject1.reset();
      testObject2.reset();
      testObject3.reset();
      testObject4.reset();

      ++step;
    } while (true);
  }

  ZS_LOG_BASIC("WAITING:      All ICE sockets have finished. Waiting for 'bogus' events to process (10 second wait).");
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
