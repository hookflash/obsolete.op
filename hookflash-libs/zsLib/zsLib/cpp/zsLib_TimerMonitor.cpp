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

#include <zsLib/Timer.h>
#include <zsLib/Log.h>
#include <zsLib/internal/zsLib_TimerMonitor.h>
#include <boost/thread.hpp>

#include <pthread.h>

#ifdef __QNX__
#include <sys/time.h>
#endif //

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

namespace zsLib
{
  namespace internal
  {
    class TimerMonitorGlobalSafeReferenceInit;

    class TimerMonitorGlobalSafeReference
    {
    public:
      TimerMonitorGlobalSafeReference(TimerMonitorPtr reference)
        : mSafeReference(reference)
      {
      }

      ~TimerMonitorGlobalSafeReference()
      {
        mSafeReference->cancel();
        mSafeReference.reset();
      }

    private:
      TimerMonitorPtr mSafeReference;
    };

    //-------------------------------------------------------------------------
    TimerMonitor::TimerMonitor() :
      mShouldShutdown(false)
    {
#ifdef __QNX__
      static pthread_cond_t defaultCondition = PTHREAD_COND_INITIALIZER;
      static pthread_mutex_t defaultMutex = PTHREAD_MUTEX_INITIALIZER;

      memcpy(&mCondition, &defaultCondition, sizeof(mCondition));
      memcpy(&mMutex, &defaultMutex, sizeof(mMutex));
#endif //__QNX__
      ZS_LOG_DEBUG("created")
    }

    //-------------------------------------------------------------------------
    void TimerMonitor::init()
    {
    }

    //-------------------------------------------------------------------------
    TimerMonitor::~TimerMonitor()
    {
      mThisWeak.reset();
      ZS_LOG_DEBUG("destroyed")
      cancel();

#ifdef __QNX__
      pthread_cond_destroy(&mCondition);
      pthread_mutex_destroy(&mMutex);
#endif //__QNX__
    }

    //-------------------------------------------------------------------------
    TimerMonitorPtr TimerMonitor::singleton()
    {
      static TimerMonitorPtr singleton = TimerMonitor::create();
      static TimerMonitorGlobalSafeReference safe(singleton);
      return singleton;
    }

    //-------------------------------------------------------------------------
    TimerMonitorPtr TimerMonitor::create()
    {
      TimerMonitorPtr pThis(new TimerMonitor);
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }

    //-------------------------------------------------------------------------
    void TimerMonitor::monitorBegin(TimerPtr timer)
    {
      AutoRecursiveLock lock(mLock);

      if (!mThread) {
        mThread = ThreadPtr(new boost::thread(boost::ref(*TimerMonitor::singleton().get())));
      }

      PUID timerID = timer->getID();
      mMonitoredTimers[timerID] = TimerWeakPtr(timer);
      wakeUp();
    }

    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
    void TimerMonitor::operator()()
    {
      bool shouldShutdown = false;

#ifdef __QNX__
      pthread_setname_np(pthread_self(), "com.zslib.timer");
#else
#ifndef _LINUX
#ifndef _ANDROID
      pthread_setname_np("com.zslib.timer");
#endif // _ANDROID
#endif // _LINUX
#endif // __QNX__


      do
      {
        Duration duration;
        // wait completed, do notifications from select
        {
          AutoRecursiveLock lock(mLock);
          shouldShutdown = mShouldShutdown;

          duration = fireTimers();
        }

#ifdef __QNX__
        struct timeval tp;
        struct timespec ts;
        memset(&tp, 0, sizeof(tp));
        memset(&ts, 0, sizeof(ts));

        int rc =  gettimeofday(&tp, NULL);
        ZS_THROW_BAD_STATE_IF(0 != rc)

        // Convert from timeval to timespec
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;

        // add the time to expire from now
        ts.tv_sec += duration.seconds();
        ts.tv_nsec += ((duration - Seconds(duration.total_seconds()))).total_nanoseconds();

        // this could have caused tv_nsec to wrapped above second mark since it started in absolute time since epoch
        if (ts.tv_nsec >= (Seconds(1).total_nanoseconds())) {
          Duration wrapSeconds = Seconds(ts.tv_nsec / (Seconds(1).total_nanoseconds()));
          ts.tv_sec += wrapSeconds.total_seconds();
          ts.tv_nsec -= wrapSeconds.total_nanoseconds();
        }

        rc = pthread_mutex_lock(&mMutex);
        ZS_THROW_BAD_STATE_IF(0 != rc)

        rc = pthread_cond_timedwait(&mCondition, &mMutex, &ts);
        ZS_THROW_BAD_STATE_IF((0 != rc) && (ETIMEDOUT != rc))

        rc = pthread_mutex_unlock(&mMutex);
        ZS_THROW_BAD_STATE_IF(0 != rc)
#else
        boost::unique_lock<boost::mutex> flagLock(mFlagLock);
        mFlagNotify.timed_wait<Duration>(flagLock, duration);
#endif //__QNX__

        // notify all those timers needing to be notified
      } while (!shouldShutdown);

      TimerMonitorPtr gracefulReference;

      {
        AutoRecursiveLock lock(mLock);

        // transfer graceful shutdown reference to thread
        gracefulReference = mGracefulShutdownReference;
        mGracefulShutdownReference.reset();

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

    //-------------------------------------------------------------------------
    void TimerMonitor::cancel()
    {
      ThreadPtr thread;
      {
        AutoRecursiveLock lock(mLock);
        mGracefulShutdownReference = mThisWeak.lock();
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

    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
    void TimerMonitor::wakeUp()
    {
#ifdef __QNX__
      int rc = pthread_mutex_lock(&mMutex);
      ZS_THROW_BAD_STATE_IF(0 != rc)

      rc = pthread_cond_signal(&mCondition);
      ZS_THROW_BAD_STATE_IF(0 != rc)

      rc = pthread_mutex_unlock(&mMutex);
      ZS_THROW_BAD_STATE_IF(0 != rc)

#else
      mFlagNotify.notify_one();
#endif //__QNX__
    }
  }
}
