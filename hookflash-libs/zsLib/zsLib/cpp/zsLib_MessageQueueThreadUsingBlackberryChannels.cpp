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

        // See note above regarding bps_initialize.
        bps_shutdown();
      }

    public:
      MessageQueueThreadUsingBlackberryChannelsPtr mThreadQueue;
    };

    static pthread_once_t messageQueueKeyOnce = PTHREAD_ONCE_INIT;
    static pthread_key_t messageQueueKey;

    //-----------------------------------------------------------------------

    void messageQueueKeyDestructor(void* value) {
      boost::shared_ptr<MessageQueueThreadUsingBlackberryChannelsWrapper>* ptr = (boost::shared_ptr<MessageQueueThreadUsingBlackberryChannelsWrapper>*) value;
      (*ptr).reset();
      delete ptr;
      pthread_setspecific(messageQueueKey, NULL);
    }

    //-----------------------------------------------------------------------

    void makeMessageQueueKeyOnce() {
      pthread_key_create(&messageQueueKey, messageQueueKeyDestructor);
    }

    //-----------------------------------------------------------------------
    static MessageQueueThreadUsingBlackberryChannelsPtr getThreadMessageQueue()
    {
      pthread_once(&messageQueueKeyOnce, makeMessageQueueKeyOnce);

      if (!pthread_getspecific(messageQueueKey)) {
        boost::shared_ptr<MessageQueueThreadUsingBlackberryChannelsWrapper>* ptr = new boost::shared_ptr<MessageQueueThreadUsingBlackberryChannelsWrapper>(new MessageQueueThreadUsingBlackberryChannelsWrapper());
        pthread_setspecific(messageQueueKey, (void*) ptr);
      }

      boost::shared_ptr<MessageQueueThreadUsingBlackberryChannelsWrapper>* returnPtr = (boost::shared_ptr<MessageQueueThreadUsingBlackberryChannelsWrapper>*) pthread_getspecific(messageQueueKey);
      return (*returnPtr).get()->mThreadQueue;
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
    MessageQueueThreadUsingBlackberryChannels::MessageQueueThreadUsingBlackberryChannels()
    {
    }

    //-----------------------------------------------------------------------
    MessageQueueThreadUsingBlackberryChannels::~MessageQueueThreadUsingBlackberryChannels()
    {
    }

    //-----------------------------------------------------------------------
    void MessageQueueThreadUsingBlackberryChannels::setup()
    {
      // NOTE!!! Must be called for the main thread.
      mCrossThreadNotifier = IQtCrossThreadNotifier::createNotifier();
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

      queue->processOnlyOneMessage(); // process only one message at a time since this must be synchronized through the GUI message queue
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
      {
        AutoLock lock(mLock);
        mCrossThreadNotifier->setDelegate(singleton());
      }
      mCrossThreadNotifier->notifyMessagePosted();
    }

    //-----------------------------------------------------------------------
    void MessageQueueThreadUsingBlackberryChannels::waitForShutdown()
    {
      AutoLock lock(mLock);
      mQueue.reset();

      if (mCrossThreadNotifier)
      {
        mCrossThreadNotifier.reset();
      }
    }
    //-----------------------------------------------------------------------
    void MessageQueueThreadUsingBlackberryChannels::processMessageFromThread()
    {
      MessageQueueThreadUsingBlackberryChannelsPtr queue(getThreadMessageQueue());
      queue->process();
    }
  }
}

#endif // __QNX__
