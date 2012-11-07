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

#ifndef ZSLIB_INTERNAL_PROXYPACK_H_3d1989252cf7499ccb86cef6b9c76857
#define ZSLIB_INTERNAL_PROXYPACK_H_3d1989252cf7499ccb86cef6b9c76857

#include <zsLib/zsTypes.h>
#include <zsLib/MessageQueue.h>
#include <zsLib/String.h>

namespace zsLib
{
  namespace internal
  {
    template <typename TYPE>
    inline void ProxyPack(TYPE &member, TYPE &value)
    {
#ifdef _WIN32
      sizeof(TYPE);
#endif //_WIN32
      member = value;
    }

    template <typename TYPE>
    inline void ProxyClean(TYPE &member)
    {
    }

    template <>
    inline void ProxyPack<CSTR>(CSTR &member, CSTR &value)
    {
      if (value) {
        size_t length = strlen(value);
        STR temp = new char[length+1];
        memcpy(temp, value, sizeof(char)*(length+1));
        member = (CSTR)temp;
      } else
        member = NULL;
    }

    template <>
    inline void ProxyClean<CSTR>(CSTR &member)
    {
      delete [] ((char *)member);
      member = NULL;
    }

    template <>
    inline void ProxyPack<CWSTR>(CWSTR &member, CWSTR &value)
    {
      if (value) {
        size_t length = wcslen(value);
        WSTR temp = new WCHAR[length+1];
        memcpy(temp, value, sizeof(WCHAR)*(length+1));
        member = (CWSTR)temp;
      } else
        member = NULL;
    }

    template <>
    inline void ProxyClean<CWSTR>(CWSTR &member)
    {
      delete [] ((WCHAR *)member);
      member = NULL;
    }

    template <>
    inline void ProxyPack<std::string>(std::string &member, std::string &value)
    {
      member = value.c_str();
    }

    template <>
    inline void ProxyClean<std::string>(std::string &member)
    {
      member.clear();
    }

    template <>
    inline void ProxyPack<std::wstring>(std::wstring &member, std::wstring &value)
    {
      member = value.c_str();
    }

    template <>
    inline void ProxyClean<std::wstring>(std::wstring &member)
    {
      member.clear();
    }

    template <>
    inline void ProxyPack<String>(String &member, String &value)
    {
      member = value.c_str();
    }

    template <>
    inline void ProxyClean<String>(String &member)
    {
      member.clear();
    }

  }
}

#endif //ZSLIB_INTERNAL_PROXYPACK_H_3d1989252cf7499ccb86cef6b9c76857
