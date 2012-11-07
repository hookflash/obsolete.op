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

#ifndef ZSLIB_EVENT_H_46ff6e637b2164767297dd635614b60e
#define ZSLIB_EVENT_H_46ff6e637b2164767297dd635614b60e

#include <zsLib/internal/Event.h>

namespace zsLib
{
  class Event : public internal::Event
  {
  public:
    static EventPtr create();

    void reset();   // after an event has been notified, reset must be called to cause the wait to happen again
    void wait();    // once an event is notified via "notify()", "wait()" will no longer wait until "reset()" is called
    void notify();  // breaks the wait from executing until the reset is called
  };
}

#endif //ZSLIB_EVENT_H_46ff6e637b2164767297dd635614b60e
