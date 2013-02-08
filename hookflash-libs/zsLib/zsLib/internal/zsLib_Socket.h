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

#ifndef ZSLIB_INTERNAL_SOCKET_H_6f504f01bdd331d0c835ccae3872ce91
#define ZSLIB_INTERNAL_SOCKET_H_6f504f01bdd331d0c835ccae3872ce91

#include <zsLib/ISocket.h>
#include <boost/noncopyable.hpp>

namespace zsLib
{
  class Socket;
  typedef boost::shared_ptr<Socket> SocketPtr;
  typedef boost::weak_ptr<Socket> SocketWeakPtr;

  namespace internal
  {
    class SocketMonitorGlobalSafeReference;
    typedef boost::shared_ptr<SocketMonitorGlobalSafeReference> SocketMonitorGlobalSafeReferencePtr;
    typedef boost::weak_ptr<SocketMonitorGlobalSafeReference> SocketMonitorGlobalSafeReferenceWeakPtr;

    class Socket : public ISocket,
                   public boost::noncopyable
    {
    protected:
      Socket() :
        mMonitorReadReady(true),
        mMonitorWriteReady(true),
        mMonitorException(true)
      {}

    public:
      ~Socket() {}

      void notifyReadReady();
      void notifyWriteReady();
      void notifyException();
      void resetSocketMonitorGlobalSafeReference();

    protected:
      ISocketDelegatePtr mDelegate;
      mutable RecursiveLock mLock;
      SocketWeakPtr mThis;
      SocketMonitorGlobalSafeReferencePtr mSocketMonitorGlobalSafeReference;

      bool mMonitorReadReady;
      bool mMonitorWriteReady;
      bool mMonitorException;
    };
  }
}

#ifndef _WIN32
typedef int SOCKET;
#endif //ndef _WIN32

#endif //ZSLIB_INTERNAL_SOCKET_H_6f504f01bdd331d0c835ccae3872ce91
