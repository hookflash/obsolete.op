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

#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>
#include <zsLib/Numeric.h>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>


#include "boost_replacement.h"

using zsLib::BYTE;
using zsLib::LONGLONG;
using zsLib::ULONGLONG;

BOOST_AUTO_TEST_SUITE(zsLibStringize)

  BOOST_AUTO_TEST_CASE(TestStringize)
  {
    BOOST_EQUAL("0", zsLib::Stringize<char>(0).string());
    BOOST_EQUAL("-128", zsLib::Stringize<char>(-128).string());
    BOOST_EQUAL("127", zsLib::Stringize<char>(127).string());
    BOOST_EQUAL("80", zsLib::Stringize<char>(-128, 16).string());

    BOOST_EQUAL("0", zsLib::Stringize<unsigned char>(0).string());
    BOOST_EQUAL("255", zsLib::Stringize<unsigned char>(255).string());
    BOOST_EQUAL("ff", zsLib::Stringize<unsigned char>(0xFF, 16).string());

    BOOST_EQUAL("ffff", zsLib::Stringize<unsigned short>(0xFFFF, 16).string());
    BOOST_EQUAL("ffffffff", zsLib::Stringize<unsigned int>(0xFFFFFFFF, 16).string());
    #define COMMENTED_BECAUSE_32BIT_COMPILE 1
    #define COMMENTED_BECAUSE_32BIT_COMPILE 2
    //BOOST_EQUAL("ffffffffffffffff", zsLib::Stringize<ULONGLONG>(0xFFFFFFFFFFFFFFFF, 16).string());

    BOOST_EQUAL("f0f0", zsLib::Stringize<unsigned short>(0xF0F0, 16).string());
    BOOST_EQUAL("f0f0f0f0", zsLib::Stringize<unsigned int>(0xF0F0F0F0, 16).string());
    #define COMMENTED_BECAUSE_32BIT_COMPILE 3
    #define COMMENTED_BECAUSE_32BIT_COMPILE 4
    //BOOST_EQUAL("f0f0f0f0f0f0f0f0", zsLib::Stringize<ULONGLONG>(0xF0F0F0F0F0F0F0F0, 16).string());

    BOOST_EQUAL("0", zsLib::Stringize<float>(0.0f).string());
    BOOST_EQUAL("0.5", zsLib::Stringize<float>(0.5).string());
    BOOST_EQUAL("-0.5", zsLib::Stringize<float>(-0.5).string());

    BOOST_EQUAL("0", zsLib::Stringize<double>(0.0).string());
    BOOST_EQUAL("123456.789", zsLib::Stringize<double>(123456.789).string());
    BOOST_EQUAL("-123456.789", zsLib::Stringize<double>(-123456.789).string());

    zsLib::UUID uuid = zsLib::Numeric<zsLib::UUID>("b0a01e87-2be5-4daa-8155-1380c98400a1");
    zsLib::String uuidStr = zsLib::Stringize<zsLib::UUID>(uuid);
    BOOST_EQUAL("b0a01e87-2be5-4daa-8155-1380c98400a1", uuidStr);
  }

BOOST_AUTO_TEST_SUITE_END()
