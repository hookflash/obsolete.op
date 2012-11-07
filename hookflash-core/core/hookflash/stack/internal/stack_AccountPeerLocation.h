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
#include <hookflash/stack/IPeerLocation.h>
#include <hookflash/stack/internal/hookflashTypes.h>
#include <hookflash/services/IRUDPICESocket.h>
#include <hookflash/services/IRUDPICESocketSession.h>
#include <hookflash/services/IRUDPMessaging.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>

#include <map>
#include <list>

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
      #pragma mark IAccountPeerLocation
      #pragma mark

      interaction IAccountPeerLocation
      {
        typedef zsLib::PUID PUID;
        typedef zsLib::String String;
        typedef zsLib::Time Time;
        typedef zsLib::Duration Duration;
        typedef zsLib::XML::DocumentPtr DocumentPtr;
        typedef services::IICESocket::CandidateList CandidateList;
        typedef services::IICESocket::Candidate Candidate;
        typedef services::IICESocket::ICEControls ICEControls;

        enum AccountPeerLocationStates
        {
          AccountPeerLocationState_Pending,
          AccountPeerLocationState_Ready,
          AccountPeerLocationState_ShuttingDown,
          AccountPeerLocationState_Shutdown,
        };

        static const char *toString(AccountPeerLocationStates state);

        virtual PUID getID() const = 0;
        virtual const String &getContactID() const = 0;
        virtual const String &getLocationID() const = 0;

        virtual const Location &getLocation() const = 0;

        virtual IPeerLocationPtr convertIPeerLocation() = 0;

        virtual void shutdown() = 0;

        virtual AccountPeerLocationStates getState() const = 0;
        virtual bool shouldRefindNow() const = 0;

        virtual bool isConnected() const = 0;
        virtual Time getTimeOfLastActivity() const = 0;

        virtual IPeerFilePublicPtr getPeerFilePublic() const = 0;

        virtual void connectLocation(
                                     const CandidateList &candidates,
                                     ICEControls control
                                     ) = 0;

        virtual void incomingRespondWhenCandidatesReady(message::PeerToFinderPeerLocationFindRequestPtr request) = 0;

        virtual bool hasReceivedCandidateInformation() const = 0;

        virtual bool sendMessage(
                                 DocumentPtr document,
                                 bool onlySendIfReady = true
                                 ) = 0;

        virtual bool sendPeerMesage(
                                    message::MessagePtr message,
                                    bool onlySendIfReady = true
                                    ) = 0;

        virtual IMessageRequesterPtr sendPeerRequest(
                                                     IMessageRequesterDelegatePtr delegate,
                                                     message::MessagePtr requestMessage,
                                                     Duration timeout,
                                                     bool onlySendIfReady = true
                                                     ) = 0;

        virtual void sendKeepAlive() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountPeerLocationAsyncDelegate
      #pragma mark

      interaction IAccountPeerLocationAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation
      #pragma mark

      class AccountPeerLocation : public zsLib::MessageQueueAssociator,
                                  public IPeerLocation,
                                  public IAccountPeerLocation,
                                  public IAccountPeerLocationAsyncDelegate,
                                  public services::IRUDPICESocketDelegate,
                                  public services::IRUDPICESocketSessionDelegate,
                                  public services::IRUDPMessagingDelegate,
                                  public IMessageRequesterDelegate
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::String String;
        typedef zsLib::Time Time;
        typedef zsLib::Duration Duration;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef services::IRUDPMessagingPtr IRUDPMessagingPtr;
        typedef services::IRUDPICESocketSubscriptionPtr IRUDPICESocketSubscriptionPtr;

      protected:
        AccountPeerLocation(IMessageQueuePtr queue);

        void init();

      public:
        ~AccountPeerLocation();

        static AccountPeerLocationPtr create(
                                             IMessageQueuePtr queue,
                                             IAccountPeerLocationDelegatePtr delegate,
                                             IAccountForAccountPeerLocationPtr outer,
                                             IPeerFilePublicPtr publicPeerFile,
                                             const char *contactID,
                                             const Location &location
                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IPeerLocation
        #pragma mark

        // (duplicate) virtual PUID getID() const;

        virtual String getContactID();
        virtual String getLocationID();

        virtual Location getLocation();

        virtual IPeerFilePublicPtr getPeerFilePublic() const;

        // (duplicate) virtual bool isConnected() const;

        virtual bool sendPeerMesage(message::MessagePtr message);

        virtual IMessageRequesterPtr sendPeerRequest(
                                                     IMessageRequesterDelegatePtr delegate,
                                                     message::MessagePtr requestMessage,
                                                     Duration timeout
                                                     );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IAccountPeerLocation
        #pragma mark

        virtual PUID getID() const {return mID;}

        virtual const String &getContactID() const;
        virtual const String &getLocationID() const;

        virtual const Location &getLocation() const;

        virtual IPeerLocationPtr convertIPeerLocation();
        virtual void shutdown();

        virtual AccountPeerLocationStates getState() const;

        virtual bool shouldRefindNow() const;

        virtual bool isConnected() const;
        virtual Time getTimeOfLastActivity() const;

        // (duplicate) virtual IPeerFilePublicPtr getPeerFilePublic() const;

        virtual void connectLocation(
                                     const CandidateList &candidates,
                                     ICEControls control
                                     );

        virtual void incomingRespondWhenCandidatesReady(message::PeerToFinderPeerLocationFindRequestPtr request);

        virtual bool hasReceivedCandidateInformation() const;

        virtual bool sendMessage(
                                 DocumentPtr document,
                                 bool onlySendIfReady
                                 );

        virtual bool sendPeerMesage(
                                    message::MessagePtr message,
                                    bool onlySendIfReady
                                    );

        virtual IMessageRequesterPtr sendPeerRequest(
                                                     IMessageRequesterDelegatePtr delegate,
                                                     message::MessagePtr requestMessage,
                                                     Duration timeout,
                                                     bool onlySendIfReady
                                                     );

        virtual void sendKeepAlive();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IAccountPeerLocationAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IRUDPICESocketDelegate
        #pragma mark

        virtual void onRUDPICESocketStateChanged(
                                                 IRUDPICESocketPtr socket,
                                                 RUDPICESocketStates state
                                                 );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IRUDPICESocketSessionDelegate
        #pragma mark

        virtual void onRUDPICESocketSessionStateChanged(
                                                        IRUDPICESocketSessionPtr session,
                                                        RUDPICESocketSessionStates state
                                                        );

        virtual void onRUDPICESocketSessionChannelWaiting(IRUDPICESocketSessionPtr session);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IRUDPMessagingDelegate
        #pragma mark

        virtual void onRUDPMessagingStateChanged(
                                                 IRUDPMessagingPtr session,
                                                 RUDPMessagingStates state
                                                 );

        virtual void onRUDPMessagingReadReady(IRUDPMessagingPtr session);
        virtual void onRUDPMessagingWriteReady(IRUDPMessagingPtr session);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IMessageReuqester
        #pragma mark

        virtual bool handleMessageRequesterMessageReceived(
                                                           IMessageRequesterPtr requester,
                                                           message::MessagePtr message
                                                           );

        virtual void onMessageRequesterTimedOut(IMessageRequesterPtr requester);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => (internal)
        #pragma mark

        bool isPending() const {return AccountPeerLocationState_Pending == mCurrentState;}
        bool isReady() const {return AccountPeerLocationState_Ready == mCurrentState;}
        bool isShuttingDown() const {return AccountPeerLocationState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return AccountPeerLocationState_Shutdown == mCurrentState;}

        RecursiveLock &getLock() const;
        IAccountForAccountPeerLocationPtr getOuter() const;
        IRUDPICESocketPtr getSocket() const;

        String log(const char *message);

        void cancel();
        void step();
        void setState(AccountPeerLocationStates state);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;

        mutable AccountPeerLocationWeakPtr mThisWeak;

        IAccountPeerLocationDelegatePtr mDelegate;
        IAccountForAccountPeerLocationWeakPtr mOuter;
        AccountPeerLocationPtr mGracefulShutdownReference;

        AccountPeerLocationStates mCurrentState;
        mutable bool mShouldRefindNow;

        IPeerFilePublicPtr mPeerFilePublic;           // the public peer file of the location become connected

        Time mLastActivity;

        typedef std::list<message::PeerToFinderPeerLocationFindRequestPtr> PendingRequestList;
        PendingRequestList mPendingRequests;

        // information about the location found
        String mContactID;
        Location mLocation;

        IRUDPICESocketSubscriptionPtr mSocketSubscription;
        IRUDPICESocketSessionPtr mSocketSession;   // this will only become valid when a connection is establishing
        IRUDPMessagingPtr mMessaging;

        IMessageRequesterPtr mIdentityRequester;

        IMessageRequesterPtr mKeepAliveRequester;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountPeerLocationDelegate
      #pragma mark

      interaction IAccountPeerLocationDelegate
      {
        typedef IAccountPeerLocation::AccountPeerLocationStates AccountPeerLocationStates;

        virtual void onAccountPeerLocationStateChanged(
                                                       IAccountPeerLocationPtr peerLocation,
                                                       AccountPeerLocationStates state
                                                       ) = 0;
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountPeerLocationDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::IAccountPeerLocationPtr, IAccountPeerLocationPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::stack::internal::IAccountPeerLocation::AccountPeerLocationStates, AccountPeerLocationStates)
ZS_DECLARE_PROXY_METHOD_2(onAccountPeerLocationStateChanged, IAccountPeerLocationPtr, AccountPeerLocationStates)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::stack::internal::IAccountPeerLocationAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
