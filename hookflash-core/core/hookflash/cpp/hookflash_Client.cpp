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

#include <hookflash/internal/hookflash_Client.h>
#include <zsLib/Exception.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Log.h>
#include <hookflash/services/IDNS.h>
#include <hookflash/services/IHelper.h>
#include <zsLib/Socket.h>

#include <map>

namespace hookflash { ZS_DECLARE_SUBSYSTEM(hookflash) }
namespace hookflash { namespace gui { ZS_DECLARE_SUBSYSTEM(hookflash_gui) } }


namespace hookflash
{
  typedef zsLib::UINT UINT;
  typedef zsLib::ULONG ULONG;
  typedef zsLib::CSTR CSTR;
  typedef zsLib::Log Log;
  typedef zsLib::LogPtr LogPtr;
  typedef zsLib::ILogDelegate ILogDelegate;
  typedef zsLib::PTRNUMBER PTRNUMBER;
  typedef zsLib::Subsystem Subsystem;
  typedef zsLib::Lock Lock;
  typedef zsLib::AutoLock AutoLock;
  typedef zsLib::RecursiveLock RecursiveLock;
  typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
  typedef zsLib::Socket Socket;
  typedef zsLib::IMessageQueueThread IMessageQueueThread;
  typedef zsLib::MessageQueue MessageQueue;
  typedef zsLib::MessageQueuePtr MessageQueuePtr;
  typedef zsLib::IMessageQueueNotify IMessageQueueNotify;
  typedef zsLib::IMessageQueueMessagePtr IMessageQueueMessagePtr;
  typedef zsLib::MessageQueueThread MessageQueueThread;
  typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
  typedef zsLib::MessageQueueThreadPtr MessageQueueThreadPtr;
  typedef zsLib::IMessageQueueThreadPtr IMessageQueueThreadPtr;

  namespace internal
  {
    static GlobalClient &getGlobal();

    //-------------------------------------------------------------------------
    static zsLib::Log::Level levelToLevel(IClient::Log::Level level)
    {
      zsLib::Log::Level result = Log::None;
      switch (level) {
        case IClient::Log::None:   result = Log::None; break;
        case IClient::Log::Basic:  result = Log::Basic; break;
        case IClient::Log::Detail: result = Log::Detail; break;
        case IClient::Log::Debug:  result = Log::Debug; break;
        case IClient::Log::Trace:  result = Log::Trace; break;
      }
      return result;
    }

    //-------------------------------------------------------------------------
    static IClient::Log::Level levelToLevel(zsLib::Log::Level level)
    {
      IClient::Log::Level result = IClient::Log::None;
      switch (level) {
        case Log::None:   result = IClient::Log::None; break;
        case Log::Basic:  result = IClient::Log::Basic; break;
        case Log::Detail: result = IClient::Log::Detail; break;
        case Log::Debug:  result = IClient::Log::Debug; break;
        case Log::Trace:  result = IClient::Log::Trace; break;
      }
      return result;
    }

    //-------------------------------------------------------------------------
    static zsLib::Log::Severity severityToSeverity(IClient::Log::Severity severity)
    {
      zsLib::Log::Severity result = Log::Informational;
      switch (severity) {
        case IClient::Log::Informational: result = Log::Informational; break;
        case IClient::Log::Warning:       result = Log::Warning; break;
        case IClient::Log::Error:         result = Log::Error; break;
        case IClient::Log::Fatal:         result = Log::Fatal; break;
      }
      return result;
    }

    //-------------------------------------------------------------------------
    static IClient::Log::Severity severityToSeverity(Log::Severity severity)
    {
      IClient::Log::Severity result = IClient::Log::Informational;
      switch (severity) {
        case Log::Informational: result = IClient::Log::Informational; break;
        case Log::Warning:       result = IClient::Log::Warning; break;
        case Log::Error:         result = IClient::Log::Error; break;
        case Log::Fatal:         result = IClient::Log::Fatal; break;
      }
      return result;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class ClientLogger;
    typedef boost::shared_ptr<ClientLogger> ClientLoggerPtr;
    typedef boost::weak_ptr<ClientLogger> ClientLoggerWeakPtr;

    class ClientLogger : public ILogDelegate
    {
    public:
      typedef std::map<PTRNUMBER, Subsystem *> SubsystemMap;

    protected:
      ClientLogger() {}

    public:
      //-----------------------------------------------------------------------
      static ClientLoggerPtr create() {
        ClientLoggerPtr pThis = ClientLoggerPtr(new ClientLogger);
        (zsLib::Log::singleton())->addListener(pThis);
        return pThis;
      }

      //-----------------------------------------------------------------------
      virtual void installLogger(IClientLogDelegatePtr delegate)
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
        IClientLogDelegatePtr delegate;

        // scope: get the delegate
        {
          AutoRecursiveLock lock(mLock);
          mSubsystems[(PTRNUMBER)(&inSubsystem)] = &inSubsystem;
          delegate = mDelegate;
        }

        if (!delegate)
          return;

        delegate->onNewSubsystem((PTRNUMBER)(&inSubsystem), inSubsystem.getName());
      }

      //-----------------------------------------------------------------------
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
        IClientLogDelegatePtr delegate;

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
      RecursiveLock  mLock;
      SubsystemMap          mSubsystems;
      IClientLogDelegatePtr mDelegate;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class GlobalClient
    {
    public:
      //-----------------------------------------------------------------------
      ClientPtr singleton()
      {
        AutoLock lock(mLock);
        ZS_THROW_INVALID_USAGE_IF(!mClient) // has setup been called?
        return mClient;
      }

      //-----------------------------------------------------------------------
      void setup(IClientDelegatePtr delegate)
      {
        AutoLock lock(mLock);
        ZS_THROW_INVALID_USAGE_IF(mClient)  // already setup?
        mClient = Client::create(delegate);
      }

      //-----------------------------------------------------------------------
      ClientLoggerPtr getLogger()
      {
        AutoLock lock(mLock);
        if (!mLogger)
          mLogger = ClientLogger::create();
        return mLogger;
      }

    private:
      Lock mLock;
      ClientPtr mClient;
      ClientLoggerPtr mLogger;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class GlobalClientInit
    {
    public:
      GlobalClientInit() {getGlobal();}
    };

    static GlobalClientInit gGlobalClientInit;  // ensure the GlobalClient object is constructed

    //-------------------------------------------------------------------------
    static GlobalClient &getGlobal()
    {
      static GlobalClient global;
      return global;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class ClientGUIThread;
    typedef boost::shared_ptr<ClientGUIThread> ClientGUIThreadPtr;
    typedef boost::weak_ptr<ClientGUIThread> ClientGUIThreadWeakPtr;

    class ClientGUIThread : public IMessageQueueThread,
                            public IMessageQueueNotify
    {
    protected:
      //-----------------------------------------------------------------------
      ClientGUIThread(IClientDelegatePtr delegate) : mDelegate(delegate) {
      }

    public:
      //-----------------------------------------------------------------------
      static ClientGUIThreadPtr create(IClientDelegatePtr delegate) {
        ClientGUIThreadPtr pThis(new ClientGUIThread(delegate));
        pThis->mThisWeak = pThis;
        pThis->mQueue = MessageQueue::create(pThis);
        return pThis;
      }

      //-----------------------------------------------------------------------
      virtual void post(IMessageQueueMessagePtr message)
      {
        MessageQueuePtr queue;
        {
          AutoLock lock(mLock);
          queue = mQueue;
        }
        queue->post(message);
      }

      //-----------------------------------------------------------------------
      virtual UINT getTotalUnprocessedMessages() const
      {
        AutoLock lock(mLock);
        return mQueue->getTotalUnprocessedMessages();
      }

      //-----------------------------------------------------------------------
      virtual void waitForShutdown()
      {
        IClientDelegatePtr delegate;
        MessageQueuePtr queue;

        {
          AutoLock lock(mLock);
          delegate = mDelegate;
          queue = mQueue;

          mDelegate.reset();
          mQueue.reset();
        }
      }

      //-----------------------------------------------------------------------
      virtual void notifyMessagePosted()
      {
        IClientDelegatePtr delegate;
        {
          AutoLock lock(mLock);
          delegate = mDelegate;
        }
        delegate->onMessagePutInGUIQueue();
      }

      //-----------------------------------------------------------------------
      void processMessage()
      {
        MessageQueuePtr queue;
        {
          AutoLock lock(mLock);
          queue = mQueue;
        }
        queue->processOnlyOneMessage();
      }

    protected:
      mutable Lock mLock;
      ClientGUIThreadPtr mThisWeak;
      IClientDelegatePtr mDelegate;
      MessageQueuePtr mQueue;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    ClientPtr Client::create(IClientDelegatePtr delegate)
    {
      return ClientPtr(new Client(delegate));
    }

    //-------------------------------------------------------------------------
    Client::Client(IClientDelegatePtr delegate)
    {
      Socket::ignoreSIGPIPEOnThisThread();

      mHookflashThreadQueue = MessageQueueThread::createBasic("com.hookflash.core.mainThread");
      mMediaThreadQueue = MessageQueueThread::createBasic("com.hookflash.core.mediaThread", zsLib::ThreadPriority_RealtimePriority);
      if (!delegate) {
        mGUIThreadQueue = MessageQueueThread::singletonUsingCurrentGUIThreadsMessageQueue();
      } else {
        mGUIThreadQueue = ClientGUIThread::create(delegate);
      }
    }

    //-------------------------------------------------------------------------
    void Client::processMessagePutInGUIQueue()
    {
      ClientGUIThreadPtr thread;
      {
        AutoLock lock(mLock);
        thread = boost::dynamic_pointer_cast<ClientGUIThread>(mGUIThreadQueue);
        ZS_THROW_INVALID_USAGE_IF(!thread)  // you can only call this method if you specified a delegate upon setup and have not already finalized the shutdown
      }

      thread->processMessage();
    }

    //-------------------------------------------------------------------------
    void Client::finalizeShutdown()
    {
      MessageQueueThreadPtr  hookflashThread;
      MessageQueueThreadPtr  mediaThread;
      IMessageQueueThreadPtr guiThread;
      {
        AutoLock lock(mLock);
        hookflashThread = mHookflashThreadQueue;
        mediaThread = mMediaThreadQueue;
        guiThread = mGUIThreadQueue;
      }

      hookflashThread->waitForShutdown();
      guiThread->waitForShutdown();
      mediaThread->waitForShutdown();

      {
        AutoLock lock(mLock);
        mHookflashThreadQueue.reset();
        mMediaThreadQueue.reset();
        mGUIThreadQueue.reset();
      }
    }

    //-------------------------------------------------------------------------
    IMessageQueuePtr Client::hookflashQueue()
    {
      return getGlobal().singleton()->getHookflashQueue();
    }

    //-------------------------------------------------------------------------
    IMessageQueuePtr Client::guiQueue()
    {
      return getGlobal().singleton()->getGUIQueue();
    }

    //-------------------------------------------------------------------------
    IMessageQueuePtr Client::mediaQueue()
    {
      return getGlobal().singleton()->getMediaQueue();
    }

    //-------------------------------------------------------------------------
    IMessageQueueThreadPtr Client::getHookflashQueue() const
    {
      AutoLock lock(mLock);
      return mHookflashThreadQueue;
    }

    //-------------------------------------------------------------------------
    IMessageQueueThreadPtr Client::getGUIQueue() const
    {
      AutoLock lock(mLock);
      return mGUIThreadQueue;
    }

    //-------------------------------------------------------------------------
    IMessageQueueThreadPtr Client::getMediaQueue() const
    {
      AutoLock lock(mLock);
      return mMediaThreadQueue;
    }
  }

  //---------------------------------------------------------------------------
  void IClient::setup(IClientDelegatePtr delegate)
  {
    return internal::getGlobal().setup(delegate);
  }

  //---------------------------------------------------------------------------
  IClientPtr IClient::singleton()
  {
    return internal::getGlobal().singleton();
  }

  //---------------------------------------------------------------------------
  void IClient::installStdOutLogger(bool colorizeOutput)
  {
    services::IHelper::installStdOutLogger(colorizeOutput);
  }

  //---------------------------------------------------------------------------
  void IClient::installFileLogger(const char *fileName, bool colorizeOutput)
  {
    services::IHelper::installFileLogger(fileName, colorizeOutput);
  }

  //---------------------------------------------------------------------------
  void IClient::installTelnetLogger(
                                    WORD listenPort,                             // what port to bind to on 0.0.0.0:port to listen for incoming telnet sessions
                                    ULONG maxSecondsWaitForSocketToBeAvailable,  // since the port might still be in use for a period of time between runs (TCP timeout), how long to wait for the port to come alive (recommend 60)
                                    bool colorizeOutput
                                    )
  {
    services::IHelper::installTelnetLogger(listenPort, maxSecondsWaitForSocketToBeAvailable, colorizeOutput);
  }

  //---------------------------------------------------------------------------
  void IClient::installOutgoingTelnetLogger(
                                            const char *serverToConnect,
                                            bool colorizeOutput,
                                            const char *stringToSendUponConnection
                                            )
  {
    services::IHelper::installOutgoingTelnetLogger(serverToConnect, colorizeOutput, stringToSendUponConnection);
  }

  //---------------------------------------------------------------------------
  void IClient::installWindowsDebuggerLogger()
  {
    services::IHelper::installWindowsDebuggerLogger();
  }

  //---------------------------------------------------------------------------
  void IClient::installCustomLogger(IClientLogDelegatePtr delegate)
  {
    internal::ClientLoggerPtr logger = internal::getGlobal().getLogger();
    logger->installLogger(delegate);
  }

  namespace gui
  {
    //-------------------------------------------------------------------------
    static PTRNUMBER getGUISubsystem()
    {
      Subsystem *subsystem = &(ZS_GET_SUBSYSTEM());
      return (PTRNUMBER)(subsystem);
    }
  }

  //---------------------------------------------------------------------------
  PTRNUMBER IClient::getGUISubsystemUniqueID()
  {
    return gui::getGUISubsystem();
  }

  //---------------------------------------------------------------------------
  IClient::Log::Level IClient::getLogLevel(PTRNUMBER subsystemUniqueID)
  {
    return internal::levelToLevel(((Subsystem *)subsystemUniqueID)->getOutputLevel());
  }

  //---------------------------------------------------------------------------
  void IClient::setLogLevel(IClient::Log::Level level)
  {
    services::IHelper::setLogLevel(internal::levelToLevel(level));
  }

  //---------------------------------------------------------------------------
  void IClient::setLogLevel(
                            PTRNUMBER subsystemUniqueID,
                            IClient::Log::Level level
                            )
  {
    ((Subsystem *)subsystemUniqueID)->setOutputLevel(internal::levelToLevel(level));
  }

  //---------------------------------------------------------------------------
  void IClient::setLogLevel(
                            const char *subsystemName,
                            IClient::Log::Level level
                            )
  {
    services::IHelper::setLogLevel(subsystemName, internal::levelToLevel(level));
  }

  //---------------------------------------------------------------------------
  void IClient::log(
                    PTRNUMBER subsystemUniqueID,
                    IClient::Log::Severity severity,
                    IClient::Log::Level level,
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
