#include <iostream>
#include <string>
#define BOOST_DISABLE_ASSERTS 1
#include "boost/smart_ptr.hpp"
#include "/Users/lawrence.gunn/Documents/op/hookflash-core/projects/bb/hfbbcore/src/hfcore.h"

using namespace std;

namespace hookflash {
  namespace blackberry {

  	  class PrintStuff
  	  {
  	  public:
  		  PrintStuff() {
  		  }

  		  void DoPrint(const std::string& text)
  		  {
  			::HFCorePrint(text.c_str());
  		  }
  	  };
  }
}

extern "C" void printStuff(const char* str);

void printStuff(const char* str)
{
	boost::shared_ptr<hookflash::blackberry::PrintStuff> printer(new hookflash::blackberry::PrintStuff());
	printer->DoPrint(str);
}
