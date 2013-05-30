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
  namespace internal
  {
    Event::Event() :
      mNotified(0)
    {
#ifdef __QNX__
      static pthread_cond_t gConditionInit = PTHREAD_COND_INITIALIZER;
      static pthread_mutex_t gMutexInit = PTHREAD_MUTEX_INITIALIZER;

      mCondition = gConditionInit;
      mMutex = gMutexInit;
#endif //__QNX__
    }

    Event::~Event()
    {
#ifdef __QNX__
      pthread_cond_destroy(&mCondition);
      pthread_mutex_destroy(&mMutex);
#endif //__QNX__
    }

  }

  EventPtr Event::create() {
    return EventPtr(new Event);
  }

  void Event::reset()
  {
    zsLib::atomicSetValue32(mNotified, 0);
  }

  void Event::wait()
  {
    DWORD notified = zsLib::atomicGetValue32(mNotified);
    if (0 != notified) {
      return;
    }

#ifdef __QNX__
    int result = pthread_mutex_lock(&mMutex);
    ZS_THROW_BAD_STATE_IF(0 != result)

    notified = zsLib::atomicGetValue32(mNotified);
    if (0 != notified) {
      result = pthread_mutex_unlock(&mMutex);
      ZS_THROW_BAD_STATE_IF(0 != result)
      return;
    }

    result = pthread_cond_wait(&mCondition, &mMutex);
    ZS_THROW_BAD_STATE_IF(0 != result)

    result = pthread_mutex_unlock(&mMutex);
    ZS_THROW_BAD_STATE_IF(0 != result)
#else
    boost::unique_lock<boost::mutex> lock(mMutex);
    notified = zsLib::atomicGetValue32(mNotified);
    if (0 != notified) return;
    mCondition.wait(lock);
#endif //__QNX__
  }

  void Event::notify()
  {
#ifdef __QNX__
    int result = pthread_mutex_lock(&mMutex);
    ZS_THROW_BAD_STATE_IF(0 != result)

    result = pthread_cond_signal(&mCondition);
    ZS_THROW_BAD_STATE_IF(0 != result)

    zsLib::atomicSetValue32(mNotified, 1);

    result = pthread_mutex_unlock(&mMutex);
    ZS_THROW_BAD_STATE_IF(0 != result)
#else
    boost::lock_guard<boost::mutex> lock(mMutex);
    zsLib::atomicSetValue32(mNotified, 1);
    mCondition.notify_one();
#endif //__QNX__
  }

}
