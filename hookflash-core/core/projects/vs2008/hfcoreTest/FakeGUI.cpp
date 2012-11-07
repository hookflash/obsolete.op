

#include "FakeGUI.h"

#include <zsLib/zsHelpers.h>
#include <zsLib/Stringize.h>

using namespace hookflash;

namespace hfcoretest
{
  FakeGUI::FakeGUI() :
    mShouldShutdown(false),
    mLoopRun(0) {}

  FakeGUIPtr FakeGUI::create() {
    FakeGUIPtr pThis = FakeGUIPtr(new FakeGUI);
    pThis->mThisWeak = pThis;
    pThis->spawn();
    return pThis;
  }

  // this thread is going to cause actions to occur so that we can prove everything works but this isn't normal to spawn another thread
  // as typically the actions would be driven by the application (which in this case does not exist because we have no GUI or user)
  void FakeGUI::operator()()
  {
    bool shouldShutdown = false;

    // first we need to create a stack
    //mStack = IStack::create(mThisWeak.lock(), mThisWeak.lock(), mThisWeak.lock(), mThisWeak.lock());


    //do
    //{
    //  ULONG loopRun = zsLib::atomicGetValue(mLoopRun);
    //  switch (loopRun) {
    //    case 10:
    //    {
    //      IAccountPtr acc = hookflash::openpeer::IAccount::generate(mStack, mThisWeak.lock(), mThisWeak.lock(), mThisWeak.lock(), mThisWeak.lock(), "zvjerka24", "111111", "192.168.2.169", "", "", "", "");
    //      break;
    //    }
    //    case 100: 
    //    {
    //      mStack->startShutdown();
    //      break;
    //    }
    //  }

    //  {
    //    zsLib::AutoRecursiveLock lock(mLock);
    //    shouldShutdown = mShouldShutdown;
    //  }
    //  boost::thread::yield();   // to not completely overwhelm the CPU
    //  zsLib::atomicIncrement(mLoopRun);
    //} while (!shouldShutdown);
  }

  // IStackDelegate
  void FakeGUI::onShutdownReady()
  {
#ifdef _WIN32
    ::PostThreadMessage(
      mThreadID,  // this won't change after spawned so it is thread safe
      WM_QUIT,
      0,
      0
    );
#endif //_WIN32
  }

  void FakeGUI::waitShutdown()
  {
    {
      zsLib::AutoRecursiveLock lock(mLock);
      mShouldShutdown = true;
    }
    mThread->join();
  }

  void FakeGUI::spawn()
  {
#ifdef _WIN32
    mThreadID = ::GetCurrentThreadId();
#endif //_WIN32
    mThread = zsLib::ThreadPtr(new boost::thread(boost::ref(*((mThisWeak.lock()).get()))));
  }

  void FakeGUI::onNewSubsystem(
                               zsLib::PTRNUMBER subsystemID,
                               const char *subsystemName
                               )
  {
    zsLib::AutoRecursiveLock lock(mLock);
    mSubsystemMap[subsystemID] = subsystemName;
    IClient::setLogLevel(subsystemID, IClient::Log::Trace);
  }

  void FakeGUI::onLog(
    zsLib::PTRNUMBER subsystemID,
    const char *subsystemName,
    IClient::Log::Severity inSeverity,
    IClient::Log::Level inLevel,
    const char *inMessage,
    const char *inFunction,
    const char *inFilePath,
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
    const char *name = NULL;
    {
      zsLib::AutoRecursiveLock lock(mLock);
      SubsystemMap::iterator iter = mSubsystemMap.find(subsystemID);
      name = (*iter).second;
    }

    zsLib::String message = inFilePath;
    message += "(";
    message += zsLib::Stringize<zsLib::ULONG>(inLineNumber).string();
    message += ") : ";
    switch (inSeverity)
    {
    case IClient::Log::Informational: message += "info"; break;
    case IClient::Log::Warning:       message += "warning"; break;
    case IClient::Log::Error:         message += "error"; break;
    case IClient::Log::Fatal:         message += "fatal"; break;
    }
    message += ") : (";
    message += inFunction;
    message += ") : ";
    message += inMessage;
    message += "\n";

    std::cout << current << " " << severity << inMessage << " @" << fileName << "(" << zsLib::Stringize<zsLib::ULONG>(inLineNumber).string() << ")\n";
#ifdef _WIN32
    switch (inSeverity) 
    {
        case zsLib::Log::Informational: severity = ") : info : "; break;
        case zsLib::Log::Warning: severity = ") : warning : "; break;
        case zsLib::Log::Error: severity = ") : error : "; break;
        case zsLib::Log::Fatal: severity = ") : fatal : "; break;
    }
    OutputDebugStringW(zsLib::String(zsLib::String(inFilePath) +  "(" + zsLib::Stringize<zsLib::ULONG>(inLineNumber).string() + severity + current + " : " + inMessage + "\n").wstring().c_str());
#endif //_WIN32
  }

}
