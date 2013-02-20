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

#include <zsLib/internal/MessageQueueThreadUsingBlackberryChannels.h>
#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

#include <boost/ref.hpp>
#include <boost/thread.hpp>
#include <boost/thread.hpp>

#include <tchar.h>

#include <bps/bps.h>

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  namespace internal
  {
//    static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

    class MessageQueueThreadUsingBlackberryChannelsGlobal
    {
    public:
      MessageQueueThreadUsingBlackberryChannelsGlobal() :
        mCustomMessageName(_T("zsLibCustomMessage4cce8fd89d067b42860cc9074d64539f"))
      {
    	// From the BB docs:
    	// Your application can call the bps_initialize() function more than once. An application that
    	// calls the bps_initialize() function multiple times should call the bps_shutdown() function the
    	//same number of times.
    	bbps_initialize();

        // [TODO] LAG figure out the equivalent for BB
    	// mCustomMessage = ::RegisterWindowMessage(mCustomMessageName);
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

    static MessageQueueThreadUsingBlackberryChannelsGlobal &getGlobal()
    {
      static MessageQueueThreadUsingBlackberryChannelsGlobal global;
      return global;
    }

    static MessageQueueThreadUsingBlackberryChannelsPtr getThreadMessageQueue()
    {
      if (! getGlobal().mThreadQueueWrapper.get())
        getGlobal().mThreadQueueWrapper.reset(new MessageQueueThreadUsingBlackberryChannelsWrapper);
      return getGlobal().mThreadQueueWrapper->mThreadQueue;
    }

    MessageQueueThreadUsingBlackberryChannelsPtr MessageQueueThreadUsingBlackberryChannels::singleton()
    {
      return getThreadMessageQueue();
    }

    MessageQueueThreadUsingBlackberryChannelsPtr MessageQueueThreadUsingBlackberryChannels::create()
    {
      MessageQueueThreadUsingBlackberryChannelsPtr thread(new MessageQueueThreadUsingBlackberryChannels);
      thread->mQueue = zsLib::MessageQueue::create(thread);
      return thread;
    }

    void MessageQueueThreadUsingBlackberryChannels::setup()
    {
      // make sure the window message queue was created by peeking a message
      MSG msg;
      memset(&msg, 0, sizeof(msg));
      ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

      mHWND = ::CreateWindow(
        getGlobal().mHiddenWindowClassName,
        _T("zsLibHiddenWindow9127b0cb49457fdb969054c57cff6ed5efe771c0"),
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        getGlobal().mModule,
        NULL
      );
      ZS_THROW_BAD_STATE_IF(NULL == mHWND)
    }

    MessageQueueThreadUsingBlackberryChannels::MessageQueueThreadUsingBlackberryChannels() :
      mHWND(NULL)
    {
    }

    MessageQueueThreadUsingBlackberryChannels::~MessageQueueThreadUsingBlackberryChannels()
    {
      if (NULL != mHWND)
      {
        ZS_THROW_BAD_STATE_IF(0 == ::DestroyWindow(mHWND))
        mHWND = NULL;
      }
    }

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

    UINT MessageQueueThreadUsingBlackberryChannels::getTotalUnprocessedMessages() const
    {
      AutoLock lock(mLock);
      if (!mQueue)
        return 0;

      return mQueue->getTotalUnprocessedMessages();
    }

    void MessageQueueThreadUsingBlackberryChannels::notifyMessagePosted()
    {
      AutoLock lock(mLock);
      if (NULL == mHWND) {
        ZS_THROW_CUSTOM(Exceptions::MessageQueueAlreadyDeleted, "message posted to message queue after message queue was deleted.")
      }

      ZS_THROW_BAD_STATE_IF(0 == ::PostMessage(mHWND, getGlobal().mCustomMessage, (WPARAM)0, (LPARAM)0))
    }

    void MessageQueueThreadUsingBlackberryChannels::waitForShutdown()
    {
      AutoLock lock(mLock);
      mQueue.reset();

      if (NULL != mHWND)
      {
        ZS_THROW_BAD_STATE_IF(0 == ::DestroyWindow(mHWND))
        mHWND = NULL;
      }
    }

    static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      if (getGlobal().mCustomMessage != uMsg)
        return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

      MessageQueueThreadUsingBlackberryChannelsPtr queue(getThreadMessageQueue());
      queue->process();
      return (LRESULT)0;
    }
  }
}

#endif // __QNX__
