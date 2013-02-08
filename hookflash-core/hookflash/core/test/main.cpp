

#include <zsLib/helpers.h>
#include <zsLib/Log.h>

#include <iostream>
#include <fstream>

#include "boost_replacement.h"

namespace hfcoretest { ZS_IMPLEMENT_SUBSYSTEM(hfcoretest) }

void doFakeGUITest();

int main (int argc, char * const argv[]) {
  
  bool failed = false;
  
  // insert code here...
  std::cout << "PASSED:       [" << BoostReplacement::getGlobalPassedVar() << "]\n";
  if (0 != BoostReplacement::getGlobalFailedVar()) {
    std::cout << "***FAILED***: [" << BoostReplacement::getGlobalFailedVar() << "]\n";
    failed = true;
  }
  
  if (failed)
    return -1;
  else
    return 0;
}
