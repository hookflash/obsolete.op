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
#include <openpeer/services/ISTUNDiscovery.h>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>

#include "config.h"
#include "boost_replacement.h"

#include <list>
#include <iostream>

namespace openpeer { namespace services { namespace test { ZS_DECLARE_SUBSYSTEM(openpeer_services_test) } } }

using zsLib::BYTE;
using zsLib::WORD;
using zsLib::ULONG;
using zsLib::Socket;
using zsLib::SocketPtr;
using zsLib::ISocketPtr;
using zsLib::IPAddress;
using openpeer::services::IDNS;
using openpeer::services::IDNSQuery;
using openpeer::services::ISTUNDiscovery;
using openpeer::services::ISTUNDiscoveryPtr;
using openpeer::services::ISTUNDiscoveryDelegate;

namespace openpeer
{
  namespace services
  {
    namespace test
    {
      class TestSTUNDiscoveryCallback;
      typedef boost::shared_ptr<TestSTUNDiscoveryCallback> TestSTUNDiscoveryCallbackPtr;
      typedef boost::weak_ptr<TestSTUNDiscoveryCallback> TestSTUNDiscoveryCallbackWeakPtr;

      class TestSTUNDiscoveryCallback : public zsLib::MessageQueueAssociator,
                                        public ISTUNDiscoveryDelegate,
                                        public IDNSDelegate,
                                        public zsLib::ISocketDelegate
      {
      private:
        TestSTUNDiscoveryCallback(zsLib::IMessageQueuePtr queue) :
          zsLib::MessageQueueAssociator(queue)
        {
        }

        void init(
                  WORD port,
                  const char *srvName,
                  bool resolveFirst
                  )
        {
          zsLib::AutoLock lock(mLock);
          mSocket = Socket::createUDP();

          IPAddress any(IPAddress::anyV4());
          any.setPort(port);

          mSocket->bind(any);
          mSocket->setBlocking(false);
          mSocket->setDelegate(mThisWeak.lock());

          if (resolveFirst) {
            mSRVQuery = IDNS::lookupSRV(mThisWeak.lock(), srvName, "stun", "udp", 3478);
          } else {
            mDiscovery = ISTUNDiscovery::create(getAssociatedMessageQueue(), mThisWeak.lock(), srvName);
          }
        }

      public:
        static TestSTUNDiscoveryCallbackPtr create(
                                                   zsLib::IMessageQueuePtr queue,
                                                   WORD port,
                                                   const char *srvName,
                                                   bool resolveFirst
                                                   )
        {
          TestSTUNDiscoveryCallbackPtr pThis(new TestSTUNDiscoveryCallback(queue));
          pThis->mThisWeak = pThis;
          pThis->init(port, srvName, resolveFirst);
          return pThis;
        }

        virtual void onLookupCompleted(IDNSQueryPtr query)
        {
          zsLib::AutoLock lock(mLock);
          BOOST_CHECK(((bool)query))
          BOOST_CHECK(query->hasResult())

          BOOST_CHECK(query == mSRVQuery);
          BOOST_CHECK(mSRVQuery);

          mDiscovery = ISTUNDiscovery::create(getAssociatedMessageQueue(), mThisWeak.lock(), query->getSRV());
          mSRVQuery.reset();
        }

        ~TestSTUNDiscoveryCallback()
        {
        }

        virtual void onSTUNDiscoverySendPacket(
                                               ISTUNDiscoveryPtr discovery,
                                               zsLib::IPAddress destination,
                                               boost::shared_array<zsLib::BYTE> packet,
                                               zsLib::ULONG packetLengthInBytes
                                               )
        {
          zsLib::AutoLock lock(mLock);
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
          zsLib::AutoLock lock(mLock);
          BOOST_CHECK(discovery);
          if (!mDiscovery) return;
          BOOST_CHECK(discovery == mDiscovery);
          BOOST_CHECK(mDiscovery);
          BOOST_CHECK(mSocket)

          mDiscoveredIP = discovery->getMappedAddress();
          mDiscovery.reset();
          mSocket->close();
          mSocket.reset();
        }

        virtual void onReadReady(ISocketPtr socket)
        {
          zsLib::AutoLock lock(mLock);
          BOOST_CHECK(socket);
          if (!mSocket) return;
          BOOST_CHECK(socket == mSocket);
          BOOST_CHECK(mDiscovery);

          IPAddress ip;
          BYTE buffer[1500];
          ULONG bufferLengthInBytes = sizeof(buffer);

          ULONG readBytes = mSocket->receiveFrom(ip, &(buffer[0]), bufferLengthInBytes);
          BOOST_CHECK(readBytes > 0)

          ISTUNDiscovery::handlePacket(ip, &(buffer[0]), readBytes);
        }

        virtual void onWriteReady(ISocketPtr socket)
        {
//          zsLib::AutoLock lock(mLock);
//          BOOST_CHECK(socket);
//          BOOST_CHECK(socket == mSocket);
        }

        virtual void onException(ISocketPtr socket)
        {
//          zsLib::AutoLock lock(mLock);
//          BOOST_CHECK(socket);
//          BOOST_CHECK(socket == mSocket);
        }

        bool isComplete()
        {
          zsLib::AutoLock lock(mLock);
          return (!((mSRVQuery) || (mDiscovery)));
        }

        zsLib::IPAddress getIP()
        {
          zsLib::AutoLock lock(mLock);
          return mDiscoveredIP;
        }

      private:
        mutable zsLib::Lock mLock;
        TestSTUNDiscoveryCallbackWeakPtr mThisWeak;

        SocketPtr mSocket;
        IDNSQueryPtr mSRVQuery;
        ISTUNDiscoveryPtr mDiscovery;

        IPAddress mDiscoveredIP;
      };

    }
  }
}

using openpeer::services::test::TestSTUNDiscoveryCallback;
using openpeer::services::test::TestSTUNDiscoveryCallbackPtr;

void doTestSTUNDiscovery()
{
  if (!OPENPEER_SERVICE_TEST_DO_STUN_TEST) return;

  BOOST_INSTALL_LOGGER();

  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());

  TestSTUNDiscoveryCallbackPtr testObject = TestSTUNDiscoveryCallback::create(thread, 45123, OPENPEER_SERVICE_TEST_STUN_SERVER, true);
  TestSTUNDiscoveryCallbackPtr testObject2 = TestSTUNDiscoveryCallback::create(thread, 45127, OPENPEER_SERVICE_TEST_STUN_SERVER, false);

  BOOST_STDOUT() << "WAITING:      Waiting for STUN discovery to complete (max wait is 180 seconds).\n";

  // check to see if all DNS routines have resolved
  {
    ULONG expecting = 2;

    ULONG found = 0;
    ULONG lastFound = 0;
    ULONG totalWait = 0;

    do
    {
      boost::this_thread::sleep(zsLib::Seconds(1));
      ++totalWait;
      if (totalWait >= 180)
        break;

      found = 0;

      found += (testObject->isComplete() ? 1 : 0);
      found += (testObject2->isComplete() ? 1 : 0);

      if (lastFound != found) {
        lastFound = found;
        std::cout << "FOUND:        [" << found << "].\n";
      }

    } while(found < expecting);

    BOOST_EQUAL(found, expecting);
  }

  BOOST_STDOUT() << "WAITING:      All STUN discoveries have finished. Waiting for 'bogus' events to process (10 second wait).\n";
  boost::this_thread::sleep(zsLib::Seconds(10));

  BOOST_CHECK(!testObject->getIP().isAddressEmpty());
  BOOST_CHECK(!testObject->getIP().isPortEmpty());

  BOOST_CHECK(!testObject2->getIP().isAddressEmpty());
  BOOST_CHECK(!testObject2->getIP().isPortEmpty());

  BOOST_CHECK(testObject->getIP().isAddressEqual(testObject2->getIP()));
  BOOST_CHECK(testObject->getIP().getPort() != testObject2->getIP().getPort());

#ifdef OPENPEER_SERVICE_TEST_WHAT_IS_MY_IP
  BOOST_EQUAL(testObject->getIP().string(false), OPENPEER_SERVICE_TEST_WHAT_IS_MY_IP);
  BOOST_EQUAL(testObject2->getIP().string(false), OPENPEER_SERVICE_TEST_WHAT_IS_MY_IP);
#endif //OPENPEER_SERVICE_TEST_WHAT_IS_MY_IP

  testObject.reset();
  testObject2.reset();

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
