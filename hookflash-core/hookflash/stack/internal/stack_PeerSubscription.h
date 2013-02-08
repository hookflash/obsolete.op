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

#include <hookflash/stack/IAccount.h>
#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/stack/internal/stack_BootstrappedNetwork.h>
#include <hookflash/stack/IMessageRequester.h>
#include <hookflash/stack/IPeerSubscription.h>
#include <hookflash/services/IRUDPICESocket.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>
#include <zsLib/Proxy.h>

#include <map>

namespace hookflash
{
  namespace stack
  {
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      interaction IPeerSubscriptionForAccount
      {
        typedef zsLib::PUID PUID;

        virtual PUID getID() const = 0;
        virtual IPeerSubscriptionPtr convertIPeerSubscription() = 0;
        virtual void notifyPeerSubscriptionLocationsChanged() = 0;
        virtual void notifyAccountNotifyPeerSubscriptionShutdown() = 0;
        virtual void notifyAccountPeerFindStateChanged(IPeerSubscription::PeerSubscriptionFindStates state) = 0;
        virtual void notifyPeerSubscriptionMessage(
                                                   const char *contactID,
                                                   const char *locationID,
                                                   IPeerSubscriptionMessagePtr message
                                                   ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      class PeerSubscription : public zsLib::MessageQueueAssociator,
                               public IPeerSubscription,
                               public IPeerSubscriptionForAccount
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;

      protected:
        PeerSubscription(IAccountForPeerSubscriptionPtr account);

        void init();

      public:
        ~PeerSubscription();

        static PeerSubscriptionPtr create(
                                          IAccountForPeerSubscriptionPtr account,
                                          const char *contactID,
                                          IPeerSubscriptionDelegatePtr delegate
                                          );

        // IPeerSubscription
        virtual String getContactID();

        // (duplicate) virtual PUID getID() const;

        virtual bool isShutdown();

        virtual IPeerSubscription::PeerSubscriptionFindStates getFindState() const;

        virtual void getPeerLocations(
                                      LocationList &outLocations,
                                      bool includeOnlyConnectedLocations
                                      );
        virtual void getPeerLocations(
                                      PeerLocationList &outPeerLocations,
                                      bool includeOnlyConnectedLocations
                                      );

        virtual bool sendPeerMesage(
                                    const char *locationID,
                                    message::MessagePtr message
                                    );

        virtual void cancel();

      protected:
        // IPeerSubscriptionForAccount
        virtual PUID getID() const {return mID;}
        virtual IPeerSubscriptionPtr convertIPeerSubscription() {return mThisWeak.lock();}
        virtual void notifyPeerSubscriptionLocationsChanged();
        virtual void notifyAccountNotifyPeerSubscriptionShutdown();
        virtual void notifyAccountPeerFindStateChanged(IPeerSubscription::PeerSubscriptionFindStates state);
        virtual void notifyPeerSubscriptionMessage(
                                                   const char *contactID,
                                                   const char *locationID,
                                                   IPeerSubscriptionMessagePtr message
                                                   );

      protected:
        RecursiveLock &getLock() const;
        String log(const char *message) const;

      protected:
        PUID mID;
        mutable RecursiveLock mBogusLock;
        IAccountForPeerSubscriptionWeakPtr mAccount;

        PeerSubscriptionWeakPtr mThisWeak;

        String mContactID;
        IPeerSubscriptionDelegatePtr mDelegate;
      };

    }
  }
}
