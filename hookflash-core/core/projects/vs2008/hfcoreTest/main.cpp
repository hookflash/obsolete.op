
#include <hookflash/hookflashTypes.h>
#include <hookflash/IClient.h>
#include <hookflash/internal/Client.h>
#include <hookflash/IStack.h>
#include <hookflash/IContact.h>
#include <hookflash/IAccount.h>
#include <hookflash/provisioning/IAccount.h>
#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/MessageQueueThread.h>
#include <iostream>
#include <curl/curl.h>
#include <hookflash/services/http/IHttp.h>

#include "FakeGUI.h"

#include "boost_replacement.h"
//#include <hookflash/stack/internal/openpeer_stack_PeerToBootstrapper.h>

using hookflash::IClient;
using hookflash::internal::Client;
using hookflash::IClientPtr;
using hookflash::IStack;
using hookflash::IStackPtr;
//using hookflash::openpeer::IAccount;
//using hookflash::openpeer::IAccountPtr;
using hookflash::provisioning::IAccount;
using hookflash::provisioning::IAccountPtr;
using hookflash::provisioning::IAccountLinkSocialSignin;
using hookflash::provisioning::IAccountLinkSocialSigninPtr;
using hookflash::provisioning::IAccountSocialInformation;
using hookflash::provisioning::IAccountSocialInformationPtr;

using namespace std;

using namespace hookflash;
using namespace hookflash::services::http;

using namespace hfcoretest;

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

class TestThread;
typedef boost::shared_ptr<TestThread> TestThreadPtr;
typedef boost::weak_ptr<TestThread> TestThreadWeakPtr;



namespace BoostReplacement
{

  class Logger;
  typedef boost::shared_ptr<Logger> LoggerPtr;
  typedef boost::weak_ptr<Logger> LoggerWeakPtr;

  class Logger : public zsLib::ILogDelegate
  {
  public:
    Logger() {}

    static LoggerPtr create() {return LoggerPtr(new Logger);}

    static LoggerPtr singleton() {
      static LoggerPtr logger = Logger::create();
      return logger;
    }

    virtual void onNewSubsystem(zsLib::Subsystem &inSubsystem)
    {
      const char *name = inSubsystem.getName();
      if (0 == strcmp(name, "hookflashServices")) {
        inSubsystem.setOutputLevel(zsLib::Log::Trace);
      } else {
        inSubsystem.setOutputLevel(zsLib::Log::Basic);
      }
    }

    // notification of a log event
    virtual void log(
      const zsLib::Subsystem &inSubsystem,
      zsLib::Log::Severity inSeverity,
      zsLib::Log::Level inLevel,
      zsLib::CSTR inMessage,
      zsLib::CSTR inFunction,
      zsLib::CSTR inFilePath,
      zsLib::ULONG inLineNumber
      )
    {
      const char *posBackslash = strrchr(inFilePath, '\\');
      const char *posSlash = strrchr(inFilePath, '/');

      const char *fileName = inFilePath;

      if (!posBackslash)
        posBackslash = posSlash;

      if (!posSlash)
        posSlash = posBackslash;

      if (posSlash) {
        if (posBackslash > posSlash)
          posSlash = posBackslash;
        fileName = posSlash + 1;
      }

      std::string current = to_simple_string(zsLib::now()).substr(12);

      const char *severity = "NONE";
      switch (inSeverity) {
  case zsLib::Log::Informational: severity = "i: "; break;
  case zsLib::Log::Warning: severity = "w: "; break;
  case zsLib::Log::Error: severity = "e: "; break;
  case zsLib::Log::Fatal: severity = "f: "; break;
      }

      std::cout << current << " " << severity << inMessage << " @" << fileName << "(" << zsLib::Stringize<zsLib::ULONG>(inLineNumber).string() << ")\n";
#ifdef _WIN32
      switch (inSeverity) {
  case zsLib::Log::Informational: severity = ") : info : "; break;
  case zsLib::Log::Warning: severity = ") : warning : "; break;
  case zsLib::Log::Error: severity = ") : error : "; break;
  case zsLib::Log::Fatal: severity = ") : fatal : "; break;
      }
      OutputDebugStringW(zsLib::String(zsLib::String(inFilePath) +  "(" + zsLib::Stringize<zsLib::ULONG>(inLineNumber).string() + severity + current + " : " + inMessage + "\n").wstring().c_str());
#endif //_WIN32
    }
  };

  void installLogger()
  {
    static bool installed = false;

    if (!installed) {
      LoggerPtr singleton = Logger::singleton();
      zsLib::LogPtr log = zsLib::Log::singleton();
      log->addListener(singleton);
      installed = true;
    }
  }  
}



void doPeerToBootstrapper();

int main ()
{
   BOOST_INSTALL_LOGGER();

   //curl test
   CURL *curl;
   CURLcode res;

   curl = curl_easy_init();
   if(curl) {
     curl_easy_setopt(curl, CURLOPT_URL, "www.google.com");
     res = curl_easy_perform(curl);

     cout << "direct"<<res;
     /* always cleanup */ 
     curl_easy_cleanup(curl);
   }

   int age;
   cin >> age;
  //curl test end
   FakeGUIPtr testGUI = FakeGUI::create();

  IClientPtr client;
  //GlobalClientInit t;
  IClient::setup();

  bool thrown = false;
  try {
    client = IClient::singleton();
  } catch(...) {
    thrown = true;
  }
  BOOST_CHECK(!thrown);
  thrown = false;

  IStackPtr stack = IStack::create(testGUI,testGUI,testGUI,testGUI);

  //openpeer::IAccountPtr acc = hookflash::openpeer::IAccount::generate(stack, testGUI, testGUI, testGUI, testGUI, "zvjerka24", "111111", "bootstrapper.yakolako.com", "64.27.57.128"/*"siptest.yakolako.com"*/, "toto", "toto", "64.27.57.128");
  zsLib::MessageQueueThreadPtr thread(zsLib::MessageQueueThread::createBasic());
  //HttpPtr pHttp;

  if (!IHttp::isSetup())
  {
    //HttpThreadPtr thread = HttpThread::create();
    IHttp::setup(thread);
  }

  BOOST_RUN_TEST_FUNC(doPeerToBootstrapper);


//  provisioning::IAccountPtr accProv2 = hookflash::provisioning::IAccount::createAccount(stack,testGUI,"bojan112233","demo",false);
// provisioning::IAccountPtr accProv = hookflash::provisioning::IAccount::createFromSocialSignin(stack,testGUI,hookflash::provisioning::IAccount::SocialSigninProvider_Facebook,"56f092681fd56c5eb795ee19f368d1ec");
//  cin >> age;
//  provisioning::IAccountPtr accProv = hookflash::provisioning::IAccount::login(stack,testGUI,"bojan112233","demo234");
//  cin >> age;
//  openpeer::IAccountPtr accPtr = accProv->getOpenPeerAccount();
  //accProv->
//  try {
//    IClient::setup();
//    testGUI = FakeGUI::create();
//    BOOST_CHECK(testGUI);
//    IClient::installLogger(testGUI);
//
//    client = IClient::singleton();
//  } catch(...) {
//    thrown = true;
//  }
//  BOOST_CHECK(!thrown);
//  BOOST_CHECK(client)
//
//#ifdef _WIN32
//  MSG msg;
//  ZeroMemory(&msg, sizeof(msg));
//  while (::GetMessage(&msg, NULL, 0, 0))
//  {
//    ::TranslateMessage(&msg);
//    ::DispatchMessage(&msg);
//  }
//#endif //_WIN32

  // wait for the testing thread to shutdown
  



  //std::cout << "PASSED:       [" << BoostReplacement::getGlobalPassedVar() << "]\n";
  //if (0 != BoostReplacement::getGlobalFailedVar()) {
  //  std::cout << "***FAILED***: [" << BoostReplacement::getGlobalFailedVar() << "]\n";
  //  return -1;
  //}

  cin >> age; 

  testGUI->waitShutdown();
  stack->startShutdown();

  client->finalizeShutdown();
  return 0;
}
