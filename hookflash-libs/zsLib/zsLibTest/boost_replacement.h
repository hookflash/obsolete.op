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

#ifndef BOOST_AUTO_TEST_SUITE

#include <zsLib/types.h>
#include <iostream>
#include <sstream>

#ifdef __QNX__
#include <QDebug>
#endif //__QNX__

namespace BoostReplacement
{
  zsLib::ULONG &getGlobalPassedVar();
  zsLib::ULONG &getGlobalFailedVar();

  void passed();
  void failed();
  void output();
}

#ifdef __QNX__
#define BOOST_STDOUT() (qDebug())
#else
#define BOOST_STDOUT() (std::cout)
#endif //__QNX___

#define BOOST_AUTO_TEST_SUITE(xParam) namespace xParam {

#define BOOST_AUTO_TEST_SUITE_END() }

#define BOOST_AUTO_TEST_CASE(xTestCase) static struct Test_##xTestCase {   \
    Test_##xTestCase()                                              \
    {                                                               \
      BOOST_STDOUT() << "STARTING:     " #xTestCase "\n";  \
      try                                                           \
      { test_func(); }                                              \
      catch(...)                                                    \
      { BOOST_STDOUT() << "***UNCAUGHT EXCEPTION IN***: " #xTestCase "\n"; BoostReplacement::failed(); }   \
      std::cout << "ENDING:       " #xTestCase "\n\n";              \
    }                                                               \
    void test_func();                                               \
  } g_Test_##xTestCase;                                             \
\
  void Test_##xTestCase::test_func()

#define BOOST_CHECK(xValue)                                           \
  {                                                                   \
    if (!(xValue))                                                    \
    { BOOST_STDOUT() << "***FAILED***: " #xValue "\n"; BoostReplacement::failed(); } \
    else                                                                             \
    { BOOST_STDOUT() << "PASSED:       " #xValue "\n"; BoostReplacement::passed(); } \
  }

#define BOOST_EQUAL(xValue1, xValue2)                               \
  if (!((xValue1) == (xValue2)))                                    \
  { std::stringstream sv1, sv2; sv1 << (xValue1); sv2 << (xValue2); BOOST_STDOUT() << "***FAILED***: " #xValue1 " == " #xValue2 ", V1=" << (sv1.str().c_str()) << ", V2=" << (sv2.str().c_str()) << "\n"; BoostReplacement::failed(); }                   \
  else                                                              \
  { std::stringstream sv1, sv2; sv1 << (xValue1); sv2 << (xValue2); BOOST_STDOUT() << "PASSED:       " #xValue1 " == " #xValue2 ", V1=" << (sv1.str().c_str()) << ", V2=" << (sv2.str().c_str()) << "\n"; BoostReplacement::passed(); }

#endif //BOOST_AUTO_TEST_SUITE
