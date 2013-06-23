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

void doFakeGUITest();
void doMediaEngineTest();

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
    std::cout << "INSTALLING LOGGER...\n\n";
    IHelper::setLogLevel("zsLib", zsLib::Log::Trace);
    IHelper::setLogLevel("hookflash_services", zsLib::Log::Trace);
    
    if (HOOKFLASH_CORE_TEST_USE_STDOUT_LOGGING) {
      IHelper::installStdOutLogger(false);
    }
    
    if (HOOKFLASH_CORE_TEST_USE_FIFO_LOGGING) {
      IHelper::installFileLogger(HOOKFLASH_CORE_TEST_FIFO_LOGGING_FILE, true);
    }
    
    if (HOOKFLASH_CORE_TEST_USE_TELNET_LOGGING) {
      IHelper::installTelnetLogger(HOOKFLASH_CORE_TEST_TELNET_LOGGING_PORT, 60, true);
    }
    
    std::cout << "INSTALLED LOGGER...\n\n";
  }
  
  void uninstallLogger()
  {
    std::cout << "REMOVING LOGGER...\n\n";
    
    if (HOOKFLASH_CORE_TEST_USE_STDOUT_LOGGING) {
      IHelper::uninstallStdOutLogger();
    }
    if (HOOKFLASH_CORE_TEST_USE_FIFO_LOGGING) {
      IHelper::uninstallFileLogger();
    }
    if (HOOKFLASH_CORE_TEST_USE_TELNET_LOGGING) {
      IHelper::uninstallTelnetLogger();
    }
    
    std::cout << "REMOVED LOGGER...\n\n";
  }
  
  void output()
  {
    std::cout << "PASSED:       [" << BoostReplacement::getGlobalPassedVar() << "]\n";
    if (0 != BoostReplacement::getGlobalFailedVar()) {
      std::cout << "***FAILED***: [" << BoostReplacement::getGlobalFailedVar() << "]\n";
    }
  }
  
  void runAllTests()
  {
    //doFakeGUITest();
    doMediaEngineTest();
  }
}
