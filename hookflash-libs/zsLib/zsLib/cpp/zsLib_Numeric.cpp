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

#include <zsLib/Numeric.h>
#include <zsLib/types.h>
#include <zsLib/Exception.h>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/string_generator.hpp>

#pragma warning(push)
#pragma warning(disable:4290)

namespace zsLib {ZS_DECLARE_SUBSYSTEM(zsLib)}

using namespace std;

namespace zsLib
{
  template<typename t_type>
  void Numeric<t_type>::get(t_type &outValue) const throw (ValueOutOfRange)
  {
    if (std::numeric_limits<t_type>::is_signed) {
      LONGLONG result = 0;
      bool converted = internal::convert(mData, result, sizeof(t_type), mIngoreWhitespace, mBase);
      if (!converted)
        ZS_THROW_CUSTOM(ValueOutOfRange, "Cannot convert value (generic): " + mData)
      outValue = static_cast<t_type>(result);
      return;
    }
    ULONGLONG result = 0;
    bool converted = internal::convert(mData, result, sizeof(t_type), mIngoreWhitespace, mBase);
    if (!converted)
      ZS_THROW_CUSTOM(ValueOutOfRange, "Cannot convert value: " + mData)
    outValue = static_cast<t_type>(result);
  }

  template<>
  void Numeric<bool>::get(bool &outValue) const throw (ValueOutOfRange)
  {
    bool result = 0;
    bool converted = internal::convert(mData, result, mIngoreWhitespace);
    if (!converted)
      ZS_THROW_CUSTOM(ValueOutOfRange, "Cannot convert value (bool): " + mData)
    outValue = result;
  }

  template<>
  void Numeric<float>::get(float &outValue) const throw (ValueOutOfRange)
  {
    float result = 0;
    bool converted = internal::convert(mData, result, mIngoreWhitespace);
    if (!converted)
      ZS_THROW_CUSTOM(ValueOutOfRange, "Cannot convert value (float): " + mData)
    outValue = result;
  }

  template<>
  void Numeric<double>::get(double &outValue) const throw (ValueOutOfRange)
  {
    double result = 0;
    bool converted = internal::convert(mData, result, mIngoreWhitespace);
    if (!converted)
      ZS_THROW_CUSTOM(ValueOutOfRange, "Cannot convert value (double): " + mData)
    outValue = result;
  }

  template<>
  void Numeric<UUID>::get(UUID &outValue) const throw (ValueOutOfRange)
  {
    UUID result;
    bool converted = internal::convert(mData, result, mIngoreWhitespace);
    if (!converted)
      ZS_THROW_CUSTOM(ValueOutOfRange, "Cannot convert value (UUID): " + mData)
    outValue = result;
  }

  template<>
  void Numeric<Time>::get(Time &outValue) const throw (ValueOutOfRange)
  {
    Time result;
    bool converted = internal::convert(mData, result, mIngoreWhitespace);
    if (!converted)
      ZS_THROW_CUSTOM(ValueOutOfRange, "Cannot convert value (Time): " + mData)
    outValue = result;
  }

  // force templates to be generated for these types
  template void Numeric<CHAR>::get(CHAR &) const throw (ValueOutOfRange);
  template void Numeric<UCHAR>::get(UCHAR &) const throw (ValueOutOfRange);
  template void Numeric<SHORT>::get(SHORT &) const throw (ValueOutOfRange);
  template void Numeric<USHORT>::get(USHORT &) const throw (ValueOutOfRange);
  template void Numeric<INT>::get(INT &) const throw (ValueOutOfRange);
  template void Numeric<UINT>::get(UINT &) const throw (ValueOutOfRange);
  template void Numeric<LONG>::get(LONG &) const throw (ValueOutOfRange);
  template void Numeric<ULONG>::get(ULONG &) const throw (ValueOutOfRange);
  template void Numeric<LONGLONG>::get(LONGLONG &) const throw (ValueOutOfRange);
  template void Numeric<ULONGLONG>::get(ULONGLONG &) const throw (ValueOutOfRange);
#ifndef _WIN32
  template void Numeric<bool>::get(bool &) const throw (ValueOutOfRange);
  template void Numeric<float>::get(float &) const throw (ValueOutOfRange);
  template void Numeric<double>::get(double &) const throw (ValueOutOfRange);
  template void Numeric<UUID>::get(UUID &) const throw (ValueOutOfRange);
#endif //_WIN32

  namespace internal
  {
    // 0    1    2    3    4   5    6    7    8    9    A    B    C    D    E    F
    //NUL  SOH  STX  ETX  EOT  ENQ  ACK  BEL  BS   HT   LF   VT   FF   CR   SO   SI   // 0
    //DLE  DC1  DC2  DC3  DC4  NAK  SYN  ETB  CAN  EM   SUB  ESC  FS   GS   RS   US   // 1
    // SP   !    "    #    $    %    &    '    (    )    *    +    ,    -    .    /   // 2
    // 0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?   // 3
    // @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O   // 4
    // P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _   // 5
    // `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o   // 6
    // p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~   DEL  // 7
    static BYTE gDigitToValue[256] = {
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 0
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 1
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 2
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 3
      0xFF,10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,   // 4
      25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  0xFF,0xFF,0xFF,0xFF,0xFF, // 5
      0xFF,10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,   // 6
      25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  0xFF,0xFF,0xFF,0xFF,0xFF, // 7

      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 8
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 9
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 10
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 11
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 12
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 13
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 14
      0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 15
    };

    static LONGLONG gMinLongLongs[] =
    {
      0xFFFFFFFFFFFFFF80LL,  // 1 bytes
      0xFFFFFFFFFFFF8000LL,  // 2 bytes
      0xFFFFFFFFFF800000LL,  // 3 bytes
      0xFFFFFFFF80000000LL,  // 4 bytes
      0xFFFFFF8000000000LL,  // 5 bytes
      0xFFFF800000000000LL,  // 6 bytes
      0xFF80000000000000LL,  // 7 bytes
      0x8000000000000000LL   // 8 bytes
    };

    static LONGLONG gMaxLongLongs[] =
    {
      0x000000000000007FLL,  // 1 bytes
      0x0000000000007FFFLL,  // 2 bytes
      0x00000000007FFFFFLL,  // 3 bytes
      0x000000007FFFFFFFLL,  // 4 bytes
      0x0000007FFFFFFFFFLL,  // 5 bytes
      0x00007FFFFFFFFFFFLL,  // 6 bytes
      0x007FFFFFFFFFFFFFLL,  // 7 bytes
      0x7FFFFFFFFFFFFFFFLL   // 8 bytes
    };

    static ULONGLONG gMaxULongLongs[] =
    {
      0x00000000000000FFLL,  // 1 bytes
      0x000000000000FFFFLL,  // 2 bytes
      0x0000000000FFFFFFLL,  // 3 bytes
      0x00000000FFFFFFFFLL,  // 4 bytes
      0x000000FFFFFFFFFFLL,  // 5 bytes
      0x0000FFFFFFFFFFFFLL,  // 6 bytes
      0x00FFFFFFFFFFFFFFLL,  // 7 bytes
      0xFFFFFFFFFFFFFFFFLL   // 8 bytes
    };

    void skipSpace(CSTR &ioStr)
    {
      while (isspace(*ioStr)) {
        ++ioStr;
      }
    }

    bool isPositive(CSTR &ioStr, bool &hadSign)
    {
      hadSign = false;
      if ('-' == *ioStr) {
        ++ioStr;
        hadSign = true;
        return false;
      }
      if ('+' == *ioStr) {
        ++ioStr;
        hadSign = true;
        return true;
      }
      return true;
    }

    size_t shouldForceBase(CSTR &ioStr, size_t base, bool hadSign)
    {
      if ('x' == ioStr[1]) {
        ioStr += 2;
        return 16;
      }
      return base;
    }

    bool convert(const String &input, LONGLONG &outResult, size_t size, bool ignoreWhiteSpace, size_t base)
    {
      ZS_THROW_INVALID_USAGE_IF((0 == size) || (size > sizeof(QWORD)))

      CSTR str = input;
      if (NULL == str)
        return false;

      if (ignoreWhiteSpace)
        skipSpace(str);

      bool hadSign = false;
      bool positive = isPositive(str, hadSign);
      base = shouldForceBase(str, base, hadSign);

      LONGLONG result = 0;

      BYTE digit = 0;
      if (positive) {
        while ((digit = gDigitToValue[*str]) < base) {
          LONGLONG lastResult = result;

          result *= base;
          result += static_cast<LONGLONG>(digit);
          if (result < lastResult)
            return false; // value had to flip
          ++str;
        }
        if (result > gMaxLongLongs[size-1])
          return false;
      } else {
        while ((digit = gDigitToValue[*str]) < base) {
          LONGLONG lastResult = result;

          result *= base;
          result -= static_cast<LONGLONG>(digit);
          if (lastResult < result)
            return false; // value had to flip
          ++str;
        }
        if (result < gMinLongLongs[size-1])
          return false;
      }

      if (ignoreWhiteSpace)
        skipSpace(str);

      if (0 != *str)
        return false;

      outResult = result;
      return true;
    }

    bool convert(const String &input, ULONGLONG &outResult, size_t size, bool ignoreWhiteSpace, size_t base)
    {
      ZS_THROW_INVALID_USAGE_IF((0 == size) || (size > sizeof(QWORD)))

      CSTR str = input;
      if (NULL == str)
        return false;

      if (ignoreWhiteSpace)
        skipSpace(str);

      bool hadSign = false;
      bool positive = isPositive(str, hadSign);
      if (!positive)
        return false;
      base = shouldForceBase(str, base, hadSign);

      ULONGLONG result = 0;

      BYTE digit = 0;
      while ((digit = gDigitToValue[*str]) < base) {
        ULONGLONG lastResult = result;

        result *= base;
        result += static_cast<ULONGLONG>(digit);
        if (result < lastResult)
          return false; // value had to flip
        ++str;
      }
      if (result > gMaxULongLongs[size-1])
        return false;

      if (ignoreWhiteSpace)
        skipSpace(str);

      if (0 != *str)
        return false;

      outResult = result;
      return true;
    }

    bool convert(const String &input, bool &outResult, bool ignoreWhiteSpace)
    {
      String temp = input;
      if (ignoreWhiteSpace)
        temp.trim();

      if ((0 == input.compareNoCase("true")) ||
          (0 == input.compareNoCase("t"))) {
        outResult = true;
        return true;
      }

      if ((0 == input.compareNoCase("false")) ||
          (0 == input.compareNoCase("f"))) {
        outResult = false;
        return true;
      }

      LONGLONG value = 0;
      bool result = convert(input, value, sizeof(LONGLONG), ignoreWhiteSpace, 10);
      if (!result)
        return false;
      outResult = (value == 0 ? false : true);
      return true;
    }

    bool convert(const String &input, float &outResult, bool ignoreWhiteSpace)
    {
      String temp = input;
      if (ignoreWhiteSpace)
        temp.trim();

      try {
        float result = boost::lexical_cast<float>(temp);
        outResult = result;
      } catch(...) {
        return false;
      }
      return true;
    }

    bool convert(const String &input, double &outResult, bool ignoreWhiteSpace)
    {
      String temp = input;
      if (ignoreWhiteSpace)
        temp.trim();

      try {
        double result = boost::lexical_cast<double>(temp);
        outResult = result;
      } catch(...) {
        return false;
      }
      return true;
    }

    bool convert(const String &input, UUID &outResult, bool ignoreWhiteSpace)
    {
      String temp = input;
      if (ignoreWhiteSpace)
        temp.trim();

      try {
        boost::uuids::string_generator gen;
        outResult = gen(temp);
      } catch (...) {
        return false;
      }
      return true;
    }

    bool convert(const String &input, Time &outResult, bool ignoreWhiteSpace)
    {
      String temp = input;
      if (ignoreWhiteSpace)
        temp.trim();

      try {
        Time result = boost::posix_time::time_from_string(input);
        if (result.is_not_a_date_time()) return false;
        outResult = result;
      } catch (boost::bad_lexical_cast &) {
        return  false;
      }
      return true;
    }
  }
}

#pragma warning(pop)
