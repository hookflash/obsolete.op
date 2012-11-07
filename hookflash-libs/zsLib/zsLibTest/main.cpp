
#include <iostream>

#include <zsLib/zsHelpers.h>

namespace BoostReplacement
{
  static zsLib::ULONG &getGlobalPassedVar()
  {
    static zsLib::ULONG value = 0;
    return value;
  }

  static zsLib::ULONG &getGlobalFailedVar()
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
}

int main (int argc, char * const argv[]) {
  // insert code here...
  std::cout << "PASSED:       [" << BoostReplacement::getGlobalPassedVar() << "]\n";
  if (0 != BoostReplacement::getGlobalFailedVar()) {
    std::cout << "***FAILED***: [" << BoostReplacement::getGlobalFailedVar() << "]\n";
    return -1;
  }
  return 0;
}

/*

 //DYLD_FALLBACK_LIBRARY_PATH
 #define BOOST_TEST_DYN_LINK
 #define BOOST_TEST_MODULE my application tests
 #include <boost/test/unit_test.hpp>

 int add( int i, int j ) { return i+j; }

 BOOST_AUTO_TEST_CASE( my_test )
 {
 // seven ways to detect and report the same error:
 BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error

 BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error

 if( add( 2,2 ) != 4 )
 BOOST_ERROR( "Ouch..." );            // #3 continues on error

 if( add( 2,2 ) != 4 )
 BOOST_FAIL( "Ouch..." );             // #4 throws on error

 if( add( 2,2 ) != 4 ) throw "Ouch..."; // #5 throws on error

 BOOST_CHECK_MESSAGE( add( 2,2 ) == 4,  // #6 continues on error
 "add(..) result: " << add( 2,2 ) );

 BOOST_CHECK_EQUAL( add( 2,2 ), 4 );	  // #7 continues on error
 }

 */

