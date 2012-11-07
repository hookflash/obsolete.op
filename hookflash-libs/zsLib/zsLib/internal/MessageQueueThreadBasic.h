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

#ifndef ZSLIB_INTERNAL_MESSAGEQUEUETHREADBASIC_H_f615413ac754769243970b0fefce2e2e
#define ZSLIB_INTERNAL_MESSAGEQUEUETHREADBASIC_H_f615413ac754769243970b0fefce2e2e

#include <zsLib/MessageQueue.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>
#include <zsLib/Event.h>

namespace zsLib
{
  namespace internal
  {
    class MessageQueueThreadBasic;
    typedef boost::shared_ptr<MessageQueueThreadBasic> MessageQueueThreadBasicPtr;
    typedef boost::weak_ptr<MessageQueueThreadBasic> MessageQueueThreadBasicWeakPtr;

    class MessageQueueThreadBasic : public MessageQueueThread,
                                    public IMessageQueueNotify
    {
    public:
      struct Exceptions
      {
        ZS_DECLARE_CUSTOM_EXCEPTION(MessageQueueAlreadyDeleted)
      };

    protected:
      MessageQueueThreadBasic(const char *threadName);

    public:
      static MessageQueueThreadBasicPtr create(const char *threadName = NULL, ThreadPriorities threadPriority = ThreadPriority_NormalPriority);

      void operator () ();

      // IMessageQueue
      virtual void post(IMessageQueueMessagePtr message);

      virtual UINT getTotalUnprocessedMessages() const;

      // IMessageQueueNotify
      virtual void notifyMessagePosted();

      // IMessageQueueThread
      virtual void waitForShutdown();

    protected:
      ThreadPtr mThread;
      String mThreadName;

      mutable zsLib::Event mEvent;
      MessageQueuePtr mQueue;

      mutable Lock mLock;
      DWORD mMustShutdown;
    };
  }
}

#endif //ZSLIB_INTERNAL_MESSAGEQUEUETHREADBASIC_H_f615413ac754769243970b0fefce2e2e
