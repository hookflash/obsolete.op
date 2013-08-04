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
#include <openpeer/stack/internal/stack_IFinderConnectionRelayChannel.h>
#include <openpeer/stack/internal/stack_IFinderConnection.h>

#include <openpeer/stack/message/types.h>
#include <openpeer/stack/message/peer-finder/ChannelMapResult.h>

#include <openpeer/stack/IMessageMonitor.h>

#include <openpeer/services/ITransportStream.h>
#include <openpeer/services/ITCPMessaging.h>
#include <openpeer/services/IWakeDelegate.h>

#include <zsLib/Timer.h>

#include <list>
#include <map>

namespace openpeer
{
  namespace stack
  {
    namespace internal
    {
      using peer_finder::ChannelMapResult;
      using peer_finder::ChannelMapResultPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnection
      #pragma mark

      class FinderConnection : public Noop,
                               public zsLib::MessageQueueAssociator,
                               public IFinderConnection,
                               public ITimerDelegate,
                               public IWakeDelegate,
                               public ITCPMessagingDelegate,
                               public ITransportStreamWriterDelegate,
                               public ITransportStreamReaderDelegate,
                               public IFinderConnectionRelayChannelDelegate,
                               public IMessageMonitorResultDelegate<ChannelMapResult>
      {
      public:
        typedef IFinderConnection::SessionStates SessionStates;

        friend interaction IFinderConnectionRelayChannelFactory;

        friend class FinderConnectionManager;
        friend class Channel;

        class Channel;
        typedef boost::shared_ptr<Channel> ChannelPtr;
        typedef boost::weak_ptr<Channel> ChannelWeakPtr;

        typedef ULONG ChannelNumber;
        typedef std::map<ChannelNumber, ChannelPtr> ChannelMap;

        typedef boost::value_initialized<ChannelNumber> AutoChannelNumber;

      protected:
        FinderConnection(
                         IMessageQueuePtr queue,
                         IPAddress remoteFinderIP
                         );

        FinderConnection(Noop) :
          Noop(true),
          zsLib::MessageQueueAssociator(IMessageQueuePtr()) {}

        void init();

      public:
        virtual ~FinderConnection();

        static FinderConnectionPtr convert(IFinderConnectionPtr connection);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => friend IFinderConnectionRelayChannelFactory
        #pragma mark

        static IFinderConnectionRelayChannelPtr connect(
                                                        IFinderConnectionRelayChannelDelegatePtr delegate,
                                                        const IPAddress &remoteFinderIP,
                                                        const char *localContextID,
                                                        const char *remoteContextID,
                                                        const char *relayDomain,
                                                        const char *relayAccessToken,
                                                        const char *relayAccessSecretProof,
                                                        ITransportStreamPtr receiveStream,
                                                        ITransportStreamPtr sendStream
                                                        );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => IFinderConnection
        #pragma mark

        static String toDebugString(IFinderConnectionPtr connection, bool includeCommaPrefix = true);

        static IFinderConnectionPtr connect(
                                            IFinderConnectionDelegatePtr delegate,
                                            const IPAddress &remoteFinderIP,
                                            ITransportStreamPtr receiveStream,
                                            ITransportStreamPtr sendStream
                                            );

        virtual PUID getID() const {return mID;}

        virtual IFinderConnectionSubscriptionPtr subscribe(IFinderConnectionDelegatePtr delegate);

        virtual void cancel();

        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const;

        virtual IFinderRelayChannelPtr accept(
                                              IFinderRelayChannelDelegatePtr delegate,        // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                              AccountPtr account,
                                              ITransportStreamPtr receiveStream,
                                              ITransportStreamPtr sendStream
                                              );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => IWakeDelegate
        #pragma mark

        virtual void onWake();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => ITCPMessagingDelegate
        #pragma mark

        virtual void onTCPMessagingStateChanged(
                                                ITCPMessagingPtr messaging,
                                                ITCPMessaging::SessionStates state
                                                );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => ITransportStreamWriterDelegate
        #pragma mark

        virtual void onTransportStreamWriterReady(ITransportStreamWriterPtr writer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => ITransportStreamReaderDelegate
        #pragma mark

        virtual void onTransportStreamReaderReady(ITransportStreamReaderPtr reader);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => IFinderConnectionRelayChannelDelegate
        #pragma mark

        virtual void onFinderConnectionRelayChannelStateChanged(
                                                                IFinderConnectionRelayChannelPtr channel,
                                                                IFinderConnectionRelayChannel::SessionStates state
                                                                );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => IMessageMonitorResultDelegate<ChannelMapResult>
        #pragma mark

        virtual bool handleMessageMonitorResultReceived(
                                                        IMessageMonitorPtr monitor,
                                                        ChannelMapResultPtr result
                                                        );

        virtual bool handleMessageMonitorErrorResultReceived(
                                                             IMessageMonitorPtr monitor,
                                                             ChannelMapResultPtr ignore, // will always be NULL
                                                             message::MessageResultPtr result
                                                             );


        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => friend Channel
        #pragma mark

        // RecursiveLock &getLock() const;

        void sendBuffer(
                        ChannelNumber channelNumber,
                        SecureByteBlockPtr buffer
                        );
        void notifyDestroyed(ChannelNumber channelNumber);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => (internal)
        #pragma mark

        bool isShutdown() const {return SessionState_Shutdown == mCurrentState;}
        bool isFinderSessionConnection() const;
        bool isFinderRelayConnection() const;

        RecursiveLock &getLock() const;
        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void setState(SessionStates state);
        void setError(WORD errorCode, const char *inReason = NULL);

        void step();
        bool stepCleanRemoval();
        bool stepConnectWire();
        bool stepMasterChannel();
        bool stepChannelMapRequest();

        IFinderConnectionRelayChannelPtr connect(
                                                 IFinderConnectionRelayChannelDelegatePtr delegate,
                                                 const char *localContextID,
                                                 const char *remoteContextID,
                                                 const char *relayDomain,
                                                 const char *relayAccessToken,
                                                 const char *relayAccessSecretProof,
                                                 ITransportStreamPtr receiveStream,
                                                 ITransportStreamPtr sendStream
                                                 );

      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection::Channel
        #pragma mark

        class Channel : public Noop,
                        public zsLib::MessageQueueAssociator,
                        public IFinderConnectionRelayChannel,
                        public ITransportStreamWriterDelegate,
                        public ITransportStreamReaderDelegate
        {
        public:
          friend class FinderConnection;
          typedef IFinderConnectionRelayChannel::SessionStates SessionStates;

        protected:
          Channel(
                  IMessageQueuePtr queue,
                  FinderConnectionPtr outer,
                  IFinderConnectionRelayChannelDelegatePtr delegate,
                  ITransportStreamPtr receiveStream,
                  ITransportStreamPtr sendStream,
                  ChannelNumber channelNumber
                  );

          struct ConnectionInfo
          {
            String mLocalContextID;
            String mRemoteContextID;
            String mRelayDomain;
            String mRelayAccessToken;
            String mRelayAccessSecretProof;
          };

          Channel(Noop) :
            Noop(true),
            zsLib::MessageQueueAssociator(IMessageQueuePtr()) {}

          void init();

        public:
          ~Channel();

          static ChannelPtr convert(IFinderConnectionRelayChannelPtr channel);

        protected:
          static String toDebugString(IFinderConnectionRelayChannelPtr channel, bool includeCommaPrefix = true);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnection::Channel => IFinderConnectionRelayChannel
          #pragma mark

          static ChannelPtr connect(
                                    FinderConnectionPtr outer,
                                    IFinderConnectionRelayChannelDelegatePtr delegate,
                                    const char *localContextID,
                                    const char *remoteContextID,
                                    const char *relayDomain,
                                    const char *relayAccessToken,
                                    const char *relayAccessSecretProof,
                                    ITransportStreamPtr receiveStream,
                                    ITransportStreamPtr sendStream,
                                    ULONG channelNumber
                                    );

          virtual PUID getID() const {return mID;}

          virtual void cancel();

          virtual SessionStates getState(
                                         WORD *outLastErrorCode = NULL,
                                         String *outLastErrorReason = NULL
                                         ) const;

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnection::Channel => ITransportStreamWriterDelegate
          #pragma mark

          virtual void onTransportStreamWriterReady(ITransportStreamWriterPtr writer);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnection::Channel => ITransportStreamReaderDelegate
          #pragma mark

          virtual void onTransportStreamReaderReady(ITransportStreamReaderPtr reader);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnection::Channel => friend FinderConnection
          #pragma mark

          static ChannelPtr incoming(
                                     FinderConnectionPtr outer,
                                     IFinderConnectionRelayChannelDelegatePtr delegate,
                                     ITransportStreamPtr receiveStream,
                                     ITransportStreamPtr sendStream,
                                     ULONG channelNumber
                                     );

          void notifyReceivedWireWriteReady();

          // (duplicate) virtual SessionStates getState(
          //                                            WORD *outLastErrorCode = NULL,
          //                                            String *outLastErrorReason = NULL
          //                                            ) const;
          // (duplicate) void setError(WORD errorCode, const char *inReason = NULL);

          void notifyDataReceived(SecureByteBlockPtr buffer);

          void getStreams(
                          ITransportStreamPtr &outReceiveStream,
                          ITransportStreamPtr &outSendStream
                          );

          const ConnectionInfo &getConnectionInfo() {return mConnectionInfo;}

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnection::Channel => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

          bool isShutdown() const {return SessionState_Shutdown == mCurrentState;}

          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          void setState(SessionStates state);
          void setError(WORD errorCode, const char *inReason = NULL);

          void step();

        protected:
          AutoPUID mID;
          mutable RecursiveLock mBogusLock;
          ChannelWeakPtr mThisWeak;

          FinderConnectionWeakPtr mOuter;

          IFinderConnectionRelayChannelDelegatePtr mDelegate;

          SessionStates mCurrentState;

          AutoWORD mLastError;
          String mLastErrorReason;

          ChannelNumber mChannelNumber;

          ITransportStreamWriterPtr mOuterReceiveStream;
          ITransportStreamReaderPtr mOuterSendStream;

          AutoBool mWireStreamNotifiedReady;
          AutoBool mOuterStreamNotifiedReady;

          ConnectionInfo mConnectionInfo;
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnection => (data)
        #pragma mark

        AutoPUID mID;
        mutable RecursiveLock mLocalLock;
        FinderConnectionWeakPtr mThisWeak;

        FinderConnectionManagerWeakPtr mOuter;

        IFinderConnectionDelegateSubscriptions mSubscriptions;
        IFinderConnectionSubscriptionPtr mDefaultSubscription;

        SessionStates mCurrentState;

        AutoWORD mLastError;
        String mLastErrorReason;

        IPAddress mRemoteIP;
        ITCPMessagingPtr mTCPMessaging;

        ITransportStreamReaderPtr mWireReceiveStream;
        ITransportStreamWriterPtr mWireSendStream;

        AutoBool mSendStreamNotifiedReady;

        Time mLastTick;
        Time mLastReceivedData;
        TimerPtr mInactivityTimer;

        ChannelMap mChannels;

        ChannelMap mPendingMapRequest;
        ChannelMap mIncomingChannels;
        ChannelMap mRemoveChannels;

        IMessageMonitorPtr mMapRequestChannelMonitor;
        AutoChannelNumber mMapRequestChannelNumber;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderConnectionRelayChannelFactory
      #pragma mark

      interaction IFinderConnectionRelayChannelFactory
      {
        static IFinderConnectionRelayChannelFactory &singleton();

        virtual IFinderConnectionPtr connect(
                                             IFinderConnectionDelegatePtr delegate,
                                             const IPAddress &remoteFinderIP,
                                             ITransportStreamPtr receiveStream,
                                             ITransportStreamPtr sendStream
                                             );

        virtual IFinderConnectionRelayChannelPtr connect(
                                                         IFinderConnectionRelayChannelDelegatePtr delegate,
                                                         const IPAddress &remoteFinderIP,
                                                         const char *localContextID,
                                                         const char *remoteContextID,
                                                         const char *relayDomain,
                                                         const char *relayAccessToken,
                                                         const char *relayAccessSecretProof,
                                                         ITransportStreamPtr receiveStream,
                                                         ITransportStreamPtr sendStream
                                                         );
      };
      
    }
  }
}
