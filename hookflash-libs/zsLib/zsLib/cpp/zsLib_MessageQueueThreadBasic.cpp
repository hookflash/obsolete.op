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

#include <zsLib/internal/zsLib_MessageQueueThreadBasic.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>

#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <pthread.h>

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  namespace internal
  {
    MessageQueueThreadBasicPtr MessageQueueThreadBasic::create(const char *threadName, ThreadPriorities threadPriority)
    {
      MessageQueueThreadBasicPtr thread(new MessageQueueThreadBasic(threadName));
      thread->mQueue = zsLib::MessageQueue::create(thread);
      thread->mThread = ThreadPtr(new boost::thread(boost::ref(*thread.get())));
#ifndef _WIN32
      const int policy = SCHED_RR;
      const int minPrio = sched_get_priority_min(policy);
      const int maxPrio = sched_get_priority_max(policy);
      sched_param param;
      switch (threadPriority)
      {
        case ThreadPriority_LowPriority:
          param.sched_priority = minPrio + 1;
          break;
        case ThreadPriority_NormalPriority:
          param.sched_priority = (minPrio + maxPrio) / 2;
          break;
        case ThreadPriority_HighPriority:
          param.sched_priority = maxPrio - 3;
          break;
        case ThreadPriority_HighestPriority:
          param.sched_priority = maxPrio - 2;
          break;
        case ThreadPriority_RealtimePriority:
          param.sched_priority = maxPrio - 1;
          break;
      }
      boost::thread::native_handle_type threadHandle = thread->mThread->native_handle();
      pthread_setschedparam(threadHandle, policy, &param);
#endif //_WIN32
      return thread;
    }

    MessageQueueThreadBasic::MessageQueueThreadBasic(const char *threadName) :
      mThreadName(threadName ? threadName : ""),
      mMustShutdown(0)
    {
    }

    void MessageQueueThreadBasic::operator()()
    {
      bool shouldShutdown = false;

      do
      {
        MessageQueuePtr queue;

        {
          AutoLock lock(mLock);
#ifndef _LINUX
#ifndef _ANDROID
          if (!mThreadName.isEmpty()) {
            pthread_setname_np(mThreadName);
          }
#endif // _ANDROID
#endif // _LINUX

          queue = mQueue;
          if (!mQueue)
            return;
        }

        queue->process(); // process all pending data now
        mEvent.reset();   // should be safe to reset the notification now that we are done processing

        queue->process(); // small window between the process and the reset where more events could have arrived so process those now
        shouldShutdown = (0 != zsLib::atomicGetValue32(mMustShutdown));

        if (!shouldShutdown) {
          mEvent.wait();    // wait for the next event to arrive
          queue->process(); // process data in case shutdown gets activated
        }

        shouldShutdown = (0 != zsLib::atomicGetValue32(mMustShutdown));
      } while(!shouldShutdown);

      {
        AutoLock lock(mLock);
        mQueue.reset();
      }
    }

    void MessageQueueThreadBasic::post(IMessageQueueMessagePtr message)
    {
      MessageQueuePtr queue;
      {
        AutoLock lock(mLock);
        queue = mQueue;
        if (!queue) {
          ZS_THROW_CUSTOM(Exceptions::MessageQueueAlreadyDeleted, "message posted to message queue after message queue was deleted.")
        }
      }
      queue->post(message);
    }

    UINT MessageQueueThreadBasic::getTotalUnprocessedMessages() const
    {
      AutoLock lock(mLock);
      if (!mQueue)
        return 0;

      return mQueue->getTotalUnprocessedMessages();
    }

    void MessageQueueThreadBasic::notifyMessagePosted()
    {
      mEvent.notify();
    }

    void MessageQueueThreadBasic::waitForShutdown()
    {
      ThreadPtr thread;
      {
        AutoLock lock(mLock);
        thread = mThread;

        zsLib::atomicSetValue32(mMustShutdown, 1);
        mEvent.notify();
      }

      if (!thread)
        return;

      thread->join();

      {
        AutoLock lock(mLock);
        mThread.reset();
        mQueue.reset();
      }
    }
  }
}
