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

#include <zsLib/zsHelpers.h>

//#include <boost/test/unit_test_suite.hpp>
//#include <boost/test/unit_test.hpp>
//#include <boost/test/test_tools.hpp>

#include "boost_replacement.h"

BOOST_AUTO_TEST_SUITE(zsLibStringTest)

  BOOST_AUTO_TEST_CASE(Test_PUID_GUID)
  {
    zsLib::PUID puid1 = zsLib::createPUID();
    zsLib::PUID puid2 = zsLib::createPUID();

    zsLib::UUID uuid1 = zsLib::createUUID();
    zsLib::UUID uuid2 = zsLib::createUUID();

    BOOST_CHECK(sizeof(puid1) == sizeof(zsLib::PUID))
    BOOST_CHECK(puid1 != puid2)

    BOOST_CHECK(sizeof(uuid1) == sizeof(zsLib::UUID))
    BOOST_CHECK(uuid1 != uuid2)
  }

  BOOST_AUTO_TEST_CASE(Test_atomic_inc_dec)
  {
    zsLib::ULONG value = 0;
    zsLib::ULONG value1 = zsLib::atomicIncrement(value);
    BOOST_EQUAL(1, value);
    BOOST_EQUAL(1, value1);
    BOOST_EQUAL(1, zsLib::atomicGetValue(value));
    zsLib::ULONG value2 = zsLib::atomicIncrement(value);
    BOOST_EQUAL(2, value);
    BOOST_EQUAL(2, value2);
    BOOST_EQUAL(2, zsLib::atomicGetValue(value));
    zsLib::ULONG value3 = zsLib::atomicDecrement(value);
    BOOST_EQUAL(1, value);
    BOOST_EQUAL(1, value3);
    BOOST_EQUAL(1, zsLib::atomicGetValue(value));
    zsLib::ULONG value4 = zsLib::atomicDecrement(value);
    BOOST_EQUAL(0, value);
    BOOST_EQUAL(0, value4);
    BOOST_EQUAL(0, zsLib::atomicGetValue(value));
  }

  BOOST_AUTO_TEST_CASE(TestHelper_atomic_get_set)
  {
    zsLib::DWORD value = 0;
    BOOST_EQUAL(0, zsLib::atomicGetValue32(value))
    zsLib::atomicSetValue32(value, 1);
    BOOST_EQUAL(1, zsLib::atomicGetValue32(value))
    zsLib::atomicSetValue32(value, 0xFFFFFFFF);
    BOOST_EQUAL(0xFFFFFFFF, zsLib::atomicGetValue32(value))
  }

BOOST_AUTO_TEST_SUITE_END()
