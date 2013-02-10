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

#include <hookflash/core/internal/types.h>
#include <hookflash/core/internal/core_CallTransport.h>

#include <hookflash/core/IAccount.h>

#include <hookflash/stack/IAccount.h>
#include <hookflash/stack/IPeerSubscription.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/stack/IPublicationRepository.h>
#include <hookflash/stack/IServicePeerContact.h>

#include <zsLib/MessageQueueAssociator.h>

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      typedef stack::IHelper::SplitMap SplitMap;


      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForCall
      #pragma mark

      interaction IAccountForCall
      {
        IAccountForCall &forCall() {return *this;}
        const IAccountForCall &forCall() const {return *this;}

        virtual CallTransportPtr getCallTransport() const = 0;
        virtual ICallDelegatePtr getCallDelegate() const = 0;

        virtual ContactPtr getSelfContact() const = 0;
        virtual ILocationPtr getSelfLocation() const = 0;

        virtual stack::IAccountPtr getStackAccount() const = 0;

        virtual IPeerFilesPtr getPeerFiles() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForContact
      #pragma mark

      interaction IAccountForContact
      {
        IAccountForContact &forContact() {return *this;}
        const IAccountForContact &forContact() const {return *this;}

        virtual RecursiveLock &getLock() const = 0;

        virtual ContactPtr getSelfContact() const = 0;

        virtual stack::IAccountPtr getStackAccount() const = 0;

        virtual ContactPtr findContact(const char *peerURI) const = 0;

        virtual void notifyAboutContact(ContactPtr contact) = 0;

        virtual void hintAboutContactLocation(
                                              ContactPtr contact,
                                              const char *locationID
                                              ) = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForConversationThread
      #pragma mark

      interaction IAccountForConversationThread
      {
        IAccountForConversationThread &forConversationThread() {return *this;}
        const IAccountForConversationThread &forConversationThread() const {return *this;}

        virtual RecursiveLock &getLock() const = 0;

        virtual ContactPtr getSelfContact() const = 0;
        virtual ILocationPtr getSelfLocation() const = 0;

        virtual ContactPtr findContact(const char *peerURI) const = 0;

        virtual stack::IAccountPtr getStackAccount() const = 0;
        virtual IPublicationRepositoryPtr getRepository() const = 0;

        virtual IPeerFilesPtr getPeerFiles() const = 0;

        virtual IConversationThreadDelegatePtr getConversationThreadDelegate() const = 0;
        virtual void notifyConversationThreadCreated(ConversationThreadPtr thread) = 0;

        virtual ConversationThreadPtr getConversationThreadByID(const char *threadID) const = 0;
        virtual void getConversationThreads(ConversationThreadList &outConversationThreads) const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountForIdentityLookup
      #pragma mark

      interaction IAccountForIdentityLookup
      {
        IAccountForIdentityLookup &forIdentityLookup() {return *this;}
        const IAccountForIdentityLookup &forIdentityLookup() const {return *this;}

        virtual RecursiveLock &getLock() const = 0;

        virtual ContactPtr findContact(const char *peerURI) const = 0;

        virtual IServicePeerContactSessionPtr getPeerContactSession() const = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountAsyncDelegate
      #pragma mark

      interaction IAccountAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account
      #pragma mark

      class Account : public MessageQueueAssociator,
                      public IAccount,
                      public IAccountForCall,
                      public IAccountForContact,
                      public IAccountForConversationThread,
                      public IAccountForIdentityLookup,
                      public ICallTransportDelegate,
                      public stack::IAccountDelegate,
                      public IPeerSubscriptionDelegate,
                      public IServicePeerContactSessionDelegate,
                      public IAccountAsyncDelegate
      {
      public:
        friend interaction IAccountFactory;
        friend interaction IAccount;

        interaction IContactSubscriptionAsyncDelegate;
        typedef boost::shared_ptr<IContactSubscriptionAsyncDelegate> IContactSubscriptionAsyncDelegatePtr;
        typedef boost::weak_ptr<IContactSubscriptionAsyncDelegate> IContactSubscriptionAsyncDelegateWeakPtr;
        typedef zsLib::Proxy<IContactSubscriptionAsyncDelegate> IContactSubscriptionAsyncDelegateProxy;

        class ContactSubscription;
        friend class ContactSubscription;
        typedef boost::shared_ptr<ContactSubscription> ContactSubscriptionPtr;
        typedef boost::weak_ptr<ContactSubscription> ContactSubscriptionWeakPtr;

        class LocationSubscription;
        friend class LocationSubscription;
        typedef boost::shared_ptr<LocationSubscription> LocationSubscriptionPtr;
        typedef boost::weak_ptr<LocationSubscription> LocationSubscriptionWeakPtr;

        typedef IAccount::AccountStates AccountStates;

        typedef String PeerURI;
        typedef std::map<PeerURI, ContactSubscriptionPtr> ContactSubscriptionMap;

        typedef String BaseThreadID;
        typedef std::map<BaseThreadID, ConversationThreadPtr> ConversationThreadMap;

        typedef std::map<PeerURI, ContactPtr> ContactMap;

        typedef PUID ServiceIdentitySessionID;
        typedef std::map<ServiceIdentitySessionID, IdentityPtr> IdentityMap;

      protected:
        Account(
                IMessageQueuePtr queue,
                IAccountDelegatePtr delegate,
                IConversationThreadDelegatePtr conversationThreadDelegate,
                ICallDelegatePtr callDelegate
                );

        void init();

      public:
        ~Account();

        static AccountPtr convert(IAccountPtr account);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccount
        #pragma mark

        static String toDebugString(IAccountPtr account, bool includeCommaPrefix = true);

        static AccountPtr login(
                                IAccountDelegatePtr delegate,
                                IConversationThreadDelegatePtr conversationThreadDelegate,
                                ICallDelegatePtr callDelegate,
                                const char *peerContactServiceDomain,
                                IIdentityPtr identity
                                );
        static AccountPtr relogin(
                                  IAccountDelegatePtr delegate,
                                  IConversationThreadDelegatePtr conversationThreadDelegate,
                                  ICallDelegatePtr callDelegate,
                                  ElementPtr peerFilePrivateEl,
                                  const char *peerFilePrivateSecret
                                  );

        virtual PUID getID() const {return mID;}

        virtual AccountStates getState(
                                       WORD *outErrorCode,
                                       String *outErrorReason
                                       ) const;

        virtual String getUserID() const;
        virtual String getLocationID() const;

        virtual void shutdown();

        virtual ElementPtr savePeerFilePrivate() const;
        virtual SecureByteBlockPtr getPeerFilePrivateSecret() const;

        virtual IdentityListPtr getAssociatedIdentities() const;
        virtual void associateIdentities(
                                         const IdentityList &identitiesToAssociate,
                                         const IdentityList &identitiesToRemove
                                         );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForCall
        #pragma mark

        virtual CallTransportPtr getCallTransport() const;
        virtual ICallDelegatePtr getCallDelegate() const;

        // (duplicate) virtual ContactPtr getSelfContact() const;
        // (duplicate) virtual ILocationPtr getSelfLocation() const;

        // (duplicate) virtual stack::IAccountPtr getStackAccount() const;

        // (duplicate) virtual IPeerFilesPtr getPeerFiles() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForContact
        #pragma mark

        // (duplicate) virtual RecursiveLock &getLock() const;

        // (duplicate) virtual ContactPtr getSelfContact() const;
        // (duplicate) virtual stack::IAccountPtr getStackAccount() const;

        virtual ContactPtr findContact(const char *peerURI) const;

        virtual void notifyAboutContact(ContactPtr contact);

        virtual void hintAboutContactLocation(
                                              ContactPtr contact,
                                              const char *locationID
                                              );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForConversationThread
        #pragma mark

        virtual RecursiveLock &getLock() const {return mLock;}

        virtual ContactPtr getSelfContact() const;
        virtual ILocationPtr getSelfLocation() const;

        // (duplicate) virtual ContactPtr findContact(const char *peerURI) const;

        virtual stack::IAccountPtr getStackAccount() const;
        virtual IPublicationRepositoryPtr getRepository() const;

        virtual IPeerFilesPtr getPeerFiles() const;

        virtual IConversationThreadDelegatePtr getConversationThreadDelegate() const;
        virtual void notifyConversationThreadCreated(ConversationThreadPtr thread);

        virtual ConversationThreadPtr getConversationThreadByID(const char *threadID) const;
        virtual void getConversationThreads(ConversationThreadList &outConversationThreads) const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountForIdentityLookup
        #pragma mark

        // (duplicate) virtual RecursiveLock &getLock() const;

        // (duplicate) virtual ContactPtr findContact(const char *peerURI) const;

        virtual IServicePeerContactSessionPtr getPeerContactSession() const;

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => ICallTransportDelegate
        #pragma mark

        virtual void onCallTransportStateChanged(
                                                 ICallTransportPtr transport,
                                                 CallTransportStates state
                                                 );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => stack::IAccountDelegate
        #pragma mark

        virtual void onAccountStateChanged(
                                           stack::IAccountPtr account,
                                           stack::IAccount::AccountStates state
                                           );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IPeerSubscriptionDelegate
        #pragma mark

        virtual void onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription);

        virtual void onPeerSubscriptionFindStateChanged(
                                                        IPeerSubscriptionPtr subscription,
                                                        IPeerPtr peer,
                                                        PeerFindStates state
                                                        );

        virtual void onPeerSubscriptionLocationConnectionStateChanged(
                                                                      IPeerSubscriptionPtr subscription,
                                                                      ILocationPtr location,
                                                                      LocationConnectionStates state
                                                                      );

        virtual void onPeerSubscriptionMessageIncoming(
                                                       IPeerSubscriptionPtr subscription,
                                                       IMessageIncomingPtr message
                                                       );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IServicePeerContactSessionDelegate
        #pragma mark

        virtual void onServicePeerContactSessionStateChanged(
                                                             IServicePeerContactSessionPtr session,
                                                             SessionStates state
                                                             );
        virtual void onServicePeerContactSessionAssociatedIdentitiesChanged(IServicePeerContactSessionPtr session);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => IAccountAsyncDelegate
        #pragma mark

        virtual void onStep();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => friend Account::ContactSubscription
        #pragma mark

        void notifyContactSubscriptionShutdown(const String &contactID);

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => friend Account::LocationSubscription
        #pragma mark

        ConversationThreadPtr notifyPublicationUpdated(
                                                       ILocationPtr peerLocation,
                                                       IPublicationMetaDataPtr metaData,
                                                       const SplitMap &split
                                                       );

        void notifyPublicationGone(
                                   ILocationPtr peerLocation,
                                   IPublicationMetaDataPtr metaData,
                                   const SplitMap &split
                                   );

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => (internal)
        #pragma mark

        bool isPending() const      {return AccountState_Pending == mCurrentState;}
        bool isReady() const        {return AccountState_Ready == mCurrentState;}
        bool isShuttingDown() const {return AccountState_ShuttingDown == mCurrentState;}
        bool isShutdown() const     {return AccountState_Shutdown == mCurrentState;}

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        void cancel();

        void step();
        bool stepStackAccount();
        bool stepPeerContactSession();
        bool stepSelfContact();
        bool stepCallTransportSetup();
        bool stepSubscribersPermissionDocument();
        bool stepPeerSubscription();
        bool stepCallTransportFinalize();

        void setState(core::IAccount::AccountStates newState);
        void setError(
                      WORD errorCode,
                      const char *reason = NULL
                      );

      public:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::IContactSubscriptionAsyncDelegate
        #pragma mark

        interaction IContactSubscriptionAsyncDelegate
        {
          virtual void onStep() = 0;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::ContactSubscription
        #pragma mark

        class ContactSubscription : public MessageQueueAssociator,
                                    public IContactSubscriptionAsyncDelegate,
                                    public IPeerSubscriptionDelegate,
                                    public ITimerDelegate
        {
        public:
          enum ContactSubscriptionStates
          {
            ContactSubscriptionState_Pending,
            ContactSubscriptionState_Ready,
            ContactSubscriptionState_ShuttingDown,
            ContactSubscriptionState_Shutdown,
          };

          static const char *toString(ContactSubscriptionStates state);

          friend class Account;
          friend class Account::LocationSubscription;

          typedef String LocationID;
          typedef std::map<LocationID, LocationSubscriptionPtr> LocationSubscriptionMap;

        protected:
          ContactSubscription(
                              AccountPtr outer,
                              ContactPtr contact
                              );

          void init(ILocationPtr peerLocation);

        public:
          ~ContactSubscription();

          static String toDebugString(ContactSubscriptionPtr contactSubscription, bool includeCommaPrefix = true);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ContactSubscription => friend Account
          #pragma mark

          static ContactSubscriptionPtr create(
                                               AccountPtr outer,
                                               ContactPtr contact,
                                               ILocationPtr peerLocation = ILocationPtr()
                                               );

          // (duplicate) bool isShuttingDown() const;
          // (duplicate) bool isShutdown() const;

          void notifyAboutLocationState(
                                        ILocationPtr location,
                                        ILocation::LocationConnectionStates state
                                        );

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ContactSubscription => IContactSubscriptionAsyncDelegate
          #pragma mark

          virtual void onStep();

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ContactSubscription => IPeerSubscriptionDelegate
          #pragma mark

          virtual void onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription);

          virtual void onPeerSubscriptionFindStateChanged(
                                                          IPeerSubscriptionPtr subscription,
                                                          IPeerPtr peer,
                                                          PeerFindStates state
                                                          );

          virtual void onPeerSubscriptionLocationConnectionStateChanged(
                                                                        IPeerSubscriptionPtr subscription,
                                                                        ILocationPtr location,
                                                                        LocationConnectionStates state
                                                                        );

          virtual void onPeerSubscriptionMessageIncoming(
                                                         IPeerSubscriptionPtr subscription,
                                                         IMessageIncomingPtr message
                                                         );

          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ContactSubscription => ITimerDelegate
          #pragma mark

          virtual void onTimer(TimerPtr timer);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ContactSubscription => friend LocationSubscription
          #pragma mark

          // (duplicate) RecursiveLock &getLock() const;
          AccountPtr getOuter() const;

          ContactPtr getContact() const {return mContact;}

          void notifyLocationShutdown(const String &locationID);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ContactSubscription => (internal)
          #pragma mark

          bool isPending() const {return ContactSubscriptionState_Pending == mCurrentState;}
          bool isReady() const {return ContactSubscriptionState_Ready == mCurrentState;}
        protected:
          bool isShuttingDown() const {return ContactSubscriptionState_ShuttingDown == mCurrentState;}
          bool isShutdown() const {return ContactSubscriptionState_Shutdown == mCurrentState;}

        private:
          virtual PUID getID() const {return mID;}

        protected:
          RecursiveLock &getLock() const;

        private:
          String log(const char *message) const;
          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          void cancel();
          void step();
          void setState(ContactSubscriptionStates state);

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::ContactSubscription => (data)
          #pragma mark

          mutable RecursiveLock mBogusLock;
          PUID mID;
          ContactSubscriptionWeakPtr mThisWeak;
          ContactSubscriptionPtr mGracefulShutdownReference;

          AccountWeakPtr mOuter;

          ContactSubscriptionStates mCurrentState;

          ContactPtr mContact;
          IPeerSubscriptionPtr mPeerSubscription;
          TimerPtr mPeerSubscriptionAutoCloseTimer;

          LocationSubscriptionMap mLocations;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::LocationSubscription
        #pragma mark

        class LocationSubscription : public MessageQueueAssociator,
                                     public IPublicationSubscriptionDelegate
        {
        public:
          enum LocationSubscriptionStates
          {
            LocationSubscriptionState_Pending,
            LocationSubscriptionState_Ready,
            LocationSubscriptionState_ShuttingDown,
            LocationSubscriptionState_Shutdown,
          };

          static const char *toString(LocationSubscriptionStates state);

          friend class Account::ContactSubscription;

          typedef String ThreadID;
          typedef std::map<ThreadID, ConversationThreadPtr> ConversationThreadMap;

        protected:
          LocationSubscription(
                               ContactSubscriptionPtr outer,
                               ILocationPtr peerLocation
                               );

          void init();

        public:
          ~LocationSubscription();

          static String toDebugString(LocationSubscriptionPtr subscription, bool includeCommaPrefix = true);

        protected:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::LocationSubscription => friend ContactSubscription
          #pragma mark

          static LocationSubscriptionPtr create(
                                                ContactSubscriptionPtr outer,
                                                ILocationPtr peerLocation
                                                );

          // (duplicate) bool isShuttingDown() const;
          // (duplicate) bool isShutdown() const;

          // (duplicate) void cancel();

        public:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::LocationSubscription => IPublicationSubscriptionDelegate
          #pragma mark

          virtual void onPublicationSubscriptionStateChanged(
                                                             IPublicationSubscriptionPtr subcription,
                                                             PublicationSubscriptionStates state
                                                             );

          virtual void onPublicationSubscriptionPublicationUpdated(
                                                                   IPublicationSubscriptionPtr subscription,
                                                                   IPublicationMetaDataPtr metaData
                                                                   );

          virtual void onPublicationSubscriptionPublicationGone(
                                                                IPublicationSubscriptionPtr subscription,
                                                                IPublicationMetaDataPtr metaData
                                                                );

        private:
          //-------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::LocationSubscription => (internal)
          #pragma mark

          bool isPending() const {return LocationSubscriptionState_Pending == mCurrentState;}
          bool isReady() const {return LocationSubscriptionState_Ready == mCurrentState;}
        protected:
          bool isShuttingDown() const {return LocationSubscriptionState_ShuttingDown == mCurrentState;}
          bool isShutdown() const {return LocationSubscriptionState_Shutdown == mCurrentState;}

        private:
          RecursiveLock &getLock() const;
          virtual PUID getID() const {return mID;}
          String log(const char *message) const;
          virtual String getDebugValueString(bool includeCommaPrefix = true) const;

          String getPeerURI() const;
          String getLocationID() const;

        protected:
          void cancel();

        private:
          void step();
          void setState(LocationSubscriptionStates state);

        private:
          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark Account::LocationSubscription => (data)
          #pragma mark

          mutable RecursiveLock mBogusLock;
          PUID mID;
          LocationSubscriptionWeakPtr mThisWeak;
          LocationSubscriptionPtr mGracefulShutdownReference;

          ContactSubscriptionWeakPtr mOuter;

          LocationSubscriptionStates mCurrentState;

          ILocationPtr mPeerLocation;

          IPublicationSubscriptionPtr mPublicationSubscription;

          ConversationThreadMap mConversationThreads;             // all the conversations which have been attached to this location
        };

      private:
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account => (data)
        #pragma mark

        mutable RecursiveLock mLock;
        PUID mID;
        AccountWeakPtr mThisWeak;
        AccountPtr mGracefulShutdownReference;

        IAccount::AccountStates mCurrentState;
        WORD mLastErrorCode;
        String mLastErrorReason;

        IAccountDelegatePtr mDelegate;

        IConversationThreadDelegatePtr mConversationThreadDelegate;
        ICallDelegatePtr mCallDelegate;

        stack::IAccountPtr mStackAccount;
        IServicePeerContactSessionPtr mPeerContactSession;

        mutable IdentityMap mIdentities;

        IPeerSubscriptionPtr mPeerSubscription;

        ContactPtr mSelfContact;

        ContactMap mContacts;
        ContactSubscriptionMap mContactSubscriptions;

        ConversationThreadMap mConversationThreads;

        CallTransportPtr mCallTransport;

        IPublicationPtr mSubscribersPermissionDocument;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IAccountFactory
      #pragma mark

      interaction IAccountFactory
      {
        static IAccountFactory &singleton();

        virtual AccountPtr login(
                                 IAccountDelegatePtr delegate,
                                 IConversationThreadDelegatePtr conversationThreadDelegate,
                                 ICallDelegatePtr callDelegate,
                                 const char *peerContactServiceDomain,
                                 IIdentityPtr identity
                                 );
        virtual AccountPtr relogin(
                                   IAccountDelegatePtr delegate,
                                   IConversationThreadDelegatePtr conversationThreadDelegate,
                                   ICallDelegatePtr callDelegate,
                                   ElementPtr peerFilePrivateEl,
                                   const char *peerFilePrivateSecret
                                   );
      };

    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::core::internal::IAccountAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::core::internal::Account::IContactSubscriptionAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
