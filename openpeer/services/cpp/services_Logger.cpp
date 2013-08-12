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

#include <openpeer/services/internal/services_Logger.h>
#include <openpeer/services/IDNS.h>
#include <openpeer/services/IHelper.h>
#include <openpeer/services/IWakeDelegate.h>

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

#ifdef __QNX__
#ifndef NDEBUG
#include <QDebug>
#endif //ndef NDEBUG
#endif //__QNX__

#include <boost/shared_array.hpp>

namespace openpeer { namespace services { ZS_DECLARE_SUBSYSTEM(openpeer_services) } }

#define OPENPEER_DEFAULT_OUTGOING_TELNET_PORT (59999)

#define OPENPEER_SERVICES_SEQUENCE_ESCAPE                    "\x1B"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET              OPENPEER_SERVICES_SEQUENCE_ESCAPE "[0m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_THREAD             OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[33m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_TIME               OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[33m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_INFO      OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[36m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_WARNING   OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[35m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_ERROR     OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[31m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_FATAL     OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[31m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY           OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[36m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_BASIC      OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[1m" OPENPEER_SERVICES_SEQUENCE_ESCAPE "[30m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_DETAIL     OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[1m" OPENPEER_SERVICES_SEQUENCE_ESCAPE "[30m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_DEBUG      OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[30m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_TRACE      OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[34m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_FILENAME           OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[32m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_LINENUMBER         OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[32m"
#define OPENPEER_SERVICES_SEQUENCE_COLOUR_FUNCTION           OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET OPENPEER_SERVICES_SEQUENCE_ESCAPE "[36m"


namespace openpeer
{
  namespace services
  {
    using zsLib::Numeric;
    using zsLib::AutoRecursiveLock;

    namespace internal
    {

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
        return string(GetCurrentThreadId());
#else
#ifdef APPLE
        return string(pthread_mach_thread_np(pthread_self()));
#else
        return string(pthread_self());
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
                                  ULONG inLineNumber,
                                  bool eol = true
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

        const char *colorSeverity = OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_INFO;
        const char *severity = "NONE";
        switch (inSeverity) {
          case Log::Informational:   severity = "i:"; colorSeverity = OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_INFO; break;
          case Log::Warning:         severity = "W:"; colorSeverity = OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_WARNING; break;
          case Log::Error:           severity = "E:"; colorSeverity = OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_ERROR; break;
          case Log::Fatal:           severity = "F:"; colorSeverity = OPENPEER_SERVICES_SEQUENCE_COLOUR_SEVERITY_FATAL; break;
        }

        const char *colorLevel = OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_TRACE;
        switch (inLevel) {
          case Log::Basic:           colorLevel = OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_BASIC; break;
          case Log::Detail:          colorLevel = OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_DETAIL; break;
          case Log::Debug:           colorLevel = OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_DEBUG; break;
          case Log::Trace:           colorLevel = OPENPEER_SERVICES_SEQUENCE_COLOUR_MESSAGE_TRACE; break;
          case Log::None:            break;
        }

        String result = String(OPENPEER_SERVICES_SEQUENCE_COLOUR_TIME) + current
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + colorSeverity + severity
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_THREAD + "<" + currentThreadIDAsString() + ">"
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + colorLevel + inMessage
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_FILENAME + "@" + fileName
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_LINENUMBER + "(" + string(inLineNumber) + ")"
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET + " "
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_FUNCTION + "[" + inFunction + "]"
                      + OPENPEER_SERVICES_SEQUENCE_COLOUR_RESET + (eol ? "\n" : "");

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
                               ULONG inLineNumber,
                               bool eol = true
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

        String result = current + " " + severity + " <"  + currentThreadIDAsString() + "> " + inMessage + " " + "@" + fileName + "(" + string(inLineNumber) + ")" + " " + "[" + inFunction + "]" + (eol ? "\n" : "");
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
                                    ULONG inLineNumber,
                                    bool eol = true
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

        String result = String(inFilePath) +  "(" + string(inLineNumber) + ") " + severity + current + " : <" + currentThreadIDAsString() + "> " + inMessage + (eol ? "\n" : "");
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
          AutoRecursiveLock lock(IHelper::getGlobalLock());
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
          AutoRecursiveLock lock(IHelper::getGlobalLock());
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
          AutoRecursiveLock lock(IHelper::getGlobalLock());
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
      #pragma mark DebuggerLogger
      #pragma mark

      class DebuggerLogger;
      typedef boost::shared_ptr<DebuggerLogger> DebuggerLoggerPtr;
      typedef boost::weak_ptr<DebuggerLogger> DebuggerLoggerWeakPtr;

      class DebuggerLogger : public ILogDelegate
      {
        //---------------------------------------------------------------------
        void init()
        {
          LogPtr log = Log::singleton();
          log->addListener(mThisWeak.lock());
        }

      public:
        //---------------------------------------------------------------------
        DebuggerLogger(bool colorizeOutput) : mColorizeOutput(colorizeOutput) {}

        //---------------------------------------------------------------------
        static DebuggerLoggerPtr create(bool colorizeOutput)
        {
          DebuggerLoggerPtr pThis(new DebuggerLogger(colorizeOutput));
          pThis->mThisWeak = pThis;
          pThis->init();
          return pThis;
        }

        //---------------------------------------------------------------------
        static DebuggerLoggerPtr singleton(bool colorizeOutput, bool reset = false)
        {
#if (defined(_WIN32)) || ((defined(__QNX__) && (!defined(NDEBUG))))
          AutoRecursiveLock lock(Helper::getGlobalLock());
          static DebuggerLoggerPtr logger = (reset ? DebuggerLoggerPtr() : DebuggerLogger::create(colorizeOutput));
          if ((reset) &&
              (logger)) {
            LogPtr log = Log::singleton();
            log->removeListener(logger->mThisWeak.lock());
            logger.reset();
          }
          if ((!reset) &&
              (!logger)) {
            logger = DebuggerLogger::create(colorizeOutput);
          }
          return logger;
#else
          return DebuggerLoggerPtr();
#endif //_WIN32
        }

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark DebuggerLogger => ILogDelegate
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
#ifdef __QNX__
#ifndef NDEBUG
          String output;
          if (mColorizeOutput)
            output = toColorString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber, false);
          else
            output = toBWString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber, false);
          qDebug() << output.c_str();
#endif //ndef NDEBUG
#endif //__QNX__
#ifdef _WIN32
          String output = toWindowsString(inSubsystem, inSeverity, inLevel, inMessage, inFunction, inFilePath, inLineNumber);
          OutputDebugStringW(output.wstring().c_str());
#endif //_WIN32
        }

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark DebuggerLogger => (data)
        #pragma mark

        DebuggerLoggerWeakPtr mThisWeak;
        bool mColorizeOutput;
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
                           public IWakeDelegate
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
            mListenPort = OPENPEER_DEFAULT_OUTGOING_TELNET_PORT;
          }

          // do this from outside the stack to prevent this from happening during any kind of lock
          IWakeDelegateProxy::create(mThisWeak.lock())->onWake();

          LogPtr log = Log::singleton();
          log->addListener(mThisWeak.lock());
        }

        //---------------------------------------------------------------------
        void listen()
        {
          if (!mListenSocket) {
            mListenSocket = Socket::createTCP();
            try {
#ifndef __QNX__
              mListenSocket->setOptionFlag(ISocket::SetOptionFlag::IgnoreSigPipe, true);
#endif //ndef __QNX__
            } catch(ISocket::Exceptions::UnsupportedSocketOption &) {
            }
            mListenSocket->setOptionFlag(Socket::SetOptionFlag::NonBlocking, true);
          }

          IPAddress any = IPAddress::anyV4();
          any.setPort(mListenPort);

          int error = 0;

          std::cout << "TELNET LOGGER: Attempting to listen for client connections on port: " << mListenPort << " (start time=" << string(mStartListenTime) << ")...\n";
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
          mThisWeak.reset();
          close();
        }

        //---------------------------------------------------------------------
        void close()
        {
          MessageQueueThreadPtr thread;

          {
            AutoRecursiveLock lock(mLock);

            if (mClosed) {
              // already closed
              return;
            }

            mClosed = true;

            TelnetLoggerPtr pThis = mThisWeak.lock();
            if (pThis) {
              LogPtr log = Log::singleton();
              log->removeListener(mThisWeak.lock());
              mThisWeak.reset();
            }

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
        bool isListening()
        {
          AutoRecursiveLock lock(mLock);
          return mListenSocket;
        }

        //---------------------------------------------------------------------
        bool isConnected()
        {
          AutoRecursiveLock lock(mLock);
          if (!mTelnetSocket) return false;
          if (mOutgoingMode) return mConnected;
          return true;
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
        static TelnetLoggerPtr &singletonListener()
        {
          static TelnetLoggerPtr singleton;
          return singleton;
        }

        //---------------------------------------------------------------------
        static TelnetLoggerPtr &singletonOutgoing()
        {
          static TelnetLoggerPtr singleton;
          return singleton;
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
#ifndef __QNX__
              mTelnetSocket->setOptionFlag(ISocket::SetOptionFlag::IgnoreSigPipe, true);
#endif //ndef __QNX__
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
        virtual void onWake()
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
            // we can guarentee result will not happen until after "onWake"
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
#ifndef __QNX__
            mTelnetSocket->setOptionFlag(ISocket::SetOptionFlag::IgnoreSigPipe, true);
#endif //ndef __QNX__
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
                  ILogger::setLogLevel(Log::Trace);
                } else if (level == "debug") {
                  ILogger::setLogLevel(Log::Debug);
                } else if (level == "detail") {
                  ILogger::setLogLevel(Log::Detail);
                } else if (level == "basic") {
                  ILogger::setLogLevel(Log::Basic);
                } else if (level == "none") {
                  ILogger::setLogLevel(Log::None);
                } else if (split.size() > 0) {
                  subsystem = level;
                  level = split.front(); split.pop_front();
                  if (level == "trace") {
                    ILogger::setLogLevel(subsystem, Log::Trace);
                  } else if (level == "debug") {
                    ILogger::setLogLevel(subsystem, Log::Debug);
                  } else if (level == "detail") {
                    ILogger::setLogLevel(subsystem, Log::Detail);
                  } else if (level == "basic") {
                    ILogger::setLogLevel(subsystem, Log::Basic);
                  } else if (level == "none") {
                    ILogger::setLogLevel(subsystem, Log::None);
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
      #pragma mark Logger
      #pragma mark


    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark services::ILogger
    #pragma mark

    //-------------------------------------------------------------------------
    void ILogger::installStdOutLogger(bool colorizeOutput)
    {
      internal::StdOutLogger::singleton(colorizeOutput);
    }

    //-------------------------------------------------------------------------
    void ILogger::installFileLogger(const char *fileName, bool colorizeOutput)
    {
      internal::FileLogger::singleton(fileName, colorizeOutput);
    }

    //-------------------------------------------------------------------------
    void ILogger::installTelnetLogger(WORD listenPort, ULONG maxSecondsWaitForSocketToBeAvailable,  bool colorizeOutput)
    {
      AutoRecursiveLock lock(IHelper::getGlobalLock());

      internal::TelnetLoggerPtr &singleton = internal::TelnetLogger::singletonListener();
      if (singleton) {
        singleton->close();
        singleton.reset();
      }

      singleton = internal::TelnetLogger::create(listenPort, maxSecondsWaitForSocketToBeAvailable, colorizeOutput);
    }

    //-------------------------------------------------------------------------
    void ILogger::installOutgoingTelnetLogger(
                                              const char *serverHostWithPort,
                                              bool colorizeOutput,
                                              const char *sendStringUponConnection
                                              )
    {
      AutoRecursiveLock lock(IHelper::getGlobalLock());

      internal::TelnetLoggerPtr &singleton = internal::TelnetLogger::singletonOutgoing();
      if (singleton) {
        singleton->close();
        singleton.reset();
      }
      singleton = internal::TelnetLogger::create(serverHostWithPort, colorizeOutput, sendStringUponConnection);
    }

    //-------------------------------------------------------------------------
    void ILogger::installDebuggerLogger(bool colorizeOutput)
    {
      internal::DebuggerLogger::singleton(colorizeOutput);
    }

    //-------------------------------------------------------------------------
    bool ILogger::isTelnetLoggerListening()
    {
      AutoRecursiveLock lock(IHelper::getGlobalLock());
      internal::TelnetLoggerPtr &singleton = internal::TelnetLogger::singletonListener();
      if (!singleton) return false;

      return singleton->isListening();
    }

    //-------------------------------------------------------------------------
    bool ILogger::isTelnetLoggerConnected()
    {
      AutoRecursiveLock lock(IHelper::getGlobalLock());
      internal::TelnetLoggerPtr &singleton = internal::TelnetLogger::singletonListener();
      if (!singleton) return false;

      return singleton->isConnected();
    }

    //-------------------------------------------------------------------------
    bool ILogger::isOutgoingTelnetLoggerConnected()
    {
      AutoRecursiveLock lock(IHelper::getGlobalLock());
      internal::TelnetLoggerPtr &singleton = internal::TelnetLogger::singletonOutgoing();
      if (!singleton) return false;

      return singleton->isConnected();
    }

    //-------------------------------------------------------------------------
    void ILogger::uninstallStdOutLogger()
    {
      internal::StdOutLogger::singleton(false, true);
    }

    //-------------------------------------------------------------------------
    void ILogger::uninstallFileLogger()
    {
      internal::FileLogger::singleton(NULL, false, true);
    }

    //-------------------------------------------------------------------------
    void ILogger::uninstallTelnetLogger()
    {
      AutoRecursiveLock lock(IHelper::getGlobalLock());

      internal::TelnetLoggerPtr &singleton = internal::TelnetLogger::singletonListener();
      if (singleton) {
        singleton->close();
        singleton.reset();
      }
    }

    //-------------------------------------------------------------------------
    void ILogger::uninstallOutgoingTelnetLogger()
    {
      AutoRecursiveLock lock(IHelper::getGlobalLock());

      internal::TelnetLoggerPtr &singleton = internal::TelnetLogger::singletonOutgoing();
      if (singleton) {
        singleton->close();
        singleton.reset();
      }
    }

    //-------------------------------------------------------------------------
    void ILogger::uninstallDebuggerLogger()
    {
      internal::DebuggerLogger::singleton(false, true);
    }

    //-------------------------------------------------------------------------
    void ILogger::setLogLevel(Log::Level logLevel)
    {
      internal::LogLevelLoggerPtr logger = internal::LogLevelLogger::singleton();
      logger->setLogLevel(logLevel);
    }

    //-------------------------------------------------------------------------
    void ILogger::setLogLevel(const char *component, Log::Level logLevel)
    {
      internal::LogLevelLoggerPtr logger = internal::LogLevelLogger::singleton();
      logger->setLogLevel(component, logLevel);
    }

  }
}
