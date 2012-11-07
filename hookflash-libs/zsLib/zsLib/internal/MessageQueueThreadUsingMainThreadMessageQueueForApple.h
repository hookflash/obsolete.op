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

#ifdef __APPLE__

#ifndef ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGMAINTHREADMESSAGEQUEUEFORAPPLE_H_e059928c0dab4631bdaeab09d5b25847
#define ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGMAINTHREADMESSAGEQUEUEFORAPPLE_H_e059928c0dab4631bdaeab09d5b25847

#include <zsLib/MessageQueue.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>

#include <CoreFoundation/CoreFoundation.h>

namespace zsLib
{
  namespace internal
  {
    class MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapper;

    class MessageQueueThreadUsingMainThreadMessageQueueForApple;

    typedef boost::shared_ptr<MessageQueueThreadUsingMainThreadMessageQueueForApple> MessageQueueThreadUsingMainThreadMessageQueueForApplePtr;
    typedef boost::weak_ptr<MessageQueueThreadUsingMainThreadMessageQueueForApple> MessageQueueThreadUsingMainThreadMessageQueueForAppleWeakPtr;
    typedef boost::thread_specific_ptr<MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapper> MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapperThreadPtr;


    class MessageQueueThreadUsingMainThreadMessageQueueForApple : public MessageQueueThread, public IMessageQueueNotify
    {
      friend class MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapper;

    public:
      struct Exceptions
      {
        ZS_DECLARE_CUSTOM_EXCEPTION(MessageQueueAlreadyDeleted);
        ZS_DECLARE_CUSTOM_EXCEPTION(MessageQueueSourceNotAdded);
        ZS_DECLARE_CUSTOM_EXCEPTION(MessageQueueSourceNotValid);
        //ZS_DECLARE_CUSTOM_EXCEPTION(MessageQueueSourceAlreadyDeleted);
      };

    protected:
      MessageQueueThreadUsingMainThreadMessageQueueForApple();
      static MessageQueueThreadUsingMainThreadMessageQueueForApplePtr create();
      void setup();

    public:
      ~MessageQueueThreadUsingMainThreadMessageQueueForApple();

      static MessageQueueThreadUsingMainThreadMessageQueueForApplePtr singleton();

      // IMessageQueue
      virtual void post(IMessageQueueMessagePtr message);

      virtual UINT getTotalUnprocessedMessages() const;

      // IMessageQueueNotify
      virtual void notifyMessagePosted();

      // IMessageQueueThread
      virtual void waitForShutdown();

    public:
      virtual void process();

    protected:
      mutable Lock mLock;
      MessageQueueThreadUsingMainThreadMessageQueueForAppleWrapperThreadPtr mThreadQueueWrapper;

      MessageQueuePtr mQueue;

      CFRunLoopRef mRunLoop;
      CFRunLoopSourceRef processMessageLoopSource;
      CFRunLoopSourceRef moreMessagesLoopSource;
    };
  }
}

#endif //ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGMAINTHREADMESSAGEQUEUEFORAPPLE_H_e059928c0dab4631bdaeab09d5b25847

#endif //__APPLE__
