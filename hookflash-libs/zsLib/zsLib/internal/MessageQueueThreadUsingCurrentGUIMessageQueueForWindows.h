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

#ifdef _WIN32

#ifndef ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGCURRENTGUIMESSAGEQUEUEFORWINDOWS_H_e059928c0dab4631bdaeab09d5b25847
#define ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGCURRENTGUIMESSAGEQUEUEFORWINDOWS_H_e059928c0dab4631bdaeab09d5b25847

#include <zsLib/MessageQueue.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Exception.h>

namespace zsLib
{
  namespace internal
  {
    class MessageQueueThreadUsingCurrentGUIMessageQueueForWindowsWrapper;

    class MessageQueueThreadUsingCurrentGUIMessageQueueForWindows;
    typedef boost::shared_ptr<MessageQueueThreadUsingCurrentGUIMessageQueueForWindows> MessageQueueThreadUsingCurrentGUIMessageQueueForWindowsPtr;
    typedef boost::weak_ptr<MessageQueueThreadUsingCurrentGUIMessageQueueForWindows> MessageQueueThreadUsingCurrentGUIMessageQueueForWindowsWeakPtr;

    class MessageQueueThreadUsingCurrentGUIMessageQueueForWindows : public MessageQueueThread,
                                                                    public IMessageQueueNotify
    {
      friend class MessageQueueThreadUsingCurrentGUIMessageQueueForWindowsWrapper;

    public:
      struct Exceptions
      {
        ZS_DECLARE_CUSTOM_EXCEPTION(MessageQueueAlreadyDeleted)
      };

    protected:
      MessageQueueThreadUsingCurrentGUIMessageQueueForWindows();
      static MessageQueueThreadUsingCurrentGUIMessageQueueForWindowsPtr create();
      void setup();

    public:
      ~MessageQueueThreadUsingCurrentGUIMessageQueueForWindows();

      static MessageQueueThreadUsingCurrentGUIMessageQueueForWindowsPtr singleton();

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
      HWND mHWND;
    };
  }
}

#endif //ZSLIB_INTERNAL_MESSAGEQUEUETHREADUSINGCURRENTGUIMESSAGEQUEUEFORWINDOWS_H_e059928c0dab4631bdaeab09d5b25847

#endif //_WIN32
