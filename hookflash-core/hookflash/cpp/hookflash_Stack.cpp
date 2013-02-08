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

#include <hookflash/internal/hookflash_Stack.h>
#include <hookflash/internal/hookflash_Client.h>
#include <hookflash/internal/hookflash_MediaEngine.h>
#include <hookflash/IConversationThread.h>
#include <hookflash/ICall.h>
#include <zsLib/Exception.h>
#include <zsLib/Log.h>
#include <hookflash/services/IHelper.h>

#include <zsLib/Proxy.h>

namespace hookflash { ZS_DECLARE_SUBSYSTEM(hookflash) }

using zsLib::Proxy;
using zsLib::AutoRecursiveLock;
using zsLib::IMessageQueuePtr;
using zsLib::UINT;
using zsLib::ULONG;

namespace hookflash
{
  namespace internal
  {
    interaction IStackShutdownCheckAgain;
    typedef boost::shared_ptr<IStackShutdownCheckAgain> IStackShutdownCheckAgainPtr;
    typedef boost::weak_ptr<IStackShutdownCheckAgain> IStackShutdownCheckAgainWeakPtr;
    typedef zsLib::Proxy<IStackShutdownCheckAgain> IStackShutdownCheckAgainProxy;

    interaction IStackShutdownCheckAgain
    {
      virtual void onCheckAgain() = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::internal::IStackShutdownCheckAgain)
ZS_DECLARE_PROXY_METHOD_0(onCheckAgain)
ZS_DECLARE_PROXY_END()

namespace hookflash
{
  namespace internal
  {
    class StackShutdwonCheckAgain;
    typedef boost::shared_ptr<StackShutdwonCheckAgain> StackShutdwonCheckAgainPtr;
    typedef boost::weak_ptr<StackShutdwonCheckAgain> StackShutdwonCheckAgainWeakPtr;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class StackShutdwonCheckAgain : public IStackShutdownCheckAgain,
                                    public zsLib::MessageQueueAssociator
    {
    protected:
      StackShutdwonCheckAgain(
                              IMessageQueuePtr queue,
                              StackPtr stack
                              ) :
        zsLib::MessageQueueAssociator(queue),
        mStack(stack)
      {}

    public:
      static StackShutdwonCheckAgainPtr create(
                                               IMessageQueuePtr queue,
                                               StackPtr stack
                                               ) {
        return StackShutdwonCheckAgainPtr(new StackShutdwonCheckAgain(queue, stack));
      }

      virtual void onCheckAgain()
      {
        mStack->onShutdownCheckAgain();
      }

    protected:
      StackPtr mStack;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    StackPtr Stack::convert(IStackPtr stack)
    {
      return boost::dynamic_pointer_cast<Stack>(stack);
    }

    //-------------------------------------------------------------------------
    StackPtr Stack::create(
                           IStackDelegatePtr stackDelegate,
                           IMediaEngineDelegatePtr mediaEngineDelegate,
                           IConversationThreadDelegatePtr conversationThreadDelegate,
                           ICallDelegatePtr callDelegate,
                           const char *deviceID,
                           const char *userAgent,
                           const char *os,
                           const char *system
                           )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!deviceID);
      ZS_THROW_INVALID_ARGUMENT_IF(!userAgent);
      StackPtr pThis(new Stack(
                               stackDelegate,
                               mediaEngineDelegate,
                               conversationThreadDelegate,
                               callDelegate,
                               deviceID,
                               userAgent,
                               os,
                               system
                               )
                     );
      pThis->mThisWeak = pThis;
      return pThis;
    }

    //-------------------------------------------------------------------------
    Stack::Stack(
                 IStackDelegatePtr stackDelegate,
                 IMediaEngineDelegatePtr mediaEngineDelegate,
                 IConversationThreadDelegatePtr conversationThreadDelegate,
                 ICallDelegatePtr callDelegate,
                 const char *deviceID,
                 const char *userAgent,
                 const char *os,
                 const char *system
                 ) :
      mStackDelegate(IStackDelegateProxy::create(Client::guiQueue(), stackDelegate)),
      mMediaEngineDelegate(IMediaEngineDelegateProxy::create(Client::guiQueue(), mediaEngineDelegate)),
      mConversationThreadDelegate(IConversationThreadDelegateProxy::create(Client::guiQueue(), conversationThreadDelegate)),
      mCallDelegate(ICallDelegateProxy::create(Client::guiQueue(), callDelegate)),
      mDeviceID(deviceID ? deviceID : ""),
      mUserAgent(userAgent ? userAgent : ""),
      mOS(os ? os : ""),
      mSystem(system ? system : "")
    {
      ZS_THROW_INVALID_ARGUMENT_IF(mDeviceID.isEmpty())
      ZS_THROW_INVALID_ARGUMENT_IF(mUserAgent.isEmpty())
      ZS_THROW_INVALID_ARGUMENT_IF(mOS.isEmpty())
      ZS_THROW_INVALID_ARGUMENT_IF(mSystem.isEmpty())
      
      IMediaEngineForStack::setup(mediaEngineDelegate);
    }

    //-------------------------------------------------------------------------
    void Stack::startShutdown()
    {
      AutoRecursiveLock lock(mLock);

      ZS_THROW_INVALID_USAGE_IF(mCheckShutdown) // should not call this routine twice

      StackShutdwonCheckAgainPtr checkAgain = StackShutdwonCheckAgain::create(Client::guiQueue(), mThisWeak.lock());
      mCheckShutdown = IStackShutdownCheckAgainProxy::create(checkAgain);

      mCheckShutdown->onCheckAgain();
    }

    //-------------------------------------------------------------------------
    void Stack::onShutdownCheckAgain()
    {
      ZS_TRACE_THIS()

      AutoRecursiveLock lock(mLock);
      ZS_THROW_INVALID_USAGE_IF(!mCheckShutdown)

      IMessageQueuePtr guiQueue = Client::guiQueue();
      IMessageQueuePtr hookflashQueue = Client::hookflashQueue();

      UINT total = 0;

      total = guiQueue->getTotalUnprocessedMessages() + hookflashQueue->getTotalUnprocessedMessages();
      if (total > 0) {
        mCheckShutdown->onCheckAgain();
        return;
      }

      // all activity has ceased on the threads so clean out the delegates remaining in this class
      mConversationThreadDelegate.reset();
      mCallDelegate.reset();

      // cleaning the delegates could cause more activity to start
      total = guiQueue->getTotalUnprocessedMessages() + hookflashQueue->getTotalUnprocessedMessages();
      if (total > 0) {
        mCheckShutdown->onCheckAgain();
        return;
      }

      // delegates are now gone and all activity has stopped, stopped sending notifications to self via the GUI thread
      mCheckShutdown.reset();

      // notify the GUI thread it is now safe to finalize the shutdown
      mStackDelegate->onShutdownReady();
      mStackDelegate.reset();

      // the telnet logger must disconnect here before anything can continue
      services::IHelper::uninstallTelnetLogger();

      // at this point all proxies to delegates should be completely destroyed - if they are not then someone forgot to do some clean-up!
      ULONG totalProxiesCreated = zsLib::proxyGetTotalConstructed();
      zsLib::proxyDump();
      ZS_THROW_BAD_STATE_IF(totalProxiesCreated > 0)  // DO NOT COMMENT THIS LINE AS A SOLUTION INSTEAD OF FINDING OUT WHERE YOU DID NOT SHUTDOWN/CLEANUP PROPERLY
    }
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  IStackPtr IStack::create(
                           IStackDelegatePtr stackDelegate,
                           IMediaEngineDelegatePtr mediaEngineDelegate,
                           IConversationThreadDelegatePtr conversationThreadDelegate,
                           ICallDelegatePtr callDelegate,
                           const char *deviceID,
                           const char *userAgent,
                           const char *os,
                           const char *system
                           )
  {
    return internal::Stack::create(
                                   stackDelegate,
                                   mediaEngineDelegate,
                                   conversationThreadDelegate,
                                   callDelegate,
                                   deviceID,
                                   userAgent,
                                   os,
                                   system
                                   );
  }
}
