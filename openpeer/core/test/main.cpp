

#include <iostream>
#include <fstream>

#include "boost_replacement.h"

int main (int argc, char * const argv[])
{
  
  // insert code here...

  std::cout << "TEST NOW STARTING...\n\n";

  BoostReplacement::runAllTests();
  BoostReplacement::output();

  if (0 != BoostReplacement::getGlobalFailedVar()) {
    return -1;
  }

  return 0;
}
