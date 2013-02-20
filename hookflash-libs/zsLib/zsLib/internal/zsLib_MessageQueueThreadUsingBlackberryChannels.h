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

#pragma once

#ifdef __QNX__

#ifndef ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGBLACKBERRYCHANNELS_H_b359128a0bad4631bdaeab09d5b25847
#define ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGBLACKBERRYCHANNELS_H_b359128a0bad4631bdaeab09d5b25847

#include <zsLib/MessageQueue.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>

namespace zsLib
{
  namespace internal
  {
    class MessageQueueThreadUsingBlackberryChannelsWrapper;

    class MessageQueueThreadUsingBlackberryChannels;
    typedef boost::shared_ptr<MessageQueueThreadUsingBlackberryChannels> MessageQueueThreadUsingBlackberryChannelsPtr;
    typedef boost::weak_ptr<MessageQueueThreadUsingBlackberryChannels> MessageQueueThreadUsingBlackberryChannelsWeakPtr;

    class MessageQueueThreadUsingBlackberryChannels : public MessageQueueThread,
                                                                    public IMessageQueueNotify
    {
      friend class MessageQueueThreadUsingBlackberryChannelsWrapper;

    public:
      struct Exceptions
      {
        ZS_DECLARE_CUSTOM_EXCEPTION(MessageQueueAlreadyDeleted)
      };

    protected:
      MessageQueueThreadUsingBlackberryChannels();
      static MessageQueueThreadUsingBlackberryChannelsPtr create();
      void setup();

    public:
      ~MessageQueueThreadUsingBlackberryChannels();

      static MessageQueueThreadUsingBlackberryChannelsPtr singleton();

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

      MessageQueuePtr mQueue;
      int mChannelId;
    };
  }
}

#endif // ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGBLACKBERRYCHANNELS_H_b359128a0bad4631bdaeab09d5b25847

#endif // __QNX__
