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

#include <openpeer/stack/internal/stack_IFinderRelayChannel.h>
#include <openpeer/stack/message/types.h>
#include <openpeer/stack/internal/types.h>

#include <openpeer/stack/IMessageLayerSecurityChannel.h>

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
      #pragma mark FinderRelayChannel
      #pragma mark

      class FinderRelayChannel : public Noop,
                                 public zsLib::MessageQueueAssociator,
                                 public IFinderRelayChannel,
                                 public IMessageLayerSecurityChannelDelegate
      {
      public:
        friend interaction IFinderRelayChannelFactory;

        typedef IFinderRelayChannel::SessionStates SessionStates;

        class Subscription;
        typedef boost::shared_ptr<Subscription> SubscriptionPtr;
        typedef boost::weak_ptr<Subscription> SubscriptionWeakPtr;

        typedef PUID SubscriptionID;
        typedef std::map<SubscriptionID, SubscriptionWeakPtr> SubscriptionMap;

        friend class Subscription;

      protected:
        FinderRelayChannel(
                           IMessageQueuePtr queue,
                           AccountPtr account
                           );

        FinderRelayChannel(Noop) :
          Noop(true),
          zsLib::MessageQueueAssociator(IMessageQueuePtr()) {}

        void init(IFinderRelayChannelDelegatePtr delegate);

      public:
        ~FinderRelayChannel();

        static FinderRelayChannelPtr convert(IFinderRelayChannelPtr channel);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderRelayChannel => IFinderRelayChannel
        #pragma mark

        static String toDebugString(IFinderRelayChannelPtr channel, bool includeCommaPrefix = true);

        static FinderRelayChannelPtr connect(
                                             IFinderRelayChannelDelegatePtr delegate,        // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                             AccountPtr account,
                                             IPAddress remoteFinderIP,
                                             const char *localContextID,
                                             const char *relayAccessToken,
                                             const char *relayAccessSecretProof,
                                             const char *encryptDataUsingEncodingPassphrase
                                             );

        static FinderRelayChannelPtr createIncoming(
                                                    IFinderRelayChannelDelegatePtr delegate, // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                                    AccountPtr account
                                                    );

        virtual PUID getID() const {return mID;}

        virtual void cancel();

        virtual SessionStates getState(
                                       WORD *outLastErrorCode = NULL,
                                       String *outLastErrorReason = NULL
                                       ) const;

        virtual IFinderRelayChannelSubscriptionPtr subscribe(IFinderRelayChannelDelegatePtr delegate);

        virtual bool send(
                          const BYTE *buffer,
                          ULONG bufferSizeInBytes
                          );

        virtual SecureByteBlockPtr getNextIncomingMessage();

        virtual void setIncomingContext(
                                        const char *contextID,
                                        const char *decryptUsingEncodingPassphrase
                                        );

        virtual String getLocalContextID() const;

        virtual String getRemoteContextID() const;

        virtual IPeerPtr getRemotePeer() const;

        virtual IRSAPublicKeyPtr getRemotePublicKey() const;

        virtual SecureByteBlockPtr getNextPendingBufferToSendOnWire();

        virtual void notifyReceivedFromWire(
                                            const BYTE *buffer,
                                            ULONG bufferLengthInBytes
                                            );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderRelayChannel => IMessageLayerSecurityChannelDelegate
        #pragma mark

        virtual void onMessageLayerSecurityChannelStateChanged(
                                                               IMessageLayerSecurityChannelPtr channel,
                                                               IMessageLayerSecurityChannel::SessionStates state
                                                               );

        virtual void onMessageLayerSecurityChannelNeedDecodingPassphrase(IMessageLayerSecurityChannelPtr channel);

        virtual void onMessageLayerSecurityChannelIncomingMessage(IMessageLayerSecurityChannelPtr channel);

        virtual void onMessageLayerSecurityChannelBufferPendingToSendOnTheWire(IMessageLayerSecurityChannelPtr channel);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderRelayChannel => IFinderRelayChannel
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;

        void notifySubscriptionGone(Subscription &subscription);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderRelayChannel => (internal)
        #pragma mark

        bool isShutdown() const {return SessionState_Shutdown == mCurrentState;}

        RecursiveLock &getLock() const;
        String log(const char *message) const;

        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void setState(SessionStates state);
        void setError(WORD errorCode, const char *inReason = NULL);

        void step();
        
      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderRelayChannel::Subscription
        #pragma mark

        class Subscription : public IFinderRelayChannelSubscription
        {
        public:
          friend class FinderRelayChannel;

        protected:
          Subscription(
                       FinderRelayChannelPtr outer,
                       IFinderRelayChannelDelegatePtr delegate
                       );

          void init();

        public:
          ~Subscription();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderRelayChannel::Subscription => IFinderRelayChannelSubscription
          #pragma mark

          virtual PUID getID() const {return mID;}

          virtual void cancel();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderRelayChannel::Subscription => friend FinderRelayChannel
          #pragma mark

          static SubscriptionPtr create(
                                        FinderRelayChannelPtr outer,
                                        IFinderRelayChannelDelegatePtr delegate
                                        );

          // (duplicate) virtual PUID getID() const;

          void notifyStateChanged(IFinderRelayChannel::SessionStates state);

          void notifylNeedsContext();

          void notifyIncomingMessage();

          void notifyBufferPendingToSendOnTheWire();

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderRelayChannel::Subscription => (internal)
          #pragma mark

          RecursiveLock &getLock() const;
          String log(const char *message) const;

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark FinderRelayChannel::Subscription => (data)
          #pragma mark

          PUID mID;
          mutable RecursiveLock mBogusLock;
          SubscriptionWeakPtr mThisWeak;

          FinderRelayChannelWeakPtr mOuter;

          IFinderRelayChannelDelegatePtr mDelegate;
        };

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark FinderRelayChannel => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mLock;
        FinderRelayChannelWeakPtr mThisWeak;

        SessionStates mCurrentState;

        WORD mLastError;
        String mLastErrorReason;

        AccountWeakPtr mAccount;

        IMessageLayerSecurityChannelPtr mMLSChannel;

        SubscriptionPtr mDefaultSubscription;
        SubscriptionMap mSubscriptions;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IFinderRelayChannelFactory
      #pragma mark

      interaction IFinderRelayChannelFactory
      {
        static IFinderRelayChannelFactory &singleton();

        virtual FinderRelayChannelPtr connect(
                                              IFinderRelayChannelDelegatePtr delegate,        // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                              AccountPtr account,
                                              IPAddress remoteFinderIP,
                                              const char *localContextID,
                                              const char *relayAccessToken,
                                              const char *relayAccessSecretProof,
                                              const char *encryptDataUsingEncodingPassphrase
                                              );

        virtual FinderRelayChannelPtr createIncoming(
                                                     IFinderRelayChannelDelegatePtr delegate, // can pass in IFinderRelayChannelDelegatePtr() if not interested in the events
                                                     AccountPtr account
                                                     );
      };
      
    }
  }
}
