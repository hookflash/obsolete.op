/*
 *  Created by Robin Raymond.
 *  Copyright 2009-2013. Robin Raymond. All rights reserved.
 *
 * This file is part of zsLib.
 *
 * zsLib is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (LGPL) as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * zsLib is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with zsLib; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <zsLib/Socket.h>
#include <zsLib/IPAddress.h>
#include <zsLib/MessageQueueThread.h>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>


#include "boost_replacement.h"

using zsLib::BYTE;
using zsLib::ULONG;

namespace async_socket
{
  class SocketServer;
  typedef boost::shared_ptr<SocketServer> SocketServerPtr;
  typedef boost::weak_ptr<SocketServer> SocketServerWeakPtr;

  class SocketServer : public zsLib::MessageQueueAssociator,
                       public zsLib::ISocketDelegate
  {
  private:
    SocketServer(zsLib::IMessageQueuePtr queue) : zsLib::MessageQueueAssociator(queue) { }

    void setup()
    {
      mReadReadyCalled = 0;
      mWriteReadyCalled = 0;
      mExceptionCalled = 0;
      mAddress = zsLib::IPAddress(zsLib::IPAddress::loopbackV4(), 43216);
      mSocket = zsLib::Socket::createUDP();
      mSocket->setOptionFlag(zsLib::Socket::SetOptionFlag::NonBlocking, true);
      mSocket->setDelegate(mThis.lock());
      mSocket->bind(mAddress);
    }

  public:
    static SocketServerPtr create(zsLib::IMessageQueuePtr queue)
    {
      SocketServerPtr object(new SocketServer(queue));
      object->mThis = object;
      object->setup();
      return object;
    }

    virtual void onReadReady(zsLib::ISocketPtr socket)
    {
      std::cout << "ON READ READY\n";
      ++mReadReadyCalled;

      zsLib::IPAddress address;
      BYTE buffer[1024];
      ULONG total = socket->receiveFrom(
                                        address,
                                        buffer,
                                        sizeof(buffer)
                                        );
      mReadData.push_back((const char *)buffer);
      mReadAddresses.push_back(address);
      std::cout << "READ " << total << " BYTES.\n";
    }

    virtual void onWriteReady(zsLib::ISocketPtr socket)
    {
      std::cout << "ON WRITE READY\n";
      ++mWriteReadyCalled;
    }

    virtual void onException(zsLib::ISocketPtr socket)
    {
      std::cout << "ONEXCEPTION\n";
      ++mExceptionCalled;
    }

    const zsLib::IPAddress &getAddress() {return mAddress;}

  public:
    ULONG mReadReadyCalled;
    ULONG mWriteReadyCalled;
    ULONG mExceptionCalled;
    std::vector<zsLib::IPAddress> mReadAddresses;
    std::vector<std::string> mReadData;

  private:
    SocketServerWeakPtr mThis;
    zsLib::SocketPtr mSocket;
    zsLib::IPAddress mAddress;
  };

  class SocketTest
  {
  public:
    SocketTest()
    {
      zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());

      SocketServerPtr server(SocketServer::create(thread));

      boost::this_thread::sleep(zsLib::Seconds(1));
      zsLib::IPAddress address = zsLib::IPAddress(zsLib::IPAddress::loopbackV4(), 43217);
      zsLib::SocketPtr socket = zsLib::Socket::createUDP();
      socket->bind(address);

      socket->sendTo(server->getAddress(), (BYTE *)"HELLO1", sizeof("HELLO1") + sizeof(char));
      boost::this_thread::sleep(zsLib::Seconds(5));

      socket->sendTo(server->getAddress(), (BYTE *)"HELLO2", sizeof("HELLO2") + sizeof(char));

      boost::this_thread::sleep(zsLib::Seconds(10));

      BOOST_EQUAL(2, server->mReadReadyCalled);
      BOOST_EQUAL(1, server->mWriteReadyCalled);
      BOOST_EQUAL(0, server->mExceptionCalled);
      BOOST_EQUAL(2, server->mReadData.size());
      BOOST_EQUAL(2, server->mReadAddresses.size());

      BOOST_EQUAL("HELLO1", server->mReadData[0]);
      BOOST_EQUAL("HELLO2", server->mReadData[1]);

      BOOST_CHECK(address == server->mReadAddresses[0]);
      BOOST_CHECK(address == server->mReadAddresses[1]);

      server.reset();

      ULONG count = 0;
      do
      {
        count = thread->getTotalUnprocessedMessages();
        if (0 != count)
          boost::this_thread::yield();
      } while (count > 0);
      thread->waitForShutdown();

      BOOST_EQUAL(zsLib::proxyGetTotalConstructed(), 0);
    }
  };
}

BOOST_AUTO_TEST_SUITE(zsLibSocketAsync)

BOOST_AUTO_TEST_CASE(TestSocketAsync)
{
  async_socket::SocketTest test;
}

BOOST_AUTO_TEST_SUITE_END()
