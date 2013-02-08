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

#include <hookflash/services/internal/services_Helper.h>
#include <hookflash/services/IDNS.h>
#include <cryptopp/osrng.h>
#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>
#include <zsLib/Log.h>
#include <zsLib/ISocket.h>
#include <zsLib/Socket.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Timer.h>
#include <zsLib/Numeric.h>

#include <iostream>
#include <fstream>
#ifndef _WIN32
#include <pthread.h>
#endif //ndef _WIN32

#include <boost/shared_array.hpp>

namespace hookflash { namespace services { ZS_DECLARE_SUBSYSTEM(hookflash_services) } }

#define HOOKFLASH_DEFAULT_OUTGOING_TELNET_PORT (59999)

#define HOOKFLASH_SERVICES_SEQUENCE_ESCAPE                    "\x1B"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET              HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[0m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_THREAD             HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[33m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_TIME               HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[33m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_INFO      HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[36m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_WARNING   HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[35m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_ERROR     HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[31m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_FATAL     HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[31m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY           HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[36m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_BASIC      HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[1m" HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[30m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_DETAIL     HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[1m" HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[30m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_DEBUG      HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[30m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_TRACE      HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[34m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_FILENAME           HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[32m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_LINENUMBER         HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[32m"
#define HOOKFLASH_SERVICES_SEQUENCE_COLOUR_FUNCTION           HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET HOOKFLASH_SERVICES_SEQUENCE_ESCAPE "[36m"


namespace hookflash
{
  namespace services
  {
    using zsLib::Stringize;
    using zsLib::Numeric;

    namespace internal
    {
      interaction ITelnetLoggerAsync;
      typedef boost::shared_ptr<ITelnetLoggerAsync> ITelnetLoggerAsyncPtr;
      typedef boost::weak_ptr<ITelnetLoggerAsync> ITelnetLoggerAsyncWeakPtr;
      typedef zsLib::Proxy<ITelnetLoggerAsync> ITelnetLoggerAsyncProxy;

      interaction ITelnetLoggerAsync
      {
        virtual void onStep() = 0;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::services::internal::ITelnetLoggerAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

namespace hookflash
{
  namespace services
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark GlobalLock
      #pragma mark

      //-----------------------------------------------------------------------
      class GlobalLock
      {
      public:
        GlobalLock() {}
        ~GlobalLock() {}

        //---------------------------------------------------------------------
        static GlobalLock &singleton()
        {
          static GlobalLock lock;
          return lock;
        }

        //---------------------------------------------------------------------
        RecursiveLock &getLock() const
        {
          return mLock;
        }

      private:
        mutable RecursiveLock mLock;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark GlobalLockInit
      #pragma mark

      //-----------------------------------------------------------------------
      class GlobalLockInit
      {
      public:
        //---------------------------------------------------------------------
        GlobalLockInit()
        {
          singleton();
        }

        //---------------------------------------------------------------------
        RecursiveLock &singleton()
        {
          return (GlobalLock::singleton()).getLock();
        }
      };

      static GlobalLockInit gGlobalLockInit;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ServiceThread
      #pragma mark

      //-----------------------------------------------------------------------
      class ServiceThread;
      typedef boost::shared_ptr<ServiceThread> ServiceThreadPtr;
      typedef boost::weak_ptr<ServiceThread> ServiceThreadWeakPtr;

      class ServiceThread
      {
        //---------------------------------------------------------------------
        ServiceThread() {}

        //---------------------------------------------------------------------
        void init()
        {
          mThread = MessageQueueThread::createBasic("com.hookflash.services.serviceThread");
        }

      public:
        //---------------------------------------------------------------------
        ~ServiceThread()
        {
          if (!mThread) return;
          mThread->waitForShutdown();
        }

        //---------------------------------------------------------------------
        static ServiceThreadPtr create()
        {
          ServiceThreadPtr pThis(new ServiceThread);
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        static ServiceThreadPtr singleton()
        {
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static ServiceThreadPtr thread = ServiceThread::create();
          return thread;
        }

        //---------------------------------------------------------------------
        MessageQueueThreadPtr getThread() const
        {
          return mThread;
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ServiceThread => (data)
        #pragma mark

        ServiceThreadWeakPtr mThisWeak;

        MessageQueueThreadPtr mThread;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark (helpers)
      #pragma mark

      //-----------------------------------------------------------------------
      static String currentThreadIDAsString()
      {
#ifdef _WIN32
        return Stringize<DWORD>(GetCurrentThreadId());
#else
#ifdef APPLE
        return Stringize<mach_port_t>(pthread_mach_thread_np(pthread_self()));
#else
        return Stringize<pthread_t>(pthread_self());
#endif //APPLE
#endif //_WIN32
      }

      //-----------------------------------------------------------------------
      static String toColorString(
                                  const Subsystem &inSubsystem,
                                  Log::Severity inSeverity,
                                  Log::Level inLevel,
                                  CSTR inMessage,
                                  CSTR inFunction,
                                  CSTR inFilePath,
                                  ULONG inLineNumber
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

        const char *colorSeverity = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_INFO;
        const char *severity = "NONE";
        switch (inSeverity) {
          case Log::Informational:   severity = "i:"; colorSeverity = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_INFO; break;
          case Log::Warning:         severity = "W:"; colorSeverity = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_WARNING; break;
          case Log::Error:           severity = "E:"; colorSeverity = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_ERROR; break;
          case Log::Fatal:           severity = "F:"; colorSeverity = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_SEVERITY_FATAL; break;
        }

        const char *colorLevel = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_TRACE;
        switch (inLevel) {
          case Log::Basic:           colorLevel = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_BASIC; break;
          case Log::Detail:          colorLevel = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_DETAIL; break;
          case Log::Debug:           colorLevel = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_DEBUG; break;
          case Log::Trace:           colorLevel = HOOKFLASH_SERVICES_SEQUENCE_COLOUR_MESSAGE_TRACE; break;
          case Log::None:            break;
        }

        String result = String(HOOKFLASH_SERVICES_SEQUENCE_COLOUR_TIME) + current
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + colorSeverity + severity
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_THREAD + "<" + currentThreadIDAsString() + ">"
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + colorLevel + inMessage
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_FILENAME + "@" + fileName
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_LINENUMBER + "(" + Stringize<ULONG>(inLineNumber).string() + ")"
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_FUNCTION + "[" + inFunction + "]"
                      + HOOKFLASH_SERVICES_SEQUENCE_COLOUR_RESET + "\n";

        return result;
      }

      //-----------------------------------------------------------------------
      static String toBWString(
                               const Subsystem &inSubsystem,
                               Log::Severity inSeverity,
                               Log::Level inLevel,
                               CSTR inMessage,
                               CSTR inFunction,
                               CSTR inFilePath,
                               ULONG inLineNumber
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
          case Log::Informational:   severity = "i:"; break;
          case Log::Warning:         severity = "W:"; break;
          case Log::Error:           severity = "E:"; break;
          case Log::Fatal:           severity = "F:"; break;
        }

        String result = current + " " + severity + " <"  + currentThreadIDAsString() + "> " + inMessage + " " + "@" + fileName + "(" + Stringize<ULONG>(inLineNumber).string() + ")" + " " + "[" + inFunction + "]" + "\n";
        return result;
      }

      //-----------------------------------------------------------------------
      static String toWindowsString(
                                    const Subsystem &inSubsystem,
                                    Log::Severity inSeverity,
                                    Log::Level inLevel,
                                    CSTR inMessage,
                                    CSTR inFunction,
                                    CSTR inFilePath,
                                    ULONG inLineNumber
                                    )
      {
        std::string current = to_simple_string(zsLib::now()).substr(12);

        const char *severity = "NONE";
        switch (inSeverity) {
          case Log::Informational:   severity = "i:"; break;
          case Log::Warning:         severity = "W:"; break;
          case Log::Error:           severity = "E:"; break;
          case Log::Fatal:           severity = "F:"; break;
        }

        String result = String(inFilePath) +  "(" + Stringize<ULONG>(inLineNumber).string() + ") " + severity + current + " : <" + currentThreadIDAsString() + "> " + inMessage + "\n";
        return result;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark LogLevelLogger
      #pragma mark

      class LogLevelLogger;
      typedef boost::shared_ptr<LogLevelLogger> LogLevelLoggerPtr;
      typedef boost::weak_ptr<LogLevelLogger> LogLevelLoggerWeakPtr;

      //-----------------------------------------------------------------------
      class LogLevelLogger : public ILogDelegate
      {
        //---------------------------------------------------------------------
        void init()
        {
          LogPtr log = Log::singleton();
          log->addListener(mThisWeak.lock());
        }

      public:
        //---------------------------------------------------------------------
        LogLevelLogger() :
          mDefaultLogLevelSet(false),
          mDefaultLogLevel(Log::None)
        {}

        //---------------------------------------------------------------------
        static LogLevelLoggerPtr create()
        {
          LogLevelLoggerPtr pThis(new LogLevelLogger());
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        static LogLevelLoggerPtr singleton() {
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static LogLevelLoggerPtr logger = LogLevelLogger::create();
          return logger;
        }

        //---------------------------------------------------------------------
        void setLogLevel(Log::Level level)
        {
          AutoRecursiveLock lock(mLock);

          mLevels.clear();

          mDefaultLogLevelSet = true;
          mDefaultLogLevel = level;
          for (SubsystemMap::iterator iter = mSubsystems.begin(); iter != mSubsystems.end(); ++iter) {
            Subsystem * &subsystem = (*iter).second;
            (*subsystem).setOutputLevel(level);
          }
        }

        //---------------------------------------------------------------------
        void setLogLevel(const char *component, Log::Level level)
        {
          AutoRecursiveLock lock(mLock);
          mLevels[component] = level;

          SubsystemMap::iterator found = mSubsystems.find(component);
          if (found == mSubsystems.end()) return;

          Subsystem * &subsystem = (*found).second;
          (*subsystem).setOutputLevel(level);
        }

        //---------------------------------------------------------------------
        virtual void onNewSubsystem(Subsystem &inSubsystem)
        {
          AutoRecursiveLock lock(mLock);

          const char *name = inSubsystem.getName();
          mSubsystems[name] = &(inSubsystem);

          LevelMap::iterator found = mLevels.find(name);
          if (found == mLevels.end()) {
            if (!mDefaultLogLevelSet) return;
            inSubsystem.setOutputLevel(mDefaultLogLevel);
            return;
          }

          Log::Level level = (*found).second;
          inSubsystem.setOutputLevel(level);
        }

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LogLevelLogger => ILogDelegate
        #pragma mark

        //---------------------------------------------------------------------
        // notification of a log event
        virtual void log(
                         const Subsystem &,
                         Log::Severity,
                         Log::Level,
                         CSTR,
                         CSTR,
                         CSTR,
                         ULONG
                         )
        {
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark LogLevelLogger => (data)
        #pragma mark

        LogLevelLoggerWeakPtr mThisWeak;

        mutable RecursiveLock mLock;

        typedef std::map<String, Subsystem *> SubsystemMap;
        typedef std::map<String, Log::Level> LevelMap;

        SubsystemMap mSubsystems;
        LevelMap mLevels;

        bool mDefaultLogLevelSet;
        Log::Level mDefaultLogLevel;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark StdOutLogger
      #pragma mark

      class StdOutLogger;
      typedef boost::shared_ptr<StdOutLogger> StdOutLoggerPtr;
      typedef boost::weak_ptr<StdOutLogger> StdOutLoggerWeakPtr;

      class StdOutLogger : public ILogDelegate
      {
        //---------------------------------------------------------------------
        void init()
        {
          LogPtr log = Log::singleton();
          log->addListener(mThisWeak.lock());
        }

      public:
        //---------------------------------------------------------------------
        StdOutLogger(bool colorizeOutput) : mColorizeOutput(colorizeOutput) {}

        //---------------------------------------------------------------------
        static StdOutLoggerPtr create(bool colorizeOutput)
        {
          StdOutLoggerPtr pThis(new StdOutLogger(colorizeOutput));
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        static StdOutLoggerPtr singleton(bool colorizeOutput, bool reset = false) {
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static StdOutLoggerPtr logger = (reset ? StdOutLoggerPtr() : StdOutLogger::create(colorizeOutput));
          if ((reset) &&
              (logger)) {
            LogPtr log = Log::singleton();
            log->removeListener(logger->mThisWeak.lock());
            logger.reset();
          }
          if ((!reset) &&
              (!logger)) {
            logger = StdOutLogger::create(colorizeOutput);
          }
          return logger;
        }

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark StdOutLogger => ILogDelegate
        #pragma mark

        virtual void onNewSubsystem(Subsystem &)
        {
        }

        //---------------------------------------------------------------------
        // notification of a log event
        virtual void log(
                         const Subsystem &inSubsystem,
                         Log::Severity inSeverity,
                         Log::Level inLevel,
                         CSTR inMessage,
                         CSTR inFunction,
                         CSTR inFilePath,
                         ULONG inLineNumber
                         )
        {
          if (mColorizeOutput) {
            std:: cout << toColorString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber);
          } else {
            std:: cout << toBWString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber);
          }
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark StdOutLogger => (data)
        #pragma mark

        StdOutLoggerWeakPtr mThisWeak;
        bool mColorizeOutput;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FileLogger
      #pragma mark

      class FileLogger;
      typedef boost::shared_ptr<FileLogger> FileLoggerPtr;
      typedef boost::weak_ptr<FileLogger> FileLoggerWeakPtr;

      class FileLogger : public ILogDelegate
      {
        //---------------------------------------------------------------------
        void init(const char *fileName)
        {
          mFile.open(fileName, std::ios::out | std::ios::binary);
          LogPtr log = Log::singleton();
          log->addListener(mThisWeak.lock());
        }

      public:
        //---------------------------------------------------------------------
        FileLogger(bool colorizeOutput) : mColorizeOutput(colorizeOutput) {}

        //---------------------------------------------------------------------
        static FileLoggerPtr create(const char *fileName, bool colorizeOutput)
        {
          FileLoggerPtr pThis(new FileLogger(colorizeOutput));
          pThis->mThisWeak = pThis;
          pThis->init(fileName);
          return pThis;
        }

        //---------------------------------------------------------------------
        static FileLoggerPtr singleton(const char *fileName, bool colorizeOutput, bool reset = false) {
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static FileLoggerPtr logger = (reset ? FileLoggerPtr() : FileLogger::create(fileName, colorizeOutput));
          if ((reset) &&
              (logger)) {
            LogPtr log = Log::singleton();
            log->removeListener(logger->mThisWeak.lock());
            logger.reset();
          }
          if ((!reset) &&
              (!logger)) {
            logger = FileLogger::create(fileName, colorizeOutput);
          }
          return logger;
        }

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FileLogger => ILogDelegate
        #pragma mark

        virtual void onNewSubsystem(Subsystem &)
        {
        }

        //---------------------------------------------------------------------
        // notification of a log event
        virtual void log(
                         const Subsystem &inSubsystem,
                         Log::Severity inSeverity,
                         Log::Level inLevel,
                         CSTR inMessage,
                         CSTR inFunction,
                         CSTR inFilePath,
                         ULONG inLineNumber
                         )
        {
          if (mFile.is_open()) {
            String output;
            if (mColorizeOutput) {
              output = toColorString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber);
            } else {
              output = toBWString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber);
            }
            mFile << output;
            mFile.flush();
          }
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FileLogger => (data)
        #pragma mark

        FileLoggerWeakPtr mThisWeak;
        bool mColorizeOutput;

        std::ofstream mFile;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark WindowsDebuggerLogger
      #pragma mark

      class WindowsDebuggerLogger;
      typedef boost::shared_ptr<WindowsDebuggerLogger> WindowsDebuggerLoggerPtr;
      typedef boost::weak_ptr<WindowsDebuggerLogger> WindowsDebuggerLoggerWeakPtr;

      class WindowsDebuggerLogger : public ILogDelegate
      {
        //---------------------------------------------------------------------
        void init()
        {
          LogPtr log = Log::singleton();
          log->addListener(mThisWeak.lock());
        }

      public:
        //---------------------------------------------------------------------
        WindowsDebuggerLogger() {}

        //---------------------------------------------------------------------
        static WindowsDebuggerLoggerPtr create()
        {
          WindowsDebuggerLoggerPtr pThis(new WindowsDebuggerLogger());
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        static WindowsDebuggerLoggerPtr singleton(bool reset = false)
        {
#ifdef _WIN32
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static WindowsDebuggerLoggerPtr logger = (reset ? WindowsDebuggerLoggerPtr() : WindowsDebuggerLogger::create());
          if ((reset) &&
              (logger)) {
            LogPtr log = Log::singleton();
            log->removeListener(logger->mThisWeak.lock());
            logger.reset();
          }
          if ((!reset) &&
              (!logger)) {
            logger = WindowsDebuggerLogger::create();
          }
          return logger;
#else
          return WindowsDebuggerLoggerPtr();
#endif //_WIN32
        }

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark WindowsDebuggerLogger => ILogDelegate
        #pragma mark

        //---------------------------------------------------------------------
        virtual void onNewSubsystem(Subsystem &)
        {
        }

        //---------------------------------------------------------------------
        // notification of a log event
        virtual void log(
                         const Subsystem &inSubsystem,
                         Log::Severity inSeverity,
                         Log::Level inLevel,
                         CSTR inMessage,
                         CSTR inFunction,
                         CSTR inFilePath,
                         ULONG inLineNumber
                         )
        {
#ifdef _WIN32
          String output = toWindowsString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber);
          OutputDebugStringW(output.wstring().c_str());
#endif //_WIN32
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark WindowsDebuggerLogger => (data)
        #pragma mark

        WindowsDebuggerLoggerWeakPtr mThisWeak;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark TelnetLogger
      #pragma mark

      class TelnetLogger;
      typedef boost::shared_ptr<TelnetLogger> TelnetLoggerPtr;
      typedef boost::weak_ptr<TelnetLogger> TelnetLoggerWeakPtr;

      class TelnetLogger : public ILogDelegate,
                           public MessageQueueAssociator,
                           public ISocketDelegate,
                           public IDNSDelegate,
                           public ITimerDelegate,
                           public ITelnetLoggerAsync
      {
        //---------------------------------------------------------------------
        void init(
                  USHORT listenPort,
                  ULONG maxSecondsWaitForSocketToBeAvailable
                  )
        {
          mListenPort = listenPort;
          mMaxWaitTimeForSocketToBeAvailable = Seconds(maxSecondsWaitForSocketToBeAvailable);

          LogPtr log = Log::singleton();
          log->addListener(mThisWeak.lock());

          listen();
        }

        //---------------------------------------------------------------------
        void init(
                  const char *serverHostWithPort,
                  const char *sendStringUponConnection
                  )
        {
          mOutgoingMode = true;
          mBackupStringToSendUponConnection = (sendStringUponConnection ? sendStringUponConnection : "");
          mServerLookupName = (serverHostWithPort ? serverHostWithPort : "");

          String::size_type pos = mServerLookupName.find(":");
          if (pos != mServerLookupName.npos) {
            String portStr = mServerLookupName.substr(pos+1);
            mServerLookupName = mServerLookupName.substr(0, pos);

            try {
              mListenPort = Numeric<WORD>(portStr);
            } catch(Numeric<WORD>::ValueOutOfRange &) {
            }
          }

          if (0 == mListenPort) {
            mListenPort = HOOKFLASH_DEFAULT_OUTGOING_TELNET_PORT;
          }

          // do this from outside the stack to prevent this from happening during any kind of lock
          ITelnetLoggerAsyncProxy::create(mThisWeak.lock())->onStep();

          LogPtr log = Log::singleton();
          log->addListener(mThisWeak.lock());
        }

        //---------------------------------------------------------------------
        void listen()
        {
          if (!mListenSocket) {
            mListenSocket = Socket::createTCP();
            try {
              mListenSocket->setOptionFlag(ISocket::SetOptionFlag::IgnoreSigPipe, true);
            } catch(ISocket::Exceptions::UnsupportedSocketOption &) {
            }
            mListenSocket->setOptionFlag(Socket::SetOptionFlag::NonBlocking, true);
          }

          IPAddress any = IPAddress::anyV4();
          any.setPort(mListenPort);

          int error = 0;

          std::cout << "TELNET LOGGER: Attempting to listen for client connections on port: " << mListenPort << " (start time=" << Stringize<Time>(mStartListenTime).string() << ")...\n";
          mListenSocket->bind(any, &error);

          Time tick = zsLib::now();

          if (0 != error) {
            mListenSocket->close();
            mListenSocket.reset();

            if (mStartListenTime + mMaxWaitTimeForSocketToBeAvailable < tick) {
              std::cout << "TELNET LOGGER: ***ABANDONED***\n";
              if (mListenTimer) {
                mListenTimer->cancel();
                mListenTimer.reset();
              }
              return;
            }
            if (!mListenTimer) {
              mListenTimer = Timer::create(mThisWeak.lock(), Seconds(1));
            }
            std::cout << "TELNET LOGGER: Failed to listen...\n";
            return;
          } else {
            std::cout << "TELNET LOGGER: Succeeded.\n\n";
          }

          if (mListenTimer) {
            mListenTimer->cancel();
            mListenTimer.reset();
          }

          mListenSocket->setDelegate(mThisWeak.lock());
          mListenSocket->listen();
        }

      public:
        //---------------------------------------------------------------------
        TelnetLogger(
                     MessageQueueThreadPtr thread,
                     bool colorizeOutput
                     ) :
          MessageQueueAssociator(thread),
          mThread(thread),
          mColorizeOutput(colorizeOutput),
          mOutgoingMode(false),
          mConnected(false),
          mClosed(false),
          mListenPort(0),
          mMaxWaitTimeForSocketToBeAvailable(Seconds(60)),
          mStartListenTime(zsLib::now())
        {}

        //---------------------------------------------------------------------
        ~TelnetLogger()
        {
          close();
        }

        //---------------------------------------------------------------------
        void close()
        {
          MessageQueueThreadPtr thread;

          {
            AutoRecursiveLock lock(mLock);

            mClosed = true;

            mBufferedList.clear();
            mConnected = false;

            if (mOutgoingServerQuery) {
              mOutgoingServerQuery->cancel();
              mOutgoingServerQuery.reset();
            }

            mServers.reset();

            if (mTelnetSocket) {
              mTelnetSocket->close();
              mTelnetSocket.reset();
            }
            if (mListenSocket) {
              mListenSocket->close();
              mListenSocket.reset();
            }
            if (mListenTimer) {
              mListenTimer->cancel();
              mListenTimer.reset();
            }
            if (mThread) {
              thread = mThread;
              mThread.reset();
            }
          }

          if (thread) {
            thread->waitForShutdown();
          }
        }

        //---------------------------------------------------------------------
        static TelnetLoggerPtr create(USHORT listenPort, ULONG maxSecondsWaitForSocketToBeAvailable, bool colorizeOutput)
        {
          MessageQueueThreadPtr thread = MessageQueueThread::createBasic();
          TelnetLoggerPtr pThis(new TelnetLogger(thread, colorizeOutput));
          pThis->mThisWeak = pThis;
          pThis->init(listenPort, maxSecondsWaitForSocketToBeAvailable);
          return pThis;
        }

        //---------------------------------------------------------------------
        static TelnetLoggerPtr create(
                                      const char *serverHostWithPort,
                                      bool colorizeOutput,
                                      const char *sendStringUponConnection
                                      )
        {
          MessageQueueThreadPtr thread = MessageQueueThread::createBasic();
          TelnetLoggerPtr pThis(new TelnetLogger(thread, colorizeOutput));
          pThis->mThisWeak = pThis;
          pThis->init(serverHostWithPort, sendStringUponConnection);
          return pThis;
        }

        //---------------------------------------------------------------------
        static TelnetLoggerPtr singleton(USHORT listenPort, ULONG maxSecondsWaitForSocketToBeAvailable, bool colorizeOutput, bool reset = false) {
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static TelnetLoggerPtr logger = (reset ? TelnetLoggerPtr() : TelnetLogger::create(listenPort, maxSecondsWaitForSocketToBeAvailable, colorizeOutput));
          if ((reset) &&
              (logger)) {
            LogPtr log = Log::singleton();
            log->removeListener(logger->mThisWeak.lock());
            logger->close();
            logger.reset();
          }
          if ((!reset) &&
              (!logger)) {
            logger = TelnetLogger::create(listenPort, maxSecondsWaitForSocketToBeAvailable, colorizeOutput);
          }
          return logger;
        }

        //---------------------------------------------------------------------
        static TelnetLoggerPtr singleton(
                                         const char *serverHostWithPort,
                                         bool colorizeOutput,
                                         const char *sendStringUponConnection
                                         )
        {
          bool reset = (NULL == serverHostWithPort);

          AutoRecursiveLock lock(Helper::getGlobalLock());
          static TelnetLoggerPtr logger = (reset ? TelnetLoggerPtr() : TelnetLogger::create(serverHostWithPort, colorizeOutput, sendStringUponConnection));
          if ((reset) &&
              (logger)) {
            LogPtr log = Log::singleton();
            log->removeListener(logger->mThisWeak.lock());
            logger->close();
            logger.reset();
          }

          if ((!reset) &&
              (!logger)) {
            logger = TelnetLogger::create(serverHostWithPort, colorizeOutput, sendStringUponConnection);
          }
          return logger;
        }

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark TelnetLogger => ILogDelegate
        #pragma mark

        //---------------------------------------------------------------------
        virtual void onNewSubsystem(Subsystem &)
        {
        }

        //---------------------------------------------------------------------
        virtual void log(
                         const Subsystem &inSubsystem,
                         Log::Severity inSeverity,
                         Log::Level inLevel,
                         CSTR inMessage,
                         CSTR inFunction,
                         CSTR inFilePath,
                         ULONG inLineNumber
                         )
        {
          AutoRecursiveLock lock(mLock);

          if (!mTelnetSocket)
            return;

          bool wouldBlock = false;
          ULONG sent = 0;

          String output;
          if (mColorizeOutput) {
            output = toColorString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber);
          } else {
            output = toBWString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber);
          }

          bool okayToSend = mBufferedList.size() < 1;

          if (mOutgoingMode) {
            if (!mConnected) return;
          }

          if (okayToSend) {
            int errorCode = 0;
            sent = mTelnetSocket->send((const BYTE *)(output.c_str()), output.length(), &wouldBlock, 0, &errorCode);
            if (!wouldBlock) {
              if (0 != errorCode) {
                connectOutgoingAgain();
                return;
              }
            }
          }

          if (sent < output.length()) {
            // we need to buffer the data for later...
            ULONG length = (output.length() - sent);
            BufferedData data;
            boost::shared_array<BYTE> buffer(new BYTE[length]);
            memcpy(&(buffer[0]), output.c_str() + sent, length);

            data.first = buffer;
            data.second = length;

            mBufferedList.push_back(data);
          }
        }

        //---------------------------------------------------------------------
        virtual void onReadReady(ISocketPtr inSocket)
        {
          AutoRecursiveLock lock(mLock);

          if (mOutgoingMode) {
            if (!mConnected) return;
          }

          if (inSocket == mListenSocket) {
            if (mTelnetSocket)
            {
              mTelnetSocket->close();
              mTelnetSocket.reset();
            }

            IPAddress ignored;
            int noThrowError = 0;
            mTelnetSocket = mListenSocket->accept(ignored, &noThrowError);
            if (!mTelnetSocket)
              return;

            try {
              mTelnetSocket->setOptionFlag(ISocket::SetOptionFlag::IgnoreSigPipe, true);
            } catch(ISocket::Exceptions::UnsupportedSocketOption &) {
            }

            mTelnetSocket->setOptionFlag(Socket::SetOptionFlag::NonBlocking, true);
            mTelnetSocket->setDelegate(mThisWeak.lock());
          }

          if (inSocket == mTelnetSocket) {
            char buffer[1024+1];
            memset(&(buffer[0]), 0, sizeof(buffer));
            ULONG length = 0;

            bool wouldBlock = false;
            int errorCode = 0;
            length = mTelnetSocket->receive((BYTE *)(&(buffer[0])), sizeof(buffer)-sizeof(buffer[0]), &wouldBlock, 0, &errorCode);

            if (length < 1) return;

            mCommand += (CSTR)(&buffer[0]);
            if (mCommand.size() > (sizeof(buffer)*3)) {
              mCommand.clear();
            }
            while (true) {
              const char *posLineFeed = strchr(mCommand, '\n');
              const char *posCarrageReturn = strchr(mCommand, '\r');

              if ((NULL == posLineFeed) &&
                  (NULL == posCarrageReturn)) {
                return;
              }

              if (NULL == posCarrageReturn)
                posCarrageReturn = posLineFeed;
              if (NULL == posLineFeed)
                posLineFeed = posCarrageReturn;

              if (posCarrageReturn < posLineFeed)
                posLineFeed = posCarrageReturn;

              String command = mCommand.substr(0, (posLineFeed - mCommand.c_str()));
              mCommand = mCommand.substr((posLineFeed - mCommand.c_str()) + 1);

              if (command.size() > 0) {
                handleCommand(command);
              }
            }
          }
        }

        //---------------------------------------------------------------------
        virtual void onWriteReady(ISocketPtr socket)
        {
          AutoRecursiveLock lock(mLock);
          if (socket != mTelnetSocket) return;

          if (mOutgoingMode) {
            mConnected = true;
          }

          if (!mStringToSendUponConnection.isEmpty()) {

            ULONG length = mStringToSendUponConnection.length();

            BufferedData data;
            boost::shared_array<BYTE> buffer(new BYTE[length]);
            memcpy(&(buffer[0]), mStringToSendUponConnection.c_str(), length);

            data.first = buffer;
            data.second = length;

            mBufferedList.push_front(data);

            mStringToSendUponConnection.clear();
          }

          while (mBufferedList.size() > 0) {
            BufferedData &data = mBufferedList.front();
            bool wouldBlock = false;
            ULONG sent = 0;

            int errorCode = 0;
            sent = mTelnetSocket->send(data.first.get(), data.second, &wouldBlock, 0, &errorCode);
            if (!wouldBlock) {
              if (0 != errorCode) {
                connectOutgoingAgain();
                return;
              }
            }

            if (sent == data.second) {
              mBufferedList.pop_front();
              continue;
            }

            ULONG length = (data.second - sent);
            memcpy(data.first.get() + sent, data.first.get(), length);
            data.second = length;
            break;
          }
        }

        //---------------------------------------------------------------------
        virtual void onException(ISocketPtr inSocket)
        {
          AutoRecursiveLock lock(mLock);
          if (inSocket == mListenSocket) {
            mListenSocket->close();
            mListenSocket.reset();
            if (!mClosed) {
              mStartListenTime = zsLib::now();
              listen();
            }
          }
          if (inSocket == mTelnetSocket) {
            connectOutgoingAgain();
          }
        }

        //---------------------------------------------------------------------
        virtual void onLookupCompleted(IDNSQueryPtr query)
        {
          AutoRecursiveLock lock(mLock);
          IDNS::AResult::IPAddressList list;
          IDNS::AResultPtr resultA = query->getA();
          if (resultA) {
            list = resultA->mIPAddresses;
          }
          IDNS::AAAAResultPtr resultAAAA = query->getAAAA();
          if (resultAAAA) {
            if (list.size() < 1) {
              list = resultAAAA->mIPAddresses;
            } else if (resultAAAA->mIPAddresses.size() > 0) {
              list.merge(resultAAAA->mIPAddresses);
            }
          }

          mOutgoingServerQuery.reset();

          if (list.size() < 1) return;

          mServers = IDNS::convertIPAddressesToSRVResult("logger", "tcp", list, mListenPort);

          connectOutgoingAgain();
        }

        //---------------------------------------------------------------------
        virtual void onStep()
        {
          String serverName;

          {
            AutoRecursiveLock lock(mLock);
            if (mServerLookupName) {
              serverName = mServerLookupName;
              mServerLookupName.clear();
            }
          }

          if (serverName.isEmpty()) return;

          // DNS is not created during any kind of lock at all...
          IDNSQueryPtr query = IDNS::lookupAorAAAA(mThisWeak.lock(), serverName);

          {
            AutoRecursiveLock lock(mLock);
            // we can guarentee result will not happen until after "onStep"
            // exits because both occupy the same thread queue...
            mOutgoingServerQuery = query;
          }
        }

        virtual void onTimer(TimerPtr timer)
        {
          if (timer != mListenTimer) {
            return;
          }
          listen();
        }

      protected:
        //---------------------------------------------------------------------
        void connectOutgoingAgain()
        {
          mConnected = false;
          mBufferedList.clear();

          if (mTelnetSocket) {
            mTelnetSocket->close();
            mTelnetSocket.reset();
          }

          if (!mOutgoingMode) return;

          if (!mServers) return;

          IPAddress result;
          if (!IDNS::extractNextIP(mServers, result)) {
            mServers.reset();
            return;
          }

          mStringToSendUponConnection = mBackupStringToSendUponConnection;

          mTelnetSocket = Socket::createTCP();
          try {
            mTelnetSocket->setOptionFlag(ISocket::SetOptionFlag::IgnoreSigPipe, true);
          } catch(ISocket::Exceptions::UnsupportedSocketOption &) {
          }
          mTelnetSocket->setBlocking(false);
          mTelnetSocket->setDelegate(mThisWeak.lock());

          bool wouldBlock = false;
          int errorCode = 0;
          mTelnetSocket->connect(result, &wouldBlock, &errorCode);
          if (0 != errorCode) {
          }
        }

        //---------------------------------------------------------------------
        void handleCommand(String command)
        {
          String input = command;

          typedef std::list<String> StringList;
          StringList split;

          // split the command by the space character...
          while (true) {
            const char *posSpace = strchr(command, ' ');
            if (NULL == posSpace) {
              if (command.size() > 0) {
                split.push_back(command);
              }
              break;
            }
            String sub = command.substr(0, (posSpace - command.c_str()));
            command = command.substr((posSpace - command.c_str()) + 1);

            if (sub.size() > 0) {
              split.push_back(sub);
            }
          }

          bool output = false;
          String subsystem;
          String level;

          if (split.size() > 0) {
            command = split.front(); split.pop_front();
            if ((command == "set") && (split.size() > 0)) {
              command = split.front(); split.pop_front();
              if ((command == "log") && (split.size() > 0)) {
                level = split.front(); split.pop_front();
                output = true;
                if (level == "trace") {
                  IHelper::setLogLevel(Log::Trace);
                } else if (level == "debug") {
                  IHelper::setLogLevel(Log::Debug);
                } else if (level == "detail") {
                  IHelper::setLogLevel(Log::Detail);
                } else if (level == "basic") {
                  IHelper::setLogLevel(Log::Basic);
                } else if (level == "none") {
                  IHelper::setLogLevel(Log::None);
                } else if (split.size() > 0) {
                  subsystem = level;
                  level = split.front(); split.pop_front();
                  if (level == "trace") {
                    IHelper::setLogLevel(subsystem, Log::Trace);
                  } else if (level == "debug") {
                    IHelper::setLogLevel(subsystem, Log::Debug);
                  } else if (level == "detail") {
                    IHelper::setLogLevel(subsystem, Log::Detail);
                  } else if (level == "basic") {
                    IHelper::setLogLevel(subsystem, Log::Basic);
                  } else if (level == "none") {
                    IHelper::setLogLevel(subsystem, Log::None);
                  } else {
                    output = false;
                  }
                } else {
                  output = false;
                }
              }
            }
          }

          String echo;
          if (output) {
            if (subsystem.size() > 0) {
              echo = "==> Setting log level for \"" + subsystem + "\" to \"" + level + "\"\n";
            } else {
              echo = "==> Setting all log compoment levels to \"" + level + "\"\n";
            }
          } else {
            echo = "==> Command not recognized \"" + input + "\"\n";
          }
          bool wouldBlock = false;
          int errorCode = 0;
          mTelnetSocket->send((const BYTE *)(echo.c_str()), echo.length(), &wouldBlock, 0, &errorCode);
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark TelnetLogger => (data)
        #pragma mark

        mutable RecursiveLock mLock;

        MessageQueueThreadPtr mThread;

        TelnetLoggerWeakPtr mThisWeak;
        bool mColorizeOutput;

        SocketPtr mListenSocket;
        ISocketPtr mTelnetSocket;

        bool mClosed;

        String mCommand;

        typedef std::pair< boost::shared_array<BYTE>, ULONG> BufferedData;
        typedef std::list<BufferedData> BufferedDataList;

        BufferedDataList mBufferedList;

        WORD mListenPort;
        Duration mMaxWaitTimeForSocketToBeAvailable;
        Time mStartListenTime;
        TimerPtr mListenTimer;

        bool mOutgoingMode;
        bool mConnected;
        IDNSQueryPtr mOutgoingServerQuery;
        String mStringToSendUponConnection;
        String mBackupStringToSendUponConnection;

        String mServerLookupName;
        IDNS::SRVResultPtr mServers;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Helper
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &Helper::getGlobalLock()
      {
        return gGlobalLockInit.singleton();
      }

      //-----------------------------------------------------------------------
      String Helper::randomString(UINT lengthInChars)
      {
        static const char *randomCharArray = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static size_t randomSize = strlen(randomCharArray);

        BYTE staticBuffer[256];
        char staticOutputBuffer[sizeof(staticBuffer)+1];

        boost::shared_array<BYTE> allocatedBuffer;
        boost::shared_array<char> allocatedOutputBuffer;

        BYTE *buffer = &(staticBuffer[0]);
        char *output = &(staticOutputBuffer[0]);
        if (lengthInChars > sizeof(staticBuffer)) {
          // use the allocated buffer instead
          allocatedBuffer = boost::shared_array<BYTE>(new BYTE[lengthInChars]);
          allocatedOutputBuffer = boost::shared_array<char>(new char[lengthInChars+1]);
          buffer = allocatedBuffer.get();
          output = allocatedOutputBuffer.get();
        }

        AutoSeededRandomPool rng;
        rng.GenerateBlock(&(buffer[0]), lengthInChars);

        memset(&(output[0]), 0, sizeof(char)*(lengthInChars+1));

        for (UINT loop = 0; loop < lengthInChars; ++loop) {
          output[loop] = randomCharArray[((buffer[loop])%randomSize)];
        }
        return String((CSTR)(&(output[0])));
      }

      //-----------------------------------------------------------------------
      SecureByteBlockPtr Helper::random(UINT lengthInBytes)
      {
        SecureByteBlockPtr output(new SecureByteBlock);
        AutoSeededRandomPool rng;
        output->CleanNew(lengthInBytes);
        rng.GenerateBlock(*output, lengthInBytes);
        return output;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Helper::getServiceQueue()
      {
        ServiceThreadPtr thread = ServiceThread::singleton();
        return thread->getThread();
      }

    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark services::IHelper
    #pragma mark

    //-------------------------------------------------------------------------
    RecursiveLock &IHelper::getGlobalLock()
    {
      return internal::Helper::getGlobalLock();
    }

    //-------------------------------------------------------------------------
    String IHelper::randomString(UINT lengthInChars)
    {
      return internal::Helper::randomString(lengthInChars);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::random(UINT lengthInBytes)
    {
      return internal::Helper::random(lengthInBytes);
    }

    //-------------------------------------------------------------------------
    IMessageQueuePtr IHelper::getServiceQueue()
    {
      return internal::Helper::getServiceQueue();
    }

    //-------------------------------------------------------------------------
    void IHelper::installStdOutLogger(bool colorizeOutput)
    {
      internal::StdOutLogger::singleton(colorizeOutput);
    }

    //-------------------------------------------------------------------------
    void IHelper::installFileLogger(const char *fileName, bool colorizeOutput)
    {
      internal::FileLogger::singleton(fileName, colorizeOutput);
    }

    //-------------------------------------------------------------------------
    void IHelper::installTelnetLogger(WORD listenPort, ULONG maxSecondsWaitForSocketToBeAvailable,  bool colorizeOutput)
    {
      internal::TelnetLogger::singleton(listenPort, maxSecondsWaitForSocketToBeAvailable, colorizeOutput);
    }

    //-------------------------------------------------------------------------
    void IHelper::installOutgoingTelnetLogger(
                                              const char *serverHostWithPort,
                                              bool colorizeOutput,
                                              const char *sendStringUponConnection
                                              )
    {
      internal::TelnetLogger::singleton(serverHostWithPort, colorizeOutput, sendStringUponConnection);
    }

    //-------------------------------------------------------------------------
    void IHelper::installWindowsDebuggerLogger()
    {
      internal::WindowsDebuggerLogger::singleton();
    }

    //-------------------------------------------------------------------------
    void IHelper::uninstallStdOutLogger()
    {
      internal::StdOutLogger::singleton(false, true);
    }

    //-------------------------------------------------------------------------
    void IHelper::uninstallFileLogger()
    {
      internal::FileLogger::singleton(NULL, false, true);
    }

    //-------------------------------------------------------------------------
    void IHelper::uninstallTelnetLogger()
    {
      internal::TelnetLogger::singleton(0, 0, false, true);
    }

    //-------------------------------------------------------------------------
    void IHelper::uninstallOutgoingTelnetLogger()
    {
      internal::TelnetLogger::singleton((const char *)NULL, false, (const char *)NULL);
    }

    //-------------------------------------------------------------------------
    void IHelper::uninstallWindowsDebuggerLogger()
    {
      internal::WindowsDebuggerLogger::singleton(true);
    }

    //-------------------------------------------------------------------------
    void IHelper::setLogLevel(Log::Level logLevel)
    {
      internal::LogLevelLoggerPtr logger = internal::LogLevelLogger::singleton();
      logger->setLogLevel(logLevel);
    }

    //-------------------------------------------------------------------------
    void IHelper::setLogLevel(const char *component, Log::Level logLevel)
    {
      internal::LogLevelLoggerPtr logger = internal::LogLevelLogger::singleton();
      logger->setLogLevel(component, logLevel);
    }

  }
}
