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

#include <openpeer/stack/internal/types.h>

#include <openpeer/stack/IAccount.h>
#include <openpeer/stack/ILocation.h>

#include <openpeer/services/IRUDPICESocket.h>
#include <openpeer/services/IRUDPICESocketSession.h>
#include <openpeer/services/IRUDPMessaging.h>
#include <openpeer/services/ITransportStream.h>

#include <openpeer/stack/message/peer-to-peer/PeerIdentifyResult.h>
#include <openpeer/stack/message/peer-to-peer/PeerKeepAliveResult.h>

#include <openpeer/services/IWakeDelegate.h>

#include <zsLib/MessageQueueAssociator.h>

#include <map>
#include <list>

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using services::IICESocket;

      typedef services::IRUDPICESocketSubscriptionPtr IRUDPICESocketSubscriptionPtr;
      typedef services::IRUDPMessagingPtr IRUDPMessagingPtr;

      using message::peer_finder::PeerLocationFindRequestPtr;

      using message::peer_to_peer::PeerIdentifyResult;
      using message::peer_to_peer::PeerIdentifyResultPtr;
      using message::peer_to_peer::PeerKeepAliveResult;
      using message::peer_to_peer::PeerKeepAliveResultPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountPeerLocationForAccount
      #pragma mark

      interaction IAccountPeerLocationForAccount
      {
        typedef IAccount::AccountStates AccountStates;

        IAccountPeerLocationForAccount &forAccount() {return *this;}
        const IAccountPeerLocationForAccount &forAccount() const {return *this;}

        static AccountPeerLocationPtr create(
                                             IAccountPeerLocationDelegatePtr delegate,
                                             AccountPtr outer,
                                             const LocationInfo &locationInfo
                                             );

        virtual PUID getID() const = 0;

        virtual LocationPtr getLocation() const = 0;
        virtual const LocationInfo &getLocationInfo() const = 0;

        virtual void shutdown() = 0;

        virtual AccountStates getState() const = 0;
        virtual bool shouldRefindNow() const = 0;

        virtual bool isConnected() const = 0;
        virtual Time getTimeOfLastActivity() const = 0;

        virtual void connectLocation(
                                     const char *remoteContextID,
                                     const char *remotePeerSecret,
                                     const char *remoteICEUsernameFrag,
                                     const char *remoteICEPassword,
                                     const CandidateList &candidates,
                                     IICESocket::ICEControls control
                                     ) = 0;

        virtual void incomingRespondWhenCandidatesReady(PeerLocationFindRequestPtr request) = 0;

        virtual bool hasReceivedCandidateInformation() const = 0;

        virtual bool send(MessagePtr message) const = 0;
        virtual IMessageMonitorPtr sendRequest(
                                               IMessageMonitorDelegatePtr delegate,
                                               MessagePtr message,
                                               Duration duration
                                               ) const = 0;

        virtual void sendKeepAlive() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark AccountPeerLocation
      #pragma mark

      class AccountPeerLocation : public Noop,
                                  public MessageQueueAssociator,
                                  public IAccountPeerLocationForAccount,
                                  public IWakeDelegate,
                                  public services::IRUDPICESocketDelegate,
                                  public services::IRUDPICESocketSessionDelegate,
                                  public services::IRUDPMessagingDelegate,
                                  public services::ITransportStreamWriterDelegate,
                                  public services::ITransportStreamReaderDelegate,
                                  public IMessageMonitorResultDelegate<PeerIdentifyResult>,
                                  public IMessageMonitorResultDelegate<PeerKeepAliveResult>
      {
      public:
        friend interaction IAccountPeerLocationFactory;

        typedef std::list<PeerLocationFindRequestPtr> PendingRequestList;

      protected:
        AccountPeerLocation(
                            IMessageQueuePtr queue,
                            IAccountPeerLocationDelegatePtr delegate,
                            AccountPtr outer,
                            const LocationInfo &locationInfo
                            );
        
        AccountPeerLocation(Noop) : Noop(true), MessageQueueAssociator(IMessageQueuePtr()) {};

        void init();

      public:
        ~AccountPeerLocation();

        static String toDebugString(AccountPeerLocationPtr peerLocation, bool includeCommaPrefix = true);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IAccountPeerLocationForAccount
        #pragma mark

        static AccountPeerLocationPtr create(
                                             IAccountPeerLocationDelegatePtr delegate,
                                             AccountPtr outer,
                                             const LocationInfo &locationInfo
                                             );

        virtual PUID getID() const {return mID;}

        virtual LocationPtr getLocation() const;
        virtual const LocationInfo &getLocationInfo() const;

        virtual void shutdown();

        virtual AccountStates getState() const;

        virtual bool shouldRefindNow() const;

        virtual bool isConnected() const;
        virtual Time getTimeOfLastActivity() const;

        virtual void connectLocation(
                                     const char *remoteContextID,
                                     const char *remotePeerSecret,
                                     const char *remoteICEUsernameFrag,
                                     const char *remoteICEPassword,
                                     const CandidateList &candidates,
                                     IICESocket::ICEControls control
                                     );

        virtual void incomingRespondWhenCandidatesReady(PeerLocationFindRequestPtr request);

        virtual bool hasReceivedCandidateInformation() const;

        virtual bool send(MessagePtr message) const;
        virtual IMessageMonitorPtr sendRequest(
                                               IMessageMonitorDelegatePtr delegate,
                                               MessagePtr message,
                                               Duration duration
                                               ) const;

        virtual void sendKeepAlive();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IWakeDelegate
        #pragma mark

        virtual void onWake();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IRUDPICESocketDelegate
        #pragma mark

        virtual void onRUDPICESocketStateChanged(
                                                 IRUDPICESocketPtr socket,
                                                 RUDPICESocketStates state
                                                 );
        virtual void onRUDPICESocketCandidatesChanged(IRUDPICESocketPtr socket);

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

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => ITransportStreamWriterDelegate
        #pragma mark

        virtual void onTransportStreamWriterReady(ITransportStreamWriterPtr writer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => ITransportStreamWriterDelegate
        #pragma mark

        virtual void onTransportStreamReaderReady(ITransportStreamReaderPtr reader);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IMessageMonitorResultDelegate<PeerIdentifyResult>,
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        PeerIdentifyResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             PeerIdentifyResultPtr ignore, // will always be NULL
                                                             MessageResultPtr result
                                                             );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => IMessageMonitorResultDelegate<PeerKeepAliveResult>,
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        PeerKeepAliveResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             PeerKeepAliveResultPtr ignore, // will always be NULL
                                                             MessageResultPtr result
                                                             );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => (internal)
        #pragma mark

        bool isPending() const      {return IAccount::AccountState_Pending == mCurrentState;}
        bool isReady() const        {return IAccount::AccountState_Ready == mCurrentState;}
        bool isShuttingDown() const {return IAccount::AccountState_ShuttingDown == mCurrentState;}
        bool isShutdown() const     {return IAccount::AccountState_Shutdown == mCurrentState;}

        RecursiveLock &getLock() const;
        IRUDPICESocketPtr getSocket() const;

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void cancel();

        void step();
        bool stepSocketSubscription(IRUDPICESocketPtr socket);
        bool stepPendingRequests(IRUDPICESocketPtr socket);
        bool stepSocketSession();
        bool stepIncomingIdentify();
        bool stepMessaging();
        bool stepIdentify();

        void setState(AccountStates state);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark AccountPeerLocation => (data)
        #pragma mark

        AutoPUID mID;
        mutable RecursiveLock mBogusLock;
        mutable AccountPeerLocationWeakPtr mThisWeak;

        IAccountPeerLocationDelegatePtr mDelegate;
        AccountWeakPtr mOuter;
        AccountPeerLocationPtr mGracefulShutdownReference;

        AccountStates mCurrentState;
        mutable bool mShouldRefindNow;

        mutable Time mLastActivity;

        PendingRequestList mPendingRequests;

        String mRemoteContextID;
        String mRemotePeerSecret;

        // information about the location found
        LocationInfo mLocationInfo;
        LocationPtr mLocation;
        PeerPtr mPeer;

        IRUDPICESocketSubscriptionPtr mSocketSubscription;
        IRUDPICESocketSessionPtr mSocketSession;   // this will only become valid when a connection is establishing
        IRUDPMessagingPtr mMessaging;
        ITransportStreamReaderPtr mReceiveStream;
        ITransportStreamWriterPtr mSendStream;

        bool mIncoming;
        Time mIdentifyTime;

        IMessageMonitorPtr mIdentifyMonitor;
        IMessageMonitorPtr mKeepAliveMonitor;
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
        typedef IAccount::AccountStates AccountStates;

        virtual void onAccountPeerLocationStateChanged(
                                                       AccountPeerLocationPtr peerLocation,
                                                       AccountStates state
                                                       ) = 0;

        virtual void onAccountPeerLocationMessageIncoming(
                                                          AccountPeerLocationPtr peerLocation,
                                                          MessagePtr message
                                                          ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountPeerLocationFactory
      #pragma mark

      interaction IAccountPeerLocationFactory
      {
        static IAccountPeerLocationFactory &singleton();

        virtual AccountPeerLocationPtr create(
                                              IAccountPeerLocationDelegatePtr delegate,
                                              AccountPtr outer,
                                              const LocationInfo &locationInfo
                                              );
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::stack::internal::IAccountPeerLocationDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::internal::AccountPeerLocationPtr, AccountPeerLocationPtr)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::internal::AccountPeerLocation::AccountStates, AccountStates)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::stack::message::MessagePtr, MessagePtr)
ZS_DECLARE_PROXY_METHOD_2(onAccountPeerLocationStateChanged, AccountPeerLocationPtr, AccountStates)
ZS_DECLARE_PROXY_METHOD_2(onAccountPeerLocationMessageIncoming, AccountPeerLocationPtr, MessagePtr)
ZS_DECLARE_PROXY_END()
