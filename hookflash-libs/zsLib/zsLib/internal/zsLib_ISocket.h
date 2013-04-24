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

#ifndef ZSLIB_INTERNAL_ISOCKET_H_12367edc74df747886bc9f66aa699b11
#define ZSLIB_INTERNAL_ISOCKET_H_12367edc74df747886bc9f66aa699b11

#include <zsLib/Exception.h>
#include <zsLib/Proxy.h>
#include <boost/noncopyable.hpp>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2def.h>
#include <ws2tcpip.h>
#else //!_WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#ifndef FIONREAD
#include <sys/filio.h>
#endif //ndef FIONREAD

namespace zsLib
{
  enum MissingSocketOptions
  {
    SD_SEND = SHUT_WR,
    SD_RECEIVE = SHUT_RD,
    SD_BOTH = SHUT_RDWR,

    SO_CONDITIONAL_ACCEPT = -1,
    SO_EXCLUSIVEADDRUSE = -1,
    SO_DONTLINGER = -1,
    SO_MAX_MSG_SIZE = -1,
#if (defined _ANDROID || defined __QNX__ || defined _LINUX)
    SO_NOSIGPIPE = -1,
#endif //(defined _ANDROID || defined __QNX__ || defined _LINUX)

    INVALID_SOCKET = -1,
    SOCKET_ERROR = -1,

    WSAEINPROGRESS = EINPROGRESS,
    WSAEWOULDBLOCK = EWOULDBLOCK,
    WSAEADDRINUSE = EADDRINUSE,
    WSAECONNRESET = ECONNRESET,
    WSAECONNREFUSED = ECONNREFUSED,
    WSAENETUNREACH = ENETUNREACH,
    WSAEHOSTUNREACH = EHOSTUNREACH,
    WSAETIMEDOUT = ETIMEDOUT,
    WSAESHUTDOWN = ESHUTDOWN,
    WSAECONNABORTED = ECONNABORTED,
    WSAENETRESET = ENETRESET,
    WSAEMSGSIZE = EMSGSIZE
  };
}

#endif //_WIN32

namespace zsLib
{
  enum WindowsSocketOptions
  {
    SO_WINDOWS_CONDITIONAL_ACCEPT = SO_CONDITIONAL_ACCEPT,
    SO_WINDOWS_EXCLUSIVEADDRUSE = SO_EXCLUSIVEADDRUSE,
    SO_WINDOWS_DONTLINGER = SO_DONTLINGER,
    SO_WINDOWS_MAX_MSG_SIZE = SO_MAX_MSG_SIZE
  };
}

#endif //ZSLIB_INTERNAL_ISOCKET_H_12367edc74df747886bc9f66aa699b11
