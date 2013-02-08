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

#ifndef ZSLIB_STRINGIZE_H_aa15b3dce8688ace2fc3f0d12deaefb8
#define ZSLIB_STRINGIZE_H_aa15b3dce8688ace2fc3f0d12deaefb8

#include <zsLib/internal/zsLib_Stringize.h>

namespace zsLib
{
  template<typename t_type>
  class Stringize
  {
  public:
    Stringize(t_type value, size_t base = 10) : mValue(value), mBase(base)        {}
    Stringize(const Stringize &value) : mValue(value.mValue), mBase(value.mBase)  {}

    operator String() const;
    String string() {return (String)(*this);}

  private:
    t_type mValue;
    size_t mBase;
  };

} // namespace zsLib

#include <zsLib/internal/zsLib_Stringize.h>

#endif //ZSLIB_STRINGIZE_H_aa15b3dce8688ace2fc3f0d12deaefb8
