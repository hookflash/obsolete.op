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
#include <zsLib/Exception.h>
#include <zsLib/internal/zsLib_TimerMonitor.h>
#include <zsLib/helpers.h>

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}


namespace zsLib
{
  namespace internal
  {
    bool Timer::tick(const Time &time, Duration &sleepTime)
    {
      AutoRecursiveLock lock(mLock);
      bool fired = false;
      UINT totalFires = 0;

      boost::posix_time::time_iterator iterator(mFireNextAt, mTimeout);
      while (iterator < time)
      {
        fired = true;
        try {
          mDelegate->onTimer(mThisWeak.lock());
        } catch (ITimerDelegateProxy::Exceptions::DelegateGone &) {
          mOnceOnly = true;   // this has to stop firing now that the proxy to the delegate points to something that is now gone
          break;
        }
        ++iterator;
        ++totalFires;

        if (mOnceOnly)  // do not allow the timer to fire more than once
          break;

        if (totalFires >= mMaxFiringsAtOnce) {
          // Do not want the timer to wake up from a sleep only to discover
          // that it has missed hundreds or thousands of events and flood
          // the delegate with missed timers. Thus, if the total number of
          // triggered timers fired at once is greater than a reasonable
          // maximum then the timer firings must be stopped to prevent a timer
          // event flood.
          mFireNextAt = time + mTimeout;  // the next timeout resets to the clock plus the timeout
          break;
        }
        mFireNextAt = *iterator;
      }

      if (!mOnceOnly) {
        Duration diff = mFireNextAt - time;
        if (diff < sleepTime)
          sleepTime = diff;
      }

      if (!fired)
        return false;

      if (mOnceOnly) {
        mDelegate.reset();
        mThisBackground.reset();
      }

      return mOnceOnly;
    }
  }

  Timer::Timer(
               ITimerDelegatePtr delegate,
               Duration timeout,
               bool repeat,
               UINT maxFiringsAtOnce
               )
  {
    mDelegate = ITimerDelegateProxy::createWeak(delegate);
    ZS_THROW_INVALID_USAGE_IF(!mDelegate)
    ZS_THROW_INVALID_USAGE_IF(!Proxy<ITimerDelegate>::isProxy(mDelegate)) // NOTE: the delegate passed in is not associated with a message queue

    mOnceOnly = !repeat;
    mMaxFiringsAtOnce = maxFiringsAtOnce;
    mTimeout = timeout;
    mID = createPUID();
    mMonitored = false;
    mFireNextAt = (boost::posix_time::microsec_clock::universal_time() + timeout);
  }

  Timer::~Timer()
  {
    mThisWeak.reset();
    cancel();
  }

  TimerPtr Timer::create(
                         ITimerDelegatePtr delegate,
                         Duration timeout,
                         bool repeat,
                         UINT maxFiringTimerAtOnce
                         )
  {
    TimerPtr timer = TimerPtr(new Timer(delegate, timeout, repeat, maxFiringTimerAtOnce));
    timer->mThisWeak = timer;

    internal::TimerMonitorPtr monitor = internal::TimerMonitor::singleton();
    monitor->monitorBegin(timer);
    timer->mMonitored = true;
    return timer;
  }

  void Timer::cancel()
  {
    {
      AutoRecursiveLock lock(mLock);
      mThisBackground.reset();
      if (!mMonitored)
        return;
    }

    internal::TimerMonitorPtr monitor = internal::TimerMonitor::singleton();
    monitor->monitorEnd(*this);

    {
      AutoRecursiveLock lock(mLock);
      mThisBackground.reset();
      mMonitored = false;
    }
  }

  void Timer::background(bool background)
  {
    AutoRecursiveLock lock(mLock);

    if (!background)
      mThisBackground.reset();
    else
      mThisBackground = mThisWeak.lock();
  }
}
