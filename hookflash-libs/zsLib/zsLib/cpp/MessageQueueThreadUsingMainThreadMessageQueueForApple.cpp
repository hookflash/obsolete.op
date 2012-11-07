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

#ifdef __APPLE__

#include <zsLib/internal/MessageQueueThreadUsingMainThreadMessageQueueForApple.h>
#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>

#include <boost/ref.hpp>
#include <boost/thread.hpp>


namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  namespace internal
  {
    // These are the CFRunLoopSourceRef callback functions.
    void RunLoopSourcePerformRoutine (void *info);
    void MoreMessagesLoopSourcePerformRoutine (void *info);

    class MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapper
    {
    public:
      MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapper() : mThreadQueue(MessageQueueThreadUsingMainThreadMessageQueueForApple::create())
      {
        mThreadQueue->setup();
      }
      ~MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapper()
      {
        mThreadQueue->waitForShutdown();
      }

    public:
      MessageQueueThreadUsingMainThreadMessageQueueForApplePtr mThreadQueue;
    };


    static MessageQueueThreadUsingMainThreadMessageQueueForApplePtr getThreadMessageQueue()
    {
      static MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapper wrapper;
      //if (! getGlobal().mThreadQueueWrapper.get())
      //getGlobal().mThreadQueueWrapper.reset(new MessageQueueThreadUsingCurrentGUIMessageQueueForWindowsWrapper);
      return wrapper.mThreadQueue;
    }

    MessageQueueThreadUsingMainThreadMessageQueueForApplePtr MessageQueueThreadUsingMainThreadMessageQueueForApple::singleton()
    {
      return getThreadMessageQueue();
    }

    MessageQueueThreadUsingMainThreadMessageQueueForApplePtr MessageQueueThreadUsingMainThreadMessageQueueForApple::create()
    {
      MessageQueueThreadUsingMainThreadMessageQueueForApplePtr thread(new MessageQueueThreadUsingMainThreadMessageQueueForApple);
      thread->mQueue = zsLib::MessageQueue::create(thread);
      return thread;
    }

    void MessageQueueThreadUsingMainThreadMessageQueueForApple::setup()
    {
      //CFRunLoopSourceContext context = {0, this, NULL, NULL, NULL, NULL, NULL, &RunLoopSourceScheduleRoutine,RunLoopSourceCancelRoutine, RunLoopSourcePerformRoutine};
      mRunLoop = CFRunLoopGetMain();

      CFRunLoopSourceContext context = {0, this, NULL, NULL, NULL, NULL, NULL, NULL,NULL, &RunLoopSourcePerformRoutine};
      processMessageLoopSource = CFRunLoopSourceCreate(NULL, 1, &context);
      if ( (NULL != processMessageLoopSource) && CFRunLoopSourceIsValid(processMessageLoopSource))
      {
        CFRunLoopAddSource(mRunLoop, processMessageLoopSource, kCFRunLoopDefaultMode);

        if (!CFRunLoopContainsSource(mRunLoop, processMessageLoopSource, kCFRunLoopDefaultMode))
        {
          ZS_THROW_CUSTOM(Exceptions::MessageQueueSourceNotAdded, "run loop input source is not added.")
        }
      }
      else
      {
        ZS_THROW_CUSTOM(Exceptions::MessageQueueSourceNotValid, "run loop input source is not valid.")
      }

      CFRunLoopSourceContext contextMoreMessages = {0, this, NULL, NULL, NULL, NULL, NULL, NULL,NULL, &MoreMessagesLoopSourcePerformRoutine};
      moreMessagesLoopSource = CFRunLoopSourceCreate(NULL, 1, &contextMoreMessages);
      if ( (NULL != moreMessagesLoopSource)  && CFRunLoopSourceIsValid(moreMessagesLoopSource))
      {
        CFRunLoopAddSource(mRunLoop, moreMessagesLoopSource, kCFRunLoopDefaultMode);

        if (!CFRunLoopContainsSource(mRunLoop, moreMessagesLoopSource, kCFRunLoopDefaultMode))
        {
          ZS_THROW_CUSTOM(Exceptions::MessageQueueSourceNotAdded, "run loop input source is not added.")
        }
      }
      else
      {
        ZS_THROW_CUSTOM(Exceptions::MessageQueueSourceNotValid, "run loop input source is not valid.")
      }

    }

    MessageQueueThreadUsingMainThreadMessageQueueForApple::MessageQueueThreadUsingMainThreadMessageQueueForApple()
    {
      processMessageLoopSource = NULL;
      moreMessagesLoopSource = NULL;
    }

    MessageQueueThreadUsingMainThreadMessageQueueForApple::~MessageQueueThreadUsingMainThreadMessageQueueForApple()
    {
      if (NULL != processMessageLoopSource)
      {
        CFRunLoopSourceInvalidate(processMessageLoopSource);
      }

      if (NULL != moreMessagesLoopSource)
      {
        CFRunLoopSourceInvalidate(moreMessagesLoopSource);
      }
    }

    void MessageQueueThreadUsingMainThreadMessageQueueForApple::process()
    {
      MessageQueuePtr queue;

      {
        AutoLock lock(mLock);
        queue = mQueue;
        if (!mQueue)
          return;
      }

      //process only one message in que
      queue->processOnlyOneMessage();

      //if there are more messages to process signal it
      if (this->getTotalUnprocessedMessages() > 0)
      {
        if ( (NULL == moreMessagesLoopSource) || !CFRunLoopSourceIsValid(moreMessagesLoopSource))
        {
          ZS_THROW_CUSTOM(Exceptions::MessageQueueSourceNotValid, "run loop input source is not valid.")
        }
        else
        {
          CFRunLoopSourceSignal(moreMessagesLoopSource);
          CFRunLoopWakeUp(mRunLoop);
        }
      }

    }

    void MessageQueueThreadUsingMainThreadMessageQueueForApple::post(IMessageQueueMessagePtr message)
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

    UINT MessageQueueThreadUsingMainThreadMessageQueueForApple::getTotalUnprocessedMessages() const
    {
      AutoLock lock(mLock);
      if (!mQueue)
        return 0;

      return mQueue->getTotalUnprocessedMessages();
    }

    void MessageQueueThreadUsingMainThreadMessageQueueForApple::notifyMessagePosted()
    {
      AutoLock lock(mLock);
      if ( (NULL == processMessageLoopSource) || !CFRunLoopSourceIsValid(processMessageLoopSource))
      {
        ZS_THROW_CUSTOM(Exceptions::MessageQueueSourceNotValid, "run loop input source is not valid.")
      }
      CFRunLoopSourceSignal(processMessageLoopSource);
      CFRunLoopWakeUp(mRunLoop);
    }

    void MessageQueueThreadUsingMainThreadMessageQueueForApple::waitForShutdown()
    {
      AutoLock lock(mLock);
      mQueue.reset();

      if (NULL != processMessageLoopSource)
      {
        CFRunLoopSourceInvalidate(processMessageLoopSource);
        processMessageLoopSource = NULL;

      }

      if (NULL != moreMessagesLoopSource)
      {
        CFRunLoopSourceInvalidate(moreMessagesLoopSource);
        moreMessagesLoopSource = NULL;
      }
    }


    void RunLoopSourcePerformRoutine (void *info)
    {
      MessageQueueThreadUsingMainThreadMessageQueueForApple* obj = (MessageQueueThreadUsingMainThreadMessageQueueForApple*)info;
      obj->process();
    }


    void MoreMessagesLoopSourcePerformRoutine (void *info)
    {
      MessageQueueThreadUsingMainThreadMessageQueueForApple* obj = (MessageQueueThreadUsingMainThreadMessageQueueForApple*)info;
      obj->notifyMessagePosted();
    }
  }
}

#endif //__APPLE__

