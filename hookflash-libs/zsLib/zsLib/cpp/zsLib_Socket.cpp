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
#include <zsLib/Stringize.h>
#include <zsLib/internal/zsLib_SocketMonitor.h>

#include <fcntl.h>
#include <signal.h>

#pragma warning(push)
#pragma warning(disable:4290)

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

namespace zsLib
{
#ifndef _WIN32
  typedef linger LINGER;

  int WSAGetLastError()
  {
    return errno;
  }

  int closesocket(SOCKET socket)
  {
    return close(socket);
  }
#else
  typedef int socklen_t;
#endif //_WIN32

  namespace internal
  {
    static boost::thread_specific_ptr<bool> threadLocalData;

    //-------------------------------------------------------------------------
    static void ignoreSigTermOnThread()
    {
      if (!threadLocalData.get()) {
        // threadLocalData.reset(new bool);

        struct sigaction act;
        memset(&act, 0, sizeof(act));

        act.sa_handler=SIG_IGN;
        sigemptyset(&act.sa_mask);
        act.sa_flags=0;
        sigaction(SIGPIPE, &act, NULL);
      }
    }

#ifdef _WIN32
    class SocketInit
    {
    public:
      //-----------------------------------------------------------------------
      SocketInit()
      {
        WSADATA data;
        memset(&data, 0, sizeof(data));
        int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (0 != result)
			ZS_THROW_CUSTOM_PROPERTIES_1(Socket::Exceptions::Unspecified, result, String("WSAStartup failed with error code: ") + Stringize<int>(result).string())
      }

      //-----------------------------------------------------------------------
      ~SocketInit()
      {
        int result = WSACleanup();
        if (0 != result)
			ZS_THROW_CUSTOM_PROPERTIES_1(Socket::Exceptions::Unspecified, result, String("WSACleanup failed with error code: ") + Stringize<int>(result).string())
      }
    };
#else
    class SocketInit
    {
    public:
      SocketInit() {}
      ~SocketInit() {}
    };
#endif //_WIN32

    //-------------------------------------------------------------------------
    static void socketInit()
    {
      static SocketInit gInit;
    }

    //-------------------------------------------------------------------------
    static void prepareRawIPAddress(const IPAddress &inAddress, sockaddr_in &inIPv4, sockaddr_in6 &inIPv6, sockaddr * &outAddress, int &outSize)
    {
      outAddress = NULL;
      outSize = 0;

      if (inAddress.isIPv6())
      {
        memset(&inIPv6, 0, sizeof(inIPv6));
        inAddress.getIPv6(inIPv6);
        outAddress = (sockaddr *)&inIPv6;
        outSize = sizeof(inIPv6);
      }
      else
      {
        memset(&inIPv4, 0, sizeof(inIPv4));
        inAddress.getIPv4(inIPv4);
        outAddress = (sockaddr *)&inIPv4;
        outSize = sizeof(inIPv4);
      }
    }

    //-------------------------------------------------------------------------
    void Socket::notifyReadReady()
    {
      ISocketPtr socket;
      ISocketDelegatePtr delegate;
      {
        AutoRecursiveLock lock(mLock);
        delegate = mDelegate;
        socket = mThis.lock();
      }
      if (!delegate)
        return;

      delegate->onReadReady(socket);
    }

    //-------------------------------------------------------------------------
    void Socket::notifyWriteReady()
    {
      ISocketPtr socket;
      ISocketDelegatePtr delegate;
      {
        AutoRecursiveLock lock(mLock);
        delegate = mDelegate;
        socket = mThis.lock();
      }
      if (!delegate)
        return;

      delegate->onWriteReady(socket);
    }

    //-------------------------------------------------------------------------
    void Socket::notifyException()
    {
      ISocketPtr socket;
      ISocketDelegatePtr delegate;
      {
        AutoRecursiveLock lock(mLock);
        delegate = mDelegate;
        socket = mThis.lock();
      }
      if (!delegate)
        return;

      delegate->onException(socket);
    }

    //-------------------------------------------------------------------------
    void Socket::resetSocketMonitorGlobalSafeReference()
    {
      mSocketMonitorGlobalSafeReference.reset();
    }
  }

  //---------------------------------------------------------------------------
  void Socket::ignoreSIGPIPEOnThisThread()
  {
    internal::ignoreSigTermOnThread();
  }

  //---------------------------------------------------------------------------
  SocketPtr Socket::create() throw(Exceptions::Unspecified)
  {
    SocketPtr object(new Socket);
    object->mThis = object;
    return object;
  }

  //---------------------------------------------------------------------------
  SocketPtr Socket::createUDP(Create::Family inFamily) throw(Exceptions::Unspecified)
  {
    return create(inFamily, Create::Datagram, Create::UDP);
  }

  //---------------------------------------------------------------------------
  SocketPtr Socket::createTCP(Create::Family inFamily) throw(Exceptions::Unspecified)
  {
    return create(inFamily, Create::Stream, Create::TCP);
  }

  //---------------------------------------------------------------------------
  SocketPtr Socket::create(Create::Family inFamily, Create::Type inType, Create::Protocol inProtocol) throw(Exceptions::Unspecified)
  {
    internal::socketInit();
    SOCKET socket = ::socket(inFamily, inType, inProtocol);
    if (INVALID_SOCKET == socket)
    {
      int error = WSAGetLastError();
      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "Could not create socket due to an unexpected error, where error=" + (Stringize<int>(error)).string())
    }
    SocketPtr object = create();
    object->mThis = object;
    object->adopt(socket);
    return object;
  }

  //---------------------------------------------------------------------------
  Socket::Socket() throw(Exceptions::Unspecified) : mSocket(INVALID_SOCKET)
  {
    internal::socketInit();
    internal::ignoreSigTermOnThread();
  }

  //---------------------------------------------------------------------------
  Socket::~Socket() throw(
                          Exceptions::WouldBlock,
                          Exceptions::Unspecified
                          )
  {
    close();
    mSocketMonitorGlobalSafeReference.reset();
  }

  //---------------------------------------------------------------------------
  bool Socket::isValid() const
  {
    AutoRecursiveLock lock(mLock);
    return INVALID_SOCKET != mSocket;
  }

  //---------------------------------------------------------------------------
  SOCKET Socket::getSocket() const
  {
    AutoRecursiveLock lock(mLock);
    return mSocket;
  }

  //---------------------------------------------------------------------------
  SOCKET Socket::orphan()
  {
    setDelegate();    // clear out the delegate

    AutoRecursiveLock lock(mLock);
    SOCKET temp = mSocket;
    mSocket = INVALID_SOCKET;
    return temp;
  }

  //---------------------------------------------------------------------------
  void Socket::adopt(SOCKET inSocket)
  {
    close();

    AutoRecursiveLock lock(mLock);
    mSocket = inSocket;
  }

  //---------------------------------------------------------------------------
  void Socket::setDelegate(ISocketDelegatePtr delegate) throw (Socket::Exceptions::InvalidSocket)
  {
    delegate = ISocketDelegateProxy::createWeak(delegate);

    if (delegate) {
      // only supported if using a proxy mechanism
      ZS_THROW_INVALID_USAGE_IF(!ISocketDelegateProxy::isProxy(delegate))
    }

    bool remove = false;
    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();

    {
      AutoRecursiveLock lock(mLock);
      if (mDelegate)
        remove = true;
      mDelegate.reset();
    }

    if (remove)
      monitor->monitorEnd(*this);

    bool monitorRead = true;
    bool monitorWrite = true;
    bool monitorException = true;

    {
      AutoRecursiveLock lock(mLock);
      if (INVALID_SOCKET == mSocket)
        return;

      mDelegate = delegate;
      monitorRead = mMonitorReadReady;
      monitorWrite = mMonitorWriteReady;
      monitorException = mMonitorException;
    }

    if (delegate)
      monitor->monitorBegin(mThis.lock(), monitorRead, monitorWrite, monitorException);
  }

  //---------------------------------------------------------------------------
  void Socket::monitor(Monitor::Options options)
  {
    ISocketDelegatePtr delegate;
    bool monitorRead = true;
    bool monitorWrite = true;
    bool monitorException = true;

    {
      AutoRecursiveLock lock(mLock);

      bool oldMonitorRead = mMonitorReadReady;
      bool oldMonitorWrite = mMonitorWriteReady;
      bool oldMonitorException = mMonitorException;

      mMonitorReadReady = (0 != (options & Monitor::Read));
      mMonitorWriteReady = (0 != (options & Monitor::Write));
      mMonitorException = (0 != (options & Monitor::Exception));

      if ((oldMonitorRead == mMonitorReadReady) &&
          (oldMonitorWrite == mMonitorWriteReady) &&
          (oldMonitorException == mMonitorException)) {
        return;
      }

      monitorRead = mMonitorReadReady;
      monitorWrite = mMonitorWriteReady;
      monitorException = mMonitorException;
      delegate = mDelegate;

      if (!mDelegate) return;
    }

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();

    monitor->monitorEnd(*this);
    monitor->monitorBegin(mThis.lock(), monitorRead, monitorWrite, monitorException);
  }

  //---------------------------------------------------------------------------
  void Socket::close() throw(Exceptions::WouldBlock, Exceptions::Unspecified)
  {
    internal::ignoreSigTermOnThread();

    setDelegate();

    AutoRecursiveLock lock(mLock);
    if (INVALID_SOCKET == mSocket)
      return;

    int result = closesocket(mSocket);
    mSocket = INVALID_SOCKET;
    if (SOCKET_ERROR == result)
    {
      int error = WSAGetLastError();
      switch (error)
      {
        case WSAEWOULDBLOCK: ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::WouldBlock, error, "closesocket on an asynchronous socket would block, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
        default:             ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "closesocket returned an unexpected error, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string())
      }
    }
  }

  //---------------------------------------------------------------------------
  IPAddress Socket::getLocalAddress() const throw (Exceptions::InvalidSocket, Exceptions::Unspecified)
  {
    internal::ignoreSigTermOnThread();

    AutoRecursiveLock lock(mLock);
    ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

    sockaddr_in6 address;
    memset(&address, 0, sizeof(address));
    socklen_t size = sizeof(address);
    int result = getsockname(
                             mSocket,
                             (sockaddr *)&address,
                             &size
                             );
    if (SOCKET_ERROR == result)
    {
      int error = WSAGetLastError();
      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "getsockname returned an unexpected error, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string())
    }

    IPAddress localIP;
    switch (size)
    {
      case sizeof(sockaddr_in):  localIP = IPAddress((sockaddr_in &)address); break;
      case sizeof(sockaddr_in6):
      default:                   localIP = IPAddress(address); break;
    }

    return localIP;
  }

  //---------------------------------------------------------------------------
  IPAddress Socket::getRemoteAddress() const throw (Exceptions::InvalidSocket, Exceptions::Unspecified)
  {
    internal::ignoreSigTermOnThread();

    AutoRecursiveLock lock(mLock);
    ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

    sockaddr_in6 address;
    memset(&address, 0, sizeof(address));
    socklen_t size = sizeof(address);
    int result = getpeername(
                             mSocket,
                             (sockaddr *)&address,
                             &size
                             );
    if (SOCKET_ERROR == result)
    {
      int error = WSAGetLastError();
      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "getpeername returned an unexpected error, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string())
    }

    IPAddress localIP;
    switch (size)
    {
      case sizeof(sockaddr_in):  localIP = IPAddress((sockaddr_in &)address); break;
      case sizeof(sockaddr_in6):
      default:                   localIP = IPAddress(address); break;
    }

    return localIP;
  }


  //---------------------------------------------------------------------------
  void Socket::bind(
                    const IPAddress &inBindIP,
                    int *noThrowErrorResult
                    ) const throw(
                                  Exceptions::InvalidSocket,
                                  Exceptions::AddressInUse,
                                  Exceptions::Unspecified
                                  )
  {
    internal::ignoreSigTermOnThread();

    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      sockaddr_in addressv4;
      sockaddr_in6 addressv6;
      sockaddr *address = NULL;
      int size = 0;
      internal::prepareRawIPAddress(inBindIP, addressv4, addressv6, address, size);

      int result = ::bind(mSocket, address, size);
#ifdef __APPLE__
#define TODO_INVESTIGATE_WHY_THIS_FAILED_WITH_IPV6_ON_MAC 1
#define TODO_INVESTIGATE_WHY_THIS_FAILED_WITH_IPV6_ON_MAC 2
#endif //__APPLE__

      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        if (noThrowErrorResult) {
          *noThrowErrorResult = error;
          return;
        }
        switch (error)
        {
          case WSAEADDRINUSE: ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::AddressInUse, error, "Cannot bind socket as address is already in use, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", bind IP=" + inBindIP.string()); break;
          default:            ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "Unexpected error binding a socket, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", bind ip=" + inBindIP.string() + ", error=" + (Stringize<int>(error)).string()); break;
        }
      }
    }

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorException)
      monitor->monitorException(*this);
  }

  //---------------------------------------------------------------------------
  void Socket::listen() const throw(
                                    Exceptions::InvalidSocket,
                                    Exceptions::AddressInUse,
                                    Exceptions::Unspecified
                                   )
  {
    internal::ignoreSigTermOnThread();

    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      int result = ::listen(mSocket, SOMAXCONN);
      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        switch (error)
        {
          case WSAEADDRINUSE: ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::AddressInUse, error, "Cannot listen on socket as address is already in use, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          default:            ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "Unexpected error listening on a socket, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string()); break;
        }
      }
    }

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorReadReady)
      monitor->monitorRead(*this);
    if (mMonitorWriteReady)
      monitor->monitorWrite(*this);
    if (mMonitorException)
      monitor->monitorException(*this);
  }

  //---------------------------------------------------------------------------
  ISocketPtr Socket::accept(
                            IPAddress &outRemoteIP,
                            int *noThrowErrorResult
                            ) const throw(
                                          Exceptions::InvalidSocket,
                                          Exceptions::ConnectionReset,
                                          Exceptions::Unspecified
                                          )
  {
    internal::ignoreSigTermOnThread();

    SOCKET acceptSocket = INVALID_SOCKET;
    if (noThrowErrorResult)
      *noThrowErrorResult = 0;

    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      sockaddr_in6 address;
      memset(&address, 0, sizeof(address));
      address.sin6_family = AF_INET6;
      socklen_t size = sizeof(address);
      acceptSocket = ::accept(mSocket, (sockaddr *)(&address), &size);
      if (INVALID_SOCKET == acceptSocket)
      {
        int error = WSAGetLastError();
        if (noThrowErrorResult) {
          *noThrowErrorResult = error;
          return ISocketPtr();
        }
        switch (error)
        {
          case WSAECONNRESET: ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionReset, error, "New connection was indicated but connection was reset before it could be accepted, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          default:            ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "Unexpected error accepting new connection, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string()); break;
        }
      }

      switch (size)
      {
        case sizeof(sockaddr_in):  outRemoteIP = IPAddress((sockaddr_in &)address); break;
        case sizeof(sockaddr_in6):
        default:                   outRemoteIP = IPAddress(address); break;
      }
    }

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorReadReady)
      monitor->monitorRead(*this);
    if (mMonitorWriteReady)
      monitor->monitorWrite(*this);
    if (mMonitorException)
      monitor->monitorException(*this);

    SocketPtr result = create();
    result->adopt(acceptSocket);
    return result;
  }

  //---------------------------------------------------------------------------
  void Socket::connect(
                       const IPAddress &inDestination,
                       bool *outWouldBlock,
                       int *noThrowErrorResult
                       ) const throw(
                                     Exceptions::InvalidSocket,
                                     Exceptions::WouldBlock,
                                     Exceptions::AddressInUse,
                                     Exceptions::NetworkNotReachable,
                                     Exceptions::HostNotReachable,
                                     Exceptions::Timeout,
                                     Exceptions::Unspecified
                                     )
  {
    internal::ignoreSigTermOnThread();

    {
      if (noThrowErrorResult)
        *noThrowErrorResult = 0;

      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      if (NULL != outWouldBlock)
        *outWouldBlock = false;

      sockaddr_in addressv4;
      sockaddr_in6 addressv6;
      sockaddr *address = NULL;
      int size = 0;
      internal::prepareRawIPAddress(inDestination, addressv4, addressv6, address, size);

      int result = ::connect(mSocket, address, size);
      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        if (noThrowErrorResult) {
          if (WSAEWOULDBLOCK == error) {
            if (outWouldBlock) {
              *outWouldBlock = true;
              goto connect_final;
            }
          }
          *noThrowErrorResult = error;
          return;
        }
        switch (error)
        {
          case WSAEINPROGRESS:
          case WSAEWOULDBLOCK:
          {
            if (NULL != outWouldBlock)
              *outWouldBlock = true;
            else
              ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::WouldBlock, error, "The connection will block, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", destination ip=" + inDestination.string());
            break;
          }
          case WSAEADDRINUSE:     ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::AddressInUse, error, "Cannot connect socket as address is already in use, where socket id" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", destination ip=" + inDestination.string()); break;
          case WSAECONNREFUSED:   ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionRefused, error, "Cannot connect socket as connection was refused, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", destination ip=" + inDestination.string()); break;
          case WSAENETUNREACH:    ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::NetworkNotReachable, error, "Cannot connect socket as network was not able to reach destination, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", destination ip=" + inDestination.string()); break;
          case WSAEHOSTUNREACH:   ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::HostNotReachable, error, "Cannot connect socket as host was not reachable, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", destination ip=" + inDestination.string()); break;
          case WSAETIMEDOUT:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot connect socket as connection timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", destination ip=" + inDestination.string()); break;
          default:                ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "Unexpected error connecting to destination, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", destination ip=" + inDestination.string() + ", error=" + (Stringize<int>(error)).string()); break;
        }
      }
    }

  connect_final:

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorReadReady)
      monitor->monitorRead(*this);
    if (mMonitorWriteReady)
      monitor->monitorWrite(*this);
    if (mMonitorException)
      monitor->monitorException(*this);
  }

  //---------------------------------------------------------------------------
  ULONG Socket::receive(
                        BYTE *ioBuffer,
                        ULONG inBufferLengthInBytes,
                        bool *outWouldBlock,
                        ULONG inFlags,
                        int *noThrowErrorResult
                        ) const throw(
                                      Exceptions::InvalidSocket,
                                      Exceptions::WouldBlock,
                                      Exceptions::Shutdown,
                                      Exceptions::ConnectionReset,
                                      Exceptions::ConnectionAborted,
                                      Exceptions::Timeout,
                                      Exceptions::BufferTooSmall,
                                      Exceptions::Unspecified
                                      )
  {
    internal::ignoreSigTermOnThread();

    int result = 0;
    if (noThrowErrorResult)
      *noThrowErrorResult = 0;

    // scope:
    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      if (NULL != outWouldBlock)
        *outWouldBlock = false;
      if (0 == inBufferLengthInBytes)
        return 0;

      ZS_THROW_INVALID_ARGUMENT_IF(NULL == ioBuffer)

      int flags = 0;

      result = ::recv(
                      mSocket,
                      (char *)ioBuffer,
                      inBufferLengthInBytes,
                      flags
                      );
      if (SOCKET_ERROR == result)
      {
        result = 0;

        int error = WSAGetLastError();
        if (noThrowErrorResult) {
          if (WSAEWOULDBLOCK == error) {
            if (outWouldBlock)
              *outWouldBlock = true;
          }
          *noThrowErrorResult = error;
          goto receive_final;
        }
        switch (error)
        {
          case WSAEWOULDBLOCK:
          {
            if (NULL != outWouldBlock)
              *outWouldBlock = true;
            else
              ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::WouldBlock, error, "Cannot receive socket data as socket would block, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string());
            break;
          }
          case WSAESHUTDOWN:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Shutdown, error, "Cannot receive socket data as connection was shutdown, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAECONNRESET:     ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionReset, error, "Cannot receive socket data as socket was abruptly closed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAECONNABORTED:   ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionAborted, error, "Cannot receive socket data as socket connection was aborted, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAENETRESET:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot receive socket data as connection keep alive timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAETIMEDOUT:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot receive socket data as socket timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAEMSGSIZE:       ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::BufferTooSmall, error, "Cannot receive socket data as buffer provided was too small, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          default:                ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "The receive unexpectedly closed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string()); break;
        }
      }
    }
  receive_final:

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorReadReady)
      monitor->monitorRead(*this);

    return (ULONG)result;
  }

  //---------------------------------------------------------------------------
  ULONG Socket::receiveFrom(
                            IPAddress &outRemoteIP,
                            BYTE *ioBuffer,
                            ULONG inBufferLengthInBytes,
                            bool *outWouldBlock,
                            ULONG inFlags,
                            int *noThrowErrorResult
                            ) const throw(
                                          Exceptions::InvalidSocket,
                                          Exceptions::WouldBlock,
                                          Exceptions::Shutdown,
                                          Exceptions::ConnectionReset,
                                          Exceptions::Timeout,
                                          Exceptions::BufferTooSmall,
                                          Exceptions::Unspecified
                                          )
  {
    internal::ignoreSigTermOnThread();

    int result = 0;
    if (noThrowErrorResult)
      *noThrowErrorResult = 0;

    // scope:
    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      if (NULL != outWouldBlock)
        *outWouldBlock = false;

      sockaddr_in6 address;
      memset(&address, 0, sizeof(address));
      address.sin6_family = AF_INET6;
      socklen_t size = sizeof(address);
      result = recvfrom(
                            mSocket,
                            (char *)ioBuffer,
                            inBufferLengthInBytes,
                            inFlags,
                            (sockaddr *)&address,
                            &size
                            );

      if (SOCKET_ERROR == result)
      {
        result = 0;

        int error = WSAGetLastError();
        if (noThrowErrorResult) {
          if (WSAEWOULDBLOCK == error) {
            if (outWouldBlock)
              *outWouldBlock = true;
          }
          *noThrowErrorResult = error;
          goto recvfrom_final;
        }
        switch (error)
        {
          case WSAEWOULDBLOCK:
          {
            if (NULL != outWouldBlock)
              *outWouldBlock = true;
            else
              ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::WouldBlock, error, "Cannot receive socket data as socket would block, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string());
            break;
          }
          case WSAESHUTDOWN:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Shutdown, error, "Cannot receive socket data as connection was shutdown, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAECONNRESET:     ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionReset, error, "Cannot receive socket data as socket was abruptly closed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAENETRESET:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot receive socket data as connection keep alive timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAETIMEDOUT:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot receive socket data as socket timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAEMSGSIZE:       ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::BufferTooSmall, error, "Cannot receive socket data as buffer provided was too small, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          default:                ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "The receive unexpectedly closed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string()); break;
        }
      } else {
        switch (size)
        {
          case sizeof(sockaddr_in):  outRemoteIP = IPAddress((sockaddr_in &)address); break;
          case sizeof(sockaddr_in6):
          default:                   outRemoteIP = IPAddress(address); break;
        }
      }
    }
  recvfrom_final:

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorReadReady)
      monitor->monitorRead(*this);

    return (ULONG)result;
  }

  //---------------------------------------------------------------------------
  ULONG Socket::send(
                     const BYTE *inBuffer,
                     ULONG inBufferLengthInBytes,
                     bool *outWouldBlock,
                     ULONG inFlags,
                     int *noThrowErrorResult
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
                                   )
  {
    internal::ignoreSigTermOnThread();

    int result = 0;
    if (noThrowErrorResult)
      *noThrowErrorResult = 0;

    // scope:
    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      if (NULL != outWouldBlock)
        *outWouldBlock = false;

      result = ::send(
                      mSocket,
                      (const char *)inBuffer,
                      inBufferLengthInBytes,
                      inFlags
                      );

      if (SOCKET_ERROR == result)
      {
        result = 0;

        int error = WSAGetLastError();
        if (noThrowErrorResult) {
          if (WSAEWOULDBLOCK == error) {
            if (outWouldBlock)
              *outWouldBlock = true;
          }
          *noThrowErrorResult = error;
          goto send_final;
        }
        switch (error)
        {
          case WSAEWOULDBLOCK:
          {
            if (NULL != outWouldBlock) {
              *outWouldBlock = true;
            } else {
              ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::WouldBlock, error, "Cannot send socket data as socket would block, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string());
            }
            break;
          }
          case WSAENETRESET:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot send socket data as connection keep alive timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAESHUTDOWN:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Shutdown, error, "Cannot send socket data as connection was shutdown, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAEHOSTUNREACH:   ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::HostNotReachable, error, "Cannot send socket data as host is unreachable at this time, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAECONNABORTED:   ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionAborted, error, "Cannot send socket data as socket connection was aborted, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAECONNRESET:     ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionReset, error, "Cannot send socket data as socket was abruptly closed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAETIMEDOUT:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot send socket data as socket timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAEMSGSIZE:       ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::BufferTooSmall, error, "Cannot send socket data as buffer provided was too big, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          default:                ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "The send unexpectedly closed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string()); break;
        }
      }
    }
  send_final:

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorWriteReady)
      monitor->monitorWrite(*this);

    return (ULONG)result;
  }

  //---------------------------------------------------------------------------
  ULONG Socket::sendTo(
                       const IPAddress &inDestination,
                       const BYTE *inBuffer,
                       ULONG inBufferLengthInBytes,
                       bool *outWouldBlock,
                       ULONG inFlags,
                       int *noThrowErrorResult
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
                                     )
  {
    internal::ignoreSigTermOnThread();

    int result = 0;
    if (noThrowErrorResult)
      *noThrowErrorResult = 0;

    // scope:
    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      if (NULL != outWouldBlock)
        *outWouldBlock = false;

      sockaddr_in addressv4;
      sockaddr_in6 addressv6;
      sockaddr *address = NULL;
      int size = 0;
      internal::prepareRawIPAddress(inDestination, addressv4, addressv6, address, size);

      result = ::sendto(
                        mSocket,
                        (const char *)inBuffer,
                        inBufferLengthInBytes,
                        inFlags,
                        address,
                        size
                        );
      if (SOCKET_ERROR == result)
      {
        result = 0;

        int error = WSAGetLastError();
        if (noThrowErrorResult) {
          if (WSAEWOULDBLOCK == error) {
            if (outWouldBlock)
              *outWouldBlock = true;
          }
          *noThrowErrorResult = error;
          goto sendto_final;
        }
        switch (error)
        {
          case WSAEWOULDBLOCK:
          {
            if (NULL != outWouldBlock)
              *outWouldBlock = true;
            else
              ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::WouldBlock, error, "Cannot send socket data as socket would block, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string());
            break;
          }
          case WSAENETRESET:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot send socket data as connection keep alive timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAESHUTDOWN:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Shutdown, error, "Cannot send socket data as connection was shutdown, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAEHOSTUNREACH:   ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::HostNotReachable, error, "Cannot send socket data as host is unreachable at this time, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAECONNABORTED:   ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionAborted, error, "Cannot send socket data as socket connection was aborted, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAECONNRESET:     ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::ConnectionReset, error, "Cannot send socket data as socket was abruptly closed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAETIMEDOUT:      ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Timeout, error, "Cannot send socket data as socket timed out, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          case WSAEMSGSIZE:       ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::BufferTooSmall, error, "Cannot send socket data as buffer provided was too big, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string()); break;
          default:                ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "The send unexpectedly closed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string()); break;
        }
      }
    }

  sendto_final:

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorWriteReady)
      monitor->monitorWrite(*this);

    return (ULONG)result;
  }

  //---------------------------------------------------------------------------
  void Socket::shutdown(Shutdown::Options inOptions) const throw(Exceptions::InvalidSocket, Exceptions::Unspecified)
  {
    internal::ignoreSigTermOnThread();

    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())

      int result = ::shutdown(mSocket, inOptions);
      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        ZS_THROW_CUSTOM_PROPERTIES_1(Exceptions::Unspecified, error, "Failed to perform a shutdown on the socket, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string())
      }
    }
    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorReadReady)
      monitor->monitorRead(*this);
    if (mMonitorWriteReady)
      monitor->monitorWrite(*this);
    if (mMonitorException)
      monitor->monitorException(*this);
  }

  namespace internal
  {
    //-------------------------------------------------------------------------
    static void setSocketOptions(
                                 SOCKET inSocket,
                                 int inLevel,
                                 int inOptionName,
                                 BYTE *inOptionValue,
                                 ULONG inOptionLength
                                 )
    {
      int result = ::setsockopt(inSocket, inLevel, inOptionName, (const char *)inOptionValue, inOptionLength);
      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        ZS_THROW_CUSTOM_PROPERTIES_1(zsLib::Socket::Exceptions::Unspecified, error, "setsockopt failed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)inSocket)).string() + ", socket option=" + (Stringize<int>(inOptionName)).string() + ", error=" + (Stringize<int>(error)).string())
      }
    }

    //-------------------------------------------------------------------------
    static void getSocketOptions(
                                 SOCKET inSocket,
                                 int inLevel,
                                 int inOptionName,
                                 BYTE *outOptionValue,
                                 ULONG inOptionLength
                                 )
    {
      socklen_t length = inOptionLength;
      int result = ::getsockopt(inSocket, inLevel, inOptionName, (char *)outOptionValue, &length);
      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        ZS_THROW_CUSTOM_PROPERTIES_1(zsLib::Socket::Exceptions::Unspecified, error, "getsockopt failed, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)inSocket)).string() + ", socket option=" + (Stringize<int>(inOptionName)).string() + ", error=" + (Stringize<int>(error)).string())
      }
      ZS_THROW_BAD_STATE_IF(length != inOptionLength)
    }

  } // namespace internal

  //---------------------------------------------------------------------------
  void Socket::setOptionFlag(Socket::SetOptionFlag::Options inOption, bool inEnabled) const throw(
                                                                                                  Exceptions::InvalidSocket,
                                                                                                  Exceptions::UnsupportedSocketOption,
                                                                                                  Exceptions::Unspecified
                                                                                                  )
  {
    internal::ignoreSigTermOnThread();

    AutoRecursiveLock lock(mLock);
    ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())
    ZS_THROW_CUSTOM_PROPERTIES_1_IF(Socket::Exceptions::UnsupportedSocketOption, -1 == inOption, ENOSYS)

    if (SetOptionFlag::NonBlocking == inOption)
    {
#ifdef _WIN32
      u_long value = inEnabled ? 1 : 0;
      int result = ioctlsocket(mSocket, inOption, &value);
#else
      int mode = inEnabled ? 1 : 0;
      int result = ioctl(mSocket, FIONBIO, &mode);
      result = fcntl(mSocket, F_GETFL, 0);
      if (SOCKET_ERROR != result) {
        if (inEnabled) {
          if (0 == (O_NONBLOCK & result)) {
            result = fcntl(mSocket, F_SETFL, O_NONBLOCK | result);   // turn on the flag with bitwise ORing the nonblocking
          }
        } else {
          if (0 != (O_NONBLOCK & result)) {
            result = fcntl(mSocket, F_SETFL, (result ^ O_NONBLOCK)); // turn off the flag with xor since it was on
          }
        }
      }
#endif //WIN32
      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        ZS_THROW_CUSTOM_PROPERTIES_1(Socket::Exceptions::Unspecified, error, "Failed to set the socket to non-blocking, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string())
      }
      return;
    }

    int level = SOL_SOCKET;
    if (SetOptionFlag::TCPNoDelay == inOption)
      level = IPPROTO_TCP;

#ifdef _WIN32
    BOOL value = (inEnabled ? 1 : 0);
#else
    int value = (inEnabled ? 1 : 0);
#endif //_WIN32
    internal::setSocketOptions(mSocket, level, inOption, (BYTE *)&value, sizeof(value));
  }

  //---------------------------------------------------------------------------
  void Socket::setOptionValue(Socket::SetOptionValue::Options inOption, ULONG inValue) const throw(
                                                                                                   Exceptions::InvalidSocket,
                                                                                                   Exceptions::UnsupportedSocketOption,
                                                                                                   Exceptions::Unspecified
                                                                                                   )
  {
    internal::ignoreSigTermOnThread();

    AutoRecursiveLock lock(mLock);
    ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())
    ZS_THROW_CUSTOM_PROPERTIES_1_IF(Socket::Exceptions::UnsupportedSocketOption, -1 == inOption, ENOSYS)

    if (SetOptionValue::LingerTimeInSeconds == inOption)
    {
      LINGER linger;
      memset(&linger, 0, sizeof(linger));
      linger.l_onoff = (inValue != 0 ? 1 : 0);
      linger.l_linger = (inValue != 0 ? (u_short)inValue : 0);
      internal::setSocketOptions(mSocket, SOL_SOCKET, SO_LINGER, (BYTE *)&linger, sizeof(linger));
      return;
    }

    int value = inValue;
    internal::setSocketOptions(mSocket, SOL_SOCKET, inOption, (BYTE *)&value, sizeof(value));
  }

  //---------------------------------------------------------------------------
  bool Socket::getOptionFlag(GetOptionFlag::Options inOption) const throw(
                                                                          Exceptions::InvalidSocket,
                                                                          Exceptions::UnsupportedSocketOption,
                                                                          Exceptions::Unspecified
                                                                          )
  {
    internal::ignoreSigTermOnThread();

    AutoRecursiveLock lock(mLock);
    ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())
    ZS_THROW_CUSTOM_PROPERTIES_1_IF(Socket::Exceptions::UnsupportedSocketOption, -1 == inOption, ENOSYS)

    if (GetOptionFlag::IsOOBAllRead == inOption)
    {
#ifdef _WIN32
      u_long value = 0;
      int result = ioctlsocket(mSocket, inOption, &value);
#else
      int value = 0;
      int result = ioctl(mSocket, inOption, &value);
#endif //_WIN32
      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        ZS_THROW_CUSTOM_PROPERTIES_1(Socket::Exceptions::Unspecified, error, "Failed to determine if OOB data was available or not, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string())
      }
      return 0 != value;
    }

    int level = SOL_SOCKET;
    if (GetOptionFlag::IsTCPNoDelay == inOption)
      level = IPPROTO_TCP;

#ifdef _WIN32
    BOOL value = 0;
#else
    int value = 0;
#endif //_WIN32
    internal::getSocketOptions(mSocket, level, inOption, (BYTE *)&value, sizeof(value));
    return 0 != value;
  }

  //---------------------------------------------------------------------------
  ULONG Socket::getOptionValue(GetOptionValue::Options inOption) const throw(
                                                                             Exceptions::InvalidSocket,
                                                                             Exceptions::UnsupportedSocketOption,
                                                                             Exceptions::Unspecified
                                                                             )
  {
    internal::ignoreSigTermOnThread();

    AutoRecursiveLock lock(mLock);
    ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())
    ZS_THROW_CUSTOM_PROPERTIES_1_IF(Socket::Exceptions::UnsupportedSocketOption, -1 == inOption, ENOSYS)

    if (GetOptionValue::ReadyToReadSizeInBytes == inOption)
    {
#ifdef _WIN32
      u_long value = 0;
      int result = ioctlsocket(mSocket, inOption, &value);
#else
      int value = 0;
      int result = ioctl(mSocket, inOption, &value);
#endif //_WIN32
      if (SOCKET_ERROR == result)
      {
        int error = WSAGetLastError();
        ZS_THROW_CUSTOM_PROPERTIES_1(Socket::Exceptions::Unspecified, error, "Failed to determine amount of data ready to read, where socket id=" + (Stringize<PTRNUMBER>((PTRNUMBER)mSocket)).string() + ", error=" + (Stringize<int>(error)).string())
      }
      return value;
    }

    int level = SOL_SOCKET;
    int value_int = 0;
    unsigned int value_unsigned_int = 0;

    BYTE *pvalue = (BYTE *)&value_int;
    int size = sizeof(value_int);
    switch (inOption)
    {
      case GetOptionValue::MaxMessageSizeInBytes:  pvalue = ((BYTE *)&value_unsigned_int); size = sizeof(value_unsigned_int); break;
      default:                                     break;
    }

    internal::getSocketOptions(mSocket, level, inOption, pvalue, size);

    switch (inOption)
    {
      case GetOptionValue::MaxMessageSizeInBytes:  return value_unsigned_int;
      default:                                     break;
    }
    return value_int;
  }

  //---------------------------------------------------------------------------
  void Socket::onReadReadyReset() const throw(Exceptions::DelegateNotSet, Exceptions::InvalidSocket, Exceptions::Unspecified)
  {
    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())
      ZS_THROW_CUSTOM_IF(Exceptions::DelegateNotSet, !mDelegate)
    }

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorReadReady)
      monitor->monitorRead(*this);
  }

  //---------------------------------------------------------------------------
  void Socket::onWriteReadyReset() const throw(Exceptions::DelegateNotSet, Exceptions::InvalidSocket, Exceptions::Unspecified)
  {
    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())
      ZS_THROW_CUSTOM_IF(Exceptions::DelegateNotSet, !mDelegate)
    }

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorWriteReady)
      monitor->monitorWrite(*this);
  }

  //---------------------------------------------------------------------------
  void Socket::onExceptionReset() const throw(Exceptions::DelegateNotSet, Exceptions::InvalidSocket, Exceptions::Unspecified)
  {
    {
      AutoRecursiveLock lock(mLock);
      ZS_THROW_CUSTOM_IF(Exceptions::InvalidSocket, !isValid())
      ZS_THROW_CUSTOM_IF(Exceptions::DelegateNotSet, !mDelegate)
    }

    internal::SocketMonitorPtr monitor = internal::SocketMonitor::singleton();
    if (mMonitorException)
      monitor->monitorException(*this);
  }

} // namespace zsLib

#pragma warning(pop)
