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

#include <zsLib/IPAddress.h>
#include <zsLib/Numeric.h>

#ifdef _LINUX
#include <stdio.h>
#endif //_LINUX

#pragma warning(push)
#pragma warning(disable:4290)
#pragma warning(disable:4996)


namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

// http://www.sfr-fresh.com/unix/misc/dante-1.2.0.tar.gz:a/dante-1.2.0/libscompat/inet_pton.c
#ifndef AF_INET6
#define     AF_INET6  24     /* IPv6 */
#endif /* !AF_INET6 */
#ifndef INADDRSZ
#define INADDRSZ        4               /* IPv4 T_A */
#endif /* !INADDRSZ */
#ifndef IN6ADDRSZ
#define IN6ADDRSZ 16      /* IPv6 T_AAAA */
#endif /* !IN6ADDRSZ */
#ifndef INT16SZ
#define INT16SZ     2     /* for systems without 16-bit ints */
#endif /* !INT16SZ */

namespace zsLib
{

  namespace internal
  {
    static int inet_pton6(const char *src, u_char *dst);
    static int inet_pton4(const char *src, u_char *dst);
    static const char *inet_ntop6(const u_char *src, char *dst, size_t size);
  } // namespace internal

  IPAddress::IPAddress()
  {
    clear();
  }

  IPAddress::IPAddress(
                       const IPAddress &inIPAddress,
                       WORD inPort
                       )
  {
    if (this == &inIPAddress)
      return;

    mIPAddress = inIPAddress.mIPAddress;
    mPort = inIPAddress.mPort;
    if (0 != inPort)
      mPort = htons(inPort);
  }

  //RFC2553 - 3.7 Compatibility with IPv4 Nodes
  //
  //   The API also provides a different type of compatibility: the ability
  //   for IPv6 applications to interoperate with IPv4 applications.  This
  //   feature uses the IPv4-mapped IPv6 address format defined in the IPv6
  //   addressing architecture specification [2].  This address format
  //   allows the IPv4 address of an IPv4 node to be represented as an IPv6
  //   address.  The IPv4 address is encoded into the low-order 32 bits of
  //   the IPv6 address, and the high-order 96 bits hold the fixed prefix
  //   0:0:0:0:0:FFFF.  IPv4- mapped addresses are written as follows:
  //
  //    ::FFFF:<IPv4-address>
  //
  IPAddress::IPAddress(     // specify an IP address by dot notation
                       BYTE w,
                       BYTE x,
                       BYTE y,
                       BYTE z,
                       WORD inPort          // host byte order
                       )
  {
    clear();
    mIPAddress.dw[2] = htonl(0xFFFF);
    mIPAddress.by[12] = w;
    mIPAddress.by[13] = x;
    mIPAddress.by[14] = y;
    mIPAddress.by[15] = z;
    mPort = htons(inPort);
  }

  IPAddress::IPAddress(
                       DWORD inIPv4Address, // host byte order
                       WORD inPort          // host byte order
                       )
  {
    clear();
    mIPAddress.dw[2] = htonl(0xFFFF);
    mIPAddress.dw[3] = htonl((u_long)inIPv4Address);
    mPort = htons(inPort);
  }

  IPAddress::IPAddress(
                       const IPv6PortPair &inPortPair,
                       WORD inPort          // host byte order
                       )
  {
    mIPAddress = inPortPair.mIPAddress;
    mPort = inPortPair.mPort;
    if (0 != inPort)
      mPort = htons(inPort);
  }

  IPAddress::IPAddress(
                       const IPv6Address &inIPAddress,
                       WORD inPort          // host byte order
                       )
  {
    mIPAddress = inIPAddress;
    mPort = htons(inPort);
  }

  IPAddress::IPAddress(
                       const in_addr &inIPAddress,
                       WORD inPort          // host byte order
                       )
  {
    clear();
    IPAddress ipv4((DWORD)ntohl(inIPAddress.s_addr), inPort);
    (*this) = ipv4;
  }

  IPAddress::IPAddress(
                       const sockaddr_in &inIPAddress,
                       WORD inPort          // host byte order
                       )
  {
    clear();
    IPAddress ipv4((DWORD)ntohl(inIPAddress.sin_addr.s_addr), 0 == inPort ? (ntohs(inIPAddress.sin_port)) : inPort);
    (*this) = ipv4;
  }

  IPAddress::IPAddress(
                       const in6_addr &inIPAddress,
                       WORD inPort          // host byte order
                       )
  {
    mIPAddress = reinterpret_cast<const IPv6Address &>(inIPAddress);
    mPort = htons(inPort);
  }

  IPAddress::IPAddress(
                       const sockaddr_in6 &inIPAddress,
                       WORD inPort          // host byte order
                       )
  {
    mIPAddress = reinterpret_cast<const IPv6Address &>(inIPAddress.sin6_addr);
    mPort = inIPAddress.sin6_port;
    if (0 != inPort)
      mPort = htons(inPort);
  }

  IPAddress::IPAddress(
                       const String &inString,
                       WORD inPort
                       ) throw (IPAddress::Exceptions::ParseError)
  {
    clear();

    {
      if (inString.isEmpty()) goto finally;

      // scope: try to parse as an IPv6 address first
      try
      {
        int openBracketPos = inString.find("[");
        int closeBracketPos = (openBracketPos >= 0 ? inString.find("]", openBracketPos+1) : String::npos);

        // this is illegal
        ZS_THROW_CUSTOM_MSG_IF(
                               Exceptions::ParseError,
                               ((String::npos != openBracketPos) || (String::npos != closeBracketPos)) &&
                               ((String::npos == openBracketPos) || (String::npos == closeBracketPos)),
                               "IPv6 address missing matching [] during parse: "
                               )

        String ipv6;
        if (String::npos != openBracketPos)
        {
          int colonPos = inString.find(":", closeBracketPos+1);
          if (String::npos != colonPos)
          {
            ZS_THROW_CUSTOM_MSG_IF(
                                   Exceptions::ParseError,
                                   colonPos != closeBracketPos+1,
                                   "Colon \':\' was illegally placed in IPv6 address (should be in format [address]:port): "
                                  )
            String port(inString.substr(colonPos+1));
            port.trim();
            inPort = (WORD)(Numeric<WORD>(port));
          }
          ipv6 = inString.substr(openBracketPos+1, (closeBracketPos-(openBracketPos+1)));
        }
        else
          ipv6 = inString;

        ipv6.trim();

        // do a trial convert, done if succeeds
        if (internal::inet_pton6(ipv6, (unsigned char *)&mIPAddress)) goto next;

        ZS_THROW_CUSTOM_MSG_IF(
                               Exceptions::ParseError,
                               ((String::npos != openBracketPos) || (String::npos != closeBracketPos)),  // could not be an IPv4 if contained brackets
                               "Found illegal [] brackets around a non-IPv6 address <" + inString + ">: "
                               )
      }
      catch (Numeric<WORD>::ValueOutOfRange &) {
        ZS_THROW_CUSTOM(Exceptions::ParseError, ("Failed to parse IP address: " + inString).c_str())
      }

      // scope: try to parse as IPv4
      try
      {
        // this has to be an IPv4 address
        String ipv4;
        int colonPos = inString.find(":");
        if (String::npos != colonPos)
        {
          String port(inString.substr(colonPos+1));
          port.trim();
          inPort = (WORD)(Numeric<WORD>(port));
          ipv4 = inString.substr(0, colonPos);
        }
        else
          ipv4 = inString;

        ipv4.trim();
        DWORD address = 0;
        if (internal::inet_pton4(ipv4, (BYTE *)&address))
        {
          IPAddress temp(ntohl(address));
          (*this) = temp;
          goto next;
        }
      }
      catch (Numeric<WORD>::ValueOutOfRange &) {
        ZS_THROW_CUSTOM(Exceptions::ParseError, ("Failed to parse IP address: " + inString).c_str())
      }

      // this was not an IPv6 or IPv4 address, conversion failed
      ZS_THROW_CUSTOM(Exceptions::ParseError, ("Failed to parse IP address: " + inString).c_str())
    }
    next:
    {
      if (0 != inPort)
        mPort = htons(inPort);
    }
    finally:
    {
    }
  }

  bool IPAddress::isConvertable(const String &inString)
  {
    WORD bogusPort = 0;

    if (inString.isEmpty()) return true;

    // scope: try to parse as an IPv6 address first
    try
    {
      int openBracketPos = inString.find("[");
      int closeBracketPos = (openBracketPos >= 0 ? inString.find("]", openBracketPos+1) : String::npos);

      // this is illegal
      if (((String::npos != openBracketPos) || (String::npos != closeBracketPos)) &&
          ((String::npos == openBracketPos) || (String::npos == closeBracketPos))) return false;

      String ipv6;
      if (String::npos != openBracketPos)
      {
        int colonPos = inString.find(":", closeBracketPos+1);
        if (String::npos != colonPos)
        {
          if (colonPos != closeBracketPos+1) return false;
          String port(inString.substr(colonPos+1));
          port.trim();
          bogusPort = (WORD)(Numeric<WORD>(port));
        }
        ipv6 = inString.substr(openBracketPos+1, (closeBracketPos-(openBracketPos+1)));
      }
      else
        ipv6 = inString;

      ipv6.trim();

      // do a trial convert, done if succeeds
      IPAddress bogus;
      if (internal::inet_pton6(ipv6, (unsigned char *)&bogus)) return true;

      if ((String::npos != openBracketPos) || (String::npos != closeBracketPos)) return false;
    }
    catch (Numeric<WORD>::ValueOutOfRange &) {
      return false;
    }

    // scope: try to parse as IPv4
    try
    {
      // this has to be an IPv4 address
      String ipv4;
      int colonPos = inString.find(":");
      if (String::npos != colonPos)
      {
        String port(inString.substr(colonPos+1));
        port.trim();
        bogusPort = (WORD)(Numeric<WORD>(port));
        ipv4 = inString.substr(0, colonPos);
      }
      else
        ipv4 = inString;

      ipv4.trim();
      DWORD address = 0;
      if (internal::inet_pton4(ipv4, (BYTE *)&address)) return true;
    }
    catch (Numeric<WORD>::ValueOutOfRange &) {
      return false;
    }

    // this was not an IPv6 or IPv4 address, conversion failed
    return false;
  }

  IPAddress IPAddress::anyV4()
  {
    static IPAddress any((DWORD)0, 0);
    return any;
  }

  IPAddress IPAddress::anyV6()
  {
    static IPAddress any;
    return any;
  }

  IPAddress IPAddress::loopbackV4()
  {
    static IPAddress loopback(127, 0, 0, 1);
    return loopback;
  }

  IPAddress IPAddress::loopbackV6()
  {
    static IPAddress loopback;
    loopback.mIPAddress.dw[3] = htonl(1);
    return loopback;
  }

  IPAddress &IPAddress::operator=(const IPAddress &inIPAddress)
  {
    mIPAddress = inIPAddress.mIPAddress;
    mPort = inIPAddress.mPort;
    return *this;
  }

  IPAddress &IPAddress::operator=(const IPv6PortPair &inPortPair)
  {
    mIPAddress = inPortPair.mIPAddress;
    mPort = inPortPair.mPort;
    return *this;
  }

  bool IPAddress::operator==(const IPAddress &inIPAddress) const
  {
    size_t size = sizeof(mIPAddress.by);
    if (0 != memcmp(&(inIPAddress.mIPAddress), &mIPAddress, size))
      return false;
    return inIPAddress.mPort == mPort;
  }

  bool IPAddress::operator==(const IPv6PortPair &inPortPair) const
  {
    size_t size = sizeof(mIPAddress.by);
    if (0 != memcmp(&(inPortPair.mIPAddress), &mIPAddress, size))
      return false;
    return inPortPair.mPort == mPort;
  }

  bool IPAddress::operator!=(const IPAddress &inIPAddress) const
  {
    return !(*this == inIPAddress);
  }

  bool IPAddress::operator!=(const IPv6PortPair &inPortPair) const
  {
    return !(*this == inPortPair);
  }

  bool IPAddress::operator<(const IPAddress &inIPAddress) const
  {
    size_t size = sizeof(mIPAddress.by);
    int compare = memcmp(&mIPAddress, &(inIPAddress.mIPAddress), size);
    if (compare < 0)
      return true;
    if (compare > 0)
      return false;
    return ntohs(mPort) < ntohs(inIPAddress.mPort);
  }

  bool IPAddress::operator<(const IPv6PortPair &inPortPair) const
  {
    size_t size = sizeof(mIPAddress.by);
    int compare = memcmp(&mIPAddress, &(inPortPair.mIPAddress), size);
    if (compare < 0)
      return true;
    if (compare > 0)
      return false;
    return ntohs(mPort) < ntohs(inPortPair.mPort);
  }

  bool IPAddress::operator>(const IPAddress &inIPAddress) const
  {
    size_t size = sizeof(mIPAddress.by);
    int compare = memcmp(&mIPAddress, &(inIPAddress.mIPAddress), size);
    if (compare > 0)
      return true;
    if (compare < 0)
      return false;
    return ntohs(mPort) > ntohs(inIPAddress.mPort);
  }

  bool IPAddress::operator>(const IPv6PortPair &inPortPair) const
  {
    size_t size = sizeof(mIPAddress.by);
    int compare = memcmp(&mIPAddress, &(inPortPair.mIPAddress), size);
    if (compare > 0)
      return true;
    if (compare < 0)
      return false;
    return ntohs(mPort) > ntohs(inPortPair.mPort);
  }

  bool IPAddress::isEqualIgnoringIPv4Format(const IPAddress &inIPAddress) const
  {
    if (!isAddressEqualIgnoringIPv4Format(inIPAddress))
      return false;
    return mPort == inIPAddress.mPort;
  }

  bool IPAddress::isEqualIgnoringIPv4Format(const IPv6PortPair &inPortPair) const
  {
    if (!isAddressEqualIgnoringIPv4Format(inPortPair))
      return false;
    return mPort == inPortPair.mPort;
  }

  bool IPAddress::isAddressEqual(const IPAddress &inIPAddress) const
  {
    size_t size = sizeof(mIPAddress.by);
    return (0 == memcmp(&(inIPAddress.mIPAddress), &mIPAddress, size));
  }

  bool IPAddress::isAddressEqual(const IPv6PortPair &inPortPair) const
  {
    size_t size = sizeof(mIPAddress.by);
    return (0 == memcmp(&(inPortPair.mIPAddress), &mIPAddress, size));
  }

  bool IPAddress::isAddressEqualIgnoringIPv4Format(const IPAddress &inIPAddress) const
  {
    if (isIPv4())
    {
      if (!inIPAddress.isIPv4())
        return false;

      return getIPv4AddressAsDWORD() == inIPAddress.getIPv4AddressAsDWORD();
    }
    return isAddressEqual(inIPAddress);
  }

  bool IPAddress::isAddressEqualIgnoringIPv4Format(const IPv6PortPair &inPortPair) const
  {
    if (isIPv4())
    {
      IPAddress temp(inPortPair);
      if (!temp.isIPv4())
        return false;

      return getIPv4AddressAsDWORD() == temp.getIPv4AddressAsDWORD();
    }
    return isAddressEqual(inPortPair);
  }

  void IPAddress::clear()
  {
    memset(&mIPAddress, 0, sizeof(mIPAddress));
    mPort = 0;
  }

  bool IPAddress::isEmpty() const
  {
    return isAddressEmpty() && isPortEmpty();
  }

  bool IPAddress::isAddressEmpty() const
  {
    return ((0 == mIPAddress.ull[0]) && (0 == mIPAddress.ull[1]));
  }

  bool IPAddress::isPortEmpty() const
  {
    return (0 == mPort);
  }

  // conversion routines between all the IPv4 structures
  void IPAddress::convertIPv4Mapped() throw(IPAddress::Exceptions::NotIPv4)
  {
    (*this) = static_cast<const IPAddress &>(*this).convertIPv4Mapped();
  }

  IPAddress IPAddress::convertIPv4Mapped() const throw(IPAddress::Exceptions::NotIPv4)
  {
    // convert to mapped
    IPAddress temp(getIPv4AddressAsDWORD(), ntohs(mPort));
    return temp;
  }

  void IPAddress::convertIPv4Compatible() throw(IPAddress::Exceptions::NotIPv4)
  {
    (*this) = static_cast<const IPAddress &>(*this).convertIPv4Compatible();
  }

  IPAddress IPAddress::convertIPv4Compatible() const throw(IPAddress::Exceptions::NotIPv4)
  {
    IPAddress temp;
    temp.mIPAddress.dw[3] = htonl(getIPv4AddressAsDWORD());
    temp.mPort = mPort;
    return temp;
  }

  void IPAddress::convertIPv46to4() throw(IPAddress::Exceptions::NotIPv4)
  {
    (*this) = static_cast<const IPAddress &>(*this).convertIPv46to4();
  }

  IPAddress IPAddress::convertIPv46to4() const throw(IPAddress::Exceptions::NotIPv4)
  {
    if (htons(0x2002) == mIPAddress.w[0])
      return *this;  // no need to convert, already an 6to4 address (this preserve all since its a 2002::/16 address)

    DWORD address = getIPv4AddressAsDWORD();
    IPAddress mapped(address, 0);

    IPAddress temp;
    temp.mIPAddress.w[0] = htons(0x2002);
    temp.mIPAddress.by[2] = mapped.mIPAddress.by[12];
    temp.mIPAddress.by[3] = mapped.mIPAddress.by[13];
    temp.mIPAddress.by[4] = mapped.mIPAddress.by[14];
    temp.mIPAddress.by[5] = mapped.mIPAddress.by[15];
    temp.mPort = mPort;
    return temp;
  }

  // IP V6 clear is '::' (all 128 bits zeroed)
  // IP V4 clear is 0000 0000 0000 0000 00000 FFFF 0000 0000 ( '::FFFF:0:0' )
  bool IPAddress::isAddrAny() const
  {
    if (isIPv6())
      return isAddressEqual(anyV6());
    return isAddressEqualIgnoringIPv4Format(anyV4());
  }

  bool IPAddress::isLoopback() const
  {
    if (isAddressEqual(loopbackV6()))
      return true;
    return isAddressEqualIgnoringIPv4Format(loopbackV4());
  }

  bool IPAddress::isIPv4() const
  {
    if (0 == mIPAddress.ull[0])
    {
      if (htonl(0xFFFF) == mIPAddress.dw[2])
        return true;

      // could be mapped or compatible address
      if (0 == mIPAddress.dw[2])
      {
        if (1 == ntohl(mIPAddress.dw[3]))   // this is an exception - loopback in compatible mode is not really an IPv4, use a mapped address instead
          return false;

        return true;
      }

      // this is not IPv4
      return false;
    }
    // 2002::/16
    if (htons(0x2002) == mIPAddress.w[0])
      return true;

    // not IPv4
    return false;
  }

  bool IPAddress::isIPv6() const
  {
    if (isIPv4Compatible())
      return true;   // an IPv4 compatible address is also an IPv6 address - both return true

    return !isIPv4();
  }

  // Unicast IPv6 addresses
  // http://technet.microsoft.com/en-us/library/cc759208%28WS.10%29.aspx
  // Link-local addresses, identified by the FP of 1111 1110 10, are used by
  // nodes when communicating with neighboring nodes on the same link. For
  // example, on a single link IPv6 network with no router, link-local
  // addresses are used to communicate between hosts on the link. Link-local
  // addresses are equivalent to Automatic Private IP Addressing (APIPA) IPv4
  // addresses (using the 169.254.0.0/16 prefix). The scope of a link-local
  // address is the local link. A link-local address is required for Neighbor
  // Discovery processes and is always automatically configured, even in the
  // absence of all other unicast addresses. For more information about the
  // address autoconfiguration process for link-local addresses, see IPv6
  // address autoconfiguration.
  //
  // Link-local addresses always begin with FE80. With the 64-bit interface
  // identifier, the prefix for link-local addresses is always FE80::/64. An
  // IPv6 router never forwards link-local traffic beyond the link.
  bool IPAddress::isLinkLocal() const
  {
    if (isIPv4())
    {
      IPAddress temp;
      temp.mIPAddress.dw[3] = htonl(getIPv4AddressAsDWORD());

      // see http://en.wikipedia.org/wiki/Private_network
      if (169 == (temp.mIPAddress.by[12]))
        return (254 == temp.mIPAddress.by[13]);

      return false;
    }

    return ((0xFE == (mIPAddress.by[0])) &&
            (0x80 == (mIPAddress.by[1] & 0xC0)));
  }

  bool IPAddress::isPrivate() const
  {
    // see http://en.wikipedia.org/wiki/Private_network
    if (isLinkLocal())
      return true;

    if (isIPv4())
    {
      IPAddress temp;
      temp.mIPAddress.dw[3] = htonl(getIPv4AddressAsDWORD());

      switch (temp.mIPAddress.by[12])
      {
        case 10: return true;
        case 172: return (16 == (temp.mIPAddress.by[13] & (0xF0)));
        case 192: return (168 == temp.mIPAddress.by[13]);
          // case 169: return (254 == temp.by[13]); // considered link local
      }
    }
    return (0xFC == (0xFe & mIPAddress.by[0]));
  }

  bool IPAddress::isIPv4Mapped() const
  {
    if (htonl(0xFFFF) != mIPAddress.dw[2])
      return false;

    return (0 == mIPAddress.ull[0]);
  }

  bool IPAddress::isIPv4Compatible() const
  {
    return ((0 == mIPAddress.ull[0]) && (0 == mIPAddress.dw[2]));
  }

  bool IPAddress::isIPv46to4() const
  {
    return (htons(0x2002) == mIPAddress.w[0]);
  }

  DWORD IPAddress::getIPv4AddressAsDWORD() const throw(IPAddress::Exceptions::NotIPv4)
  {
    if ((isIPv4Mapped()) || (isIPv4Compatible()))
      return ntohl(mIPAddress.dw[3]);

    if (isIPv46to4())
      return (((DWORD)ntohs(mIPAddress.w[1])) << (sizeof(u_short)*8)) | ((DWORD)ntohs(mIPAddress.w[2]));

    ZS_THROW_CUSTOM(Exceptions::NotIPv4, ("Attempting to convert IP address to IPv4 when address is an IPv6 address: " + (*this).string()).c_str())
    return 0;
  }

  WORD IPAddress::getPort() const
  {
    return ntohs(mPort);
  }

  void IPAddress::setPort(WORD inPort)
  {
    mPort = htons(inPort);
  }


  void IPAddress::getIPv4(sockaddr_in &outAddress) const throw(IPAddress::Exceptions::NotIPv4)
  {
    memset(&(outAddress), 0, sizeof(outAddress));
#ifndef _LINUX
#ifndef _WIN32
#ifndef _ANDROID
    outAddress.sin_len = sizeof(outAddress);
#endif //_ANDROID
#endif //_WIN32
#endif //_LINUX
    outAddress.sin_family = AF_INET;
#ifdef _WIN32
    if (isLoopback())
      outAddress.sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);         // loopback is a special case since it could be stored as ::1 in IPv6
    else
      outAddress.sin_addr.S_un.S_addr = htonl(getIPv4AddressAsDWORD());
#else
    if (isLoopback())
      outAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);              // loopback is a special case since it could be stored as ::1 in IPv6
    else
      outAddress.sin_addr.s_addr = htonl(getIPv4AddressAsDWORD());
#endif //_WIN32

    outAddress.sin_port = mPort;
  }

  void IPAddress::getIPv6(sockaddr_in6 &outAddress) const
  {
    memset(&(outAddress), 0, sizeof(outAddress));
#ifndef _LINUX
#ifndef _WIN32
#ifndef _ANDROID
    outAddress.sin6_len = sizeof(outAddress);
#endif //_ANDROID
#endif //_WIN32
#endif //_LINUX
    outAddress.sin6_family = AF_INET6;
    memcpy(&(outAddress.sin6_addr), &mIPAddress, sizeof(outAddress.sin6_addr));
    outAddress.sin6_port = mPort;
  }

  String IPAddress::string(bool inIncludePort) const
  {
    WORD port = getPort();
    if (!inIncludePort)
      port = 0;

    if (isIPv4())
    {
      char buffer[sizeof("255.255.255.255:65535")+1];
      const char *format = "%u.%u.%u.%u";
      if (0 != port)
        format = "%u.%u.%u.%u:%u";

      int bytePos = 12;
      if (isIPv46to4())
        bytePos = 2;

      sprintf(
              buffer,
              format,
              (UINT)mIPAddress.by[bytePos],
              (UINT)mIPAddress.by[bytePos+1],
              (UINT)mIPAddress.by[bytePos+2],
              (UINT)mIPAddress.by[bytePos+3],
              (UINT)port
              );

      String temp(buffer);
      return temp;
    }

    return stringAsIPv6(inIncludePort);
  }

  String IPAddress::stringAsIPv6(bool inIncludePort) const
  {
    WORD port = getPort();
    if (!inIncludePort)
      port = 0;

    char buffer[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")+1];
    memset(&(buffer[0]), 0, sizeof(buffer));
    const char *result = internal::inet_ntop6((BYTE *)&(mIPAddress), &(buffer[0]), sizeof(buffer));
    if (result)
    {
      String temp;
      if (0 != port) {
        char bufferWithPort[sizeof("[ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255]:65535")+1];
        sprintf(
                bufferWithPort,
                "[%s]:%u",
                &(buffer[0]),
                (UINT)port
                );
        temp = &(bufferWithPort[0]);
      } else
        temp = &(buffer[0]);
      return temp;
    }
    return String();
  }

  namespace internal
  {

    // taken from: http://www.sfr-fresh.com/unix/misc/dante-1.2.0.tar.gz:a/dante-1.2.0/libscompat/inet_pton.c
    //int
    //inet_pton6(src, dst)
    //  convert presentation level address to network order binary form.
    //return:
    //  1 if `src' is a valid [RFC1884 2.2] address, else 0.
    //notice:
    //  (1) does not touch `dst' unless it's returning 1.
    //  (2) :: in a full address is silently ignored.
    //credit:
    //  inspired by Mark Andrews.
    //author:
    //  Paul Vixie, 1996.
    //
    static int inet_pton6(const char *src, u_char *dst)
    {
      static const char xdigits_l[] = "0123456789abcdef",
      xdigits_u[] = "0123456789ABCDEF";
      u_char tmp[IN6ADDRSZ], *tp, *endp, *colonp;
      const char *xdigits, *curtok;
      int ch, saw_xdigit;
      u_int val;

      memset((tp = tmp), '\0', IN6ADDRSZ);
      endp = tp + IN6ADDRSZ;
      colonp = NULL;
      /* Leading :: requires some special handling. */
      if (*src == ':')
        if (*++src != ':')
          return (0);
      curtok = src;
      saw_xdigit = 0;
      val = 0;
      while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
          pch = strchr((xdigits = xdigits_u), ch);
        if (pch != NULL) {
          val <<= 4;
          val |= (pch - xdigits);
          if (val > 0xffff)
            return (0);
          saw_xdigit = 1;
          continue;
        }
        if (ch == ':') {
          curtok = src;
          if (!saw_xdigit) {
            if (colonp)
              return (0);
            colonp = tp;
            continue;
          }
          if (tp + INT16SZ > endp)
            return (0);
          *tp++ = (u_char) (val >> 8) & 0xff;
          *tp++ = (u_char) val & 0xff;
          saw_xdigit = 0;
          val = 0;
          continue;
        }
        if (ch == '.' && ((tp + INADDRSZ) <= endp) &&
            inet_pton4(curtok, tp) > 0) {
          tp += INADDRSZ;
          saw_xdigit = 0;
          break;   /* '\0' was seen by inet_pton4(). */
        }
        return (0);
      }
      if (saw_xdigit) {
        if (tp + INT16SZ > endp)
          return (0);
        *tp++ = (u_char) (val >> 8) & 0xff;
        *tp++ = (u_char) val & 0xff;
      }
      if (colonp != NULL) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
//#pragma warning(push)
//#pragma warning(disable:4244)
        const int n = tp - colonp;
//#pragma warning(pop)
        int i;

        for (i = 1; i <= n; i++) {
          endp[- i] = colonp[n - i];
          colonp[n - i] = 0;
        }
        tp = endp;
      }
      if (tp != endp)
        return (0);
      memcpy(dst, tmp, IN6ADDRSZ);
      return (1);
    }

    //int
    //inet_pton4(src, dst)
    //  like inet_aton() but without all the hexadecimal and shorthand.
    //return:
    //  1 if `src' is a valid dotted quad, else 0.
    //notice:
    //  does not touch `dst' unless it's returning 1.
    //author:
    //  Paul Vixie, 1996.
    //
    static int inet_pton4(const char *src, u_char *dst)
    {
      static const char digits[] = "0123456789";
      int saw_digit, octets, ch;
      u_char tmp[INADDRSZ], *tp;

      saw_digit = 0;
      octets = 0;
      *(tp = tmp) = 0;
      while ((ch = *src++) != '\0') {
        const char *pch;

        if ((pch = strchr(digits, ch)) != NULL) {
//#pragma warning(push)
//#pragma warning(disable:4244)
          u_int newTemp = *tp * 10 + (pch - digits);
//#pragma warning(pop)

          if (newTemp > 255)
            return (0);
          if (! saw_digit) {
            if (++octets > 4)
              return (0);
            saw_digit = 1;
          }
          *tp = newTemp;
        } else if (ch == '.' && saw_digit) {
          if (octets == 4)
            return (0);
          *++tp = 0;
          saw_digit = 0;
        } else
          return (0);
      }
      if (octets < 4)
        return (0);

      memcpy(dst, tmp, INADDRSZ);
      return (1);
    }

    // http://www.bgnett.no/~giva/watt-doc/a01735.html
    //
    //Copyright (c) 1996 by Internet Software Consortium.
    //
    //Permission to use, copy, modify, and distribute this software for any
    //purpose with or without fee is hereby granted, provided that the above
    //copyright notice and this permission notice appear in all copies.
    //
    //THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
    //ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
    //OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
    //CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
    //DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
    //PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
    //ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
    //SOFTWARE.
    //

    static const char *inet_ntop4 (const u_char *src, char *dst, size_t size)
    {
      char tmp[sizeof("255.255.255.255")+1];

      if ((size_t)sprintf(tmp,"%u.%u.%u.%u",src[0],src[1],src[2],src[3]) > size)
        return (NULL);

      return strcpy(dst, tmp);
    }

    static const char *inet_ntop6(const u_char *src, char *dst, size_t size)
    {
      /*
       * Note that int32_t and int16_t need only be "at least" large enough
       * to contain a value of the specified size.  On some systems, like
       * Crays, there is no such thing as an integer variable with 16 bits.
       * Keep this in mind if you think this function should have been coded
       * to use pointer overlays.  All the world's not a VAX.
       */
      char  tmp[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")+1];
      char *tp;
      struct {
        long base;
        long len;
      } best, cur;
      u_long words [IN6ADDRSZ / INT16SZ];
      int    i;

      /* Preprocess:
       *  Copy the input (bytewise) array into a wordwise array.
       *  Find the longest run of 0x00's in src[] for :: shorthanding.
       */
      memset (words, 0, sizeof(words));
      for (i = 0; i < IN6ADDRSZ; i++)
        words[i/2] |= (src[i] << ((1 - (i % 2)) << 3));

      best.base = -1;
      best.len  = 0;
      cur.base  = -1;
      cur.len   = 0;
      for (i = 0; i < (IN6ADDRSZ / INT16SZ); i++)
      {
        if (words[i] == 0)
        {
          if (cur.base == -1)
            cur.base = i, cur.len = 1;
          else cur.len++;
        }
        else if (cur.base != -1)
        {
          if (best.base == -1 || cur.len > best.len)
            best = cur;
          cur.base = -1;
        }
      }
      if ((cur.base != -1) && (best.base == -1 || cur.len > best.len))
        best = cur;

      if (best.base != -1 && best.len < 2)
        best.base = -1;

      /* Format the result.
       */
      tp = tmp;
      for (i = 0; i < (IN6ADDRSZ / INT16SZ); i++)
      {
        /* Are we inside the best run of 0x00's?
         */
        if (best.base != -1 && i >= best.base && i < (best.base + best.len))
        {
          if (i == best.base)
            *tp++ = ':';
          continue;
        }

        /* Are we following an initial run of 0x00s or any real hex?
         */
        if (i != 0)
          *tp++ = ':';

        /* Is this address an encapsulated IPv4?
         */
        if (i == 6 && best.base == 0 &&
            (best.len == 6 || (best.len == 5 && words[5] == 0xffff)))
        {
          if (!inet_ntop4(src+12, tp, sizeof(tmp) - (tp - tmp)))
            return (NULL);

          tp += strlen (tp);
          break;
        }
        tp += sprintf (tp, "%lX", words[i]);
      }

      /* Was it a trailing run of 0x00's?
       */
      if (best.base != -1 && (best.base + best.len) == (IN6ADDRSZ / INT16SZ))
        *tp++ = ':';
      *tp++ = '\0';

      /* Check for overflow, copy, and we're done.
       */
      if ((size_t)(tp - tmp) > size)
        return (NULL);

      return strcpy (dst, tmp);
    }

  } // namespace internal

} // namespace zsLib

#pragma warning(pop)
