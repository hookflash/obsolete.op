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

#ifndef ZSLIB_INTERNAL_EVENT_H_2070e4cfb8f24209647d3c9ec55098ee
#define ZSLIB_INTERNAL_EVENT_H_2070e4cfb8f24209647d3c9ec55098ee

#include <zsLib/zsTypes.h>

namespace zsLib
{
  namespace internal
  {
    class Event : public boost::noncopyable
    {
    public:
      Event() : mNotified(0) {}

    protected:
      DWORD mNotified;
      boost::mutex mMutex;
      boost::condition_variable mCondition;
    };
  }
}

#endif //ZSLIB_INTERNAL_EVENT_H_2070e4cfb8f24209647d3c9ec55098ee
