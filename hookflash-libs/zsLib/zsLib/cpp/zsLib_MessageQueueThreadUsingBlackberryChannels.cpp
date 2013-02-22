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

#ifdef __QNX__

#include <zsLib/internal/zsLib_MessageQueueThreadUsingBlackberryChannels.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

#include <boost/ref.hpp>
#include <boost/thread.hpp>

#include <bps/bps.h>

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  namespace internal
  {
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark zsLib::internal::MessageQueueThreadUsingBlackberryChannelsWrapper
    #pragma mark
    class MessageQueueThreadUsingBlackberryChannelsWrapper
    {
    public:
      MessageQueueThreadUsingBlackberryChannelsWrapper() : mThreadQueue(MessageQueueThreadUsingBlackberryChannels::create())
      {
        mThreadQueue->setup();
      }
      ~MessageQueueThreadUsingBlackberryChannelsWrapper()
      {
        mThreadQueue->waitForShutdown();
      }

    public:
      MessageQueueThreadUsingBlackberryChannelsPtr mThreadQueue;
    };

    typedef boost::thread_specific_ptr<MessageQueueThreadUsingBlackberryChannelsWrapper> MessageQueueThreadUsingBlackberryChannelsWrapperThreadPtr;

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark zsLib::internal::MessageQueueThreadUsingBlackberryChannelsGlobal
    #pragma mark
    class MessageQueueThreadUsingBlackberryChannelsGlobal
    {
    public:
      MessageQueueThreadUsingBlackberryChannelsGlobal()
      {
        // From the BB docs:
        // Your application can call the bps_initialize() function more than once. An application that
        // calls the bps_initialize() function multiple times should call the bps_shutdown() function the
        //same number of times.
        bps_initialize();
      }

      ~MessageQueueThreadUsingBlackberryChannelsGlobal()
      {
        // See not above regarding bps_initialize.
        bps_shutdown();
      }

    public:
      MessageQueueThreadUsingBlackberryChannelsWrapperThreadPtr mThreadQueueWrapper;
      const TCHAR *mCustomMessageName;
      UINT mCustomMessage;
    };

    //-----------------------------------------------------------------------
    static MessageQueueThreadUsingBlackberryChannelsGlobal &getGlobal()
    {
      static MessageQueueThreadUsingBlackberryChannelsGlobal global;
      return global;
    }

    //-----------------------------------------------------------------------
    static MessageQueueThreadUsingBlackberryChannelsPtr getThreadMessageQueue()
    {
      if (! getGlobal().mThreadQueueWrapper.get())
        getGlobal().mThreadQueueWrapper.reset(new MessageQueueThreadUsingBlackberryChannelsWrapper);
      return getGlobal().mThreadQueueWrapper->mThreadQueue;
    }

    //-----------------------------------------------------------------------
    MessageQueueThreadUsingBlackberryChannelsPtr MessageQueueThreadUsingBlackberryChannels::singleton()
    {
      return getThreadMessageQueue();
    }

    //-----------------------------------------------------------------------
    MessageQueueThreadUsingBlackberryChannelsPtr MessageQueueThreadUsingBlackberryChannels::create()
    {
      MessageQueueThreadUsingBlackberryChannelsPtr thread(new MessageQueueThreadUsingBlackberryChannels);
      thread->mQueue = zsLib::MessageQueue::create(thread);
      return thread;
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark zsLib::internal::MessageQueueThreadUsingBlackberryChannels
    #pragma mark

    //-----------------------------------------------------------------------
    MessageQueueThreadUsingBlackberryChannels::MessageQueueThreadUsingBlackberryChannels() :
    mChannelId(0)
    {
    }

    //-----------------------------------------------------------------------
    MessageQueueThreadUsingBlackberryChannels::~MessageQueueThreadUsingBlackberryChannels()
    {
      if (0 != mChannelId)
      {
        ZS_THROW_BAD_STATE_IF(BPS_FAILURE == bps_channel_destroy(mChannelId))
        mChannelId = 0;
      }
    }

    //-----------------------------------------------------------------------
    void MessageQueueThreadUsingBlackberryChannels::setup()
    {
      int status = bps_channel_create(&mChannelId, 0); // last arg is reserved for future use.
      ZS_THROW_BAD_STATE_IF(BPS_SUCCESS != status)
    }

    //-----------------------------------------------------------------------
    void MessageQueueThreadUsingBlackberryChannels::process()
    {
      MessageQueuePtr queue;

      {
        AutoLock lock(mLock);
        queue = mQueue;
        if (!mQueue)
          return;
      }

      queue->processOnlyOneMessage(); // process only one messsage at a time since this must be syncrhonized through the GUI message queue
    }

    //-----------------------------------------------------------------------
    void MessageQueueThreadUsingBlackberryChannels::post(IMessageQueueMessagePtr message)
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

    //-----------------------------------------------------------------------
    UINT MessageQueueThreadUsingBlackberryChannels::getTotalUnprocessedMessages() const
    {
      AutoLock lock(mLock);
      if (!mQueue)
        return 0;

      return mQueue->getTotalUnprocessedMessages();
    }

    //-----------------------------------------------------------------------
    void MessageQueueThreadUsingBlackberryChannels::notifyMessagePosted()
    {
      AutoLock lock(mLock);
      if (0 == mChannelId) {
        ZS_THROW_CUSTOM(Exceptions::MessageQueueAlreadyDeleted, "message posted to message queue after message queue was deleted.")
      }

      int status = bps_channel_exec(mChannelId, &MessageQueueThreadUsingBlackberryChannels::channelExec, NULL);
      ZS_THROW_BAD_STATE_IF(BPS_SUCCESS != status)
    }

    //-----------------------------------------------------------------------
    void MessageQueueThreadUsingBlackberryChannels::waitForShutdown()
    {
      AutoLock lock(mLock);
      mQueue.reset();

      if (0 != mChannelId)
      {
        ZS_THROW_BAD_STATE_IF(BPS_FAILURE == bps_channel_destroy(mChannelId))
        mChannelId = 0;
      }
    }

    //-----------------------------------------------------------------------
    int MessageQueueThreadUsingBlackberryChannels::channelExec(void* thisPtr)
    {
      MessageQueueThreadUsingBlackberryChannelsPtr queue(getThreadMessageQueue());
      queue->process();
      return BPS_SUCCESS;
    }
  }
}

#endif // __QNX__
