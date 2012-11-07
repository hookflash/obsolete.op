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

#pragma once

#ifndef ZSLIB_IPADDRESS_H_084245516d22e267a3ef86fb4ff67430
#define ZSLIB_IPADDRESS_H_084245516d22e267a3ef86fb4ff67430

#include <zsLib/internal/IPAddress.h>

#pragma warning(push)
#pragma warning(disable: 4290)

namespace zsLib
{
  union IPv6Address
  {
    BYTE by[16];
    WORD w[8];
    DWORD dw[4];
    ULONGLONG ull[2];
  };

  struct IPv6PortPair
  {
    IPv6Address mIPAddress; // the raw IP address, stored in IPv6 format, network byte order
    WORD mPort;             // stored in network byte order
  };

  class IPAddress : public IPv6PortPair
  {
  public:
    struct Exceptions
    {
      ZS_DECLARE_CUSTOM_EXCEPTION(ParseError)
      ZS_DECLARE_CUSTOM_EXCEPTION(NotIPv4)
    };

  public:
    IPAddress();

    IPAddress(
              const IPAddress &inIPAddress,
              WORD inPort = 0      // host byte order
              );

    explicit IPAddress(     // specify an IP address by dot notation, stores as an IPv4 mapped address
                       BYTE w,
                       BYTE x,
                       BYTE y,
                       BYTE z,
                       WORD inPort = 0      // host byte order
                       );

    explicit IPAddress(
                       DWORD inIPv4Address, // host byte order
                       WORD inPort = 0      // host byte order
                       );

    explicit IPAddress(
                       const IPv6PortPair &inPortPair,
                       WORD inPort = 0      // host byte order
                       );

    explicit IPAddress(
                       const IPv6Address &inIPAddress,
                       WORD inPort = 0      // host byte order
                       );

    explicit IPAddress(
                       const sockaddr_in &inIPAddress,
                       WORD inPort = 0      // host byte order
                       );

    explicit IPAddress(
                       const in_addr &inIPAddress,
                       WORD inPort = 0      // host byte order
                       );

    explicit IPAddress(
                       const in6_addr &inIPAddress,
                       WORD inPort = 0      // host byte order
                       );

    explicit IPAddress(
                       const sockaddr_in6 &inIPAddress,
                       WORD inPort = 0      // host byte order
                       );

    explicit IPAddress(
                       const String &inString,
                       WORD inPort = 0
                       ) throw(Exceptions::ParseError);

    static bool isConvertable(const String &inString);

    static IPAddress anyV4();    // returns IPv4 mapped
    static IPAddress anyV6();

    static IPAddress loopbackV4();   // returns IPv4 mapped
    static IPAddress loopbackV6();

    IPAddress &operator=(const IPAddress &inIPAddress);
    IPAddress &operator=(const IPv6PortPair &inPortPair);

    // checks both address and port
    bool operator==(const IPAddress &inIPAddress) const;                          // warning: IPv4 in different formats are not considered equal, use isEqualIgnoringIPv4Format if your want to ignore IPv4 format
    bool operator==(const IPv6PortPair &inPortPair) const;                        // warning: IPv4 in different formats are not considered equal, use isEqualIgnoringIPv4Format if your want to ignore IPv4 format

    bool operator!=(const IPAddress &inIPAddress) const;                          // warning: IPv4 in different formats are not considered equal, use isEqualIgnoringIPv4Format
    bool operator!=(const IPv6PortPair &inPortPair) const;                        // warning: IPv4 in different formats are not considered equal, use isEqualIgnoringIPv4Format

    bool operator<(const IPAddress &inIPAddress) const;                           // warning: IPv4 in different formats are not considered equal, use isEqualIgnoringIPv4Format if your want to ignore IPv4 format
    bool operator<(const IPv6PortPair &inPortPair) const;                         // warning: IPv4 in different formats are not considered equal, use isEqualIgnoringIPv4Format if your want to ignore IPv4 format

    bool operator>(const IPAddress &inIPAddress) const;                           // warning: IPv4 in different formats are not considered equal, use isEqualIgnoringIPv4Format if your want to ignore IPv4 format
    bool operator>(const IPv6PortPair &inPortPair) const;                         // warning: IPv4 in different formats are not considered equal, use isEqualIgnoringIPv4Format if your want to ignore IPv4 format

    // checks both address and port
    bool isEqualIgnoringIPv4Format(const IPAddress &inIPAddress) const;           // does an intelligent IPv4 comparison which looks at the IPv4 address
    bool isEqualIgnoringIPv4Format(const IPv6PortPair &inPortPair) const;         // does an intelligent IPv4 comparison which looks at the IPv4 address

    bool isAddressEqual(const IPAddress &inIPAddress) const;                      // warning: IPv4 in different formats will return false, use isAddressEqualIgnoringIPv4Format if your want to ignore IPv4 format
    bool isAddressEqual(const IPv6PortPair &inPortPair) const;                    // warning: IPv4 in different formats will return false, use isAddressEqualIgnoringIPv4Format if your want to ignore IPv4 format

    bool isAddressEqualIgnoringIPv4Format(const IPAddress &inIPAddress) const;    // does an intelligent IPv4 comparison which looks at the IPv4 address
    bool isAddressEqualIgnoringIPv4Format(const IPv6PortPair &inPortPair) const;  // does an intelligent IPv4 comparison which looks at the IPv4 address

    void clear();

    bool isEmpty() const;
    bool isAddressEmpty() const;
    bool isPortEmpty() const;

    // conversion routines between all the IPv4 structures
    void      convertIPv4Mapped() throw(Exceptions::NotIPv4);
    IPAddress convertIPv4Mapped() const throw(Exceptions::NotIPv4);
    void      convertIPv4Compatible() throw(Exceptions::NotIPv4);                 // an IPv4 compatible address will also return IPv6 as "true" since it is both
    IPAddress convertIPv4Compatible() const throw(Exceptions::NotIPv4);           // an IPv4 compatible address will also return IPv6 as "true" since it is both
    void      convertIPv46to4() throw(Exceptions::NotIPv4);
    IPAddress convertIPv46to4() const throw(Exceptions::NotIPv4);

    bool isAddrAny() const;                                                       // is either a IPv4 or IPv6 addr any
    bool isLoopback() const;                                                      // is either a IPv4 or IPv6 loopback address

    bool isIPv4() const;                                                          // is some kind of IPv4 address (mapped, compatible, 6to4)
    bool isIPv6() const;

    bool isLinkLocal() const;
    bool isPrivate() const;                                                       // is a private IP address

    bool isIPv4Mapped() const;                                                    // is an IPv4 mapped address
    bool isIPv4Compatible() const;                                                // is an IPv4 compatible address
    bool isIPv46to4() const;                                                      // is a 6to4 IPv4 address

    DWORD getIPv4AddressAsDWORD() const throw(Exceptions::NotIPv4);               // returns in host byte order
    WORD getPort() const;                                                         // returns in host byte order
    void setPort(WORD inPort);

    void getIPv4(sockaddr_in &outAddress) const throw(Exceptions::NotIPv4);
    void getIPv6(sockaddr_in6 &outAddress) const;

    String string(bool inIncludePort = true) const;                               // if the IP is an IPv4 address then output as IPv4 otherwise output as IPv6
    String stringAsIPv6(bool inIncludePort = true) const;                         // force the output to be in IPv6 format, even if the IP is IPv4 encoded

  private:
  };

}

#pragma warning(pop)

#endif //ZSLIB_IPADDRESS_H_084245516d22e267a3ef86fb4ff67430
