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

#include <hookflash/core/internal/core_Stack.h>
#include <hookflash/core/internal/core_MediaEngine.h>
#include <hookflash/core/IConversationThread.h>
#include <hookflash/core/ICall.h>
#include <hookflash/services/IHelper.h>

#include <hookflash/stack/IStack.h>

#include <zsLib/helpers.h>
#include <zsLib/MessageQueueThread.h>
#include <zsLib/Socket.h>


namespace hookflash { namespace core { ZS_DECLARE_SUBSYSTEM(hookflash_core) } }

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      interaction IShutdownCheckAgainDelegate
      {
        virtual void onShutdownCheckAgain() = 0;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::core::internal::IShutdownCheckAgainDelegate)
ZS_DECLARE_PROXY_METHOD_0(onShutdownCheckAgain)
ZS_DECLARE_PROXY_END()

namespace hookflash
{
  namespace core
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IStackAutoCleanup
    #pragma mark

    interaction IStackAutoCleanup
    {
    };

    namespace internal
    {
      class ShutdownCheckAgain;
      typedef boost::shared_ptr<ShutdownCheckAgain> ShutdownCheckAgainPtr;
      typedef boost::weak_ptr<ShutdownCheckAgain> ShutdownCheckAgainWeakPtr;

      class StackAutoCleanup;
      typedef boost::shared_ptr<StackAutoCleanup> StackAutoCleanupPtr;
      typedef boost::weak_ptr<StackAutoCleanup> StackAutoCleanupWeakPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ShutdownCheckAgain
      #pragma mark

      class ShutdownCheckAgain : public IShutdownCheckAgainDelegate,
                                 public zsLib::MessageQueueAssociator
      {
      protected:
        //---------------------------------------------------------------------
        ShutdownCheckAgain(
                           IMessageQueuePtr queue,
                           StackPtr stack
                           ) :
          zsLib::MessageQueueAssociator(queue),
          mStack(stack)
        {}

      public:
        //---------------------------------------------------------------------
        static ShutdownCheckAgainPtr create(
                                            IMessageQueuePtr queue,
                                            StackPtr stack
                                            )
        {
          return ShutdownCheckAgainPtr(new ShutdownCheckAgain(queue, stack));
        }

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ShutdownCheckAgain => IShutdownCheckAgainDelegate
        #pragma mark

        //---------------------------------------------------------------------
        virtual void onShutdownCheckAgain()
        {
          mStack->forShutdownCheckAgain().notifyShutdownCheckAgain();
        }

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ShutdownCheckAgain => (data)
        #pragma mark

        StackPtr mStack;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark StackAutoCleanup
      #pragma mark

      class StackAutoCleanup : public IStackAutoCleanup
      {
      protected:
        StackAutoCleanup() {}

      public:
        ~StackAutoCleanup()
        {
          IStackForInternal::finalShutdown();
        }

        static StackAutoCleanupPtr create()
        {
          return StackAutoCleanupPtr(new StackAutoCleanup);
        }
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack
      #pragma mark

      class InterceptApplicationThread;
      typedef boost::shared_ptr<InterceptApplicationThread> InterceptApplicationThreadPtr;
      typedef boost::weak_ptr<InterceptApplicationThread> InterceptApplicationThreadWeakPtr;

      class InterceptApplicationThread : public IMessageQueueThread,
                                         public IMessageQueueNotify
      {
      public:

      protected:
        //---------------------------------------------------------------------
        InterceptApplicationThread(IStackMessageQueueDelegatePtr delegate) :
          mDelegate(delegate)
        {
        }

      public:
        //---------------------------------------------------------------------
        static InterceptApplicationThreadPtr create(IStackMessageQueueDelegatePtr delegate) {
          InterceptApplicationThreadPtr pThis(new InterceptApplicationThread(delegate));
          pThis->mThisWeak = pThis;
          pThis->mQueue = MessageQueue::create(pThis);
          return pThis;
        }

        //---------------------------------------------------------------------
        virtual void post(IMessageQueueMessagePtr message)
        {
          MessageQueuePtr queue;
          {
            AutoLock lock(mLock);
            queue = mQueue;
          }
          queue->post(message);
        }

        //---------------------------------------------------------------------
        virtual UINT getTotalUnprocessedMessages() const
        {
          AutoLock lock(mLock);
          return mQueue->getTotalUnprocessedMessages();
        }

        //---------------------------------------------------------------------
        virtual void waitForShutdown()
        {
          IStackMessageQueueDelegatePtr delegate;
          MessageQueuePtr queue;

          {
            AutoLock lock(mLock);
            delegate = mDelegate;
            queue = mQueue;

            mDelegate.reset();
            mQueue.reset();
          }
        }

        //---------------------------------------------------------------------
        virtual void notifyMessagePosted()
        {
          IStackMessageQueueDelegatePtr delegate;
          {
            AutoLock lock(mLock);
            delegate = mDelegate;
          }
          delegate->onStackMessageQueueWakeUpCustomThreadAndProcess();
        }

        //---------------------------------------------------------------------
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
        InterceptApplicationThreadPtr mThisWeak;
        IStackMessageQueueDelegatePtr mDelegate;
        MessageQueuePtr mQueue;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IStackForInternal
      #pragma mark

      //-----------------------------------------------------------------------
      const String &IStackForInternal::appID()
      {
        return (Stack::singleton())->getAppID();
      }

      //-----------------------------------------------------------------------
      const String &IStackForInternal::appName()
      {
        return (Stack::singleton())->getAppName();
      }

      //-----------------------------------------------------------------------
      const String &IStackForInternal::appImageURL()
      {
        return (Stack::singleton())->getAppImageURL();
      }

      //-----------------------------------------------------------------------
      const String &IStackForInternal::appURL()
      {
        return (Stack::singleton())->getAppURL();
      }

      //-----------------------------------------------------------------------
      const String &IStackForInternal::userAgent()
      {
        return (Stack::singleton())->getUserAgent();
      }

      //-----------------------------------------------------------------------
      const String &IStackForInternal::deviceID()
      {
        return (Stack::singleton())->getDeviceID();
      }

      //-----------------------------------------------------------------------
      const String &IStackForInternal::os()
      {
        return (Stack::singleton())->getOS();
      }

      //-----------------------------------------------------------------------
      const String &IStackForInternal::system()
      {
        return (Stack::singleton())->getSystem();
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr IStackForInternal::queueApplication()
      {
        return (Stack::singleton())->getQueueApplication();
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr IStackForInternal::queueCore()
      {
        return (Stack::singleton())->getQueueCore();
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr IStackForInternal::queueMedia()
      {
        return (Stack::singleton())->getQueueMedia();
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr IStackForInternal::queueServices()
      {
        return (Stack::singleton())->getQueueServices();
      }

      //-----------------------------------------------------------------------
      IMediaEngineDelegatePtr IStackForInternal::mediaEngineDelegate()
      {
        return (Stack::singleton())->getMediaEngineDelegate();
      }

      //-----------------------------------------------------------------------
      void IStackForInternal::finalShutdown()
      {
        return (Stack::singleton())->finalShutdown();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack
      #pragma mark

      //-----------------------------------------------------------------------
      Stack::Stack() :
        mID(zsLib::createPUID())
      {
      }

      //-----------------------------------------------------------------------
      StackPtr Stack::convert(IStackPtr stack)
      {
        return boost::dynamic_pointer_cast<Stack>(stack);
      }

      //-----------------------------------------------------------------------
      StackPtr Stack::create()
      {
        StackPtr pThis(new Stack());
        pThis->mThisWeak = pThis;
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack => IStack
      #pragma mark

      //-----------------------------------------------------------------------
      StackPtr Stack::singleton()
      {
        static StackPtr singleton = Stack::create();
        return singleton;
      }

      //-----------------------------------------------------------------------
      void Stack::setup(
                        IStackDelegatePtr stackDelegate,
                        IMediaEngineDelegatePtr mediaEngineDelegate,
                        const char *appID,
                        const char *appName,
                        const char *appImageURL,
                        const char *appURL,
                        const char *userAgent,
                        const char *deviceID,
                        const char *os,
                        const char *system
                        )
      {
        AutoRecursiveLock lock(mLock);

        makeReady();

        if (stackDelegate) {
          mStackDelegate = IStackDelegateProxy::create(getQueueApplication(), stackDelegate);
        }

        if (mediaEngineDelegate) {
          mMediaEngineDelegate = IMediaEngineDelegateProxy::create(getQueueApplication(), mediaEngineDelegate);
          IMediaEngineForStack::setup(mMediaEngineDelegate);
        }
        
        if (appID) {
          mAppID = appID;
        }
        if (appName) {
          mAppName = appName;
        }
        if (appImageURL) {
          mAppImageURL = appImageURL;
        }
        if (appURL) {
          mAppURL = appURL;
        }
        if (userAgent) {
          mUserAgent = String(userAgent);
        }
        if (deviceID) {
          mDeviceID = String(deviceID);
        }
        if (os) {
          mOS = String(os);
        }
        if (system) {
          mSystem = String(system);
        }

        ZS_THROW_INVALID_ARGUMENT_IF(mAppID.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mAppName.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mAppImageURL.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mAppURL.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mUserAgent.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mDeviceID.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mOS.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mSystem.isEmpty())

        stack::IStack::setup(mApplicationThreadQueue, mCoreThreadQueue, mServicesThreadQueue, mAppID, mAppName, mAppImageURL, mAppURL, mUserAgent, mDeviceID, mOS, mSystem);
      }

      //-----------------------------------------------------------------------
      void Stack::shutdown()
      {
        AutoRecursiveLock lock(mLock);

        if (!mApplicationThreadQueue) {
          // already shutdown...
          return;
        }

        if (mShutdownCheckAgainDelegate) {
          // already shutting down...
          return;
        }

        ShutdownCheckAgainPtr checkAgain = ShutdownCheckAgain::create(getQueueApplication(), mThisWeak.lock());
        mShutdownCheckAgainDelegate = IShutdownCheckAgainDelegateProxy::create(checkAgain);

        mShutdownCheckAgainDelegate->onShutdownCheckAgain();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack => IStackMessageQueue
      #pragma mark

      //-----------------------------------------------------------------------
      void Stack::interceptProcessing(IStackMessageQueueDelegatePtr delegate)
      {
        ZS_THROW_INVALID_USAGE_IF(mApplicationThreadQueue)
        mStackMessageQueueDelegate = delegate;
      }

      //-----------------------------------------------------------------------
      void Stack::notifyProcessMessageFromCustomThread()
      {
        InterceptApplicationThreadPtr thread;
        {
          AutoRecursiveLock lock(mLock);
          thread = boost::dynamic_pointer_cast<InterceptApplicationThread>(mApplicationThreadQueue);
          ZS_THROW_INVALID_USAGE_IF(!thread)  // you can only call this method if you specified a delegate upon setup and have not already finalized the shutdown
        }

        thread->processMessage();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack => IStackForShutdownCheckAgain
      #pragma mark

      //-----------------------------------------------------------------------
      void Stack::notifyShutdownCheckAgain()
      {
        ZS_TRACE_THIS()

        AutoRecursiveLock lock(mLock);

        ZS_THROW_BAD_STATE_IF(!mShutdownCheckAgainDelegate)

        UINT total = 0;

        total = mApplicationThreadQueue->getTotalUnprocessedMessages() +
                mCoreThreadQueue->getTotalUnprocessedMessages() +
                mMediaThreadQueue->getTotalUnprocessedMessages() +
                mServicesThreadQueue->getTotalUnprocessedMessages();

        if (total > 0) {
          mShutdownCheckAgainDelegate->onShutdownCheckAgain();
          return;
        }

        // all activity has ceased on the threads so clean out the delegates remaining in this class
        mMediaEngineDelegate.reset();

        // cleaning the delegates could cause more activity to start
        total = mApplicationThreadQueue->getTotalUnprocessedMessages() +
                mCoreThreadQueue->getTotalUnprocessedMessages() +
                mMediaThreadQueue->getTotalUnprocessedMessages() +
                mServicesThreadQueue->getTotalUnprocessedMessages();

        if (total > 0) {
          mShutdownCheckAgainDelegate->onShutdownCheckAgain();
          return;
        }

        // delegates are now gone and all activity has stopped, stopped sending notifications to self via the GUI thread
        mShutdownCheckAgainDelegate.reset();

        IStackAutoCleanupPtr autoCleanup = StackAutoCleanup::create();

        // notify the GUI thread it is now safe to finalize the shutdown
        mStackDelegate->onStackShutdown(autoCleanup);
        mStackDelegate.reset();

        // the telnet logger must disconnect here before anything can continue
        services::IHelper::uninstallTelnetLogger();

        // at this point all proxies to delegates should be completely destroyed - if they are not then someone forgot to do some clean-up!
        ULONG totalProxiesCreated = zsLib::proxyGetTotalConstructed();
        zsLib::proxyDump();
        ZS_THROW_BAD_STATE_IF(totalProxiesCreated > 0)  // DO NOT COMMENT THIS LINE AS A SOLUTION INSTEAD OF FINDING OUT WHERE YOU DID NOT SHUTDOWN/CLEANUP PROPERLY
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack => IStackForInternal
      #pragma mark

      //-----------------------------------------------------------------------
      IMessageQueuePtr Stack::getQueueApplication() const
      {
        AutoRecursiveLock lock(mLock);
        return mApplicationThreadQueue;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Stack::getQueueCore() const
      {
        AutoRecursiveLock lock(mLock);
        return mCoreThreadQueue;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Stack::getQueueMedia() const
      {
        AutoRecursiveLock lock(mLock);
        return mMediaThreadQueue;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Stack::getQueueServices() const
      {
        AutoRecursiveLock lock(mLock);
        return mServicesThreadQueue;
      }

      //-----------------------------------------------------------------------
      IMediaEngineDelegatePtr Stack::getMediaEngineDelegate() const
      {
        AutoRecursiveLock lock(mLock);
        return mMediaEngineDelegate;
      }

      //-----------------------------------------------------------------------
      void Stack::finalShutdown()
      {
        IMessageQueueThreadPtr applicationThread;
        MessageQueueThreadPtr  coreThread;
        MessageQueueThreadPtr  mediaThread;
        MessageQueueThreadPtr  servicesThread;
        IStackMessageQueueDelegatePtr stackMessage;

        {
          AutoRecursiveLock lock(mLock);
          applicationThread = mApplicationThreadQueue;
          coreThread = mCoreThreadQueue;
          mediaThread = mMediaThreadQueue;
          servicesThread = mServicesThreadQueue;
          stackMessage = mStackMessageQueueDelegate;
        }

        applicationThread->waitForShutdown();
        coreThread->waitForShutdown();
        mediaThread->waitForShutdown();
        servicesThread->waitForShutdown();

        {
          AutoRecursiveLock lock(mLock);
          mApplicationThreadQueue.reset();
          mCoreThreadQueue.reset();
          mMediaThreadQueue.reset();
          mServicesThreadQueue.reset();
          mStackMessageQueueDelegate.reset();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      void Stack::makeReady()
      {
        if (mCoreThreadQueue) return;

        Socket::ignoreSIGPIPEOnThisThread();

        mCoreThreadQueue = MessageQueueThread::createBasic("com.hookflash.core.mainThread");
        mMediaThreadQueue = MessageQueueThread::createBasic("com.hookflash.core.mediaThread", zsLib::ThreadPriority_RealtimePriority);
        mServicesThreadQueue = MessageQueueThread::createBasic("com.hookflash.core.servicesThread");
        if (!mStackMessageQueueDelegate) {
          mApplicationThreadQueue = MessageQueueThread::singletonUsingCurrentGUIThreadsMessageQueue();
        } else {
          mApplicationThreadQueue = InterceptApplicationThread::create(mStackMessageQueueDelegate);
          mStackMessageQueueDelegate.reset();
        }
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IStack
    #pragma mark

    //-------------------------------------------------------------------------
    IStackPtr IStack::singleton()
    {
      return internal::Stack::singleton();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IStackMessageQueue
    #pragma mark

    //-------------------------------------------------------------------------
    IStackMessageQueuePtr IStackMessageQueue::singleton()
    {
      return internal::Stack::singleton();
    }
  }
}
