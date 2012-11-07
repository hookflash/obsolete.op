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

#ifndef ZSLIB_MESSAGEQUEUETHREAD_H_573a8d930ea451693ccd53632e481cb0
#define ZSLIB_MESSAGEQUEUETHREAD_H_573a8d930ea451693ccd53632e481cb0

namespace zsLib {

  enum ThreadPriorities
  {
    ThreadPriority_LowPriority,
    ThreadPriority_NormalPriority,
    ThreadPriority_HighPriority,
    ThreadPriority_HighestPriority,
    ThreadPriority_RealtimePriority
  };
} // namespace zsLib

#include <zsLib/internal/MessageQueueThread.h>

namespace zsLib
{
  interaction IMessageQueueThread : public IMessageQueue
  {
    virtual void waitForShutdown() = 0;
  };

  class MessageQueueThread : public IMessageQueueThread
  {
  public:
    static MessageQueueThreadPtr createBasic(const char *threadName = NULL, ThreadPriorities threadPriority = ThreadPriority_NormalPriority);
    static MessageQueueThreadPtr singletonUsingCurrentGUIThreadsMessageQueue();
  };
}

#endif //ZSLIB_MESSAGEQUEUETHREAD_H_573a8d930ea451693ccd53632e481cb0
