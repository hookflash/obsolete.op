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

#include "boost_replacement.h"
#include "config.h"

#include <zsLib/types.h>
#include <zsLib/helpers.h>
#include <hookflash/services/IHelper.h>

#include <iostream>


typedef hookflash::services::IHelper IHelper;

void doTestCanonicalXML();
void doTestDNS();
void doTestICESocket();
void doTestSTUNDiscovery();
void doTestTURNSocket();
void doTestRUDPListener();
void doTestRUDPICESocket();
void doTestRUDPICESocketLoopback();


namespace BoostReplacement
{
  zsLib::ULONG &getGlobalPassedVar()
  {
    static zsLib::ULONG value = 0;
    return value;
  }
  
  zsLib::ULONG &getGlobalFailedVar()
  {
    static zsLib::ULONG value = 0;
    return value;
  }
  
  void passed()
  {
    zsLib::atomicIncrement(getGlobalPassedVar());
  }
  void failed()
  {
    zsLib::atomicIncrement(getGlobalFailedVar());
  }
  
  void installLogger()
  {
    BOOST_STDOUT() << "INSTALLING LOGGER...\n\n";
    IHelper::setLogLevel("zsLib", zsLib::Log::Trace);
    IHelper::setLogLevel("hookflash_services", zsLib::Log::Trace);

    if (HOOKFLASH_SERVICE_TEST_USE_STDOUT_LOGGING) {
      IHelper::installStdOutLogger(false);
    }

    if (HOOKFLASH_SERVICE_TEST_USE_FIFO_LOGGING) {
      IHelper::installFileLogger(HOOKFLASH_SERVICE_TEST_FIFO_LOGGING_FILE, true);
    }

    if (HOOKFLASH_SERVICE_TEST_USE_TELNET_LOGGING) {
      IHelper::installTelnetLogger(HOOKFLASH_SERVICE_TEST_TELNET_LOGGING_PORT, 60, true);
    }

    if (HOOKFLASH_SERVICE_TEST_USE_DEBUGGER_LOGGING) {
      IHelper::installDebuggerLogger();
    }

    BOOST_STDOUT() << "INSTALLED LOGGER...\n\n";
  }
  
  void uninstallLogger()
  {
    BOOST_STDOUT() << "REMOVING LOGGER...\n\n";

    if (HOOKFLASH_SERVICE_TEST_USE_STDOUT_LOGGING) {
      IHelper::uninstallStdOutLogger();
    }
    if (HOOKFLASH_SERVICE_TEST_USE_FIFO_LOGGING) {
      IHelper::uninstallFileLogger();
    }
    if (HOOKFLASH_SERVICE_TEST_USE_TELNET_LOGGING) {
      IHelper::uninstallTelnetLogger();
    }
    if (HOOKFLASH_SERVICE_TEST_USE_DEBUGGER_LOGGING) {
      IHelper::uninstallDebuggerLogger();
    }

    BOOST_STDOUT() << "REMOVED LOGGER...\n\n";
  }
  
  void output()
  {
    BOOST_STDOUT() << "PASSED:       [" << BoostReplacement::getGlobalPassedVar() << "]\n";
    if (0 != BoostReplacement::getGlobalFailedVar()) {
      BOOST_STDOUT() << "***FAILED***: [" << BoostReplacement::getGlobalFailedVar() << "]\n";
    }
  }
  
  void runAllTests()
  {
    BOOST_RUN_TEST_FUNC(doTestCanonicalXML)
    BOOST_RUN_TEST_FUNC(doTestDNS)
    BOOST_RUN_TEST_FUNC(doTestICESocket)
    BOOST_RUN_TEST_FUNC(doTestSTUNDiscovery)
    BOOST_RUN_TEST_FUNC(doTestTURNSocket)
    BOOST_RUN_TEST_FUNC(doTestRUDPICESocketLoopback)
    BOOST_RUN_TEST_FUNC(doTestRUDPListener)
    BOOST_RUN_TEST_FUNC(doTestRUDPICESocket)
  }
}
