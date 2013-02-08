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

#ifndef ZSLIB_MESSAGEQUEUEASSOCIATOR_H_b30d8a76e5e6d9d9602d98953384ac76
#define ZSLIB_MESSAGEQUEUEASSOCIATOR_H_b30d8a76e5e6d9d9602d98953384ac76

#include <zsLib/types.h>
#include <zsLib/MessageQueue.h>

namespace zsLib
{
  class MessageQueueAssociator
  {
  public:
    MessageQueueAssociator(IMessageQueuePtr queue) : mQueue(queue) {}

    IMessageQueuePtr getAssociatedMessageQueue() const {return mQueue;}

  private:
    IMessageQueuePtr mQueue;
  };

} // namespace zsLib

#endif //ZSLIB_MESSAGEQUEUEASSOCIATOR_H_b30d8a76e5e6d9d9602d98953384ac76
