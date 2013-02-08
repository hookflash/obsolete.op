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

#pragma once

#ifndef ZSLIB_SOCKET_H_e70b2dd35334d2e569aea2d714e24c6a
#define ZSLIB_SOCKET_H_e70b2dd35334d2e569aea2d714e24c6a

#include <zsLib/internal/zsLib_Socket.h>

#pragma warning(push)
#pragma warning(disable: 4290)

namespace zsLib
{
  class Socket : public internal::Socket
  {
  public:
    struct Create {
      enum Family {
        IPv4 =      AF_INET,
        IPv6 =      AF_INET6
      };
      enum Type {
        Stream =    SOCK_STREAM,
        Datagram =  SOCK_DGRAM,
        Raw =       SOCK_RAW
      };
      enum Protocol {
        None = 0,
        TCP =       IPPROTO_TCP,
        UDP =       IPPROTO_UDP
      };
    };

  public:
    static void ignoreSIGPIPEOnThisThread();

    static SocketPtr create() throw(Exceptions::Unspecified);
    static SocketPtr createUDP(Create::Family inFamily = Create::IPv4) throw(Exceptions::Unspecified);
    static SocketPtr createTCP(Create::Family inFamily = Create::IPv4) throw(Exceptions::Unspecified);
    static SocketPtr create(Create::Family inFamily, Create::Type inType, Create::Protocol inProtocol) throw(Exceptions::Unspecified);

    ~Socket() throw(
                    Exceptions::WouldBlock,
                    Exceptions::Unspecified
                    );

    SOCKET getSocket() const;
    SOCKET orphan();                                                      // orphaning a socket will automatically remove any socket monitor
    void adopt(SOCKET inSocket);

    virtual bool isValid() const;

    // socket must be valid in order to monitor the socket or an exception will be thrown
    virtual void setDelegate(ISocketDelegatePtr delegate = ISocketDelegatePtr()) throw (Exceptions::InvalidSocket);
    virtual void monitor(Monitor::Options options = Monitor::All);

    virtual void close() throw(Exceptions::WouldBlock, Exceptions::Unspecified);  // closing a socket will automaticlaly remove any socket monitor

    virtual IPAddress getLocalAddress() const throw (Exceptions::InvalidSocket, Exceptions::Unspecified);
    virtual IPAddress getRemoteAddress() const throw (Exceptions::InvalidSocket, Exceptions::Unspecified);

    void bind(
              const IPAddress &inBindIP,
              int *noThrowErrorResult = NULL
              ) const throw(
                            Exceptions::InvalidSocket,
                            Exceptions::AddressInUse,
                            Exceptions::Unspecified
                            );

    virtual void listen() const throw(
                                      Exceptions::InvalidSocket,
                                      Exceptions::AddressInUse,
                                      Exceptions::Unspecified
                                      );

    virtual ISocketPtr accept(
                              IPAddress &outRemoteIP,
                              int *noThrowErrorResult = NULL
                              ) const throw(
                                            Exceptions::InvalidSocket,
                                            Exceptions::ConnectionReset,
                                            Exceptions::Unspecified
                                            );

    virtual void connect(
                         const IPAddress &inDestination,     // destination of the connection
                         bool *outWouldBlock = NULL,         // if this param is used, will return the "would block" as a result rather than throwing an exception
                         int *noThrowErrorResult = NULL
                         ) const throw(
                                       Exceptions::InvalidSocket,
                                       Exceptions::WouldBlock,
                                       Exceptions::AddressInUse,
                                       Exceptions::NetworkNotReachable,
                                       Exceptions::HostNotReachable,
                                       Exceptions::Timeout,
                                       Exceptions::Unspecified
                                       );

    virtual ULONG receive(
                          BYTE *ioBuffer,
                          ULONG inBufferLengthInBytes,
                          bool *outWouldBlock = NULL,         // if this param is used, will return the "would block" as a result rather than throwing an exception
                          ULONG flags = (ULONG)(Receive::None),
                          int *noThrowErrorResult = NULL
                          ) const throw(
                                        Exceptions::InvalidSocket,
                                        Exceptions::WouldBlock,
                                        Exceptions::Shutdown,
                                        Exceptions::ConnectionReset,
                                        Exceptions::ConnectionAborted,
                                        Exceptions::Timeout,
                                        Exceptions::BufferTooSmall,
                                        Exceptions::Unspecified
                                        );

    virtual ULONG receiveFrom(
                              IPAddress &outRemoteIP,
                              BYTE *ioBuffer,
                              ULONG inBufferLengthInBytes,
                              bool *outWouldBlock = NULL,         // if this param is used, will return the "would block" as a result rather than throwing an exception
                              ULONG flags = (ULONG)(Receive::None),
                              int *noThrowErrorResult = NULL
                              ) const throw(
                                            Exceptions::InvalidSocket,
                                            Exceptions::WouldBlock,
                                            Exceptions::Shutdown,
                                            Exceptions::ConnectionReset,
                                            Exceptions::Timeout,
                                            Exceptions::BufferTooSmall,
                                            Exceptions::Unspecified
                                            );

    virtual ULONG send(
                       const BYTE *inBuffer,
                       ULONG inBufferLengthInBytes,
                       bool *outWouldBlock = NULL,         // if this param is used, will return the "would block" as a result rather than throwing an exception
                       ULONG flags = (ULONG)(Send::None),
                       int *noThrowErrorResult = NULL
                       ) const throw(
                                     Exceptions::InvalidSocket,
                                     Exceptions::WouldBlock,
                                     Exceptions::Shutdown,
                                     Exceptions::HostNotReachable,
                                     Exceptions::ConnectionAborted,
                                     Exceptions::ConnectionReset,
                                     Exceptions::Timeout,
                                     Exceptions::BufferTooSmall,
                                     Exceptions::Unspecified
                                     );

    virtual ULONG sendTo(
                         const IPAddress &inDestination,
                         const BYTE *inBuffer,
                         ULONG inBufferLengthInBytes,
                         bool *outWouldBlock = NULL,         // if this param is used, will return the "would block" as a result rather than throwing an exception
                         ULONG flags = (ULONG)(Send::None),
                         int *noThrowErrorResult = NULL
                         ) const throw(
                                       Exceptions::InvalidSocket,
                                       Exceptions::WouldBlock,
                                       Exceptions::Shutdown,
                                       Exceptions::Timeout,
                                       Exceptions::HostNotReachable,
                                       Exceptions::ConnectionAborted,
                                       Exceptions::ConnectionReset,
                                       Exceptions::Timeout,
                                       Exceptions::BufferTooSmall,
                                       Exceptions::Unspecified
                                       );

    virtual void shutdown(Shutdown::Options inOptions = Shutdown::Both) const throw(Exceptions::InvalidSocket, Exceptions::Unspecified);

    virtual void setOptionFlag(SetOptionFlag::Options inOption, bool inEnabled) const throw(Exceptions::InvalidSocket, Exceptions::UnsupportedSocketOption, Exceptions::Unspecified);
    virtual void setOptionValue(SetOptionValue::Options inOption, ULONG inValue) const throw(Exceptions::InvalidSocket, Exceptions::UnsupportedSocketOption, Exceptions::Unspecified);

    virtual bool getOptionFlag(GetOptionFlag::Options inOption) const throw(Exceptions::InvalidSocket, Exceptions::UnsupportedSocketOption, Exceptions::Unspecified);
    virtual ULONG getOptionValue(GetOptionValue::Options inOption) const throw(Exceptions::InvalidSocket, Exceptions::UnsupportedSocketOption, Exceptions::Unspecified);

    virtual void onReadReadyReset() const throw(Exceptions::DelegateNotSet, Exceptions::InvalidSocket, Exceptions::Unspecified);
    virtual void onWriteReadyReset() const throw(Exceptions::DelegateNotSet, Exceptions::InvalidSocket, Exceptions::Unspecified);
    virtual void onExceptionReset() const throw(Exceptions::DelegateNotSet, Exceptions::InvalidSocket, Exceptions::Unspecified);

  private:
    Socket() throw(Exceptions::Unspecified);

    SOCKET mSocket;
  };
}

#pragma warning(pop)

#endif //ZSLIB_SOCKET_H_e70b2dd35334d2e569aea2d714e24c6a
