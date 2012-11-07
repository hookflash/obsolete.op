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

#include <zsLib/Numeric.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>


#include "boost_replacement.h"

using zsLib::BYTE;
using zsLib::LONGLONG;
using zsLib::ULONGLONG;

BOOST_AUTO_TEST_SUITE(zsLibNumeric)

  BOOST_AUTO_TEST_CASE(TestNumeric)
  {
    BOOST_EQUAL(0, (char)zsLib::Numeric<char>("0"));
    BOOST_EQUAL(127, (char)zsLib::Numeric<char>("127"));
    BOOST_EQUAL(127, (char)zsLib::Numeric<char>("+127"));
    BOOST_EQUAL(-128, (char)zsLib::Numeric<char>("-128"));
    BOOST_EQUAL(127, (char)zsLib::Numeric<char>("0x7F"));
    BOOST_EQUAL(127, (char)zsLib::Numeric<char>("0x7f"));
    BOOST_EQUAL(127, (char)zsLib::Numeric<char>(" 7F ", true, 16));
    BOOST_EQUAL(-128, (char)zsLib::Numeric<char>("-0x80"));
    BOOST_EQUAL(0x80, (unsigned char)zsLib::Numeric<unsigned char>("0x80"));

    BOOST_EQUAL(0, (unsigned char)zsLib::Numeric<unsigned char>("0"));
    BOOST_EQUAL(255, (unsigned char)zsLib::Numeric<unsigned char>("255"));

    BOOST_EQUAL(32767, (short)zsLib::Numeric<short>("32767"));
    BOOST_EQUAL(-32768, (short)zsLib::Numeric<short>("-32768"));

    BOOST_EQUAL(0, (unsigned short)zsLib::Numeric<unsigned short>("0"));
    BOOST_EQUAL(65535, (unsigned short)zsLib::Numeric<unsigned short>("65535"));

    BOOST_EQUAL(2147483647, (int)zsLib::Numeric<int>("2147483647"));
#pragma warning(push)
#pragma warning(disable:4146)
    BOOST_EQUAL(-2147483648, (int)zsLib::Numeric<int>("-2147483648"));
#pragma warning(pop)

    BOOST_EQUAL(0, (unsigned int)zsLib::Numeric<unsigned int>("0"));
    BOOST_EQUAL(4294967295, (unsigned int)zsLib::Numeric<unsigned int>("4294967295"));

    #define COMMENTED_BECAUSE_32BIT_COMPILE 1
    #define COMMENTED_BECAUSE_32BIT_COMPILE 2
    //BOOST_EQUAL(0x7FFFFFFFFFFFFFFFL, (LONGLONG)zsLib::Numeric<LONGLONG>("9223372036854775807"));
    //BOOST_EQUAL(0x8000000000000000L, (LONGLONG)zsLib::Numeric<LONGLONG>("-9223372036854775808"));

    //BOOST_EQUAL(0x7FFFFFFFFFFFFFFFL, (LONGLONG)zsLib::Numeric<LONGLONG>("0x7FFFFFFFFFFFFFFF"));
    //BOOST_EQUAL(0x8000000000000000L, (LONGLONG)zsLib::Numeric<LONGLONG>("-0x8000000000000000"));

    BOOST_EQUAL(0x0, (ULONGLONG)zsLib::Numeric<ULONGLONG>("0"));
    #define COMMENTED_BECAUSE_32BIT_COMPILE 3
    #define COMMENTED_BECAUSE_32BIT_COMPILE 4
    //BOOST_EQUAL(0xFFFFFFFFFFFFFFFFL, (ULONGLONG)zsLib::Numeric<ULONGLONG>("18446744073709551615"));

    BOOST_EQUAL(0x0, (ULONGLONG)zsLib::Numeric<ULONGLONG>("0x0"));
    #define COMMENTED_BECAUSE_32BIT_COMPILE 5
    #define COMMENTED_BECAUSE_32BIT_COMPILE 6
    //BOOST_EQUAL(0xFFFFFFFFFFFFFFFFL, (ULONGLONG)zsLib::Numeric<ULONGLONG>("0xFFFFFFFFFFFFFFFF"));

    BOOST_EQUAL(0.0f, (double)zsLib::Numeric<float>("0.0"));
    BOOST_EQUAL(-0.5f, (double)zsLib::Numeric<float>("-0.5"));
    BOOST_EQUAL(0.5f, (double)zsLib::Numeric<float>("0.5"));

    BOOST_EQUAL(0.0f, (double)zsLib::Numeric<double>("0.0"));
    BOOST_EQUAL(-123456.78, (double)zsLib::Numeric<double>("-123456.78"));
    BOOST_EQUAL(123456.78, (double)zsLib::Numeric<double>("123456.78"));

    zsLib::UUID uuid = zsLib::createUUID();
    zsLib::String uuidStr = zsLib::Stringize<zsLib::UUID>(uuid);
    BOOST_CHECK(!uuidStr.isEmpty());

    zsLib::UUID uuid2((zsLib::UUID)zsLib::Numeric<zsLib::UUID>(uuidStr));
    BOOST_EQUAL(uuid, uuid2);

    uuidStr = "{" + uuidStr + "}";
    zsLib::UUID uuid3((zsLib::UUID)zsLib::Numeric<zsLib::UUID>(uuidStr));
    BOOST_EQUAL(uuid, uuid3);

    { bool thrown = false; try { (char)zsLib::Numeric<char>("128"); } catch(zsLib::Numeric<char>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
    { bool thrown = false; try { (char)zsLib::Numeric<char>("-129"); } catch(zsLib::Numeric<char>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
    { bool thrown = false; try { (char)zsLib::Numeric<char>(" 7F ", false, 16); } catch(zsLib::Numeric<char>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }

    { bool thrown = false; try { (unsigned char)zsLib::Numeric<unsigned char>("-1"); } catch(zsLib::Numeric<unsigned char>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
    { bool thrown = false; try { (unsigned char)zsLib::Numeric<unsigned char>("256"); } catch(zsLib::Numeric<unsigned char>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }

    { bool thrown = false; try { (short)zsLib::Numeric<short>("32768"); } catch(zsLib::Numeric<short>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
    { bool thrown = false; try { (short)zsLib::Numeric<short>("-32769"); } catch(zsLib::Numeric<short>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }

    { bool thrown = false; try { (unsigned short)zsLib::Numeric<unsigned short>("65536"); } catch(zsLib::Numeric<unsigned short>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
    { bool thrown = false; try { (unsigned short)zsLib::Numeric<unsigned short>("-1"); } catch(zsLib::Numeric<unsigned short>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }

    { bool thrown = false; try { (int)zsLib::Numeric<int>("2147483648"); } catch(zsLib::Numeric<int>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
    { bool thrown = false; try { (int)zsLib::Numeric<int>("-2147483649"); } catch(zsLib::Numeric<int>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }

    { bool thrown = false; try { (unsigned int)zsLib::Numeric<unsigned int>("4294967296"); } catch(zsLib::Numeric<unsigned int>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
    { bool thrown = false; try { (unsigned int)zsLib::Numeric<unsigned int>("-1"); } catch(zsLib::Numeric<unsigned int>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }

    { bool thrown = false; try { (LONGLONG)zsLib::Numeric<LONGLONG>("9223372036854775808"); } catch(zsLib::Numeric<LONGLONG>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
    { bool thrown = false; try { (LONGLONG)zsLib::Numeric<LONGLONG>("-9223372036854775809"); } catch(zsLib::Numeric<LONGLONG>::ValueOutOfRange &) {thrown = true;} BOOST_CHECK(thrown); }
  }

BOOST_AUTO_TEST_SUITE_END()
