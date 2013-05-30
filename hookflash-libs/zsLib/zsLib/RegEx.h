/*
 *  Created by Lawrence Gunn.
 *  Copyright 2013. Lawrence Gunn. All rights reserved.
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

#ifndef ZSLIB_REGEX_H_5e468a30314448b8a02f8d696eeca86c
#define ZSLIB_REGEX_H_5e468a30314448b8a02f8d696eeca86c

#include <zsLib/internal/zsLib_RegEx.h>

namespace zsLib
{
  class RegEx
  {
  public:
    struct Exceptions
    {
      ZS_DECLARE_CUSTOM_EXCEPTION(ParseError)
    };

    RegEx(const String& regularExpression);

    bool hasMatch(const String& stringToSearch);

  private:
    String mRegularExpression;
  };

} // namespace zsLib

#endif //ZSLIB_REGEX_H_5e468a30314448b8a02f8d696eeca86c
