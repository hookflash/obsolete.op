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

#ifndef ZSLIB_INTERNAL_STRINGIZE_H_0c235f6defcccb275d602da44da60e58
#define ZSLIB_INTERNAL_STRINGIZE_H_0c235f6defcccb275d602da44da60e58

#include <zsLib/types.h>
#include <zsLib/String.h>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace zsLib
{
  namespace internal
  {
    String convert(ULONGLONG value, size_t base);
  }
} // namespace zsLib

#endif //ZSLIB_INTERNAL_STRINGIZE_H_0c235f6defcccb275d602da44da60e58

#ifdef ZSLIB_INTERNAL_STRINGIZE_H_0c235f6defcccb275d602da44da60e58_SECOND_INCLUDE
#undef ZSLIB_INTERNAL_STRINGIZE_H_0c235f6defcccb275d602da44da60e58_SECOND_INCLUDE
namespace zsLib
{
  template<typename t_type>
  inline Stringize<t_type>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);

    return internal::convert((ULONGLONG)mValue, mBase);
  }

  template<>
  inline Stringize<bool>::operator String() const
  {
    return mValue ? String("true") : String("false");
  }

  template<>
  inline Stringize<const char *>::operator String() const
  {
    return mValue ? String(mValue) : String();
  }

  template<>
  inline Stringize<CHAR>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>((int)mValue);
    return internal::convert((ULONGLONG)((UCHAR)mValue), mBase);
  }

  template<>
  inline Stringize<UCHAR>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>((UINT)mValue);
    return internal::convert((ULONGLONG)((UINT)mValue), mBase);
  }

  template<>
  inline Stringize<SHORT>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);
    return internal::convert((ULONGLONG)((USHORT)mValue), mBase);
  }

  template<>
  inline Stringize<USHORT>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);
    return internal::convert((ULONGLONG)mValue, mBase);
  }

  template<>
  inline Stringize<INT>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);
    return internal::convert((ULONGLONG)((UINT)mValue), mBase);
  }

  template<>
  inline Stringize<UINT>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);
    return internal::convert((ULONGLONG)mValue, mBase);
  }

  template<>
  inline Stringize<LONG>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);
    return internal::convert((ULONGLONG)((ULONG)mValue), mBase);
  }

  template<>
  inline Stringize<ULONG>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);
    return internal::convert((ULONGLONG)mValue, mBase);
  }

  template<>
  inline Stringize<LONGLONG>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);
    return internal::convert((ULONGLONG)mValue, mBase);
  }

  template<>
  inline Stringize<ULONGLONG>::operator String() const
  {
    if (10 == mBase)
      return boost::lexical_cast<String>(mValue);
    return internal::convert(mValue, mBase);
  }

  template<>
  inline Stringize<float>::operator String() const
  {
    return boost::lexical_cast<String>(mValue);
  }

  template<>
  inline Stringize<double>::operator String() const
  {
    return boost::lexical_cast<String>(mValue);
  }

  template<>
  inline Stringize<UUID>::operator String() const
  {
    return String(boost::lexical_cast<std::string>(mValue));
  }

  template<>
  inline Stringize<Time>::operator String() const
  {
    return String(boost::posix_time::to_simple_string(mValue));
  }
}
#else
#define ZSLIB_INTERNAL_STRINGIZE_H_0c235f6defcccb275d602da44da60e58_SECOND_INCLUDE
#endif //ZSLIB_INTERNAL_STRINGIZE_H_0c235f6defcccb275d602da44da60e58_SECOND_INCLUDE
