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

#pragma once

#include <openpeer/core/types.h>

namespace openpeer
{
  namespace core
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ILogger
    #pragma mark

    interaction ILogger
    {
      enum Severity
      {
        Informational,
        Warning,
        Error,
        Fatal
      };

      enum Level
      {
        None,     // no logging of any kind
        Basic,    // most basic logging only
        Detail,
        Debug,
        Trace     // most detailed log level
      };

      static const char *toString(Severity severity);
      static const char *toString(Level level);

      //-----------------------------------------------------------------------
      // PURPOSE: Install a logger to output to the standard out
      static void installStdOutLogger(bool colorizeOutput);

      //-----------------------------------------------------------------------
      // PURPOSE: Install a logger to output to a file.
      // NOTE:    On a linux based system you can do this trick:
      //
      //          From the command prompt type:
      //          mkfifo /tmp/openpeer.fifo
      //          while true; do cat /tmp/openpeer.fifo; sleep 1; done
      //
      //          Pass in filename as: "/tmp/openpeer.fifo"
      //
      //          Alternatively, pass in the name of a file and it will create
      //          the file and output all logging to the file.
      static void installFileLogger(const char *fileName, bool colorizeOutput);

      //-----------------------------------------------------------------------
      // PURPOSE: Install a logger to output to a telnet prompt.
      // NOTE:    If listen port 59999 is used, then from the system type:
      //
      //          telnet 1.2.3.4 59999
      //
      //          (where 1.2.3.4 is the IP address of the client software)
      //
      static void installTelnetLogger(
                                      WORD listenPort,                             // what port to bind to on 0.0.0.0:port to listen for incoming telnet sessions
                                      ULONG maxSecondsWaitForSocketToBeAvailable,  // since the port might still be in use for a period of time between runs (TCP timeout), how long to wait for the port to come alive (recommend 60)
                                      bool colorizeOutput
                                      );

      //-----------------------------------------------------------------------
      // PURPOSE: Install a logger that sends a telnet outgoing to a telnet
      //          listening server.
      static void installOutgoingTelnetLogger(
                                              const char *serverToConnect,
                                              bool colorizeOutput,
                                              const char *stringToSendUponConnection = NULL
                                              );

      //-----------------------------------------------------------------------
      // PURPOSE: Install a logger to output to the debugger window.
      static void installDebuggerLogger();

      //-----------------------------------------------------------------------
      // PURPOSE: Install a logger to monitor the functioning of the application
      //          internally.
      static void installCustomLogger(ILoggerDelegatePtr delegate = ILoggerDelegatePtr());

      //-----------------------------------------------------------------------
      // PURPOSE: Mirror methods to install routines to uninstall various
      //          types of loggers.
      static void uninstallFileLogger();
      static void uninstallTelnetLogger();
      static void uninstallOutgoingTelnetLogger();
      static void uninstallDebuggerLogger();

      //-----------------------------------------------------------------------
      // PURPOSE: Gets the unique ID for the application's subsystem
      //          (to pass into the log routine for GUI application logging)
      static SubsystemID getApplicationSubsystemID();

      //-----------------------------------------------------------------------
      // PURPOSE: Gets the currently set log level for a particular subsystem.
      static Level getLogLevel(SubsystemID subsystemUniqueID);

      //-----------------------------------------------------------------------
      // PURPOSE: Filter all subsystems to a specific log level.
      // NOTE:    The log level will log at this level and above.
      static void setLogLevel(Level level);

      //-----------------------------------------------------------------------
      // PURPOSE: Sets a particular subsystem's log level by unique ID.
      // NOTE:    The log level will log at this level and above.
      static void setLogLevel(
                              SubsystemID subsystemUniqueID,
                              Level level
                              );

      //-----------------------------------------------------------------------
      // PURPOSE: Sets a particular subsystem's log level by its subsystem name.
      static void setLogLevel(
                              const char *subsystemName,
                              Level level
                              );

      //-----------------------------------------------------------------------
      // PURPOSE: Sends a message to the logger(s) for a particular subsystem.
      static void log(
                      SubsystemID subsystemUniqueID,
                      Severity severity,
                      Level level,
                      const char *message,
                      const char *function,
                      const char *filePath,
                      ULONG lineNumber
                      );
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ILoggerDelegate
    #pragma mark

    interaction ILoggerDelegate
    {
      typedef ILogger::Severity Severity;
      typedef ILogger::Level Level;

      //-----------------------------------------------------------------------
      // PURPOSE: Notify that a new subsystem has been created.
      // WARNING: These methods may be called from ANY thread thus these methods
      //          must be thread safe and non-blocking. zsLib::Proxy should *NOT*
      //          be used to create a proxy to this delegate.
      virtual void onNewSubsystem(
                                  SubsystemID subsystemUniqueID,
                                  const char *subsystemName
                                  ) = 0;

      //-----------------------------------------------------------------------
      // PURPOSE: Notify a new log message has been created.
      // WARNING: These methods may be called from ANY thread thus these methods
      //          must be thread safe and non-blocking. zsLib::Proxy should *NOT*
      //          be used to create a proxy to this delegate.
      virtual void onLog(
                         SubsystemID subsystemUniqueID,
                         const char *subsystemName,
                         Severity severity,
                         Level level,
                         const char *message,
                         const char *function,
                         const char *filePath,
                         ULONG lineNumber
                         ) = 0;
    };
  }
}
