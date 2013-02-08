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

#include <zsLib/Event.h>
#include <zsLib/helpers.h>
#include <zsLib/Exception.h>

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

namespace zsLib
{
  EventPtr Event::create() {
    return EventPtr(new Event);
  }

  void Event::reset() {
    zsLib::atomicSetValue32(mNotified, 0);
  }

  void Event::wait() {
    DWORD notified = zsLib::atomicGetValue32(mNotified);
    if (0 != notified)
      return;

    boost::unique_lock<boost::mutex> lock(mMutex);
    mCondition.wait(lock);
  }

  void Event::notify() {
    boost::lock_guard<boost::mutex> lock(mMutex);
    zsLib::atomicSetValue32(mNotified, 1);
    mCondition.notify_one();
  }
}
