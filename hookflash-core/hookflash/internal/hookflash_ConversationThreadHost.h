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

#include <hookflash/internal/hookflashTypes.h>
#include <hookflash/internal/hookflash_ConversationThread.h>
#include <hookflash/internal/hookflash_ConversationThreadDocumentFetcher.h>
#include <hookflash/internal/hookflash_IConversationThreadParser.h>

#include <hookflash/stack/IPeerSubscription.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>
#include <zsLib/Timer.h>

namespace hookflash
{
  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadHostForConversationThread
    #pragma mark

    interaction IConversationThreadHostForConversationThread : public IConversationThreadHostSlaveBase
    {
      static IConversationThreadHostForConversationThreadPtr create(
                                                                    IConversationThreadForHostPtr baseThread,
                                                                    IConversationThreadParser::Details::ConversationThreadStates state = IConversationThreadParser::Details::ConversationThreadState_Open
                                                                    );

      virtual void close() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadHostAsync
    #pragma mark

    interaction IConversationThreadHostAsync
    {
      virtual void onStep() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThreadHost
    #pragma mark

    class ConversationThreadHost  : public zsLib::MessageQueueAssociator,
                                    public IConversationThreadHostForConversationThread,
                                    public IConversationThreadHostAsync
    {
    public:
      enum ConversationThreadHostStates
      {
        ConversationThreadHostState_Pending,
        ConversationThreadHostState_Ready,
        ConversationThreadHostState_ShuttingDown,
        ConversationThreadHostState_Shutdown,
      };

      static const char *toString(ConversationThreadHostStates state);

      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
      typedef zsLib::PUID PUID;
      typedef zsLib::String String;
      typedef zsLib::TimerPtr TimerPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
      typedef stack::IPeerLocationPtr IPeerLocationPtr;
      typedef stack::IPublicationRepositoryPtr IPublicationRepositoryPtr;
      typedef IConversationThreadParser::Contacts::ContactIDList ContactIDList;
      typedef IConversationThreadParser::ThreadPtr ThreadPtr;

      interaction IPeerContactAsync;
      typedef boost::shared_ptr<IPeerContactAsync> IPeerContactAsyncPtr;
      typedef boost::weak_ptr<IPeerContactAsync> IPeerContactAsyncWeakPtr;
      typedef zsLib::Proxy<IPeerContactAsync> IPeerContactAsyncProxy;

      class PeerContact;
      typedef boost::shared_ptr<PeerContact> PeerContactPtr;
      typedef boost::weak_ptr<PeerContact> PeerContactWeakPtr;
//      friend class PeerContact;

      class PeerLocation;
      typedef boost::shared_ptr<PeerLocation> PeerLocationPtr;
      typedef boost::weak_ptr<PeerLocation> PeerLocationWeakPtr;
//      friend class PeerLocation;

    protected:
      ConversationThreadHost(
                             IAccountForConversationThreadPtr account,
                             IConversationThreadForHostPtr baseThread,
                             const char *threadID
                             );

      void init(IConversationThreadParser::Details::ConversationThreadStates state);

    public:
      ~ConversationThreadHost();

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => IConversationThreadHostSlaveBase
      #pragma mark

      static ConversationThreadHostPtr create(
                                              IConversationThreadForHostPtr baseThread,
                                              IConversationThreadParser::Details::ConversationThreadStates state = IConversationThreadParser::Details::ConversationThreadState_Open
                                              );

      virtual const String &getThreadID() const;

      virtual bool isHost() const {return true;}
      virtual bool isSlave() const {return false;}

      virtual void shutdown();
      // (duplicate) virtual bool isShutdown() const;

      virtual IConversationThreadHostForConversationThreadPtr toHost() const {return mThisWeak.lock();}
      virtual IConversationThreadSlaveForConversationThreadPtr toSlave() const {return IConversationThreadSlaveForConversationThreadPtr();}

      virtual bool isHostThreadOpen() const;

      virtual void notifyPublicationUpdated(
                                            IPeerLocationPtr peerLocation,
                                            IPublicationMetaDataPtr metaData,
                                            const SplitMap &split
                                            );

      virtual void notifyPublicationGone(
                                         IPeerLocationPtr peerLocation,
                                         IPublicationMetaDataPtr metaData,
                                         const SplitMap &split
                                         );

      virtual void notifyPeerDisconnected(IPeerLocationPtr peerLocation);

      virtual bool sendMessages(const MessageList &messages);

      virtual Time getHostCreationTime() const;

      virtual bool safeToChangeContacts() const;

      virtual void getContacts(ContactMap &outContacts) const;
      virtual bool inConversation(IContactForConversationThreadPtr contact) const;
      virtual void addContacts(const ContactInfoList &contacts);
      virtual void removeContacts(const ContactList &contacts);

      virtual ContactStates getContactState(IContactForConversationThreadPtr contact) const;

      virtual bool placeCalls(const PendingCallMap &pendingCalls);
      virtual void notifyCallStateChanged(ICallForConversationThreadPtr call);
      virtual void notifyCallCleanup(ICallForConversationThreadPtr call);

      virtual void gatherDialogReplies(
                                       const char *callID,
                                       LocationDialogMap &outDialogs
                                       ) const;

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => IConversationThreadHostForConversationThread
      #pragma mark

      virtual void close();

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => IConversationThreadHostAsync
      #pragma mark

      virtual void onStep() {step();}

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => friend PeerContact
      #pragma mark

      // (duplicate) RecursiveLock &getLock() const;

      ThreadPtr getHostThread() const;
      IAccountForConversationThreadPtr getAccount() const;
      IPublicationRepositoryPtr getRepository() const;
      IConversationThreadForHostPtr getBaseThread() const;

      void notifyMessagesReceived(const MessageList &messages);
      void notifyMessageDeliveryStateChanged(
                                             const String &messageID,
                                             IConversationThread::MessageDeliveryStates state
                                             );
      virtual void notifyMessagePush(
                                     MessagePtr message,
                                     IContactForConversationThreadPtr toContact
                                     );

      void notifyStateChanged(PeerContactPtr peerContact);
      void notifyContactState(
                              IContactForConversationThreadPtr contact,
                              ContactStates state
                              );

      bool hasCallPlacedTo(IContactForConversationThreadPtr toContact);

    private:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => (internal)
      #pragma mark

      bool isPending() const {return ConversationThreadHostState_Pending == mCurrentState;}
      bool isReady() const {return ConversationThreadHostState_Ready == mCurrentState;}
      bool isShuttingDown() const {return ConversationThreadHostState_ShuttingDown == mCurrentState;}
      virtual bool isShutdown() const {AutoRecursiveLock(getLock()); return ConversationThreadHostState_Shutdown == mCurrentState;}

      String log(const char *message) const;

    protected:
      RecursiveLock &getLock() const;

    private:
      void cancel();
      void step();

      void setState(ConversationThreadHostStates state);

      void publish(
                   bool publishHostPublication,
                   bool publishHostPermissionPublication
                   ) const;

      void removeContacts(const ContactIDList &contacts);

      PeerContactPtr findContact(IPeerLocationPtr peerLocation) const;

    public:
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::IPeerContactAsync
      #pragma mark

      interaction IPeerContactAsync
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerContact
      #pragma mark

      class PeerContact : public MessageQueueAssociator,
                          public stack::IPeerSubscriptionDelegate,
                          public IPeerContactAsync,
                          public zsLib::ITimerDelegate
      {
      public:
        enum PeerContactStates
        {
          PeerContactState_Pending,
          PeerContactState_Ready,
          PeerContactState_ShuttingDown,
          PeerContactState_Shutdown,
        };

        static const char *toString(PeerContactStates state);

        typedef zsLib::PUID PUID;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef stack::IPeerSubscriptionPtr IPeerSubscriptionPtr;
        typedef stack::IPeerSubscriptionMessagePtr IPeerSubscriptionMessagePtr;
        typedef stack::IPeerLocationPtr IPeerLocationPtr;
        typedef stack::IPublicationPtr IPublicationPtr;
        typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
        typedef stack::IPublicationRepositoryPtr IPublicationRepositoryPtr;
        typedef IConversationThreadParser::ThreadPtr ThreadPtr;
        typedef IConversationThreadParser::Thread::MessageReceiptMap MessageReceiptMap;
        typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
        typedef IConversationThreadParser::Contacts::ContactIDList ContactIDList;

        friend class ConversationThreadHost;
        friend class PeerLocation;

      private:
        PeerContact(
                    IMessageQueuePtr queue,
                    ConversationThreadHostPtr host,
                    IContactForConversationThreadPtr contact,
                    ElementPtr profileBundleEl
                    );

        void init();

      public:
        ~PeerContact();

      protected:
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerContact => friend ConversationThreadHost
        #pragma mark

        static PeerContactPtr create(
                                     IMessageQueuePtr queue,
                                     ConversationThreadHostPtr host,
                                     IContactForConversationThreadPtr contact,
                                     ElementPtr profileBundleEL
                                     );

        void notifyPublicationUpdated(
                                      IPeerLocationPtr peerLocation,
                                      IPublicationMetaDataPtr metaData,
                                      const SplitMap &split
                                      );
        void notifyPublicationGone(
                                   IPeerLocationPtr peerLocation,
                                   IPublicationMetaDataPtr metaData,
                                   const SplitMap &split
                                   );
        void notifyPeerDisconnected(IPeerLocationPtr peerLocation);

        IContactForConversationThreadPtr getContact() const;
        const ElementPtr &getProfileBundle() const;

        ContactStates getContactState() const;

        void gatherMessageReceipts(MessageReceiptMap &receipts) const;

        void gatherContactsToAdd(ContactMap &contacts) const;
        void gatherContactsToRemove(ContactIDList &contacts) const;

        void gatherDialogReplies(
                                 const char *callID,
                                 LocationDialogMap &outDialogs
                                 ) const;

        void notifyStep(bool performStepAsync = true);

        // (duplicate) void cancel();

      public:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerContact => IPeerSubscriptionDelegate
        #pragma mark

        virtual void onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription);

        virtual void onPeerSubscriptionFindStateChanged(
                                                        IPeerSubscriptionPtr subscription,
                                                        PeerSubscriptionFindStates state
                                                        );

        virtual void onPeerSubscriptionLocationsChanged(IPeerSubscriptionPtr subscription);

        virtual void onPeerSubscriptionMessage(
                                               IPeerSubscriptionPtr subscription,
                                               IPeerSubscriptionMessagePtr incomingMessage
                                               );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerContact => IPeerContactAsync
        #pragma mark

        virtual void onStep() {step();}

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerContact => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerContact => friend ConversationThreadHost::PeerLocation
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;

        ConversationThreadHostPtr getOuter() const;
        IConversationThreadForHostPtr getBaseThread() const;
        ThreadPtr getHostThread() const;
        IAccountForConversationThreadPtr getAccount() const;
        IPublicationRepositoryPtr getRepository() const;

        void notifyMessagesReceived(const MessageList &messages);
        void notifyMessageDeliveryStateChanged(
                                               const String &messageID,
                                               IConversationThread::MessageDeliveryStates state
                                               );

        void notifyStateChanged(PeerLocationPtr peerLocation);

        void notifyPeerLocationShutdown(PeerLocationPtr location);

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerContact => (internal)
        #pragma mark

        RecursiveLock &getLock() const;
        PUID getID() const {return mID;}
        String log(const char *message) const;
        bool isPending() const {return PeerContactState_Pending == mCurrentState;}
        bool isReady() const {return PeerContactState_Ready == mCurrentState;}
        bool isShuttingDown() const {return PeerContactState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return PeerContactState_Shutdown == mCurrentState;}

        void cancel();
        void step();
        void setState(PeerContactStates state);

        PeerLocationPtr findPeerLocation(IPeerLocationPtr peerLocation) const;

        bool isStillPartOfCurrentConversation(IContactForConversationThreadPtr contact) const;

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerContact => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;
        PeerContactWeakPtr mThisWeak;
        PeerContactPtr mGracefulShutdownReference;
        ConversationThreadHostWeakPtr mOuter;
        PeerContactStates mCurrentState;

        IContactForConversationThreadPtr mContact;
        ElementPtr mProfileBundleEl;

        IPeerSubscriptionPtr mSlaveSubscription;
        TimerPtr mSlaveMessageDeliveryTimer;

        typedef zsLib::String LocationID;
        typedef std::map<LocationID, PeerLocationPtr> PeerLocationMap;
        PeerLocationMap mPeerLocations;

        typedef String MessageID;
        typedef Time StateChangedTime;
        typedef std::pair<StateChangedTime, IConversationThread::MessageDeliveryStates> DeliveryStatePair;
        typedef std::map<MessageID, DeliveryStatePair> MessageDeliveryStatesMap;
        MessageDeliveryStatesMap mMessageDeliveryStates;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerLocation
      #pragma mark

      class PeerLocation : public MessageQueueAssociator,
                           public IConversationThreadDocumentFetcherDelegate
      {
      public:
        typedef zsLib::PUID PUID;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
        typedef stack::IPeerLocationPtr IPeerLocationPtr;
        typedef stack::IPublicationPtr IPublicationPtr;
        typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
        typedef IConversationThreadParser::ThreadPtr ThreadPtr;
        typedef IConversationThreadParser::MessageReceipts::MessageReceiptMap MessageReceiptMap;
        typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
        typedef IConversationThreadParser::Contacts::ContactIDList ContactIDList;

        friend class PeerContact;

      protected:
        PeerLocation(
                     IMessageQueuePtr queue,
                     PeerContactPtr peerContact,
                     IPeerLocationPtr peerLocation
                     );

        void init();

      public:
        ~PeerLocation();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerLocation => friend ConversationThreadHost::PeerContact
        #pragma mark

        static PeerLocationPtr create(
                                      IMessageQueuePtr queue,
                                      PeerContactPtr peerContact,
                                      IPeerLocationPtr peerLocation
                                      );

        PUID getID() const {return mID;}
        String getLocationID() const;
        
        bool isConnected() const;

        void notifyPublicationUpdated(
                                      IPeerLocationPtr peerLocation,
                                      IPublicationMetaDataPtr metaData,
                                      const SplitMap &split
                                      );
        void notifyPublicationGone(
                                   IPeerLocationPtr peerLocation,
                                   IPublicationMetaDataPtr metaData,
                                   const SplitMap &split
                                   );
        void notifyPeerDisconnected(IPeerLocationPtr peerLocation);

        void gatherMessageReceipts(MessageReceiptMap &receipts) const;

        void gatherContactsToAdd(ContactMap &contacts) const;
        void gatherContactsToRemove(ContactIDList &contacts) const;

        void gatherDialogReplies(
                                 const char *callID,
                                 LocationDialogMap &outDialogs
                                 ) const;

        // (duplicate) void cancel();
        // (duplicate) void step();

      public:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerLocation => IConversationThreadDocumentFetcherDelegate
        #pragma mark

        virtual void onConversationThreadDocumentFetcherPublicationUpdated(
                                                                           IConversationThreadDocumentFetcherPtr fetcher,
                                                                           IPeerLocationPtr peerLocation,
                                                                           IPublicationPtr publication
                                                                           );

        virtual void onConversationThreadDocumentFetcherPublicationGone(
                                                                        IConversationThreadDocumentFetcherPtr fetcher,
                                                                        IPeerLocationPtr peerLocation,
                                                                        IPublicationMetaDataPtr metaData
                                                                        );

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerLocation => (internal)
        #pragma mark

      private:
        String log(const char *message) const;
        bool isShutdown() {return mShutdown;}

      protected:
        RecursiveLock &getLock() const;

        void cancel();
        void step();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThreadHost::PeerLocation => (data)
        #pragma mark

        PUID mID;
        mutable RecursiveLock mBogusLock;
        PeerLocationWeakPtr mThisWeak;
        PeerContactWeakPtr mOuter;
        bool mShutdown;

        IPeerLocationPtr mPeerLocation;

        ThreadPtr mSlaveThread;

        IConversationThreadDocumentFetcherPtr mFetcher;

        typedef String MessageID;
        typedef std::map<MessageID, IConversationThread::MessageDeliveryStates> MessageDeliveryStatesMap;
        MessageDeliveryStatesMap mMessageDeliveryStates;

        typedef String CallID;
        typedef std::map<CallID, ICallForConversationThreadPtr> CallHandlers;

        CallHandlers mIncomingCallHandlers;
      };

    protected:
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => (data)
      #pragma mark

      mutable RecursiveLock mBogusLock;
      PUID mID;
      ConversationThreadHostWeakPtr mThisWeak;
      ConversationThreadHostPtr mGracefulShutdownReference;

      IConversationThreadForHostWeakPtr mBaseThread;
      IAccountForConversationThreadWeakPtr mAccount;

      IContactForConversationThreadPtr mSelfContact;

      String mThreadID;

      ConversationThreadHostStates mCurrentState;

      ThreadPtr mHostThread;

      typedef String MessageID;
      typedef std::map<MessageID, IConversationThread::MessageDeliveryStates> MessageDeliveryStatesMap;
      MessageDeliveryStatesMap mMessageDeliveryStates;

      typedef String ContactID;
      typedef std::map<ContactID, PeerContactPtr> PeerContactMap;
      PeerContactMap mPeerContacts;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::internal::IConversationThreadHostAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::internal::ConversationThreadHost::IPeerContactAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
