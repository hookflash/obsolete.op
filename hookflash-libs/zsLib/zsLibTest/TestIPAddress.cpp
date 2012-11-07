/*
 *  Created by Robin Raymond.
 *  Copyright 2009-2011. Robin Raymond. All rights reserved.
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

#include <zsLib/IPAddress.h>
#ifndef _WIN32
#include <arpa/inet.h>
#endif //_WIN32



//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>


#include "boost_replacement.h"

#define HlZeroStruct(xValue) memset(&(xValue), 0, sizeof(xValue))

using zsLib::BYTE;
using zsLib::DWORD;

class TestIPAddress
{
public:
  TestIPAddress()
  {
    testConstructors();
    testOtherMethods();
  }

  void testConstructors()
  {
    {
      zsLib::IPAddress ip;
      BOOST_CHECK(ip.isEmpty())
      BOOST_CHECK(ip.string() == "0.0.0.0")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(ip.isIPv4Compatible())
      BOOST_CHECK(!ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip.isAddressEmpty())
      BOOST_CHECK(ip.isPortEmpty())

      zsLib::IPAddress ip2(ip);
      BOOST_CHECK(ip2.isEmpty())
      BOOST_CHECK(ip2.string() == "0.0.0.0")
      BOOST_CHECK(ip2.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(ip2.isIPv4Compatible())
      BOOST_CHECK(!ip2.isIPv4Mapped())
      BOOST_CHECK(!ip2.isIPv46to4())
      BOOST_CHECK(ip2.isAddressEmpty())
      BOOST_CHECK(ip2.isPortEmpty())

      zsLib::IPAddress ip3(ip, 5060);
      BOOST_CHECK(!ip3.isEmpty())
      BOOST_CHECK(5060 == ip3.getPort())
      BOOST_CHECK(ip3.string() == "0.0.0.0:5060")
      BOOST_CHECK(ip3.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(ip3.isAddressEmpty())
      BOOST_CHECK(!ip3.isPortEmpty())
    }
    {
      zsLib::IPAddress ipOriginal(192,168,1,17);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPAddress ipOriginal(192,168,1,17, 5060);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      BYTE address[4] = {192,168,1,17};
      DWORD value = ntohl(*((DWORD *)&(address[0])));
      zsLib::IPAddress ipOriginal(value);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      BYTE address[4] = {192,168,1,17};
      DWORD value = ntohl(*((DWORD *)&(address[0])));
      zsLib::IPAddress ipOriginal(value, 5060);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPv6PortPair ipRaw;
      memset(&(ipRaw), 0, sizeof(ipRaw));

      ipRaw.mIPAddress.dw[2] = htonl(0xFFFF);
      ipRaw.mIPAddress.by[12] = 192;
      ipRaw.mIPAddress.by[13] = 168;
      ipRaw.mIPAddress.by[14] = 1;
      ipRaw.mIPAddress.by[15] = 17;
      ipRaw.mPort = htons(5060);

      zsLib::IPAddress ipOriginal(ipRaw);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      sockaddr_in address;
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = inet_addr("192.168.1.17");
      address.sin_port = htons(5060);

      zsLib::IPAddress ipOriginal(address);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      sockaddr_in6 address;
      address.sin6_family = AF_INET6;
#ifdef _WIN32
      address.sin6_addr.u.Word[0] = htons(0x1020);
      address.sin6_addr.u.Word[1] = htons(0xa1b1);
      address.sin6_addr.u.Word[2] = htons(0xc2d2);
      address.sin6_addr.u.Word[3] = htons(0xe3f3);
      address.sin6_addr.u.Word[4] = htons(0xa4b4);
      address.sin6_addr.u.Word[5] = htons(0xc5d5);
      address.sin6_addr.u.Word[6] = htons(0xe6f6);
      address.sin6_addr.u.Word[7] = htons(0xa7f7);
#elif defined __linux__
      address.sin6_addr.s6_addr[0] = htons(0x1020);
      address.sin6_addr.s6_addr[1] = htons(0xa1b1);
      address.sin6_addr.s6_addr[2] = htons(0xc2d2);
      address.sin6_addr.s6_addr[3] = htons(0xe3f3);
      address.sin6_addr.s6_addr[4] = htons(0xa4b4);
      address.sin6_addr.s6_addr[5] = htons(0xc5d5);
      address.sin6_addr.s6_addr[6] = htons(0xe6f6);
      address.sin6_addr.s6_addr[7] = htons(0xa7f7);
#else
      address.sin6_addr.__u6_addr.__u6_addr16[0] = htons(0x1020);
      address.sin6_addr.__u6_addr.__u6_addr16[1] = htons(0xa1b1);
      address.sin6_addr.__u6_addr.__u6_addr16[2] = htons(0xc2d2);
      address.sin6_addr.__u6_addr.__u6_addr16[3] = htons(0xe3f3);
      address.sin6_addr.__u6_addr.__u6_addr16[4] = htons(0xa4b4);
      address.sin6_addr.__u6_addr.__u6_addr16[5] = htons(0xc5d5);
      address.sin6_addr.__u6_addr.__u6_addr16[6] = htons(0xe6f6);
      address.sin6_addr.__u6_addr.__u6_addr16[7] = htons(0xa7f7);
#endif //_WIN32
      address.sin6_port = htons(5060);

      zsLib::IPAddress ipOriginal(address);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPAddress ipOriginal(zsLib::String("192.168.1.17"), 5060);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPAddress ipOriginal(zsLib::String("192.168.1.17:5060"));
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPAddress ipOriginal(zsLib::String("1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7"),5060);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPAddress ipOriginal(zsLib::String("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"));
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPAddress ipOriginal(zsLib::String("::ffff:192.168.1.17"),5060);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("192.168.1.17:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPAddress ipOriginal(zsLib::String("2002:c000:022a::"), 5060);
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("192.0.2.42:5060"))
      BOOST_CHECK(0 == (ip.stringAsIPv6()).compareNoCase("[2002:c000:22a::]:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv46to4())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip == ipOriginal)
    }
    {
      zsLib::IPAddress ipOriginal(zsLib::String("[2002:c000:022a::]:5060"));
      zsLib::IPAddress ip(ipOriginal);
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("192.0.2.42:5060"))
      BOOST_CHECK(0 == (ip.stringAsIPv6()).compareNoCase("[2002:c000:22a::]:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv46to4())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip == ipOriginal)
    }

    {int i = 0; ++i;}
  }

  void testOtherMethods()
  {
    {
      zsLib::IPAddress ip(zsLib::IPAddress::anyV4());
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip.isPortEmpty())
      BOOST_CHECK(ip.isAddrAny())
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("0.0.0.0"))
    }
    {
      zsLib::IPAddress ip(zsLib::IPAddress::anyV6());
      BOOST_CHECK(ip.isEmpty())
      BOOST_CHECK(ip.isAddressEmpty())
      BOOST_CHECK(ip.isPortEmpty())
      BOOST_CHECK(ip.isAddrAny())
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isIPv4Mapped())
      BOOST_CHECK(ip.isIPv4Compatible())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("0.0.0.0"))
    }
    {
      zsLib::IPAddress ip(zsLib::IPAddress::loopbackV4());
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip.isPortEmpty())
      BOOST_CHECK(ip.isLoopback())
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("127.0.0.1"))
    }
    {
      zsLib::IPAddress ip(zsLib::IPAddress::loopbackV6());
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip.isPortEmpty())
      BOOST_CHECK(ip.isLoopback())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("::1"))
    }
    {
      zsLib::IPAddress ipDifferent("[2010:b1a1:d2c2:f3e3:b4a4:d5c5:f6e6:f7a7]:5060");
      BOOST_CHECK(0 == (ipDifferent.string()).compareNoCase("[2010:b1a1:d2c2:f3e3:b4a4:d5c5:f6e6:f7a7]:5060"))

      zsLib::IPAddress ipOriginal(zsLib::String("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"));
      zsLib::IPAddress ip;
      ip = ipOriginal;
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip == ipOriginal)
      BOOST_CHECK(ip != ipDifferent)
    }
    {
      zsLib::IPAddress ipDifferent(zsLib::String("192.168.1.17:5060"));
      ipDifferent.convertIPv46to4();
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")

      zsLib::IPv6PortPair ipRaw;
      memset(&(ipRaw), 0, sizeof(ipRaw));

      ipRaw.mIPAddress.dw[2] = htonl(0xFFFF);
      ipRaw.mIPAddress.by[12] = 192;
      ipRaw.mIPAddress.by[13] = 168;
      ipRaw.mIPAddress.by[14] = 1;
      ipRaw.mIPAddress.by[15] = 17;
      ipRaw.mPort = htons(5060);

      zsLib::IPAddress ipOriginal(ipRaw);
      zsLib::IPAddress ip;
      ip = ipOriginal;
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqual(ipRaw))
      BOOST_CHECK(!ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))

      ipDifferent.setPort(5061);
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5061")
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(!ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
    }
    {
      zsLib::IPAddress ipDifferent(zsLib::String("[2010:b1a1:d2c2:f3e3:b4a4:d5c5:f6e6:f7a7]:5061"));
      BOOST_CHECK(0 == (ipDifferent.string()).compareNoCase("[2010:b1a1:d2c2:f3e3:b4a4:d5c5:f6e6:f7a7]:5061"))

      zsLib::IPAddress ipOriginal(zsLib::String("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"));
      zsLib::IPAddress ip;
      ip = ipOriginal;
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(0 == (ip.string()).compareNoCase("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(ip == ipOriginal)
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(!ip.isAddressEqual(ipDifferent))
      BOOST_CHECK(!ipDifferent.isEmpty())
      BOOST_CHECK(!ipDifferent.isAddressEmpty())
      BOOST_CHECK(!ipDifferent.isPortEmpty())

      ipDifferent = ipOriginal;
      BOOST_CHECK(0 == (ipDifferent.string()).compareNoCase("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5060"))
      ipDifferent.setPort(5061);
      BOOST_CHECK(0 == (ipDifferent.string()).compareNoCase("[1020:a1b1:c2d2:e3f3:a4b4:c5d5:e6f6:a7f7]:5061"))
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isAddressEqual(ipDifferent))

      ipDifferent.clear();
      BOOST_CHECK(0 == (ipDifferent.string()).compareNoCase("0.0.0.0"))
      BOOST_CHECK(0 == (ipDifferent.stringAsIPv6()).compareNoCase("::"))
      BOOST_CHECK(ipDifferent.isAddrAny())
      BOOST_CHECK(ipDifferent.isEmpty())
      BOOST_CHECK(ipDifferent.isAddressEmpty())
      BOOST_CHECK(ipDifferent.isPortEmpty())
      BOOST_CHECK(ipDifferent.isIPv4())
      BOOST_CHECK(ipDifferent.isIPv6())
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(!ip.isAddressEqual(ipDifferent))

      ip.clear();
      BOOST_CHECK(ip == ipDifferent)
      BOOST_CHECK(ip.isAddressEqual(ipDifferent))
    }
    {
      zsLib::IPAddress ipDifferent("192.168.1.17:5060");
      ipDifferent.convertIPv46to4();
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")

      zsLib::IPv6PortPair ipRaw;
      memset(&(ipRaw), 0, sizeof(ipRaw));

      ipRaw.mIPAddress.dw[2] = htonl(0xFFFF);
      ipRaw.mIPAddress.by[12] = 192;
      ipRaw.mIPAddress.by[13] = 168;
      ipRaw.mIPAddress.by[14] = 1;
      ipRaw.mIPAddress.by[15] = 17;
      ipRaw.mPort = htons(5060);

      zsLib::IPAddress ipOriginal(ipRaw);
      zsLib::IPAddress ip;
      ip = ipOriginal;
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqual(ipRaw))
      BOOST_CHECK(!ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))

      ipOriginal = ipDifferent;
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(ipOriginal == ipDifferent)

      ipDifferent.convertIPv4Mapped();
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(ipDifferent.isIPv4Mapped())
      BOOST_CHECK(!ipDifferent.isIPv46to4())
      BOOST_CHECK(ip == ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))

      ipDifferent.convertIPv4Compatible();
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")
      BOOST_CHECK(ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(!ipDifferent.isIPv46to4())
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(!ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))

      ipDifferent.convertIPv46to4();
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(ipOriginal == ipDifferent)
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(!ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))

      ipDifferent.convertIPv46to4();   // this should be a NOOP
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(ipOriginal == ipDifferent)
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(!ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))
    }
    {
      zsLib::IPAddress ipDifferent("192.168.1.17:5060");
      ipDifferent.convertIPv46to4();
      constTest(ipDifferent);
    }
    {
      BOOST_CHECK(zsLib::IPAddress::anyV4().isAddrAny())
      BOOST_CHECK(zsLib::IPAddress::anyV6().isAddrAny())

      zsLib::IPAddress ip("0.0.0.0:5060");
      BOOST_CHECK(0 == ip.string().compareNoCase("0.0.0.0:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(ip.isAddrAny())
      BOOST_CHECK(!ip.isLoopback())
      BOOST_CHECK(zsLib::IPAddress::anyV4() != ip)
      BOOST_CHECK(zsLib::IPAddress::anyV4().isAddressEqual(ip))

      ip.clear();
      BOOST_CHECK(0 == ip.string().compareNoCase("0.0.0.0"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(ip.isAddrAny())
      BOOST_CHECK(!ip.isLoopback())
      BOOST_CHECK(zsLib::IPAddress::anyV6() == ip)

      ip = zsLib::IPAddress("192.168.1.17");
      BOOST_CHECK(0 == ip.string().compareNoCase("192.168.1.17"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddrAny())
      BOOST_CHECK(!ip.isLoopback())
    }
    {
      BOOST_CHECK(zsLib::IPAddress::loopbackV4().isLoopback())
      BOOST_CHECK(zsLib::IPAddress::loopbackV6().isLoopback())

      zsLib::IPAddress ip("127.0.0.1:5060");
      BOOST_CHECK(0 == ip.string().compareNoCase("127.0.0.1:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddrAny())
      BOOST_CHECK(ip.isLoopback())
      BOOST_CHECK(zsLib::IPAddress::loopbackV4() != ip)
      BOOST_CHECK(zsLib::IPAddress::loopbackV4().isAddressEqual(ip))

      ip = zsLib::IPAddress("::1");
      BOOST_CHECK(0 == ip.string().compareNoCase("::1"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isAddrAny())
      BOOST_CHECK(ip.isLoopback())
      BOOST_CHECK(zsLib::IPAddress::loopbackV6() == ip)

      ip = zsLib::IPAddress("192.168.1.17");
      BOOST_CHECK(0 == ip.string().compareNoCase("192.168.1.17"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddrAny())
      BOOST_CHECK(!ip.isLoopback())
    }
    {
      zsLib::IPAddress ip("127.0.0.1:5060");
      BOOST_CHECK(0 == ip.string().compareNoCase("127.0.0.1:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(ip.isLoopback())

      ip = zsLib::IPAddress("::1");
      BOOST_CHECK(0 == ip.string().compareNoCase("::1"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(ip.isLoopback())
    }
    {
      // test isLinkLocal, isPrivate

      zsLib::IPAddress ip("169.254.0.0:5060");
      BOOST_CHECK(0 == ip.string().compareNoCase("169.254.0.0:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(ip.isLinkLocal())
      BOOST_CHECK(ip.isPrivate())

      sockaddr_in raw;
      ip.getIPv4(raw);
      BOOST_CHECK(raw.sin_family == AF_INET)
#ifdef _WIN32
      BOOST_CHECK(raw.sin_addr.S_un.S_addr == htonl(0xA9FE0000))
#else
      BOOST_CHECK(raw.sin_addr.s_addr == htonl(0xA9FE0000))
#endif //_WIN32
      BOOST_CHECK(raw.sin_port == htons(5060))

      ip = zsLib::IPAddress("192.168.1.1:5060");
      BOOST_CHECK(0 == ip.string().compareNoCase("192.168.1.1:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(ip.isPrivate())

      HlZeroStruct(raw);
      ip.getIPv4(raw);
      BOOST_CHECK(raw.sin_family == AF_INET)
#ifdef _WIN32
      BOOST_CHECK(raw.sin_addr.S_un.S_addr == htonl(0xC0A80101))
#else
      BOOST_CHECK(raw.sin_addr.s_addr == htonl(0xC0A80101))
#endif //_WIN32
      BOOST_CHECK(raw.sin_port == htons(5060))

      ip = zsLib::IPAddress("10.1.2.3:5060");
      BOOST_CHECK(0 == ip.string().compareNoCase("10.1.2.3:5060"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(ip.isPrivate())

      HlZeroStruct(raw);
      ip.getIPv4(raw);
      BOOST_CHECK(raw.sin_family == AF_INET)
#ifdef _WIN32
      BOOST_CHECK(raw.sin_addr.S_un.S_addr == htonl(0x0A010203))
#else
      BOOST_CHECK(raw.sin_addr.s_addr == htonl(0x0A010203))
#endif //_WIN32
      BOOST_CHECK(raw.sin_port == htons(5060))

      // FE80::/10 or 11111110 10XXXXXX (link local)
      ip = zsLib::IPAddress("[fe80::abcd]:5060");
      BOOST_CHECK(0 == ip.string().compareNoCase("[fe80::abcd]:5060"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(ip.isLinkLocal())
      BOOST_CHECK(ip.isPrivate())

      HlZeroStruct(raw);
      bool caught = false;
      try {ip.getIPv4(raw);} catch(zsLib::IPAddress::Exceptions::NotIPv4) {caught = true;}
      BOOST_CHECK(caught)

      sockaddr_in6 raw6;
      ip.getIPv6(raw6);
      BOOST_CHECK(raw6.sin6_family == AF_INET6)
#ifdef _WIN32
      BOOST_CHECK(raw6.sin6_addr.u.Word[0] == htons(0xfe80))
      BOOST_CHECK(raw6.sin6_addr.u.Word[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[7] == htons(0xabcd))
#elif defined __linux__
      BOOST_CHECK(raw6.sin6_addr.s6_addr[0] == htons(0xfe80))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[7] == htons(0xabcd))
#else
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[0] == htons(0xfe80))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[7] == htons(0xabcd))
#endif //WIN32
      BOOST_CHECK(raw6.sin6_port == htons(5060))

      // make bit pattern is FEC0::/10 or 11111110 11XXXXXX (NOT link local)
      ip = zsLib::IPAddress("fec0::abcd");
      BOOST_CHECK(0 == ip.string().compareNoCase("fec0::abcd"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(!ip.isPrivate())

      HlZeroStruct(raw6);
      ip.getIPv6(raw6);
      BOOST_CHECK(raw6.sin6_family == AF_INET6)
#ifdef _WIN32
      BOOST_CHECK(raw6.sin6_addr.u.Word[0] == htons(0xfec0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[7] == htons(0xabcd))
#elif defined __linux__
      BOOST_CHECK(raw6.sin6_addr.s6_addr[0] == htons(0xfec0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[7] == htons(0xabcd))
#else
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[0] == htons(0xfec0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[7] == htons(0xabcd))
#endif //_WIN32
      BOOST_CHECK(raw6.sin6_port == htons(0))

      // bit pattern is FC00::/7 or 1111110X (private)
      ip = zsLib::IPAddress("fc00::abcd");
      BOOST_CHECK(0 == ip.string().compareNoCase("fc00::abcd"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(ip.isPrivate())

      HlZeroStruct(raw6);
      ip.getIPv6(raw6);
      BOOST_CHECK(raw6.sin6_family == AF_INET6)
#ifdef _WIN32
      BOOST_CHECK(raw6.sin6_addr.u.Word[0] == htons(0xfc00))
      BOOST_CHECK(raw6.sin6_addr.u.Word[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[7] == htons(0xabcd))
#elif defined __linux__
      BOOST_CHECK(raw6.sin6_addr.s6_addr[0] == htons(0xfc00))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[7] == htons(0xabcd))
#else
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[0] == htons(0xfc00))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[7] == htons(0xabcd))
#endif //_WIN32
      BOOST_CHECK(raw6.sin6_port == htons(0))

      // make bit pattern 1111111X (not private)
      ip = zsLib::IPAddress("fe00::abcd");
      BOOST_CHECK(0 == ip.string().compareNoCase("fe00::abcd"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(!ip.isPrivate())

      HlZeroStruct(raw6);
      ip.getIPv6(raw6);
      BOOST_CHECK(raw6.sin6_family == AF_INET6)
#ifdef _WIN32
      BOOST_CHECK(raw6.sin6_addr.u.Word[0] == htons(0xfe00))
      BOOST_CHECK(raw6.sin6_addr.u.Word[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[7] == htons(0xabcd))
#elif defined __linux__
      BOOST_CHECK(raw6.sin6_addr.s6_addr[0] == htons(0xfe00))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[7] == htons(0xabcd))
#else
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[0] == htons(0xfe00))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[7] == htons(0xabcd))
#endif //_WIN32
      BOOST_CHECK(raw6.sin6_port == htons(0))

      ip = zsLib::IPAddress::loopbackV4();
      BOOST_CHECK(0 == ip.string().compareNoCase("127.0.0.1"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(ip.isLoopback())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(!ip.isPrivate())

      HlZeroStruct(raw6);
      ip.getIPv6(raw6);
      BOOST_CHECK(raw6.sin6_family == AF_INET6)
#ifdef _WIN32
      BOOST_CHECK(raw6.sin6_addr.u.Word[0] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[5] == htons(0xFFFF))
      BOOST_CHECK(raw6.sin6_addr.u.Word[6] == htons(0x7F00))
      BOOST_CHECK(raw6.sin6_addr.u.Word[7] == htons(0x0001))
#elif defined __linux__
      BOOST_CHECK(raw6.sin6_addr.s6_addr[0] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[5] == htons(0xFFFF))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[6] == htons(0x7F00))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[7] == htons(0x0001))
#else
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[0] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[5] == htons(0xFFFF))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[6] == htons(0x7F00))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[7] == htons(0x0001))
#endif //_WIN32
      BOOST_CHECK(raw6.sin6_port == htons(0))

      ip = zsLib::IPAddress::loopbackV6();
      BOOST_CHECK(0 == ip.string().compareNoCase("::1"))
      BOOST_CHECK(!ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(!ip.isPrivate())

      HlZeroStruct(raw6);
      ip.getIPv6(raw6);
      BOOST_CHECK(raw6.sin6_family == AF_INET6)
#ifdef _WIN32
      BOOST_CHECK(raw6.sin6_addr.u.Word[0] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.u.Word[7] == htons(0x0001))
#elif defined __linux__
      BOOST_CHECK(raw6.sin6_addr.s6_addr[0] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.s6_addr[7] == htons(0x0001))
#else
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[0] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[1] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[2] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[3] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[4] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[5] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[6] == htons(0))
      BOOST_CHECK(raw6.sin6_addr.__u6_addr.__u6_addr16[7] == htons(0x0001))
#endif //_WIN32
      BOOST_CHECK(raw6.sin6_port == htons(0))

      ip = zsLib::IPAddress::anyV4();
      BOOST_CHECK(0 == ip.string().compareNoCase("0.0.0.0"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(!ip.isPrivate())

      HlZeroStruct(raw);
      ip.getIPv4(raw);
      BOOST_CHECK(raw.sin_family == AF_INET)
#ifdef _WIN32
      BOOST_CHECK(raw.sin_addr.S_un.S_addr == htonl(0))
#else
      BOOST_CHECK(raw.sin_addr.s_addr == htonl(0))
#endif //_WIN32
      BOOST_CHECK(raw.sin_port == htons(0))

      ip = zsLib::IPAddress::anyV6();
      BOOST_CHECK(0 == ip.string().compareNoCase("0.0.0.0"))
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(ip.isIPv6())
      BOOST_CHECK(!ip.isLinkLocal())
      BOOST_CHECK(!ip.isPrivate())

      HlZeroStruct(raw);
      ip.getIPv4(raw);
      BOOST_CHECK(raw.sin_family == AF_INET)
#ifdef _WIN32
      BOOST_CHECK(raw.sin_addr.S_un.S_addr == htonl(0))
#else
      BOOST_CHECK(raw.sin_addr.s_addr == htonl(0))
#endif //_WIN32
      BOOST_CHECK(raw.sin_port == htons(0))
    }
    {int i = 0; ++i;}
  }

  void constTest(const zsLib::IPAddress &constIP)
  {
    {
      BOOST_CHECK(!constIP.isIPv4Compatible())
      BOOST_CHECK(!constIP.isIPv4Mapped())
      BOOST_CHECK(constIP.isIPv46to4())
      BOOST_CHECK(constIP.string() == "192.168.1.17:5060")
      BOOST_CHECK(!constIP.isEmpty())
      BOOST_CHECK(!constIP.isAddressEmpty())
      BOOST_CHECK(!constIP.isPortEmpty())
      BOOST_CHECK(!constIP.isAddrAny())
      BOOST_CHECK(!constIP.isLoopback())
      BOOST_CHECK(constIP.isIPv4())
      BOOST_CHECK(!constIP.isIPv6())
      BOOST_CHECK(!constIP.isLinkLocal())
      BOOST_CHECK(constIP.isPrivate())
      BYTE byteIP[4] = {192,168,1,17};
      DWORD value = htonl(constIP.getIPv4AddressAsDWORD());
      BOOST_CHECK(0 == memcmp(&value, &(byteIP[0]), sizeof(value)))
      BOOST_CHECK(5060 == constIP.getPort())
      sockaddr_in ipv4;
      constIP.getIPv4(ipv4);
      BOOST_CHECK(AF_INET == ipv4.sin_family)
#ifdef _WIN32
      BOOST_CHECK(0 == memcmp(&(ipv4.sin_addr.S_un.S_addr), &(byteIP[0]), sizeof(ipv4.sin_addr.S_un.S_addr)))
#else
      BOOST_CHECK(0 == memcmp(&(ipv4.sin_addr.s_addr), &(byteIP[0]), sizeof(ipv4.sin_addr.s_addr)))
#endif //_WIN32
      BOOST_CHECK(5060 == ntohs(ipv4.sin_port))

      sockaddr_in6 ipv6;
      constIP.getIPv6(ipv6);
      BOOST_CHECK(AF_INET6 == ipv6.sin6_family)
#ifdef _WIN32
      BOOST_CHECK(0x2002 == ntohs(ipv6.sin6_addr.u.Word[0]))
      BOOST_CHECK(0 == ipv6.sin6_addr.u.Word[3])
      BOOST_CHECK(0 == ipv6.sin6_addr.u.Word[4])
      BOOST_CHECK(0 == ipv6.sin6_addr.u.Word[5])
      BOOST_CHECK(0 == ipv6.sin6_addr.u.Word[6])
      BOOST_CHECK(0 == ipv6.sin6_addr.u.Word[7])
      BOOST_CHECK(0 == memcmp(&(ipv6.sin6_addr.u.Word[1]), &(byteIP[0]), sizeof(byteIP)))
#elif defined __linux__
      BOOST_CHECK(0x2002 == ntohs(ipv6.sin6_addr.s6_addr[0]))
      BOOST_CHECK(0 == ipv6.sin6_addr.s6_addr[3])
      BOOST_CHECK(0 == ipv6.sin6_addr.s6_addr[4])
      BOOST_CHECK(0 == ipv6.sin6_addr.s6_addr[5])
      BOOST_CHECK(0 == ipv6.sin6_addr.s6_addr[6])
      BOOST_CHECK(0 == ipv6.sin6_addr.s6_addr[7])
      BOOST_CHECK(0 == memcmp(&(ipv6.sin6_addr.s6_addr[1]), &(byteIP[0]), sizeof(byteIP)))
#else
      BOOST_CHECK(0x2002 == ntohs(ipv6.sin6_addr.__u6_addr.__u6_addr16[0]))
      BOOST_CHECK(0 == ipv6.sin6_addr.__u6_addr.__u6_addr16[3])
      BOOST_CHECK(0 == ipv6.sin6_addr.__u6_addr.__u6_addr16[4])
      BOOST_CHECK(0 == ipv6.sin6_addr.__u6_addr.__u6_addr16[5])
      BOOST_CHECK(0 == ipv6.sin6_addr.__u6_addr.__u6_addr16[6])
      BOOST_CHECK(0 == ipv6.sin6_addr.__u6_addr.__u6_addr16[7])
      BOOST_CHECK(0 == memcmp(&(ipv6.sin6_addr.__u6_addr.__u6_addr16[1]), &(byteIP[0]), sizeof(byteIP)))
#endif //_WIN32
      BOOST_CHECK(5060 == ntohs(ipv6.sin6_port))

      zsLib::IPAddress ipDifferent("192.168.1.17:5060");
      ipDifferent.convertIPv46to4();
      ipDifferent == constIP;
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")

      BOOST_CHECK(constIP == ipDifferent)
      BOOST_CHECK(!(constIP != ipDifferent))
      BOOST_CHECK(constIP.isAddressEqual(ipDifferent))
      BOOST_CHECK(constIP.isAddressEqual(ipDifferent))
      BOOST_CHECK(constIP.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(constIP.isAddressEqualIgnoringIPv4Format(ipDifferent))

      zsLib::IPv6PortPair ipRaw;
      memset(&(ipRaw), 0, sizeof(ipRaw));

      ipRaw.mIPAddress.dw[2] = htonl(0xFFFF);
      ipRaw.mIPAddress.by[12] = 192;
      ipRaw.mIPAddress.by[13] = 168;
      ipRaw.mIPAddress.by[14] = 1;
      ipRaw.mIPAddress.by[15] = 17;
      ipRaw.mPort = htons(5060);

      BOOST_CHECK(!(constIP == ipRaw))
      BOOST_CHECK(constIP != ipRaw)
      BOOST_CHECK(!constIP.isAddressEqual(ipRaw))
      BOOST_CHECK(constIP.isEqualIgnoringIPv4Format(ipRaw))
      BOOST_CHECK(constIP.isAddressEqualIgnoringIPv4Format(ipRaw))

      zsLib::IPAddress ipOriginal(ipRaw);
      zsLib::IPAddress ip;
      ip = ipOriginal;
      BOOST_CHECK(!ip.isEmpty())
      BOOST_CHECK(!ip.isPortEmpty())
      BOOST_CHECK(ip.string() == "192.168.1.17:5060")
      BOOST_CHECK(ip.isIPv4())
      BOOST_CHECK(!ip.isIPv6())
      BOOST_CHECK(!ip.isAddressEmpty())
      BOOST_CHECK(!ip.isIPv4Compatible())
      BOOST_CHECK(ip.isIPv4Mapped())
      BOOST_CHECK(!ip.isIPv46to4())
      BOOST_CHECK(ip == ipOriginal)
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqual(ipRaw))
      BOOST_CHECK(!ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))

      ipOriginal = ipDifferent;
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(ipOriginal == ipDifferent)

      ipDifferent = constIP.convertIPv4Mapped();
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(ipDifferent.isIPv4Mapped())
      BOOST_CHECK(!ipDifferent.isIPv46to4())
      BOOST_CHECK(ip == ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))

      ipDifferent = constIP.convertIPv4Compatible();
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")
      BOOST_CHECK(ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(!ipDifferent.isIPv46to4())
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(!ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))

      ipDifferent = constIP.convertIPv46to4();
      BOOST_CHECK(ipDifferent.string() == "192.168.1.17:5060")
      BOOST_CHECK(!ipDifferent.isIPv4Compatible())
      BOOST_CHECK(!ipDifferent.isIPv4Mapped())
      BOOST_CHECK(ipDifferent.isIPv46to4())
      BOOST_CHECK(ipOriginal == ipDifferent)
      BOOST_CHECK(ip != ipDifferent)
      BOOST_CHECK(ip.isEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(ip.isAddressEqualIgnoringIPv4Format(ipDifferent))
      BOOST_CHECK(!ipDifferent.isAddressEqual(ipRaw))
      BOOST_CHECK(ipDifferent.isAddressEqualIgnoringIPv4Format(ipRaw))
    }
    {int i = 0; ++i;}
  }
};


BOOST_AUTO_TEST_SUITE(zsLibIPAddress)

  BOOST_AUTO_TEST_CASE(TestIPAddress)
  {
    TestIPAddress test;
  }

BOOST_AUTO_TEST_SUITE_END()
