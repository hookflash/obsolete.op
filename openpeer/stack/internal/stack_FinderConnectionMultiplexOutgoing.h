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

#include <openpeer/stack/internal/stack_IFinderConnectionRelayChannel.h>
#include <openpeer/stack/message/types.h>
#include <openpeer/stack/internal/types.h>
#include <openpeer/stack/internal/stack_IFinderConnection.h>

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
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark FinderConnectionMultiplexOutgoing
      #pragma mark

      class FinderConnectionMultiplexOutgoing : public Noop,
                                                public zsLib::MessageQueueAssociator,
                                                public IFinderConnection,
                                                public ITimerDelegate,
                                                public IWakeDelegate,
                                                public ITCPMessagingDelegate,
                                                public ITransportStreamWriterDelegate,
                                                public ITransportStreamReaderDelegate,
                                                public IFinderConnectionRelayChannelDelegate
      {
      public:
        typedef IFinderConnection::SessionStates SessionStates;

        friend interaction IFinderConnectionRelayChannelFactory;

        friend class FinderConnectionMultiplexOutgoingManager;
        friend class Channel;

        class Channel;
        typedef boost::shared_ptr<Channel> ChannelPtr;
        typedef boost::weak_ptr<Channel> ChannelWeakPtr;

        typedef ULONG ChannelNumber;
        typedef std::map<ChannelNumber, ChannelPtr> ChannelMap;

      protected:
        FinderConnectionMultiplexOutgoing(
                                          IMessageQueuePtr queue,
                                          IPAddress remoteFinderIP
                                          );

        FinderConnectionMultiplexOutgoing(Noop) :
          Noop(true),
          zsLib::MessageQueueAssociator(IMessageQueuePtr()) {}

        void init();

      public:
        virtual ~FinderConnectionMultiplexOutgoing();

        static FinderConnectionMultiplexOutgoingPtr convert(IFinderConnectionPtr connection);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => friend IFinderConnectionRelayChannelFactory
        #pragma mark

        static IFinderConnectionRelayChannelPtr connect(
                                                        IFinderConnectionRelayChannelDelegatePtr delegate,
                                                        const IPAddress &remoteFinderIP,
                                                        const char *localContextID,
                                                        const char *relayAccessToken,
                                                        const char *relayAccessSecretProof,
                                                        ITransportStreamPtr receiveStream,
                                                        ITransportStreamPtr sendStream
                                                        );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => IFinderConnection
        #pragma mark

        static String toDebugString(IFinderConnectionPtr connection, bool includeCommaPrefix = true);

        static IFinderConnectionPtr connect(
                                            IFinderConnectionDelegatePtr delegate,
                                            const IPAddress &remoteFinderIP,
                                            ITransportStreamPtr receiveStream,
                                            ITransportStreamPtr sendStream
                                            );

        virtual PUID getID() const {return mID;}

        virtual IFinderConnectionSubscriptionPtr subscribe(IFinderConnectionDelegatePtr delegate) = 0;

        virtual void cancel();

        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const = 0;

        virtual IFinderRelayChannelPtr accept(
                                              IFinderRelayChannelDelegatePtr delegate,        // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                              AccountPtr account,
                                              ITransportStreamPtr receiveStream,
                                              ITransportStreamPtr sendStream
                                              );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => IWakeDelegate
        #pragma mark

        virtual void onWake();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => ITCPMessagingDelegate
        #pragma mark

        virtual void onTCPMessagingStateChanged(
                                                ITCPMessagingPtr messaging,
                                                ITCPMessaging::SessionStates state
                                                );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => ITransportStreamWriterDelegate
        #pragma mark

        virtual void onTransportStreamWriterReady(ITransportStreamWriterPtr writer);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => ITransportStreamReaderDelegate
        #pragma mark

        virtual void onTransportStreamReaderReady(ITransportStreamReaderPtr reader);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => IFinderConnectionRelayChannelDelegate
        #pragma mark

        virtual void onFinderRelayChannelTCPOutgoingStateChanged(
                                                                 IFinderConnectionRelayChannelPtr channel,
                                                                 IFinderConnectionRelayChannel::SessionStates state
                                                                 );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => friend Channel
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
        #pragma mark FinderConnectionMultiplexOutgoing => (internal)
        #pragma mark

        bool isShutdown() const {return SessionState_Shutdown == mCurrentState;}

        RecursiveLock &getLock() const;
        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void setState(SessionStates state);
        void setError(WORD errorCode, const char *inReason = NULL);

        void step();
        bool stepCleanRemoval();

        static FinderConnectionMultiplexOutgoingPtr create(IPAddress remoteFinderIP);

        IFinderConnectionRelayChannelPtr connect(
                                                 IFinderConnectionRelayChannelDelegatePtr delegate,
                                                 const char *localContextID,
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
        #pragma mark FinderConnectionMultiplexOutgoing::Channel
        #pragma mark

        class Channel : public Noop,
                        public zsLib::MessageQueueAssociator,
                        public IFinderConnectionRelayChannel,
                        public ITransportStreamWriterDelegate,
                        public ITransportStreamReaderDelegate
        {
        public:
          friend class FinderConnectionMultiplexOutgoing;
          typedef IFinderConnectionRelayChannel::SessionStates SessionStates;

        protected:
          Channel(
                  IMessageQueuePtr queue,
                  IFinderConnectionRelayChannelDelegatePtr delegate,
                  ITransportStreamPtr receiveStream,
                  ITransportStreamPtr sendStream,
                  ChannelNumber channelNumber
                  );

          Channel(Noop) :
            Noop(true),
            zsLib::MessageQueueAssociator(IMessageQueuePtr()) {}

          void init();

        public:
          ~Channel();

          static ChannelPtr convert(IFinderConnectionRelayChannelPtr channel);

        protected:
          static String toDebugString(IFinderRelayChannelPtr channel, bool includeCommaPrefix = true);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnectionMultiplexOutgoing::Channel => IFinderConnectionRelayChannel
          #pragma mark

          static ChannelPtr connect(
                                    FinderConnectionMultiplexOutgoingPtr outer,
                                    IFinderConnectionRelayChannelDelegatePtr delegate,
                                    const char *localContextID,
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
          #pragma mark FinderConnectionMultiplexOutgoing::Channel => ITransportStreamWriterDelegate
          #pragma mark

          virtual void onTransportStreamWriterReady(ITransportStreamWriterPtr writer);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnectionMultiplexOutgoing::Channel => ITransportStreamReaderDelegate
          #pragma mark

          virtual void onTransportStreamReaderReady(ITransportStreamReaderPtr reader);

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnectionMultiplexOutgoing::Channel => friend FinderConnectionMultiplexOutgoing
          #pragma mark

          static ChannelPtr incoming(
                                     FinderConnectionMultiplexOutgoingPtr outer,
                                     IFinderConnectionRelayChannelDelegatePtr delegate,
                                     ITransportStreamPtr receiveStream,
                                     ITransportStreamPtr sendStream,
                                     ULONG channelNumber
                                     );

          void notifyReceivedWireWriteReady();

          // (duplicate) void setError(WORD errorCode, const char *inReason = NULL);
          void notifyDataReceived(SecureByteBlockPtr buffer);

          void getStreams(
                          ITransportStreamPtr &outReceiveStream,
                          ITransportStreamPtr &outSendStream
                          );

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderConnectionMultiplexOutgoing::Channel => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          void setState(SessionStates state);
          void setError(WORD errorCode, const char *inReason = NULL);

        protected:
          AutoPUID mID;
          mutable RecursiveLock mBogusLock;
          ChannelPtr mThisWeak;

          FinderConnectionMultiplexOutgoingWeakPtr mOuter;

          IFinderConnectionRelayChannelDelegatePtr mDelegate;

          SessionStates mCurrentState;

          AutoWORD mLastError;
          String mLastErrorReason;

          ChannelNumber mChannelNumber;

          ITransportStreamWriterPtr mOuterReceiveStream;
          ITransportStreamReaderPtr mOuterSendStream;
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderConnectionMultiplexOutgoing => (data)
        #pragma mark

        AutoPUID mID;
        mutable RecursiveLock mLocalLock;
        FinderConnectionMultiplexOutgoingWeakPtr mThisWeak;

        FinderConnectionMultiplexOutgoingManagerWeakPtr mOuter;

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
                                                         const char *relayAccessToken,
                                                         const char *relayAccessSecretProof,
                                                         ITransportStreamPtr receiveStream,
                                                         ITransportStreamPtr sendStream
                                                         );
      };
      
    }
  }
}
