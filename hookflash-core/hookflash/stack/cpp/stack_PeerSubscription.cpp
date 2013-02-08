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

#include <hookflash/stack/internal/stack_PeerSubscription.h>
#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_AccountFinder.h>
#include <hookflash/stack/internal/stack_AccountPeerLocation.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/message/MessageResult.h>
#include <hookflash/services/IRUDPICESocket.h>

#include <zsLib/Log.h>
#include <zsLib/zsHelpers.h>

#include <algorithm>

//#define HOOKFLASH_STACK_SESSION_USERAGENT "hookflash/1.0.1001a (iOS/iPad)"
#define HOOKFLASH_STACK_PEER_LOCATION_FIND_TIMEOUT_IN_SECONDS (60*2)

namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }



namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef zsLib::String String;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      PeerSubscription::PeerSubscription(IAccountForPeerSubscriptionPtr account) :
        MessageQueueAssociator(account->getAssociatedMessageQueue()),
        mID(zsLib::createPUID()),
        mAccount(account)
      {
        ZS_LOG_DEBUG(log("constructed"))
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::init()
      {
      }

      //-----------------------------------------------------------------------
      PeerSubscription::~PeerSubscription()
      {
        ZS_LOG_DEBUG(log("destroyed"))
        mThisWeak.reset();
      }

      //-----------------------------------------------------------------------
      PeerSubscriptionPtr PeerSubscription::create(
                                                   IAccountForPeerSubscriptionPtr account,
                                                   const char *contactID,
                                                   IPeerSubscriptionDelegatePtr delegate
                                                   )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        PeerSubscriptionPtr pThis(new PeerSubscription(account));
        pThis->mThisWeak = pThis;
        pThis->mContactID = contactID;
        ZS_THROW_INVALID_ARGUMENT_IF(pThis->mContactID.length() < 1)
        pThis->mDelegate = IPeerSubscriptionDelegateProxy::createWeak(account->getAssociatedMessageQueue(), delegate);
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      String PeerSubscription::getContactID()
      {
        AutoRecursiveLock lock(getLock());
        return mContactID;
      }

      //-----------------------------------------------------------------------
      IPeerSubscription::PeerSubscriptionFindStates PeerSubscription::getFindState() const
      {
        AutoRecursiveLock lock(getLock());
        IAccountForPeerSubscriptionPtr account = mAccount.lock();
        if (!account) return PeerSubscriptionFindState_Idle;
        return (account->isFinding(mContactID) ? PeerSubscriptionFindState_Finding : PeerSubscriptionFindState_Idle);
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::getPeerLocations(
                                              LocationList &outLocations,
                                              bool includeOnlyConnectedLocations
                                              )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("unable to get peer locations as subscription is shutdown"))
          return;
        }
        IAccountForPeerSubscriptionPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Debug, log("unable to get peer locations as account is now gone"))
          return;
        }
        account->getPeerLocations(mContactID, outLocations, includeOnlyConnectedLocations);
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::getPeerLocations(
                                              PeerLocationList &outPeerLocations,
                                              bool includeOnlyConnectedLocations
                                              )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("unable to get peer locations as subscription is shutdown"))
          return;
        }
        IAccountForPeerSubscriptionPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Debug, log("unable to get peer locations as account is now gone"))
          return;
        }
        account->getPeerLocations(mContactID, outPeerLocations, includeOnlyConnectedLocations);
      }

      //-----------------------------------------------------------------------
      bool PeerSubscription::sendPeerMesage(
                                            const char *locationID,
                                            message::MessagePtr message
                                            )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("unable to send peer location a message as the subscription is shutdown"))
          return false;
        }

        IAccountForPeerSubscriptionPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Debug, log("unable to send peer location a message as account is gone"))
          return false;
        }
        return account->sendPeerMessage(mContactID, locationID, message);
      }

      //-----------------------------------------------------------------------
      bool PeerSubscription::isShutdown()
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::cancel()
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("cancel called"))

        if (mDelegate) {
          try {
            mDelegate->onPeerSubscriptionShutdown(mThisWeak.lock());
          } catch(IPeerSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          }
        }
        mDelegate.reset();

        IAccountForPeerSubscriptionPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Debug, log("unable to notify account of subscription being gone as account is gone"))
          return;
        }

        PeerSubscriptionPtr pThis = mThisWeak.lock();
        if (pThis) {
          (IAccountForPeerSubscriptionProxy::create(account))->onPeerSubscriptionShutdown(mThisWeak.lock());
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      void PeerSubscription::notifyPeerSubscriptionLocationsChanged()
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Debug, log("unable to notify of location change as subscription is already shutdown"))
          return;
        }

        try {
          mDelegate->onPeerSubscriptionLocationsChanged(mThisWeak.lock());
        } catch(IPeerSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::notifyAccountNotifyPeerSubscriptionShutdown()
      {
        AutoRecursiveLock lock(getLock());
        cancel();
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::notifyAccountPeerFindStateChanged(IPeerSubscription::PeerSubscriptionFindStates state)
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("unable to notify of find state changed as subscription is already shutdown"))
          return;
        }

        try {
          mDelegate->onPeerSubscriptionFindStateChanged(mThisWeak.lock(), state);
        } catch(IPeerSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
        }
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::notifyPeerSubscriptionMessage(
                                                           const char *contactID,
                                                           const char *locationID,
                                                           IPeerSubscriptionMessagePtr message
                                                           )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Debug, log("unable to notify of peer message as subscription is already shutdown"))
          return;
        }

        try {
          mDelegate->onPeerSubscriptionMessage(mThisWeak.lock(), message);
        } catch(IPeerSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      RecursiveLock &PeerSubscription::getLock() const
      {
        IAccountForPeerSubscriptionPtr account(mAccount.lock());
        if (!account) return mBogusLock;
        return account->getLock();
      }

      //-----------------------------------------------------------------------
      String PeerSubscription::log(const char *message) const
      {
        return String("PeerSubscription [") + Stringize<PUID>(mID).string() + "] " + message;
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    const char *IPeerSubscription::toString(PeerSubscriptionFindStates state)
    {
      switch (state)
      {
        case PeerSubscriptionFindState_Idle:      return "Idle";
        case PeerSubscriptionFindState_Finding:   return "Finding";
        case PeerSubscriptionFindState_Completed: return "Completed";
      }
      return "UNDEFINED";
    }
  }
}
