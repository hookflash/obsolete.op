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

#include <hookflash/stack/IAccount.h>
#include <hookflash/stack/internal/types.h>
#include <hookflash/stack/IPeerSubscription.h>

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
      #pragma mark
      #pragma mark IPeerSubscriptionForAccount
      #pragma mark

      interaction IPeerSubscriptionForAccount
      {
        typedef IPeer::PeerFindStates PeerFindStates;
        typedef ILocation::LocationConnectionStates LocationConnectionStates;

        IPeerSubscriptionForAccount &forAccount() {return *this;}
        const IPeerSubscriptionForAccount &forAccount() const {return *this;}

        virtual PUID getID() const = 0;

        virtual IPeerPtr getSubscribedToPeer() const = 0;

        virtual void notifyFindStateChanged(
                                            PeerPtr peer,
                                            PeerFindStates state
                                            ) = 0;

        virtual void notifyLocationConnectionStateChanged(
                                                          LocationPtr location,
                                                          LocationConnectionStates state
                                                          ) = 0;

        virtual void notifyMessageIncoming(IMessageIncomingPtr message) = 0;

        virtual void notifyShutdown() = 0;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark PeerSubscription
      #pragma mark

      class PeerSubscription : public IPeerSubscription,
                               public IPeerSubscriptionForAccount
      {
      public:
        friend interaction IPeerSubscriptionFactory;
        friend interaction IPeerSubscription;

      protected:
        PeerSubscription(
                         AccountPtr account,
                         IPeerSubscriptionDelegatePtr delegate
                         );

        void init();

      public:
        ~PeerSubscription();

        static PeerSubscriptionPtr convert(IPeerSubscriptionPtr subscription);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerSubscription => IPeerSubscription
        #pragma mark

        static String toDebugString(IPeerSubscriptionPtr subscription, bool includeCommaPrefix = true);

        static PeerSubscriptionPtr subscribeAll(
                                                IAccountPtr account,
                                                IPeerSubscriptionDelegatePtr delegate
                                                );

        static PeerSubscriptionPtr subscribe(
                                             IPeerPtr peer,
                                             IPeerSubscriptionDelegatePtr delegate
                                             );

        virtual PUID getID() const {return mID;}

        virtual IPeerPtr getSubscribedToPeer() const;

        virtual bool isShutdown() const;

        virtual void cancel();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerSubscription => IPeerSubscriptionForAccount
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        // (duplicate) virtual IPeerPtr getSubscribedToPeer() const;

        virtual void notifyFindStateChanged(
                                            PeerPtr peer,
                                            PeerFindStates state
                                            );

        virtual void notifyLocationConnectionStateChanged(
                                                          LocationPtr location,
                                                          LocationConnectionStates state
                                                          );

        virtual void notifyMessageIncoming(IMessageIncomingPtr message);

        virtual void notifyShutdown();

        // (duplicate) virtual String getDebugValueString(bool includeCommaPrefix = true) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerSubscription => (internal)
        #pragma mark

        RecursiveLock &getLock() const;
        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark PeerSubscription => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;
        PeerSubscriptionWeakPtr mThisWeak;

        AccountWeakPtr mAccount;

        PeerPtr mPeer;

        IPeerSubscriptionDelegatePtr mDelegate;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IPeerSubscriptionFactory
      #pragma mark

      interaction IPeerSubscriptionFactory
      {
        static IPeerSubscriptionFactory &singleton();

        virtual PeerSubscriptionPtr subscribeAll(
                                                 IAccountPtr account,
                                                 IPeerSubscriptionDelegatePtr delegate
                                                 );

        virtual PeerSubscriptionPtr subscribe(
                                              IPeerPtr peer,
                                              IPeerSubscriptionDelegatePtr delegate
                                              );
      };
      
    }
  }
}
