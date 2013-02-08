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

#ifndef ZSLIB_INTERNAL_SOCKETMONITOR_H_c01514fd3a9af7d11f32093baae8c546
#define ZSLIB_INTERNAL_SOCKETMONITOR_H_c01514fd3a9af7d11f32093baae8c546

#include <zsLib/types.h>
#include <zsLib/Socket.h>
#include <zsLib/IPAddress.h>
#include <zsLib/Event.h>

#include <boost/noncopyable.hpp>
#include <map>
#include <set>

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

    class SocketMonitor;
    typedef boost::shared_ptr<SocketMonitor> SocketMonitorPtr;
    typedef boost::weak_ptr<SocketMonitor> SocketMonitorWeakPtr;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SocketSet
    #pragma mark

    class SocketSet
    {
    public:
      SocketSet();
      ~SocketSet();

      ::fd_set *getSet() const;
      u_int &count() const;

      SOCKET getHighestSocket() const;

      void clear();

      void add(SOCKET inSocket);
      void remove(SOCKET inSocket);
      bool isSet(SOCKET inSocket);

    private:
      mutable u_int mCount;
      ::fd_set mSet;

      typedef std::set<SOCKET> MonitoredSocketSet;
      MonitoredSocketSet mMonitoredSockets;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark SocketMonitor
    #pragma mark

    class SocketMonitor : public boost::noncopyable
    {
    public:
      friend class SocketMonitorGlobalSafeReference;

    protected:
      SocketMonitor();

      static SocketMonitorPtr create();

    public:
      ~SocketMonitor();
      static SocketMonitorPtr singleton();

      void monitorBegin(
                        SocketPtr socket,
                        bool monitorRead,
                        bool monitorWrite,
                        bool monitorException
                        );
      void monitorEnd(zsLib::Socket &socket);

      void monitorRead(const zsLib::Socket &socket);
      void monitorWrite(const zsLib::Socket &socket);
      void monitorException(const zsLib::Socket &socket);

      void operator()();

    private:
      void cancel();

      void processWaiting();
      void wakeUp();
      void createWakeUpSocket();

    private:
      RecursiveLock mLock;
      SocketMonitorWeakPtr mThisWeak;
      SocketMonitorPtr mGracefulReference;

      ThreadPtr mThread;
      bool mShouldShutdown;
      typedef std::map<SOCKET, SocketWeakPtr> SocketMap;
      SocketMap mMonitoredSockets;

      typedef std::list<zsLib::EventPtr> EventList;
      EventList mWaitingForRebuildList;

      IPAddress mWakeUpAddress;
      SocketPtr mWakeUpSocket;

      SocketSet mReadSet;
      SocketSet mWriteSet;
      SocketSet mExceptionSet;
    };
  }
}

#endif //ZSLIB_INTERNAL_SOCKETMONITOR_H_c01514fd3a9af7d11f32093baae8c546
