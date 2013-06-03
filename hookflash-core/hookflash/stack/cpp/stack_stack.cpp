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

#include <hookflash/stack/internal/stack_Stack.h>
#include <hookflash/stack/message/bootstrapped-finder/MessageFactoryBootstrappedFinder.h>
#include <hookflash/stack/message/bootstrapper/MessageFactoryBootstrapper.h>
#include <hookflash/stack/message/certificates/MessageFactoryCertificates.h>
#include <hookflash/stack/message/identity/MessageFactoryIdentity.h>
#include <hookflash/stack/message/identity-lookup/MessageFactoryIdentityLookup.h>
#include <hookflash/stack/message/identity-lockbox/MessageFactoryIdentityLockbox.h>
#include <hookflash/stack/message/namespace-grant/MessageFactoryNamespaceGrant.h>
#include <hookflash/stack/message/rolodex/MessageFactoryRolodex.h>
#include <hookflash/stack/message/peer/MessageFactoryPeer.h>
#include <hookflash/stack/message/peer-common/MessageFactoryPeerCommon.h>
#include <hookflash/stack/message/peer-finder/MessageFactoryPeerFinder.h>
#include <hookflash/stack/message/peer-salt/MessageFactoryPeerSalt.h>
#include <hookflash/stack/message/peer-to-peer/MessageFactoryPeerToPeer.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>
#include <zsLib/Stringize.h>

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;
      using message::bootstrapped_finder::MessageFactoryBootstrappedFinder;
      using message::bootstrapper::MessageFactoryBootstrapper;
      using message::certificates::MessageFactoryCertificates;
      using message::identity::MessageFactoryIdentity;
      using message::identity_lookup::MessageFactoryIdentityLookup;
      using message::identity_lockbox::MessageFactoryIdentityLockbox;
      using message::namespace_grant::MessageFactoryNamespaceGrant;
      using message::rolodex::MessageFactoryRolodex;
      using message::peer::MessageFactoryPeer;
      using message::peer_common::MessageFactoryPeerCommon;
      using message::peer_finder::MessageFactoryPeerFinder;
      using message::peer_salt::MessageFactoryPeerSalt;
      using message::peer_to_peer::MessageFactoryPeerToPeer;

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
      AgentInfo IStackForInternal::agentInfo()
      {
        AgentInfo info;
        (Stack::singleton())->getAgentInfo(info);
        return info;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr IStackForInternal::queueDelegate()
      {
        return (Stack::singleton())->queueDelegate();
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr IStackForInternal::queueStack()
      {
        return (Stack::singleton())->queueStack();
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr IStackForInternal::queueServices()
      {
        return (Stack::singleton())->queueServices();
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
        ZS_LOG_DEBUG(log("created"))
        MessageFactoryBootstrappedFinder::singleton();
        MessageFactoryBootstrapper::singleton();
        MessageFactoryCertificates::singleton();
        MessageFactoryIdentity::singleton();
        MessageFactoryIdentityLookup::singleton();
        MessageFactoryIdentityLockbox::singleton();
        MessageFactoryNamespaceGrant::singleton();
        MessageFactoryRolodex::singleton();
        MessageFactoryPeer::singleton();
        MessageFactoryPeerCommon::singleton();
        MessageFactoryPeerFinder::singleton();
        MessageFactoryPeerSalt::singleton();
        MessageFactoryPeerToPeer::singleton();
      }

      //-----------------------------------------------------------------------
      Stack::~Stack()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
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
                        IMessageQueuePtr defaultDelegateMessageQueue,
                        IMessageQueuePtr stackMessageQueue,
                        IMessageQueuePtr servicesMessageQueue,
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

        if (defaultDelegateMessageQueue) {
          mDelegateQueue = defaultDelegateMessageQueue;
        }

        if (stackMessageQueue) {
          mStackQueue = stackMessageQueue;
        }

        if (servicesMessageQueue) {
          mServicesQueue = servicesMessageQueue;
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
        if (deviceID) {
          mDeviceID = String(deviceID);
        }
        if (userAgent) {
          mUserAgent = String(userAgent);
        }
        if (os) {
          mOS = String(os);
        }
        if (system) {
          mSystem = String(system);
        }

        ZS_THROW_INVALID_ARGUMENT_IF(!mDelegateQueue)
        ZS_THROW_INVALID_ARGUMENT_IF(!mStackQueue)
        ZS_THROW_INVALID_ARGUMENT_IF(!mServicesQueue)
        ZS_THROW_INVALID_ARGUMENT_IF(mAppID.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mAppName.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mAppImageURL.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mAppURL.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mUserAgent.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mDeviceID.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mOS.isEmpty())
        ZS_THROW_INVALID_ARGUMENT_IF(mSystem.isEmpty())
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack => IStackForInternal
      #pragma mark

      //-----------------------------------------------------------------------
      void Stack::getAgentInfo(AgentInfo &result) const
      {
        result.mUserAgent = getUserAgent();
        result.mName = getAppName();
        result.mImageURL = getAppImageURL();
        result.mAgentURL = getAppURL();
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Stack::queueDelegate() const
      {
        return mDelegateQueue;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Stack::queueStack() const
      {
        return mStackQueue;
      }

      //-----------------------------------------------------------------------
      IMessageQueuePtr Stack::queueServices() const
      {
        return mServicesQueue;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Stack => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String Stack::log(const char *message) const
      {
        return String("Stack [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
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
    void IStack::setup(
                       IMessageQueuePtr defaultDelegateMessageQueue,
                       IMessageQueuePtr stackMessageQueue,
                       IMessageQueuePtr servicesQueue,
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
      return internal::Stack::singleton()->setup(defaultDelegateMessageQueue, stackMessageQueue, servicesQueue, appID, appName, appImageURL, appURL, userAgent, deviceID, os, system);
    }
  }
}
