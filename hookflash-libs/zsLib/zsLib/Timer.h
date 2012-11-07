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

#ifndef ZSLIB_TIMER_H_625827a73b7747cea09db99f967d6a64
#define ZSLIB_TIMER_H_625827a73b7747cea09db99f967d6a64

#include <zsLib/internal/Timer.h>

#define ZSLIB_MAX_TIMER_FIRED_AT_ONCE   5

namespace zsLib
{
  typedef Proxy<ITimerDelegate> ITimerDelegateProxy;

  class Timer : public internal::Timer
  {
  private:
    Timer(ITimerDelegatePtr delegate, Duration timeout, bool repeat, UINT maxFiringTimerAtOnce);
  public:
    //-------------------------------------------------------------------------
    // PURPOSE: Create a timer which will fire once or at a repeat interval
    //          until cancelled.
    // NOTES:   The timer is serviced internally by a clock. It's possible
    //          that the clock is put to sleep and wakes up later only to
    //          discover that the timer never fired for many times when it
    //          should have. For example, a computer might be put to sleep
    //          and then the clock is wokeup only to find it is hours later
    //          and missed many events. To fire off hundreds of events at once
    //          is not desirable behaviour so a limit is put which is specified
    //          with "maxFiringTimerAtOnce" to prevent timer wakeup floods.
    static TimerPtr create(
                           ITimerDelegatePtr delegate,
                           Duration timeout,
                           bool repeat = true,
                           UINT maxFiringTimerAtOnce = ZSLIB_MAX_TIMER_FIRED_AT_ONCE
                           );

    ~Timer();

    PUID getID() const {return mID;}
    void cancel();      // cancel a timer (it is no longer needed)

    void background(bool background = true);  // background the timer (will run until timer is cancelled even if reference to object is forgotten)
  };

  interaction ITimerDelegate
  {
    virtual void onTimer(TimerPtr timer) = 0;
  };
}

ZS_DECLARE_PROXY_BEGIN(zsLib::ITimerDelegate)
ZS_DECLARE_PROXY_METHOD_1(onTimer, zsLib::TimerPtr)
ZS_DECLARE_PROXY_END()

#endif //ZSLIB_TIMER_H_625827a73b7747cea09db99f967d6a64
