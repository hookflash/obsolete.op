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

#ifndef ZSLIB_INTERNAL_SOCKETMONITOR_H_c01514fd3a9af7d11f32093baae8c546
#define ZSLIB_INTERNAL_SOCKETMONITOR_H_c01514fd3a9af7d11f32093baae8c546

#include <zsLib/zsTypes.h>

#include <boost/noncopyable.hpp>
#include <map>
#include <list>


namespace zsLib
{
  class Timer;
  typedef boost::shared_ptr<Timer> TimerPtr;
  typedef boost::weak_ptr<Timer> TimerWeakPtr;

  namespace internal
  {
    class TimerMonitorGlobal;

    class TimerMonitor;
    typedef boost::shared_ptr<TimerMonitor> TimerMonitorPtr;
    typedef boost::weak_ptr<TimerMonitor> TimerMonitorWeakPtr;

    class TimerMonitor : public boost::noncopyable
    {
      friend class TimerMonitorGlobal;
    protected:
      TimerMonitor() : mShouldShutdown(false) {}

    public:
      static TimerMonitorPtr singleton();

      void monitorBegin(TimerPtr timer);
      void monitorEnd(Timer &timer);

      void operator()();
      void waitForShutdown();

    private:
      Duration fireTimers();
      void wakeUp();

    private:
      RecursiveLock mLock;
      Lock mFlagLock;
      boost::condition_variable mFlagNotify;

      ThreadPtr mThread;
      bool mShouldShutdown;

      typedef std::map<PUID, TimerWeakPtr> TimerMap;

      TimerMap mMonitoredTimers;
    };
  }
}

#endif //ZSLIB_INTERNAL_SOCKETMONITOR_H_c01514fd3a9af7d11f32093baae8c546
