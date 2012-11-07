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

#ifndef ZSLIB_INTERNAL_NUMERIC_H_f1973c558a596895b01b96e4a76de63f
#define ZSLIB_INTERNAL_NUMERIC_H_f1973c558a596895b01b96e4a76de63f

#include <zsLib/String.h>
#include <zsLib/zsTypes.h>
#include <zsLib/Exception.h>

namespace zsLib
{
  namespace internal
  {
    bool convert(const String &input, LONGLONG &outResult, size_t size, bool ignoreWhiteSpace, size_t base);
    bool convert(const String &input, ULONGLONG &outResult, size_t size, bool ignoreWhiteSpace, size_t base);
    bool convert(const String &input, bool &outResult, bool ignoreWhiteSpace);
    bool convert(const String &input, float &outResult, bool ignoreWhiteSpace);
    bool convert(const String &input, double &outResult, bool ignoreWhiteSpace);
    bool convert(const String &input, UUID &outResult, bool ignoreWhiteSpace);
    bool convert(const String &input, Time &outResult, bool ignoreWhiteSpace);
  }
}

#endif //ZSLIB_INTERNAL_NUMERIC_H_f1973c558a596895b01b96e4a76de63f

#ifdef ZSLIB_INTERNAL_NUMERIC_H_f1973c558a596895b01b96e4a76de63f_SECOND_INCLUDE

#pragma warning(push)
#pragma warning(disable:4290)

namespace zsLib
{
  template<>
  void Numeric<bool>::get(bool &outValue) const throw (ValueOutOfRange);

  template<>
  void Numeric<float>::get(float &outValue) const throw (ValueOutOfRange);

  template<>
  void Numeric<double>::get(double &outValue) const throw (ValueOutOfRange);

  template<>
  void Numeric<UUID>::get(UUID &outValue) const throw (ValueOutOfRange);

  template<>
  void Numeric<Time>::get(Time &outValue) const throw (ValueOutOfRange);
}

#pragma warning(pop)

#else
#define ZSLIB_INTERNAL_NUMERIC_H_f1973c558a596895b01b96e4a76de63f_SECOND_INCLUDE
#endif //ZSLIB_INTERNAL_NUMERIC_H_f1973c558a596895b01b96e4a76de63f_SECOND_INCLUDE
