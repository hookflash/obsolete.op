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

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>


#include "boost_replacement.h"

#define HlZeroStruct(xValue) memset(&(xValue), 0, sizeof(xValue))
#define HlZeroMemory(xValue, xSize) memset((xValue), 0, xSize)

#ifndef _WIN32
using zsLib::INVALID_SOCKET;
#endif //ndef _WIN32
using zsLib::BYTE;

class TestSocket
{
public:
  TestSocket()
  {
    {
      zsLib::SocketPtr socket = zsLib::Socket::createUDP();
      BOOST_CHECK(INVALID_SOCKET != socket->getSocket())
    }
    {
      zsLib::SocketPtr socket = zsLib::Socket::createTCP();
      BOOST_CHECK(INVALID_SOCKET != socket->getSocket())
    }
    {
      zsLib::SocketPtr socket = zsLib::Socket::create(zsLib::Socket::Create::IPv6, zsLib::Socket::Create::Datagram, zsLib::Socket::Create::UDP);
      BOOST_CHECK(INVALID_SOCKET != socket->getSocket())
    }
    {
      zsLib::SocketPtr socket = zsLib::Socket::create(zsLib::Socket::Create::IPv6, zsLib::Socket::Create::Datagram, zsLib::Socket::Create::UDP);
      BOOST_CHECK(INVALID_SOCKET != socket->getSocket())
    }
    {
      zsLib::IPAddress address1(zsLib::IPAddress::loopbackV4(), 43016);
      zsLib::IPAddress address2(zsLib::IPAddress::loopbackV4(), 43017);
      zsLib::SocketPtr socket1 = zsLib::Socket::createUDP();
      socket1->bind(address1);

      zsLib::SocketPtr socket2 = zsLib::Socket::createUDP();
      socket2->bind(address2);

      socket1->sendTo(address2, (BYTE *)"HELLO", sizeof("HELLO"));

      zsLib::IPAddress address3;
      BYTE buffer[1024];
      HlZeroMemory(&(buffer[0]), sizeof(buffer));
      int length = socket2->receiveFrom(address3, buffer, sizeof(buffer));
      BOOST_CHECK(sizeof("HELLO") == length)
      BOOST_CHECK(0 == memcmp(&(buffer[0]), "HELLO", sizeof("HELLO")))
      BOOST_CHECK(address3 == address1)

      zsLib::IPAddress getaddress1 = socket1->getLocalAddress();
      zsLib::IPAddress getaddress2 = socket2->getLocalAddress();
      BOOST_CHECK(address1 == getaddress1)
      BOOST_CHECK(address2 == getaddress2)
      socket1->connect(address2);
      socket2->connect(address1);
      zsLib::IPAddress remoteaddress1 = socket1->getRemoteAddress();
      zsLib::IPAddress remoteaddress2 = socket2->getRemoteAddress();
      BOOST_CHECK(address1 == remoteaddress2)
      BOOST_CHECK(address2 == remoteaddress1)

      socket1->send((BYTE *)"HELLO", sizeof("HELLO"));

      HlZeroMemory(&(buffer[0]), sizeof(buffer));
      length = socket2->receive(buffer, sizeof(buffer));
      BOOST_CHECK(sizeof("HELLO") == length)
      BOOST_CHECK(0 == memcmp(&(buffer[0]), "HELLO", sizeof("HELLO")))
    }
    {
      zsLib::IPAddress address1(zsLib::IPAddress::loopbackV6(), 43016);
      zsLib::IPAddress address2(zsLib::IPAddress::loopbackV6(), 43017);
      zsLib::SocketPtr socket1 = zsLib::Socket::createUDP(zsLib::Socket::Create::IPv6);
      socket1->bind(address1);

      zsLib::SocketPtr socket2 = zsLib::Socket::createUDP(zsLib::Socket::Create::IPv6);
      socket2->bind(address2);

      socket1->sendTo(address2, (BYTE *)"HELLO", sizeof("HELLO"));

      zsLib::IPAddress address3;
      BYTE buffer[1024];
      HlZeroMemory(&(buffer[0]), sizeof(buffer));
      int length = socket2->receiveFrom(address3, buffer, sizeof(buffer));
      BOOST_CHECK(sizeof("HELLO") == length)
      BOOST_CHECK(0 == memcmp(&(buffer[0]), "HELLO", sizeof("HELLO")))
      BOOST_CHECK(address3 == address1)

      zsLib::IPAddress getaddress1 = socket1->getLocalAddress();
      zsLib::IPAddress getaddress2 = socket2->getLocalAddress();
      BOOST_CHECK(address1 == getaddress1)
      BOOST_CHECK(address2 == getaddress2)
    }
    {
      zsLib::IPAddress address1(zsLib::IPAddress::loopbackV6(), 43016);
      zsLib::IPAddress address2(zsLib::IPAddress::loopbackV6(), 43017);
      zsLib::SocketPtr socket1 = zsLib::Socket::createTCP(zsLib::Socket::Create::IPv6);
      zsLib::SocketPtr socket2 = zsLib::Socket::createTCP(zsLib::Socket::Create::IPv6);

      socket1->bind(address1);
      socket1->listen();
      socket2->connect(address1);

      zsLib::IPAddress remoteIP;
      zsLib::ISocketPtr socket3 = socket1->accept(remoteIP);
      BOOST_CHECK(socket3)
      BOOST_CHECK(address2.isAddressEqual(remoteIP))

      socket3->send((BYTE *)"HELLO", sizeof("HELLO"));

      BYTE buffer[1024];
      HlZeroMemory(&(buffer[0]), sizeof(buffer));
      int length = socket2->receive(buffer, sizeof(buffer));
      BOOST_CHECK(sizeof("HELLO") == length)
      BOOST_CHECK(0 == memcmp(&(buffer[0]), "HELLO", sizeof("HELLO")))

      zsLib::IPAddress getaddress1 = socket1->getLocalAddress();
      zsLib::IPAddress getaddress2 = socket2->getLocalAddress();
      zsLib::IPAddress getaddress3 = socket3->getLocalAddress();
      BOOST_CHECK(address1 == getaddress1)
      BOOST_CHECK(address2.isAddressEqual(getaddress2))
      BOOST_CHECK(remoteIP == getaddress2)

      zsLib::IPAddress remoteaddress1 = socket3->getRemoteAddress();
      zsLib::IPAddress remoteaddress2 = socket2->getRemoteAddress();
      BOOST_CHECK(address2.isAddressEqual(remoteaddress1))
      BOOST_CHECK(getaddress2 == remoteaddress1)
      BOOST_CHECK(getaddress3 == remoteaddress2)
      BOOST_CHECK(address1 == remoteaddress2)
    }

    {int i = 0; ++i;}
  }
};



BOOST_AUTO_TEST_SUITE(zsLibSocket)

  BOOST_AUTO_TEST_CASE(TestSocket)
  {
    TestSocket test;
  }

BOOST_AUTO_TEST_SUITE_END()
