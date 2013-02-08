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

#include <zsLib/MessageQueueThread.h>
#include <zsLib/internal/zsLib_MessageQueueThreadBasic.h>
#include <zsLib/internal/zsLib_MessageQueueThreadUsingCurrentGUIMessageQueueForWindows.h>
#include <zsLib/internal/zsLib_MessageQueueThreadUsingMainThreadMessageQueueForApple.h>
#include <zsLib/Log.h>

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  MessageQueueThreadPtr MessageQueueThread::createBasic(const char *threadName, ThreadPriorities threadPriority)
  {
    return internal::MessageQueueThreadBasic::create(threadName);
  }

  MessageQueueThreadPtr MessageQueueThread::singletonUsingCurrentGUIThreadsMessageQueue()
  {
#ifdef _WIN32
    return internal::MessageQueueThreadUsingCurrentGUIMessageQueueForWindows::singleton();
#else
#if __APPLE__
      return internal::MessageQueueThreadUsingMainThreadMessageQueueForApple::singleton();
#else
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : warning: "
#pragma message(__LOC__"Need to implement this method on current target platform...")

    return internal::MessageQueueThreadBasic::create();
#endif //_APPLE
#endif //_WIN32
  }
}
