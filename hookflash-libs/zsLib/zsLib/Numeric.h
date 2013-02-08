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

#ifndef ZSLIB_NUMERIC_H_505af97f61feb236ca87601e6bf58e58
#define ZSLIB_NUMERIC_H_505af97f61feb236ca87601e6bf58e58

#include <zsLib/internal/zsLib_Numeric.h>

#pragma warning(push)
#pragma warning(disable:4290)

namespace zsLib
{
  template<typename t_type>
  class Numeric
  {
  public:
    ZS_DECLARE_CUSTOM_EXCEPTION(ValueOutOfRange)

  public:
    Numeric(bool ignoreWhitespace = true, size_t base = 10) : mIngoreWhitespace(ignoreWhitespace), mBase(base)                                          {}
    Numeric(CSTR value, bool ignoreWhitespace = true, size_t base = 10) : mData(value), mIngoreWhitespace(ignoreWhitespace), mBase(base)                {}
    Numeric(const String &value, bool ignoreWhitespace = true, size_t base = 10) : mData(value), mIngoreWhitespace(ignoreWhitespace), mBase(base)       {}
    Numeric(const Numeric &value, bool ignoreWhitespace = true, size_t base = 10) : mData(value.mData),mIngoreWhitespace(ignoreWhitespace), mBase(base) {}

    Numeric &operator=(const String &value)         {mData = value; return *this;}
    Numeric &operator=(const Numeric &value)        {mData = value.mData; return *this;}

    void get(t_type &outValue) const throw (ValueOutOfRange);

    bool ignoreSpace() const                        {return mIngoreWhitespace;}
    int getBase() const                             {return mBase;}

    operator t_type() const                         {t_type value; get(value); return value;}

  private:

    String mData;
    bool mIngoreWhitespace;
    size_t mBase;
  };
}

#pragma warning(pop)

#include <zsLib/internal/zsLib_Numeric.h>

#endif // ZSLIB_NUMERIC_H_505af97f61feb236ca87601e6bf58e58
