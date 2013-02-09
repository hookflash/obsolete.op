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

#include <hookflash/stack/internal/stack_PeerSubscription.h>
#include <hookflash/stack/internal/stack_Account.h>
#include <hookflash/stack/internal/stack_Peer.h>
#include <hookflash/stack/internal/stack_Location.h>
#include <hookflash/stack/internal/stack_Stack.h>
#include <hookflash/stack/internal/stack_Helper.h>

#include <hookflash/stack/IMessageIncoming.h>

#include <zsLib/Log.h>
#include <zsLib/helpers.h>

//#include <algorithm>


namespace hookflash { namespace stack { ZS_DECLARE_SUBSYSTEM(hookflash_stack) } }


namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      using zsLib::Stringize;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      PeerSubscription::PeerSubscription(
                                         AccountPtr account,
                                         IPeerSubscriptionDelegatePtr delegate
                                         ) :
        mID(zsLib::createPUID()),
        mAccount(account),
        mDelegate(IPeerSubscriptionDelegateProxy::createWeak(IStackForInternal::queueDelegate(), delegate))
      {
        ZS_LOG_DEBUG(log("constructed"))
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::init()
      {
        mAccount.lock()->forPeerSubscription().subscribe(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      PeerSubscription::~PeerSubscription()
      {
        ZS_LOG_DEBUG(log("destroyed"))
        mThisWeak.reset();

        cancel();
      }

      //-----------------------------------------------------------------------
      PeerSubscriptionPtr PeerSubscription::convert(IPeerSubscriptionPtr subscription)
      {
        return boost::dynamic_pointer_cast<PeerSubscription>(subscription);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerSubscription => IPeerSubscription
      #pragma mark

      //-----------------------------------------------------------------------
      String PeerSubscription::toDebugString(IPeerSubscriptionPtr subscription, bool includeCommaPrefix)
      {
        if (!subscription) return includeCommaPrefix ? String(", subscription=(null)") : String("subscription=(null)");
        return PeerSubscription::convert(subscription)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      PeerSubscriptionPtr PeerSubscription::subscribeAll(
                                                         IAccountPtr account,
                                                         IPeerSubscriptionDelegatePtr delegate
                                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(account)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        PeerSubscriptionPtr pThis(new PeerSubscription(Account::convert(account), delegate));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      PeerSubscriptionPtr PeerSubscription::subscribe(
                                                      IPeerPtr inPeer,
                                                      IPeerSubscriptionDelegatePtr delegate
                                                      )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inPeer)
        ZS_THROW_INVALID_ARGUMENT_IF(!delegate)

        PeerPtr peer = Peer::convert(inPeer);
        AccountPtr account = peer->forPeerSubscription().getAccount();

        PeerSubscriptionPtr pThis(new PeerSubscription(account, delegate));
        pThis->mPeer = peer;
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      IPeerPtr PeerSubscription::getSubscribedToPeer() const
      {
        AutoRecursiveLock lock(getLock());
        return mPeer;
      }

      //-----------------------------------------------------------------------
      bool PeerSubscription::PeerSubscription::isShutdown() const
      {
        AutoRecursiveLock lock(getLock());
        return !mDelegate;
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (!mDelegate) {
          ZS_LOG_DEBUG(log("cancel called but already shutdown (probably okay)"))
          return;
        }

        PeerSubscriptionPtr subscription = mThisWeak.lock();

        if ((mDelegate) &&
            (subscription)) {
          try {
            mDelegate->onPeerSubscriptionShutdown(subscription);
          } catch(IPeerSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("delegate already gone"))
          }
        }

        mDelegate.reset();

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_DEBUG(log("cancel called but account gone"))
          return;
        }

        account->forPeerSubscription().notifyDestroyed(*this);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerSubscription => IPeerSubscriptionForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      void PeerSubscription::notifyFindStateChanged(
                                                    PeerPtr peer,
                                                    PeerFindStates state
                                                    )
      {
        AutoRecursiveLock lock(getLock());
        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("notify of find state changed after shutdown"))
          return;
        }

        if (mPeer) {
          if (mPeer->forPeerSubscription().getPeerURI() != peer->forPeerSubscription().getPeerURI()) {
            ZS_LOG_DEBUG(log("ignoring find state for peer") + ", notified peer: " + IPeer::toDebugString(peer, false) + ", subscribing peer" + IPeer::toDebugString(mPeer, false))
            return;
          }
        }

        try {
          mDelegate->onPeerSubscriptionFindStateChanged(mThisWeak.lock(), peer, state);
        } catch(IPeerSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
        }
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::notifyLocationConnectionStateChanged(
                                                                  LocationPtr location,
                                                                  LocationConnectionStates state
                                                                  )
      {
        AutoRecursiveLock lock(getLock());

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("notify of location connection state changed after shutdown"))
          return;
        }

        if (mPeer) {
          PeerPtr peer = location->forPeerSubscription().getPeer();
          if (!peer) {
            ZS_LOG_DEBUG(log("ignoring location connection state change from non-peer, subscribing=") + IPeer::toDebugString(mPeer))
            return;
          }
          if (mPeer->forPeerSubscription().getPeerURI() != peer->forPeerSubscription().getPeerURI()) {
            ZS_LOG_DEBUG(log("ignoring location connection state change") + ", notified peer: " + IPeer::toDebugString(peer, false) + ", subscribing peer" + IPeer::toDebugString(mPeer, false))
            return;
          }
        }

        try {
          mDelegate->onPeerSubscriptionLocationConnectionStateChanged(mThisWeak.lock(), location, state);
        } catch(IPeerSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
        }
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::notifyMessageIncoming(IMessageIncomingPtr message)
      {
        AutoRecursiveLock lock(getLock());

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("notify of incoming message after shutdown"))
          return;
        }

        LocationPtr location = Location::convert(message->getLocation());
        if (!location) {
          ZS_LOG_DEBUG(log("ignoring incoming message missing location"))
          return;
        }
        if (mPeer) {
          PeerPtr peer = Peer::convert(location->forPeerSubscription().getPeer());
          if (!peer) {
            ZS_LOG_DEBUG(log("ignoring incoming message from non-peer") + ", subscribing peer" + IPeer::toDebugString(mPeer, false) + ", incoming: " + IMessageIncoming::toDebugString(message, false))
            return;
          }
          if (mPeer->forPeerSubscription().getPeerURI() != peer->forPeerSubscription().getPeerURI()) {
            ZS_LOG_DEBUG(log("ignoring incoming message for peer") + ", subscribing peer" + IPeer::toDebugString(mPeer, false) + ", incoming: " + IMessageIncoming::toDebugString(message, false))
            return;
          }
        }

        try {
          mDelegate->onPeerSubscriptionMessageIncoming(mThisWeak.lock(), message);
        } catch(IPeerSubscriptionDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate gone"))
        }
      }

      //-----------------------------------------------------------------------
      void PeerSubscription::notifyShutdown()
      {
        AutoRecursiveLock lock(getLock());
        cancel();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerSubscription => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &PeerSubscription::getLock() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) return mBogusLock;
        return account->forPeerSubscription().getLock();
      }

      //-----------------------------------------------------------------------
      String PeerSubscription::log(const char *message) const
      {
        return String("PeerSubscription [") + Stringize<PUID>(mID).string() + "] " + message;
      }

      //-----------------------------------------------------------------------
      String PeerSubscription::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("peer subscription id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("subscribing", mPeer ? "peer" : "all", firstTime) +
               IPeer::toDebugString(mPeer);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IPeerSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    String IPeerSubscription::toDebugString(IPeerSubscriptionPtr subscription, bool includeCommaPrefix)
    {
      return internal::PeerSubscription::toDebugString(subscription, includeCommaPrefix);
    }

    //-------------------------------------------------------------------------
    IPeerSubscriptionPtr IPeerSubscription::subscribeAll(
                                                         IAccountPtr account,
                                                         IPeerSubscriptionDelegatePtr delegate
                                                         )
    {
      return internal::IPeerSubscriptionFactory::singleton().subscribeAll(account, delegate);
    }

    //-------------------------------------------------------------------------
    IPeerSubscriptionPtr IPeerSubscription::subscribe(
                                                      IPeerPtr peer,
                                                      IPeerSubscriptionDelegatePtr delegate
                                                      )
    {
      return internal::IPeerSubscriptionFactory::singleton().subscribe(peer, delegate);
    }
  }
}
