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

#include <zsLib/MessageQueue.h>
#include <zsLib/Log.h>

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  MessageQueue::MessageQueue(IMessageQueueNotifyPtr notify) : internal::MessageQueue(notify)
  {
  }

  MessageQueuePtr MessageQueue::create(IMessageQueueNotifyPtr notify)
  {
    return MessageQueuePtr(new MessageQueue(notify));
  }

  void MessageQueue::post(IMessageQueueMessagePtr message)
  {
    {
      AutoLock lock(mLock);
      mMessages.push(message);
    }
    mNotify->notifyMessagePosted();
  }

  void MessageQueue::process()
  {
    do
    {
      IMessageQueueMessagePtr message;

      {
        AutoLock lock(mLock);
        if (0 == mMessages.size())
          return;
        message = mMessages.front();
        mMessages.pop();
      }

      // process the next message
      message->processMessage();
    } while (true);
  }

  void MessageQueue::processOnlyOneMessage()
  {
    IMessageQueueMessagePtr message;

    {
      AutoLock lock(mLock);
      if (0 == mMessages.size())
        return;
      message = mMessages.front();
      mMessages.pop();
    }

    // process the next message
    message->processMessage();
  }

  UINT MessageQueue::getTotalUnprocessedMessages() const
  {
    AutoLock lock(mLock);
    return mMessages.size();
  }

}
