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
#include <hookflash/services/ITURNSocket.h>
#include <hookflash/services/STUNPacket.h>
#include <hookflash/services/ISTUNDiscovery.h>

#include <boost/shared_array.hpp>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>

#include "config.h"
#include "boost_replacement.h"

#include <list>
#include <iostream>

namespace hookflash { namespace services { namespace test { ZS_DECLARE_SUBSYSTEM(hookflash_services_test) } } }

typedef zsLib::ULONG ULONG;
typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
typedef zsLib::ISocketDelegate ISocketDelegate;
typedef zsLib::ITimerDelegate ITimerDelegate;
typedef zsLib::MessageQueueThread MessageQueueThread;
typedef zsLib::Seconds Seconds;
typedef zsLib::MessageQueueThreadPtr MessageQueueThreadPtr;

namespace hookflash
{
  namespace services
  {
    namespace test
    {
      static const char *gUsername = HOOKFLASH_SERVICE_TEST_TURN_USERNAME;
      static const char *gPassword = HOOKFLASH_SERVICE_TEST_TURN_PASSWORD;

      class TestTURNSocketCallback;
      typedef boost::shared_ptr<TestTURNSocketCallback> TestTURNSocketCallbackPtr;
      typedef boost::weak_ptr<TestTURNSocketCallback> TestTURNSocketCallbackWeakPtr;

      class TestTURNSocketCallback : public MessageQueueAssociator,
                                     public ISTUNDiscoveryDelegate,
                                     public ITURNSocketDelegate,
                                     public IDNSDelegate,
                                     public ISocketDelegate,
                                     public ITimerDelegate
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::BYTE BYTE;
        typedef zsLib::BYTE WORD;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::Milliseconds Milliseconds;
        typedef zsLib::IPAddress IPAddress;
        typedef zsLib::Socket Socket;
        typedef zsLib::SocketPtr SocketPtr;
        typedef zsLib::ISocketPtr ISocketPtr;
        typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
        typedef zsLib::Timer Timer;
        typedef zsLib::TimerPtr TimerPtr;
        typedef zsLib::RecursiveLock RecursiveLock;

      private:
        TestTURNSocketCallback(IMessageQueuePtr queue) :
          MessageQueueAssociator(queue),
          mID(0),
          mExpectConnected(false),
          mExpectFailedToConnect(false),
          mExpectGracefulShutdown(false),
          mExpectErrorShutdown(false),
          mConnected(false),
          mFailedToConnect(false),
          mGracefulShutdown(false),
          mErrorShutdown(false),
          mShutdownCalled(false),
          mTotalReceived(0)
        {
        }

        void init(
                  WORD port,
                  const char *srvName,
                  bool resolveFirst
                  )
        {
          AutoRecursiveLock lock(mLock);

          mSocket = Socket::createUDP();

          IPAddress any(IPAddress::anyV4());
          any.setPort(port);

          mSocket->bind(any);
          mSocket->setBlocking(false);
          mSocket->setDelegate(mThisWeak.lock());

          if (resolveFirst) {
            mUDPSRVQuery = IDNS::lookupSRV(mThisWeak.lock(), srvName, "turn", "udp", 3478);
            mTCPSRVQuery = IDNS::lookupSRV(mThisWeak.lock(), srvName, "turn", "tcp", 3478);
            mSTUNSRVQuery = IDNS::lookupSRV(mThisWeak.lock(), srvName, "stun", "udp", 3478);
          } else {
            mDiscovery = ISTUNDiscovery::create(getAssociatedMessageQueue(), mThisWeak.lock(), srvName);
            mTURNSocket = ITURNSocket::create(
                                              getAssociatedMessageQueue(),
                                              mThisWeak.lock(),
                                              srvName,
                                              gUsername,
                                              gPassword,
                                              true
                                              );
            mID = mTURNSocket->getID();
          }

          mTimer = Timer::create(mThisWeak.lock(), Milliseconds(rand()%400+200));
        }

      public:
        static TestTURNSocketCallbackPtr create(
                                                IMessageQueuePtr queue,
                                                WORD port,
                                                const char *srvName,
                                                bool resolveFirst,
                                                bool expectConnected = true,
                                                bool expectGracefulShutdown = true,
                                                bool expectErrorShutdown = false,
                                                bool expectFailedToConnect = false
                                                )
        {
          TestTURNSocketCallbackPtr pThis(new TestTURNSocketCallback(queue));
          pThis->mThisWeak = pThis;
          pThis->mExpectConnected = expectConnected;
          pThis->mExpectGracefulShutdown = expectGracefulShutdown;
          pThis->mExpectErrorShutdown = expectErrorShutdown;
          pThis->mExpectFailedToConnect = expectFailedToConnect;
          pThis->init(port, srvName, resolveFirst);
          return pThis;
        }

        ~TestTURNSocketCallback()
        {
        }

        virtual void onLookupCompleted(IDNSQueryPtr query)
        {
          BOOST_CHECK(query)
          BOOST_CHECK(query->hasResult())
          AutoRecursiveLock lock(mLock);

          if (query == mUDPSRVQuery) {
            mUDPSRVResult = query->getSRV();
            mUDPSRVQuery.reset();
          }
          if (query == mTCPSRVQuery) {
            mTCPSRVResult = query->getSRV();
            mTCPSRVQuery.reset();
          }
          if (query == mSTUNSRVQuery) {
            mDiscovery = ISTUNDiscovery::create(getAssociatedMessageQueue(), mThisWeak.lock(), query->getSRV());
            mSTUNSRVQuery.reset();
            // do not allow the routine to continue in the case STUN SRV lookup completes otherwise the TURN server could get created twice
            return;
          }

          if ((!mUDPSRVResult) ||
              (!mTCPSRVResult))
            return;

          BOOST_CHECK(!mUDPSRVQuery)
          BOOST_CHECK(!mTCPSRVQuery)

          mTURNSocket = ITURNSocket::create(
                                            getAssociatedMessageQueue(),
                                            mThisWeak.lock(),
                                            mUDPSRVResult,
                                            mTCPSRVResult,
                                            gUsername,
                                            gPassword,
                                            true
                                            );
          mID = mTURNSocket->getID();
        }

        // ITURNSocketDelegate
        virtual void handleTURNSocketReceivedPacket(
                                                    ITURNSocketPtr socket,
                                                    IPAddress source,
                                                    const BYTE *packet,
                                                    ULONG packetLengthInBytes
                                                    )
        {
          AutoRecursiveLock lock(mLock);

          // see if this matches any of the expected data
          for (DataList::iterator iter = mSentData.begin(); iter != mSentData.end(); ++iter) {
            if (packetLengthInBytes == (*iter).second) {
              if (0 == memcmp(packet, (*iter).first.get(), packetLengthInBytes)) {
                // forget the data
                mSentData.erase(iter);
                ++mTotalReceived;
                return;
              }
            }
          }

          BOOST_CHECK(false); // received unknown data from the socket
        }

        virtual void onSTUNDiscoverySendPacket(
                                               ISTUNDiscoveryPtr discovery,
                                               IPAddress destination,
                                               boost::shared_array<BYTE> packet,
                                               ULONG packetLengthInBytes
                                               )
        {
          AutoRecursiveLock lock(mLock);
          if (!mSocket) return;
          BOOST_CHECK(discovery);
          BOOST_CHECK(!destination.isAddressEmpty());
          BOOST_CHECK(!destination.isPortEmpty());
          BOOST_CHECK(packet.get());
          BOOST_CHECK(packetLengthInBytes > 0);
          BOOST_CHECK(mSocket);

          mSocket->sendTo(destination, packet.get(), packetLengthInBytes);
        }

        virtual void onSTUNDiscoveryCompleted(ISTUNDiscoveryPtr discovery)
        {
          AutoRecursiveLock lock(mLock);
          BOOST_CHECK(discovery);
          if (!mDiscovery) return;
          BOOST_CHECK(discovery == mDiscovery);
          BOOST_CHECK(mDiscovery);
          BOOST_CHECK(mSocket)

          mSTUNDiscoveredIP = discovery->getMappedAddress();
          mDiscovery.reset();
        }

        virtual bool notifyTURNSocketSendPacket(
                                                ITURNSocketPtr socket,
                                                IPAddress destination,
                                                const BYTE *packet,
                                                ULONG packetLengthInBytes
                                                )
        {
          AutoRecursiveLock lock(mLock);
          return 0 != mSocket->sendTo(destination, packet, packetLengthInBytes);
        }

        virtual void onTURNSocketStateChanged(
                                              ITURNSocketPtr socket,
                                              TURNSocketStates state
                                              )
        {
          AutoRecursiveLock lock(mLock);
          BOOST_CHECK(socket == mTURNSocket)

          switch (state)
          {
            case ITURNSocket::TURNSocketState_Pending: break;
            case ITURNSocket::TURNSocketState_Ready: {
              onTURNSocketConnected(socket);
              break;
            }
            case ITURNSocket::TURNSocketState_ShuttingDown: break;
            case ITURNSocket::TURNSocketState_Shutdown: {
              if (!mConnected) {
                onTURNSocketFailedToConnect(socket);
              } else {
                onTURNSocketShutdown(socket);
              }
              break;
            }
          }
        }

        void onTURNSocketConnected(ITURNSocketPtr socket)
        {
          AutoRecursiveLock lock(mLock);
          BOOST_CHECK(socket == mTURNSocket)
          BOOST_CHECK(mExpectConnected)

          mConnected = true;
          mDiscoveredIP = socket->getReflectedIP();
          BOOST_CHECK(!mDiscoveredIP.isAddressEmpty())
        }

        void onTURNSocketFailedToConnect(ITURNSocketPtr socket)
        {
          AutoRecursiveLock lock(mLock);
          BOOST_CHECK(mExpectFailedToConnect)
          mFailedToConnect = true;
          mTURNSocket.reset();
        }

        void onTURNSocketShutdown(ITURNSocketPtr socket)
        {
          AutoRecursiveLock lock(mLock);
          BOOST_CHECK(socket == mTURNSocket);

          if (mShutdownCalled) {
            BOOST_CHECK(mExpectGracefulShutdown);
            mGracefulShutdown = true;
            mTURNSocket.reset();
            return;
          }

          BOOST_CHECK(mExpectErrorShutdown);
          mErrorShutdown = true;
          mTURNSocket.reset();
        }

        virtual void onTURNSocketWriteReady(ITURNSocketPtr socket)
        {
          AutoRecursiveLock lock(mLock);
        }

        virtual void onReadReady(ISocketPtr socket)
        {
          AutoRecursiveLock lock(mLock);
          BOOST_CHECK(socket);
          if (!mSocket) return;
          BOOST_CHECK(socket == mSocket);
          if (!mTURNSocket) return;

          IPAddress ip;
          BYTE buffer[1500];
          ULONG bufferLengthInBytes = sizeof(buffer);

          ULONG readBytes = mSocket->receiveFrom(ip, &(buffer[0]), bufferLengthInBytes);
          BOOST_CHECK(readBytes > 0);

          if (mTURNSocket->handleChannelData(ip, &(buffer[0]), readBytes)) return;

          STUNPacketPtr stun = STUNPacket::parseIfSTUN(&(buffer[0]), readBytes, static_cast<STUNPacket::RFCs>(STUNPacket::RFC_5766_TURN | STUNPacket::RFC_5389_STUN));
          if (!stun) return;
          if (mTURNSocket->handleSTUNPacket(ip, stun)) return;
        }

        virtual void onWriteReady(ISocketPtr socket)
        {
          //          AutoLock lock(mLock);
          //          BOOST_CHECK(socket);
          //          BOOST_CHECK(socket == mSocket);
        }

        virtual void onException(ISocketPtr socket)
        {
          //          AutoLock lock(mLock);
          //          BOOST_CHECK(socket);
          //          BOOST_CHECK(socket == mSocket);
        }

        virtual void onTimer(TimerPtr timer)
        {
          AutoRecursiveLock lock(mLock);
          if (timer != mTimer) return;
          if (!mTURNSocket) return;

          if (mShutdownCalled) return;

          ULONG length = (rand()%500)+1;
          boost::shared_array<BYTE> buffer(new BYTE[length]);

          // fill the buffer with random data
          for (ULONG loop = 0; loop < length; ++loop) {
            (buffer.get())[loop] = rand()%(sizeof(BYTE) << 8);
          }

          // send the random data to location on the internet...
          IPAddress relayedIP = mTURNSocket->getRelayedIP();
          IPAddress reflextedIP = mTURNSocket->getReflectedIP();
          if (!mSTUNDiscoveredIP.isAddressEmpty())
            reflextedIP = mSTUNDiscoveredIP;  // this will be more accurate because when using TCP-only it will not discover the port
          if (relayedIP.isAddressEmpty())
            return;
          if (reflextedIP.isAddressEmpty())
            return;

          mSocket->sendTo(relayedIP, buffer.get(), length);
          mTURNSocket->sendPacket(reflextedIP, buffer.get(), length, true);

          DataPair data(buffer, length);
          mSentData.push_back(data);
        }

        void shutdown()
        {
          AutoRecursiveLock lock(mLock);
          BOOST_CHECK(mTURNSocket);

          mTURNSocket->shutdown();

          mShutdownCalled = true;
          if (mTimer) {
            mTimer->cancel();
            mTimer.reset();
          }
        }

        bool isComplete()
        {
          AutoRecursiveLock lock(mLock);
          return (!((mUDPSRVQuery) || (mTCPSRVQuery) || (mSTUNSRVQuery) || (mTURNSocket)));
        }

        PUID getID() const
        {
          AutoRecursiveLock lock(mLock);
          return mID;
        }

        IPAddress getIP()
        {
          AutoRecursiveLock lock(mLock);
          return mDiscoveredIP;
        }

        void expectationsOkay() {
          AutoRecursiveLock lock(mLock);

          if (mExpectConnected) {
            BOOST_CHECK(mConnected);
          } else {
            BOOST_CHECK(!mConnected);
          }

          if (mExpectFailedToConnect) {
            BOOST_CHECK(mFailedToConnect);
          } else {
            BOOST_CHECK(!mFailedToConnect);
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
        }
        ULONG getTotalReceived() {
          AutoRecursiveLock lock(mLock);
          return mTotalReceived;
        }
        ULONG getTotalUnreceived() {
          AutoRecursiveLock lock(mLock);
          return mSentData.size();
        }

      private:
        mutable RecursiveLock mLock;
        PUID mID;

        TestTURNSocketCallbackWeakPtr mThisWeak;

        bool mExpectConnected;
        bool mExpectFailedToConnect;
        bool mExpectGracefulShutdown;
        bool mExpectErrorShutdown;

        bool mConnected;
        bool mFailedToConnect;
        bool mGracefulShutdown;
        bool mErrorShutdown;

        bool mShutdownCalled;

        SocketPtr mSocket;
        IDNSQueryPtr mSTUNSRVQuery;
        IDNSQueryPtr mUDPSRVQuery;
        IDNSQueryPtr mTCPSRVQuery;

        IDNS::SRVResultPtr mUDPSRVResult;
        IDNS::SRVResultPtr mTCPSRVResult;

        ITURNSocketPtr mTURNSocket;
        ISTUNDiscoveryPtr mDiscovery;

        IPAddress mDiscoveredIP;
        IPAddress mSTUNDiscoveredIP;

        TimerPtr mTimer;

        ULONG mTotalReceived;

        typedef std::pair< boost::shared_array<BYTE>, ULONG> DataPair;
        typedef std::list<DataPair> DataList;
        DataList mSentData;
      };
    }
  }
}

using hookflash::services::test::TestTURNSocketCallback;
using hookflash::services::test::TestTURNSocketCallbackPtr;

void doTestTURNSocket()
{
  if (!HOOKFLASH_SERVICE_TEST_DO_TURN_TEST) return;

  BOOST_INSTALL_LOGGER();

  MessageQueueThreadPtr thread(MessageQueueThread::createBasic());

  TestTURNSocketCallbackPtr testObject1 = TestTURNSocketCallback::create(thread, 0, "siptest." HOOKFLASH_SERVICE_TEST_TURN_SERVER_DOMAIN, true);
  TestTURNSocketCallbackPtr testObject2 = TestTURNSocketCallback::create(thread, 0, "siptest." HOOKFLASH_SERVICE_TEST_TURN_SERVER_DOMAIN, false);
  TestTURNSocketCallbackPtr testObject3 = TestTURNSocketCallback::create(thread, 0, "bogus." HOOKFLASH_SERVICE_TEST_TURN_SERVER_DOMAIN, false, false, false, false, true);
  TestTURNSocketCallbackPtr testObject4 = TestTURNSocketCallback::create(thread, 0, "turntest1." HOOKFLASH_SERVICE_TEST_TURN_SERVER_DOMAIN, false);
  TestTURNSocketCallbackPtr testObject5 = TestTURNSocketCallback::create(thread, 0, "turntest4." HOOKFLASH_SERVICE_TEST_TURN_SERVER_DOMAIN, false);

  std::cout << "WAITING:      Waiting for TURN testing to complete (max wait is 180 seconds).\n";

  // check to see if all DNS routines have resolved
  {
    ULONG expecting = 0;
    if (testObject1) ++expecting;
    if (testObject2) ++expecting;
    if (testObject3) ++expecting;
    if (testObject4) ++expecting;
    if (testObject5) ++expecting;

    ULONG found = 0;
    ULONG lastFound = 0;
    ULONG totalWait = 0;

    do
    {
      boost::this_thread::sleep(Seconds(1));
      ++totalWait;
      if (totalWait >= 180)
        break;

      if (20 == totalWait) {
        if (testObject1) testObject1->shutdown();
        if (testObject2) testObject2->shutdown();
//        if (testObject3) testObject3->shutdown();
        if (testObject4) testObject4->shutdown();
      }

      if (120 == totalWait) {
        if (testObject5) testObject5->shutdown();
      }

      found = 0;

      if (testObject1) found += (testObject1->isComplete() ? 1 : 0);
      if (testObject2) found += (testObject2->isComplete() ? 1 : 0);
      if (testObject3) found += (testObject3->isComplete() ? 1 : 0);
      if (testObject4) found += (testObject4->isComplete() ? 1 : 0);
      if (testObject5) found += (testObject5->isComplete() ? 1 : 0);
      if (lastFound != found) {
        lastFound = found;
        std::cout << "FOUND:        [" << found << "].\n";
      }

    } while(found < expecting);

    BOOST_EQUAL(found, expecting);
  }

  std::cout << "WAITING:      All TURN sockets have finished. Waiting for 'bogus' events to process (10 second wait).\n";
  boost::this_thread::sleep(Seconds(10));

  if (testObject1) {std::cout << "object1: [" << testObject1->getID() << "]\n";}
  if (testObject2) {std::cout << "object2: [" << testObject2->getID() << "]\n";}
  if (testObject3) {std::cout << "object3: [" << testObject3->getID() << "]\n";}
  if (testObject4) {std::cout << "object4: [" << testObject4->getID() << "]\n";}
  if (testObject5) {std::cout << "object5: [" << testObject5->getID() << "]\n";}

  if (testObject1) {
    BOOST_CHECK(!testObject1->getIP().isAddressEmpty());
    BOOST_CHECK(!testObject1->getIP().isPortEmpty());
  }
  if (testObject2) {
    BOOST_CHECK(!testObject2->getIP().isAddressEmpty());
    BOOST_CHECK(!testObject2->getIP().isPortEmpty());
  }
  if (testObject3) {
    BOOST_CHECK(testObject3->getIP().isAddressEmpty());
    BOOST_CHECK(testObject3->getIP().isPortEmpty());
  }
  if (testObject4) {
    BOOST_CHECK(!testObject4->getIP().isAddressEmpty());
    BOOST_CHECK(!testObject4->getIP().isPortEmpty());
  }
  if (testObject5) {
    BOOST_CHECK(!testObject5->getIP().isAddressEmpty());
    BOOST_CHECK(!testObject5->getIP().isPortEmpty());
  }

  if (testObject1) testObject1->expectationsOkay();
  if (testObject2) testObject2->expectationsOkay();
  if (testObject3) testObject3->expectationsOkay();
  if (testObject4) testObject4->expectationsOkay();
  if (testObject5) testObject5->expectationsOkay();

  if (testObject1) {
    BOOST_CHECK(testObject1->getTotalReceived() > 10)
    BOOST_CHECK(testObject1->getTotalUnreceived() < 10)
  }
  if (testObject2) {
    BOOST_CHECK(testObject2->getTotalReceived() > 10)
    BOOST_CHECK(testObject2->getTotalUnreceived() < 10)
  }
  if (testObject3) {
    BOOST_EQUAL(testObject3->getTotalReceived(), 0)
    BOOST_EQUAL(testObject3->getTotalUnreceived(), 0)
  }
  if (testObject4) {
    BOOST_CHECK(testObject4->getTotalReceived() > 10)
    BOOST_CHECK(testObject4->getTotalUnreceived() < 10)
  }
  if (testObject5) {
    BOOST_CHECK(testObject5->getTotalReceived() > 10)
    BOOST_CHECK(testObject5->getTotalUnreceived() < 10)
  }

#ifdef HOOKFLASH_SERVICE_TEST_WHAT_IS_MY_IP
  BOOST_EQUAL(testObject1->getIP().string(false), HOOKFLASH_SERVICE_TEST_WHAT_IS_MY_IP);
  BOOST_EQUAL(testObject2->getIP().string(false), HOOKFLASH_SERVICE_TEST_WHAT_IS_MY_IP);
#endif //HOOKFLASH_SERVICE_TEST_WHAT_IS_MY_IP

  testObject1.reset();
  testObject2.reset();
  testObject3.reset();
  testObject4.reset();
  testObject5.reset();

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
