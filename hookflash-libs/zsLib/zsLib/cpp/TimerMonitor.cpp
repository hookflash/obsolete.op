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

#include <zsLib/Timer.h>
#include <zsLib/Log.h>
#include <zsLib/internal/TimerMonitor.h>
#include <boost/thread.hpp>

#include <pthread.h>

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

namespace zsLib
{
  namespace internal
  {
    class TimerMonitorGlobalInit;

    class TimerMonitorGlobal
    {
    public:
      TimerMonitorGlobal(TimerMonitorGlobalInit &) : mTimerMonitor(new TimerMonitor) { }
      ~TimerMonitorGlobal() { mTimerMonitor->waitForShutdown(); }

      TimerMonitorPtr &get() {return mTimerMonitor;}

    private:
      TimerMonitorPtr mTimerMonitor;
    };

    static TimerMonitorPtr getTimerMonitor();

    class TimerMonitorGlobalInit
    {
    public:
      TimerMonitorGlobalInit() {getTimerMonitor();}
      ~TimerMonitorGlobalInit() {}
    };

    static TimerMonitorGlobalInit gTimerMonitorGlobalInit;        // this symbol isn't used but forces the object to be created

    static TimerMonitorPtr getTimerMonitor()
    {
      static TimerMonitorGlobal global(gTimerMonitorGlobalInit);  // forces a reference to the static global variable
      return global.get();
    }

    TimerMonitorPtr TimerMonitor::singleton()
    {
      return getTimerMonitor();
    }

    void TimerMonitor::monitorBegin(TimerPtr timer)
    {
      AutoRecursiveLock lock(mLock);

      if (!mThread) {
        mThread = ThreadPtr(new boost::thread(boost::ref(*getTimerMonitor().get())));
      }

      PUID timerID = timer->getID();
      mMonitoredTimers[timerID] = TimerWeakPtr(timer);
      wakeUp();
    }

    void TimerMonitor::monitorEnd(Timer &timer)
    {
      AutoRecursiveLock lock(mLock);

      PUID timerID = timer.getID();

      TimerMap::iterator result = mMonitoredTimers.find(timerID);   // if the timer was scheduled to be monitored, then need to remove it from being monitored
      if (mMonitoredTimers.end() == result)
        return;

      mMonitoredTimers.erase(result);
      wakeUp();
    }

    void TimerMonitor::operator()()
    {
      bool shouldShutdown = false;

#ifndef _LINUX
      pthread_setname_np("com.zslib.timer");
#endif

      do
      {
        Duration duration;
        // wait completed, do notifications from select
        {
          AutoRecursiveLock lock(mLock);
          shouldShutdown = mShouldShutdown;

          duration = fireTimers();
        }

        boost::unique_lock<boost::mutex> flagLock(mFlagLock);
        mFlagNotify.timed_wait<Duration>(flagLock, duration);

        // notify all those timers needing to be notified
      } while (!shouldShutdown);

      {
        AutoRecursiveLock lock(mLock);

        // go through timers and cancel them completely
        for (TimerMap::iterator monIter = mMonitoredTimers.begin(); monIter != mMonitoredTimers.end(); ) {
          TimerMap::iterator current = monIter;
          ++monIter;

          TimerPtr timer = current->second.lock();
          if (timer)
            timer->background(false);
        }
        mMonitoredTimers.clear();
      }
    }

    void TimerMonitor::waitForShutdown()
    {
      ThreadPtr thread;
      {
        AutoRecursiveLock lock(mLock);
        thread = mThread;

        mShouldShutdown = true;
        wakeUp();
      }

      if (!thread)
        return;

      thread->join();

      {
        AutoRecursiveLock lock(mLock);
        mThread.reset();
      }
    }

    Duration TimerMonitor::fireTimers()
    {
      AutoRecursiveLock lock(mLock);

      Time time = boost::posix_time::microsec_clock::universal_time();

      Duration duration = boost::posix_time::seconds(1);

      for (TimerMap::iterator monIter = mMonitoredTimers.begin(); monIter != mMonitoredTimers.end(); )
      {
        TimerMap::iterator current = monIter;
        ++monIter;

        TimerPtr timer = (current->second).lock();
        bool done = true;

        if (timer)
          done = timer->tick(time, duration);

        if (done)
          mMonitoredTimers.erase(current);
      }
      return duration;
    }

    void TimerMonitor::wakeUp()
    {
      mFlagNotify.notify_one();
    }
  }
}
