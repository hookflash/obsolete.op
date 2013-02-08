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

#include <zsLib/Stringize.h>
#include <zsLib/Exception.h>

#include <boost/shared_array.hpp>

namespace zsLib { ZS_DECLARE_SUBSYSTEM(zsLib) }

namespace zsLib
{
  namespace internal
  {
    static char gDigits[10+26] =
    {
      '0','1','2','3','4','5','6','7','8','9',
      'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
    };

    String convert(ULONGLONG value, size_t base)
    {
      ZS_THROW_INVALID_USAGE_IF((base < 2) || (base > (10+26)))

      boost::shared_array<char> buffer(new char[sizeof(ULONGLONG)*8+2]);
      memset(buffer.get(), 0, sizeof(char)*((sizeof(ULONGLONG)*8)+2));

      STR end = (buffer.get() + (sizeof(ULONGLONG)*8));
      STR dest = end;
      *dest = 0;
      while (value > 0)
      {
        --dest;
        *dest = gDigits[value % base];
        value /= base;
      }

      if (dest == end)
        *dest = '0';
      return String(dest);
    }
  }
}
