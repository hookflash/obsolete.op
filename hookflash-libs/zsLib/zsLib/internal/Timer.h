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

#ifndef ZSLIB_INTERNAL_TIMER_H_1d1227118903c8b55faa0906dd0a99f8
#define ZSLIB_INTERNAL_TIMER_H_1d1227118903c8b55faa0906dd0a99f8

#include <zsLib/zsTypes.h>
#include <zsLib/Proxy.h>

namespace zsLib
{
  interaction ITimerDelegate;
  typedef boost::shared_ptr<ITimerDelegate> ITimerDelegatePtr;
  typedef boost::weak_ptr<ITimerDelegate> ITimerDelegateWeakPtr;

  class Timer;
  typedef boost::shared_ptr<Timer> TimerPtr;
  typedef boost::weak_ptr<Timer> TimerWeakPtr;

  namespace internal
  {
    class Timer
    {
      friend class zsLib::Timer;
    public:
      bool tick(const Time &time, Duration &sleepTime);  // returns true if should expire the timer
      PUID getID() const {return mID;}

    protected:
      RecursiveLock mLock;
      PUID mID;
      TimerWeakPtr mThisWeak;
      TimerPtr mThisBackground;
      ITimerDelegatePtr mDelegate;

      UINT mMaxFiringsAtOnce;
      Time mFireNextAt;
      Duration mTimeout;
      bool mOnceOnly;
      bool mMonitored;
    };
  }
}

#endif //ZSLIB_INTERNAL_TIMER_H_1d1227118903c8b55faa0906dd0a99f8
