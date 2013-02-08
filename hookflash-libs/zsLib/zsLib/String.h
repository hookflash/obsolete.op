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

#ifndef ZSLIB_STRING_H_8f1651797cde55890f27c67aa67d9976
#define ZSLIB_STRING_H_8f1651797cde55890f27c67aa67d9976

#include <string>

#include <zsLib/types.h>

namespace zsLib
{
  class String : public std::string {

  public:
    String();
    String(const String &);

    String(CSTR);
    String(CWSTR);
    String(const std::string &);
    explicit String(const std::wstring &);

    static String copyFrom(CSTR, size_t maxCharacters);
    static String copyFromUnicodeSafe(CSTR, size_t maxCharacters);

    std::wstring wstring() const;

    bool isEmpty() const;
    bool hasData() const;
    size_t getLength() const;

    operator CSTR() const;

    String &operator=(const std::string &);
    String &operator=(const std::wstring &);
    String &operator=(const String &);
    String &operator=(CSTR);
    String &operator=(CWSTR);

    int compareNoCase(CSTR) const;
    int compareNoCase(const String &) const;

    void toLower();
    void toUpper();

    void trim(CSTR chars = " \t\r\n\v\f");
    void trimLeft(CSTR chars = " \t\r\n\v\f");
    void trimRight(CSTR chars = " \t\r\n\v\f");

    size_t lengthUnicodeSafe() const;
    String substrUnicodeSafe(size_t pos = 0, size_t n = std::string::npos ) const;
    WCHAR atUnicodeSafe(size_t pos) const;

    void replaceAll(CSTR findStr, CSTR replaceStr, size_t totalOccurances = std::string::npos);
  };
}

#endif //ZSLIB_STRING_H_8f1651797cde55890f27c67aa67d9976
