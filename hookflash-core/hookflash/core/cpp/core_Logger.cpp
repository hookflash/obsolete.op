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

#include <hookflash/core/internal/core_Logger.h>
#include <hookflash/services/IHelper.h>

namespace hookflash { namespace core { ZS_DECLARE_SUBSYSTEM(hookflash_core) } }
namespace hookflash { namespace core { namespace application { ZS_DECLARE_SUBSYSTEM(hookflash_application) } } }

namespace hookflash
{
  namespace core
  {
    using zsLib::Subsystem;
    using zsLib::LogPtr;

    namespace internal
    {
      //-------------------------------------------------------------------------
      static zsLib::Log::Level levelToLevel(ILogger::Level level)
      {
        switch (level) {
          case ILogger::None:   return zsLib::Log::None;
          case ILogger::Basic:  return zsLib::Log::Basic;
          case ILogger::Detail: return zsLib::Log::Detail;
          case ILogger::Debug:  return zsLib::Log::Debug;
          case ILogger::Trace:  return zsLib::Log::Trace;
        }
        return Log::None;
      }

      //-------------------------------------------------------------------------
      static ILogger::Level levelToLevel(zsLib::Log::Level level)
      {
        switch (level) {
          case Log::None:   return ILogger::None;
          case Log::Basic:  return ILogger::Basic;
          case Log::Detail: return ILogger::Detail;
          case Log::Debug:  return ILogger::Debug;
          case Log::Trace:  return ILogger::Trace;
        }
        return ILogger::None;
      }

      //-------------------------------------------------------------------------
      static zsLib::Log::Severity severityToSeverity(ILogger::Severity severity)
      {
        switch (severity) {
          case ILogger::Informational: return Log::Informational;
          case ILogger::Warning:       return Log::Warning;
          case ILogger::Error:         return Log::Error;
          case ILogger::Fatal:         return Log::Fatal;
        }
        return Log::Informational;
      }

      //-------------------------------------------------------------------------
      static ILogger::Severity severityToSeverity(zsLib::Log::Severity severity)
      {
        switch (severity) {
          case Log::Informational: return ILogger::Informational;
          case Log::Warning:       return ILogger::Warning;
          case Log::Error:         return ILogger::Error;
          case Log::Fatal:         return ILogger::Fatal;
        }
        return ILogger::Informational;
      }

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      class CustomLogger;
      typedef boost::shared_ptr<CustomLogger> CustomLoggerPtr;
      typedef boost::weak_ptr<CustomLogger> CustomLoggerWeakPtr;

      class CustomLogger : public zsLib::ILogDelegate
      {
      public:
        typedef zsLib::ULONG ULONG;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::PTRNUMBER SubsystemID;
        typedef zsLib::Subsystem Subsystem;
        typedef std::map<SubsystemID, Subsystem *> SubsystemMap;

      protected:
        CustomLogger() {}

      public:
        //-----------------------------------------------------------------------
        static CustomLoggerPtr create() {
          CustomLoggerPtr pThis = CustomLoggerPtr(new CustomLogger);
          (Log::singleton())->addListener(pThis);
          return pThis;
        }

        static CustomLoggerPtr singleton()
        {
          static CustomLoggerPtr singleton = create();
          return singleton;
        }

        //-----------------------------------------------------------------------
        virtual void installLogger(ILoggerDelegatePtr delegate)
        {
          SubsystemMap subsystems;

          // scope: remember the client logger delegate and get the subsystems
          {
            AutoRecursiveLock lock(mLock);
            mDelegate = delegate;
            subsystems = mSubsystems;
          }

          if (!delegate)
            return;

          for (SubsystemMap::iterator iter = subsystems.begin(); iter != subsystems.end(); ++iter) {
            delegate->onNewSubsystem((*iter).first, (*iter).second->getName());
          }
        }

        //-----------------------------------------------------------------------
        virtual void onNewSubsystem(Subsystem &inSubsystem)
        {
          ILoggerDelegatePtr delegate;

          // scope: get the delegate
          {
            AutoRecursiveLock lock(mLock);
            mSubsystems[(SubsystemID)(&inSubsystem)] = &inSubsystem;
            delegate = mDelegate;
          }

          if (!delegate)
            return;

          delegate->onNewSubsystem((SubsystemID)(&inSubsystem), inSubsystem.getName());
        }

        //-----------------------------------------------------------------------
        virtual void log(
                         const Subsystem &inSubsystem,
                         zsLib::Log::Severity inSeverity,
                         zsLib::Log::Level inLevel,
                         CSTR inMessage,
                         CSTR inFunction,
                         CSTR inFilePath,
                         ULONG inLineNumber
                         )
        {
          ILoggerDelegatePtr delegate;

          // scope: get the delegate
          {
            AutoRecursiveLock lock(mLock);
            delegate = mDelegate;
          }
          if (!delegate)
            return;

          delegate->onLog(
                          (PTRNUMBER)(&inSubsystem),
                          inSubsystem.getName(),
                          severityToSeverity(inSeverity),
                          levelToLevel(inLevel),
                          inMessage,
                          inFunction,
                          inFilePath,
                          inLineNumber
                          );
        }

      protected:
        RecursiveLock      mLock;
        SubsystemMap       mSubsystems;
        ILoggerDelegatePtr mDelegate;
      };


    }

    //---------------------------------------------------------------------------
    void ILogger::installStdOutLogger(bool colorizeOutput)
    {
      services::IHelper::installStdOutLogger(colorizeOutput);
    }

    //---------------------------------------------------------------------------
    void ILogger::installFileLogger(const char *fileName, bool colorizeOutput)
    {
      services::IHelper::installFileLogger(fileName, colorizeOutput);
    }

    //---------------------------------------------------------------------------
    void ILogger::installTelnetLogger(
                                      WORD listenPort,                             // what port to bind to on 0.0.0.0:port to listen for incoming telnet sessions
                                      ULONG maxSecondsWaitForSocketToBeAvailable,  // since the port might still be in use for a period of time between runs (TCP timeout), how long to wait for the port to come alive (recommend 60)
                                      bool colorizeOutput
                                      )
    {
      services::IHelper::installTelnetLogger(listenPort, maxSecondsWaitForSocketToBeAvailable, colorizeOutput);
    }

    //---------------------------------------------------------------------------
    void ILogger::installOutgoingTelnetLogger(
                                              const char *serverToConnect,
                                              bool colorizeOutput,
                                              const char *stringToSendUponConnection
                                              )
    {
      services::IHelper::installOutgoingTelnetLogger(serverToConnect, colorizeOutput, stringToSendUponConnection);
    }

    //---------------------------------------------------------------------------
    void ILogger::installWindowsDebuggerLogger()
    {
      services::IHelper::installDebuggerLogger();
    }

    //---------------------------------------------------------------------------
    void ILogger::installCustomLogger(ILoggerDelegatePtr delegate)
    {
      internal::CustomLoggerPtr logger = internal::CustomLogger::singleton();
      logger->installLogger(delegate);
    }

    namespace application
    {
      //-------------------------------------------------------------------------
      static PTRNUMBER getGUISubsystem()
      {
        Subsystem *subsystem = &(ZS_GET_SUBSYSTEM());
        return (PTRNUMBER)(subsystem);
      }
    }

    //---------------------------------------------------------------------------
    PTRNUMBER ILogger::getApplicationSubsystemID()
    {
      return application::getGUISubsystem();
    }

    //---------------------------------------------------------------------------
    ILogger::Level ILogger::getLogLevel(SubsystemID subsystemUniqueID)
    {
      return internal::levelToLevel(((Subsystem *)subsystemUniqueID)->getOutputLevel());
    }

    //---------------------------------------------------------------------------
    void ILogger::setLogLevel(ILogger::Level level)
    {
      services::IHelper::setLogLevel(internal::levelToLevel(level));
    }

    //---------------------------------------------------------------------------
    void ILogger::setLogLevel(
                              PTRNUMBER subsystemUniqueID,
                              ILogger::Level level
                              )
    {
      ((Subsystem *)subsystemUniqueID)->setOutputLevel(internal::levelToLevel(level));
    }

    //---------------------------------------------------------------------------
    void ILogger::setLogLevel(
                              const char *subsystemName,
                              ILogger::Level level
                              )
    {
      services::IHelper::setLogLevel(subsystemName, internal::levelToLevel(level));
    }

    //---------------------------------------------------------------------------
    void ILogger::log(
                      PTRNUMBER subsystemUniqueID,
                      ILogger::Severity severity,
                      ILogger::Level level,
                      const char *message,
                      const char *function,
                      const char *filePath,
                      ULONG lineNumber
                      )
    {
      LogPtr log = zsLib::Log::singleton();
      log->log(
               *(((Subsystem *)subsystemUniqueID)),
               internal::severityToSeverity(severity),
               internal::levelToLevel(level),
               message,
               function,
               filePath,
               lineNumber
               );
    }
  }
}
