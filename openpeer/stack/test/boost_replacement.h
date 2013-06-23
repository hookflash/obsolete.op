/*
 
 Copyright (c) 2013, SMB Phone Inc.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */
#ifndef BOOST_AUTO_TEST_SUITE

#include <zsLib/types.h>

#include <iostream>

namespace BoostReplacement
{
  zsLib::ULONG &getGlobalPassedVar();
  zsLib::ULONG &getGlobalFailedVar();

  void installLogger();
  void uninstallLogger();
  void passed();
  void failed();
  void output();
  void runAllTests();
}

#define BOOST_INSTALL_LOGGER()                                             \
  {BoostReplacement::installLogger();}

#define BOOST_UNINSTALL_LOGGER()                                           \
  {BoostReplacement::uninstallLogger();}

#define BOOST_AUTO_TEST_SUITE(xParam) namespace xParam {

#define BOOST_AUTO_TEST_SUITE_END() }

#define BOOST_AUTO_TEST_CASE(xTestCase) static struct Test_##xTestCase {   \
           Test_##xTestCase()                                              \
           {                                                               \
             std::cout << "STARTING:     " #xTestCase "\n";                \
             try                                                           \
             { test_func(); }                                              \
             catch(...)                                                    \
             { std::cout << "***UNCAUGHT EXCEPTION IN***: " #xTestCase "\n"; BoostReplacement::failed(); }   \
             std::cout << "ENDING:       " #xTestCase "\n\n";              \
           }                                                               \
           void test_func();                                               \
         } g_Test_##xTestCase;                                             \
         \
        void Test_##xTestCase::test_func()

#define BOOST_CHECK(xValue)                                         \
  if (!(xValue))                                                    \
  { std::cout << "***FAILED***: " #xValue "\n"; BoostReplacement::failed(); }                   \
  else                                                              \
  { std::cout << "PASSED:       " #xValue "\n"; BoostReplacement::passed(); }

#define BOOST_EQUAL(xValue1, xValue2)                               \
  if (!((xValue1) == (xValue2)))                                    \
  { std::cout << "***FAILED***: " #xValue1 " == " #xValue2 ", V1=" << (xValue1) << ", V2=" << (xValue2) << "\n"; BoostReplacement::failed(); }                   \
  else                                                              \
  { std::cout << "PASSED:       " #xValue1 " == " #xValue2 ", V1=" << (xValue1) << ", V2=" << (xValue2) << "\n"; BoostReplacement::passed(); }

#define BOOST_RUN_TEST_FUNC(xTestCase) \
{                                                               \
std::cout << "STARTING:     " #xTestCase "\n";                \
try                                                           \
{ xTestCase(); }                                              \
catch(...)                                                    \
{ std::cout << "***UNCAUGHT EXCEPTION IN***: " #xTestCase "\n"; BoostReplacement::failed(); }   \
std::cout << "ENDING:       " #xTestCase "\n\n";              \
}                                                               \

#endif //BOOST_AUTO_TEST_SUITE
