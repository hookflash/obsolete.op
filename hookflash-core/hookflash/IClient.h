/*
 
 Copyright (c) 2012, SMB Phone Inc.
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

#include <hookflash/hookflashTypes.h>

namespace hookflash
{
  //==========================================================================
  //==========================================================================
  //==========================================================================
  interaction IClient
  {
    typedef zsLib::WORD WORD;
    typedef zsLib::ULONG ULONG;
    typedef zsLib::PTRNUMBER PTRNUMBER;

    struct Log
    {
      enum Severity {
        Informational,
        Warning,
        Error,
        Fatal
      };

      enum Level {
        None,
        Basic,
        Detail,
        Debug,
        Trace
      };
    };

    //-------------------------------------------------------------------------
    // PURPOSE: This routine must be called only ONCE from the GUI thread.
    //          This method will through an invalid usage exception if the
    //          IClient::setup routine was already called.
    // PARAMS:  delegate - the delegate param may be NULL if on Windows, OSX
    //                     IOS but must be set if on the Andriod platform. If
    //                     NULL, the IClient will attempt to install a hidden
    //                     GUI message queue handler on the GUI thread to
    //                     handle messages sent to the GUI thread.
    static void setup(IClientDelegatePtr delegate = IClientDelegatePtr());

    //-------------------------------------------------------------------------
    // PURPOSE: Obtain a pointer to the previously setup IClient. This routine
    //          will throw an invalid usage exception if the IClient::setup
    //          method was not previously called.
    static IClientPtr singleton();

    //-------------------------------------------------------------------------
    // PURPOSE: This is to notify the hookflash client that a messaage put
    //          GUI queue is now being processed by the GUI queue.
    virtual void processMessagePutInGUIQueue() = 0;

    //-------------------------------------------------------------------------
    // PURPOSE: This method will block until the hookflash API is fully
    //          shutdown.
    virtual void finalizeShutdown() = 0;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // LOGGING ROUTINES

    //-------------------------------------------------------------------------
    // PURPOSE: Install a logger to output to the standard out
    static void installStdOutLogger(bool colorizeOutput);

    //-------------------------------------------------------------------------
    // PURPOSE: Install a logger to output to a file.
    // NOTE:    On a linux based system you can do this trick:
    //
    //          From the command prompt type:
    //          mkfifo /tmp/hookflash.fifo
    //          while true; do cat /tmp/hookflash.fifo; sleep 1; done
    //
    //          Pass in filename as: "/tmp/hookflash.fifo"
    //
    //          Alternatively, pass in the name of a file and it will create
    //          the file and output all logging to the file.
    static void installFileLogger(const char *fileName, bool colorizeOutput);

    //-------------------------------------------------------------------------
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

    //-------------------------------------------------------------------------
    // PURPOSE: Install a logger that sends a telnet outgoing to a telnet
    //          server.
    static void installOutgoingTelnetLogger(
                                            const char *serverToConnect,
                                            bool colorizeOutput,
                                            const char *stringToSendUponConnection = NULL
                                            );

    //-------------------------------------------------------------------------
    // PURPOSE: Install a logger to output to the windows debugger window.
    static void installWindowsDebuggerLogger();

    //-------------------------------------------------------------------------
    // PURPOSE: Install a logger to monitor the functioning of the application
    //          internally.
    static void installCustomLogger(IClientLogDelegatePtr delegate = IClientLogDelegatePtr());

    //------------------------------------------------------------------------
    // PURPOSE: Gets the unique ID for the GUI's subsystem (to pass into the
    //          log routine for GUI logging)
    static PTRNUMBER getGUISubsystemUniqueID();

    //-------------------------------------------------------------------------
    // PURPOSE: Gets the currently set log level for a particular subsystem.
    static Log::Level getLogLevel(PTRNUMBER subsystemUniqueID);

    //-------------------------------------------------------------------------
    // PURPOSE: Sets all subsystems to a specific log level.
    static void setLogLevel(Log::Level level);

    //-------------------------------------------------------------------------
    // PURPOSE: Sets a particular subsystem's log level by unique ID.
    static void setLogLevel(
                            PTRNUMBER subsystemUniqueID,
                            Log::Level level
                            );
    //-------------------------------------------------------------------------
    // PURPOSE: Sets a particular subsystem's log level by its subsystem name.
    static void setLogLevel(
                            const char *subsystemName,
                            Log::Level level
                            );

    //-------------------------------------------------------------------------
    // PURPOSE: Sends a message to the logger(s) for a particular subsystem.
    static void log(
                    PTRNUMBER subsystemUniqueID,
                    Log::Severity severity,
                    Log::Level level,
                    const char *message,
                    const char *function,
                    const char *filePath,
                    ULONG lineNumber
                    );
  };


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  interaction IClientDelegate
  {
    //-------------------------------------------------------------------------
    // PURPOSE: This is to notify the GUI thread that a message was put in
    //          queue associated with the GUI thread. The GUI should respond
    //          by posting a custom message to its own GUI queue and upon
    //          recieving that messaage it must call:
    //
    //          IClient::processMessagePutInGUIQueue()
    //
    // WARNING: This method may be called from ANY thread thus this method
    //          must be thread safe. zsLib::Proxy should NOT be used to create
    //          a proxy to this delegate.
    virtual void onMessagePutInGUIQueue() = 0;
  };

  interaction IClientLogDelegate
  {
    typedef zsLib::ULONG ULONG;
    typedef zsLib::PTRNUMBER PTRNUMBER;

    //-------------------------------------------------------------------------
    // WARNING: These methods may be called from ANY thread thus these methods
    //          must be thread safe. zsLib::Proxy should NOT be used to create
    //          a proxy to this delegate.
    virtual void onNewSubsystem(
                                PTRNUMBER subsystemUniqueID,
                                const char *subsystemName
                                ) = 0;

    virtual void onLog(
                       PTRNUMBER subsystemUniqueID,
                       const char *subsystemName,
                       IClient::Log::Severity severity,
                       IClient::Log::Level level,
                       const char *message,
                       const char *function,
                       const char *filePath,
                       ULONG lineNumber
                       ) = 0;
  };
}
