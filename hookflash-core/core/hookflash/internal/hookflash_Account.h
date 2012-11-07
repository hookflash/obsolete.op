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
#include <hookflash/IAccount.h>
#include <hookflash/internal/hookflash_CallTransport.h>
#include <hookflash/stack/IBootstrappedNetwork.h>
#include <hookflash/stack/IAccount.h>
#include <hookflash/stack/IMessageRequester.h>
#include <hookflash/stack/IPeerSubscription.h>
#include <hookflash/stack/IPublicationRepository.h>
#include <hookflash/stack/IConnectionSubscription.h>
#include <hookflash/stack/IHelper.h>

#include <zsLib/MessageQueueAssociator.h>
#include <zsLib/String.h>
#include <zsLib/Timer.h>
#include <zsLib/XML.h>

namespace hookflash
{
  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountForConversationThread
    #pragma mark

    interaction IAccountForConversationThread
    {
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
      typedef zsLib::String String;
      typedef stack::IPublicationRepositoryPtr IPublicationRepositoryPtr;
      typedef stack::IPeerFilesPtr IPeerFilesPtr;

      static IAccountForConversationThreadPtr convert(IAccountPtr account);

      virtual RecursiveLock &getLock() const = 0;

      virtual IMessageQueuePtr getAssociatedMessageQueue() const = 0;
      virtual IConversationThreadDelegatePtr getConversationThreadDelegate() const = 0;

      virtual IContactPtr getSelfContact() const = 0;

      virtual String getContactID() const = 0;
      virtual String getLocationID() const = 0;

      virtual IContactForConversationThreadPtr findContact(const char *contactID) const = 0;

      virtual stack::IAccountPtr getStackAccount() const = 0;
      virtual IPublicationRepositoryPtr getRepository() const = 0;

      virtual IPeerFilesPtr getPeerFiles() const = 0;
      virtual String getPassword() const = 0;

      virtual void notifyConversationThreadCreated(IConversationThreadForAccountPtr thread) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountForCall
    #pragma mark

    interaction IAccountForCall
    {
      typedef zsLib::String String;
      typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
      typedef stack::IPeerFilesPtr IPeerFilesPtr;

      static IAccountForCallPtr convert(IAccountForConversationThreadPtr account);

      virtual IMessageQueuePtr getAssociatedMessageQueue() const = 0;

      virtual ICallTransportForAccountPtr getCallTransport() const = 0;

      virtual IContactPtr getSelfContact() const = 0;

      virtual ICallDelegatePtr getCallDelegate() const = 0;

      virtual String getContactID() const = 0;
      virtual String getLocationID() const = 0;

      virtual stack::IAccountPtr getStackAccount() const = 0;

      virtual IPeerFilesPtr getPeerFiles() const = 0;
      virtual String getPassword() const = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountForContact
    #pragma mark

    interaction IAccountForContact
    {
      typedef zsLib::String String;

      static IAccountForContactPtr convert(IAccountForConversationThreadPtr account);

      virtual IAccountPtr convertIAccount() const = 0;

      virtual String getPassword() const = 0;

      virtual void notifyAboutContact(IContactPtr contact) = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountForProvisioningAccount
    #pragma mark

    interaction IAccountForProvisioningAccount
    {
      typedef zsLib::String String;
      typedef zsLib::XML::ElementPtr ElementPtr;

      static IAccountForProvisioningAccountPtr convert(IAccountPtr account);

      virtual IAccountPtr convertIAccount() const = 0;

      static IAccountForProvisioningAccountPtr loadExisting(
                                                            IStackPtr stack,
                                                            IAccountDelegatePtr openpeerDelegate,
                                                            ElementPtr privatePeerFile,
                                                            const char *password,
                                                            const char *networkURI,
                                                            const char *turnServer,
                                                            const char *turnServerUsername,
                                                            const char *turnServerPassword,
                                                            const char *stunServer
                                                            );

      static IAccountForProvisioningAccountPtr generate(
                                                        IStackPtr stack,
                                                        IAccountDelegatePtr openpeerDelegate,
                                                        ElementPtr publicProfileXML,
                                                        ElementPtr privateProfileXML,
                                                        const char *password,
                                                        const char *networkURI,
                                                        const char *turnServer,
                                                        const char *turnServerUsername,
                                                        const char *turnServerPassword,
                                                        const char *stunServer
                                                        );

      virtual void shutdown() = 0;

      virtual hookflash::IAccount::AccountStates getState() const = 0;

      virtual IAccountSubscriptionPtr subscribe(IAccountDelegatePtr delegate) = 0;

      virtual String getContactID() const = 0;
      virtual String getLocationID() const = 0;
      virtual ElementPtr savePrivatePeer() = 0;
      virtual ElementPtr savePublicPeer() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountAsyncDelegate
    #pragma mark

    interaction IAccountAsyncDelegate
    {
      virtual void onStep() = 0;
    };

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account
    #pragma mark

    class Account : public zsLib::MessageQueueAssociator,
                    public IAccount,
                    public IAccountAsyncDelegate,
                    public IAccountForConversationThread,
                    public IAccountForCall,
                    public IAccountForContact,
                    public IAccountForProvisioningAccount,
                    public ICallTransportDelegate,
                    public stack::IBootstrappedNetworkDelegate,
                    public stack::IBootstrappedNetworkFetchSignedSaltQueryDelegate,
                    public stack::IAccountDelegate,
                    public stack::IConnectionSubscriptionDelegate
    {
    public:
      friend interaction IAccountForProvisioningAccount;

      class AccountSubscription;
      friend class AccountSubscription;
      typedef boost::shared_ptr<AccountSubscription> AccountSubscriptionPtr;
      typedef boost::weak_ptr<AccountSubscription> AccountSubscriptionWeakPtr;

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

      typedef zsLib::PUID PUID;
      typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::String String;
      typedef zsLib::IMessageQueuePtr IMessageQueuePtr;
      typedef zsLib::XML::ElementPtr ElementPtr;

      typedef hookflash::internal::AccountPtr AccountPtr;
      typedef hookflash::IAccountPtr IAccountPtr;
      typedef hookflash::IAccountDelegatePtr IAccountDelegatePtr;

      typedef stack::IBootstrappedNetwork IBootstrappedNetwork;
      typedef stack::IBootstrappedNetworkPtr IBootstrappedNetworkPtr;
      typedef stack::IBootstrappedNetworkFetchSignedSaltQueryPtr IBootstrappedNetworkFetchSignedSaltQueryPtr;
      typedef stack::IConnectionSubscriptionPtr IConnectionSubscriptionPtr;
      typedef stack::IConnectionSubscriptionMessagePtr IConnectionSubscriptionMessagePtr;
      typedef stack::IPeerFiles IPeerFiles;
      typedef stack::IPeerFilesPtr IPeerFilesPtr;
      typedef stack::IPeerFilePublicPtr IPeerFilePublicPtr;
      typedef stack::IPeerContactProfilePtr IPeerContactProfilePtr;
      typedef stack::IPeerLocationPtr IPeerLocationPtr;
      typedef stack::IPeerSubscription IPeerSubscription;
      typedef stack::IPublicationPtr IPublicationPtr;
      typedef stack::IPublicationPublisherPtr IPublicationPublisherPtr;
      typedef stack::IPublicationPublisherDelegateProxy IPublicationPublisherDelegateProxy;
      typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
      typedef stack::IHelper::SplitMap SplitMap;

    protected:
      Account(
              IMessageQueuePtr queue,
              StackPtr stack
              );

      void init(
                const char *networkURI,
                const char *turnServer,
                const char *turnServerUsername,
                const char *turnServerPassword,
                const char *stunServer
                );

    public:
      ~Account();

      static AccountPtr convert(IAccountPtr account);

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccount
      #pragma mark

      virtual void shutdown();

      virtual hookflash::IAccount::AccountStates getState() const;
      virtual AccountErrors getLastError() const;

      virtual IAccountSubscriptionPtr subscribe(IAccountDelegatePtr delegate);

      virtual IContactPtr getSelfContact() const;
      virtual String getLocationID() const;

      virtual ElementPtr savePrivatePeer();
      virtual ElementPtr savePublicPeer();

      virtual void notifyAboutContact(IContactPtr contact);

      virtual void hintAboutContactLocation(
                                            IContactPtr contact,
                                            const char *locationID
                                            );

      virtual IConversationThreadPtr getConversationThreadByID(const char *threadID) const;
      virtual void getConversationThreads(ConversationThreadList &outConversationThreads) const;

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForConversationThread
      #pragma mark

      virtual RecursiveLock &getLock() const {return mLock;}

      virtual IMessageQueuePtr getAssociatedMessageQueue() const {return MessageQueueAssociator::getAssociatedMessageQueue(); }
      virtual IConversationThreadDelegatePtr getConversationThreadDelegate() const;

      // (duplicate) virtual IContactPtr getSelfContact() const;
      virtual String getContactID() const;
      // (duplicate) virtual String getLocationID() const;

      virtual IContactForConversationThreadPtr findContact(const char *contactID) const;

      virtual stack::IAccountPtr getStackAccount() const;
      virtual IPublicationRepositoryPtr getRepository() const;

      // (duplicate) virtual stack::IAccountPtr getStackAccount() const;

      virtual IPeerFilesPtr getPeerFiles() const;
      // (duplicate) virtual String getPassword() const;

      virtual void notifyConversationThreadCreated(IConversationThreadForAccountPtr thread);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForCall
      #pragma mark

      // (duplicate) virtual IMessageQueuePtr getAssociatedMessageQueue() const;
      virtual ICallTransportForAccountPtr getCallTransport() const;

      // (duplicate) virtual IContactPtr getSelfContact() const;

      virtual ICallDelegatePtr getCallDelegate() const;

      // (duplicate) virtual String getContactID() const;
      // (duplicate) virtual String getLocationID() const;

      // (duplicate) virtual IPeerFilesPtr getPeerFiles() const;
      // (duplicate) virtual String getPassword() const;

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForContact
      #pragma mark

      virtual IAccountPtr convertIAccount() const {return mThisWeak.lock(); }

      virtual String getPassword() const;

      // (duplicate) virtual void notifyAboutContact(IContactPtr contact);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountForProvisioningAccount
      #pragma mark

      // (duplicate) virtual IAccountPtr convertIAccount() const;

      static AccountPtr loadExisting(
                                     IStackPtr stack,
                                     IAccountDelegatePtr openpeerDelegate,
                                     ElementPtr privatePeerFile,
                                     const char *password,
                                     const char *networkURI,
                                     const char *turnServer,
                                     const char *turnServerUsername,
                                     const char *turnServerPassword,
                                     const char *stunServer
                                     );

      static AccountPtr generate(
                                 IStackPtr stack,
                                 IAccountDelegatePtr openpeerDelegate,
                                 ElementPtr publicProfileXML,
                                 ElementPtr privateProfileXML,
                                 const char *password,
                                 const char *networkURI,
                                 const char *turnServer,
                                 const char *turnServerUsername,
                                 const char *turnServerPassword,
                                 const char *stunServer
                                 );

      // (duplicate) virtual void shutdown();

      // (duplicate) virtual hookflash::IAccount::AccountStates getState() const;

      // (duplicate) virtual IAccountSubscriptionPtr subscribe(IAccountDelegatePtr delegate);

      // (duplicate) virtual String getContactID() const;
      // (duplicate) virtual String getLocationID() const;
      // (duplicate) virtual ElementPtr savePrivatePeer();
      // (duplicate) virtual String savePublicPeer();

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => ICallTransportDelegate
      #pragma mark

      virtual void onCallTransportStateChanged(
                                               ICallTransportPtr transport,
                                               CallTransportStates state
                                               );

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IBootstrappedNetworkDelegate
      #pragma mark

      virtual void onBootstrappedNetworkStateChanged(
                                                     IBootstrappedNetworkPtr bootstrapper,
                                                     BootstrappedNetworkStates state
                                                     );

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IBootstrappedNetworkFetchSignedSaltQueryDelegate
      #pragma mark

      virtual void onBootstrappedNetworkFetchSignedSaltComplete(
                                                                IBootstrappedNetworkPtr bootstrapper,
                                                                IBootstrappedNetworkFetchSignedSaltQueryPtr query
                                                                );

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => stack::IAccountDelegate
      #pragma mark

      virtual void onAccountStateChanged(
                                         stack::IAccountPtr account,
                                         stack::IAccount::AccountStates state
                                         );

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IConnectionSubscriptionDelegate
      #pragma mark

      virtual void onConnectionSubscriptionShutdown(IConnectionSubscriptionPtr subscription);

      virtual void onConnectionSubscriptionFinderConnectionStateChanged(
                                                                        IConnectionSubscriptionPtr subscription,
                                                                        ConnectionStates state
                                                                        );

      virtual void onConnectionSubscriptionPeerLocationConnectionStateChanged(
                                                                              IConnectionSubscriptionPtr subscription,
                                                                              IPeerLocationPtr location,
                                                                              ConnectionStates state
                                                                              );

      virtual void onConnectionSubscriptionIncomingMessage(
                                                           IConnectionSubscriptionPtr subscription,
                                                           IConnectionSubscriptionMessagePtr message
                                                           );

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => IAccountAsyncDelegate
      #pragma mark

      virtual void onStep();

    protected:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => friend Account::AccountSubscription
      #pragma mark

      void notifySubscriptionCancelled(PUID id);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => friend Account::ContactSubscription
      #pragma mark

      void notifyContactSubscriptionShutdown(const String &contactID);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => friend Account::LocationSubscription
      #pragma mark

      IConversationThreadForAccountPtr notifyPublicationUpdated(
                                                                IPeerLocationPtr peerLocation,
                                                                IPublicationMetaDataPtr metaData,
                                                                const SplitMap &split
                                                                );

      void notifyPublicationGone(
                                 IPeerLocationPtr peerLocation,
                                 IPublicationMetaDataPtr metaData,
                                 const SplitMap &split
                                 );

    private:
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => (internal)
      #pragma mark

      bool isPending() {return AccountState_Pending == mCurrentState;}
      bool isReady() {return AccountState_Ready == mCurrentState;}
      bool isShuttingDown() {return AccountState_ShuttingDown == mCurrentState;}
      bool isShutdown() {return AccountState_Shutdown == mCurrentState;}

      String log(const char *message) const;

      void cancel();
      void step();

      void setState(hookflash::IAccount::AccountStates newState);
      void setLastError(hookflash::IAccount::AccountErrors error);

      StackPtr getOuter();

      void getServers(
                      String &outTURNServer,
                      String &outTURNUsername,
                      String &outTURNPassword,
                      String &outSTUNServer
                      );

    public:
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::AccountSubscription
      #pragma mark

      class AccountSubscription : public IAccountSubscription
      {
      public:
        typedef zsLib::PUID PUID;
        typedef hookflash::internal::AccountPtr AccountPtr;

        friend class Account;

      protected:
        AccountSubscription(AccountPtr outer);

        void init();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::AccountSubscription => friend Account
        #pragma mark

        static AccountSubscriptionPtr create(AccountPtr outer);

        PUID getID() const {return mID;}

      public:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::AccountSubscription => IAccountSubscription
        #pragma mark

        virtual IAccountPtr getAccount() const;
        virtual void cancel();

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::AccountSubscription => (internal)
        #pragma mark

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::AccountSubscription => (data)
        #pragma mark

        AccountSubscriptionWeakPtr mThisWeak;
        PUID mID;
        AccountPtr mOuter;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::IContactSubscriptionAsyncDelegate
      #pragma mark

      interaction IContactSubscriptionAsyncDelegate
      {
        virtual void onStep() = 0;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::ContactSubscription
      #pragma mark

      class ContactSubscription : public zsLib::MessageQueueAssociator,
                                  public IContactSubscriptionAsyncDelegate,
                                  public stack::IPeerSubscriptionDelegate,
                                  public zsLib::ITimerDelegate
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

        typedef zsLib::TimerPtr TimerPtr;
        typedef zsLib::PUID PUID;
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;

        typedef stack::IPeerLocationPtr IPeerLocationPtr;
        typedef stack::IPeerSubscriptionPtr IPeerSubscriptionPtr;
        typedef stack::IPeerSubscriptionMessagePtr IPeerSubscriptionMessagePtr;
        typedef stack::IConnectionSubscription IConnectionSubscription;

        friend class Account;
        friend class Account::LocationSubscription;

      protected:
        ContactSubscription(
                            AccountPtr outer,
                            IContactForAccountPtr contact
                            );

        void init(IPeerLocationPtr peerLocation);

      public:
        ~ContactSubscription();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::ContactSubscription => friend Account
        #pragma mark

        static ContactSubscriptionPtr create(
                                             AccountPtr outer,
                                             IContactForAccountPtr contact,
                                             IPeerLocationPtr peerLocation = IPeerLocationPtr()
                                             );

        // (duplicate) bool isShuttingDown() const;
        // (duplicate) bool isShutdown() const;

        void notifyAboutLocationState(
                                      IPeerLocationPtr location,
                                      IConnectionSubscription::ConnectionStates state
                                      );

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::ContactSubscription => IContactSubscriptionAsyncDelegate
        #pragma mark

        virtual void onStep();

        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::ContactSubscription => IPeerSubscriptionDelegate
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
        #pragma mark Account::ContactSubscription => ITimerDelegate
        #pragma mark

        virtual void onTimer(TimerPtr timer);

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::ContactSubscription => friend LocationSubscription
        #pragma mark

        // (duplicate) RecursiveLock &getLock() const;
        AccountPtr getOuter() const;

        IContactForAccountPtr getContact() const {return mContact;}

        void notifyLocationShutdown(const String &locationID);

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::ContactSubscription => (internal)
        #pragma mark

        bool isPending() const {return ContactSubscriptionState_Pending == mCurrentState;}
        bool isReady() const {return ContactSubscriptionState_Ready == mCurrentState;}
      protected:
        bool isShuttingDown() const {return ContactSubscriptionState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return ContactSubscriptionState_Shutdown == mCurrentState;}

      private:
        PUID getID() const {return mID;}

      protected:
        RecursiveLock &getLock() const;

      private:
        String log(const char *message) const;

        void cancel();
        void step();
        void setState(ContactSubscriptionStates state);

      private:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::ContactSubscription => (data)
        #pragma mark

        mutable RecursiveLock mBogusLock;
        PUID mID;
        ContactSubscriptionWeakPtr mThisWeak;
        ContactSubscriptionPtr mGracefulShutdownReference;

        AccountWeakPtr mOuter;

        ContactSubscriptionStates mCurrentState;

        IContactForAccountPtr mContact;
        IPeerSubscriptionPtr mPeerSubscription;
        TimerPtr mPeerSubscriptionAutoCloseTimer;

        typedef String LocationID;
        typedef std::map<LocationID, LocationSubscriptionPtr> LocationSubscriptionMap;

        LocationSubscriptionMap mLocations;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account::LocationSubscription
      #pragma mark

    public:
      class LocationSubscription : public zsLib::MessageQueueAssociator,
                                   public stack::IPublicationSubscriptionDelegate
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

        typedef zsLib::PUID PUID;
        typedef zsLib::String String;
        typedef zsLib::RecursiveLock RecursiveLock;
        typedef stack::IPublicationSubscriptionPtr IPublicationSubscriptionPtr;
        typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;

        friend class Account::ContactSubscription;

      protected:
        LocationSubscription(
                             ContactSubscriptionPtr outer,
                             IPeerLocationPtr peerLocation
                             );

        void init();

      public:
        ~LocationSubscription();

      protected:
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark Account::LocationSubscription => friend ContactSubscription
        #pragma mark

        static LocationSubscriptionPtr create(
                                              ContactSubscriptionPtr outer,
                                              IPeerLocationPtr peerLocation
                                              );

        // (duplicate) bool isShuttingDown() const;
        // (duplicate) bool isShutdown() const;

        // (duplicate) void cancel();

      public:
        //---------------------------------------------------------------------
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
        //---------------------------------------------------------------------
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
        PUID getID() const {return mID;}
        String log(const char *message) const;

        String getContactID() const;
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

        IPeerLocationPtr mPeerLocation;

        IPublicationSubscriptionPtr mPublicationSubscription;

        typedef String ThreadID;
        typedef std::map<ThreadID, IConversationThreadForAccountPtr> ConversationThreadMap;
        ConversationThreadMap mConversationThreads;             // all the conversations which have been attached to this location
      };

    private:
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Account => (data)
      #pragma mark

      mutable RecursiveLock mLock;
      PUID mID;
      AccountWeakPtr mThisWeak;
      AccountPtr mGracefulShutdownReference;

      StackWeakPtr mOuter;
      IAccount::AccountStates mCurrentState;
      IAccount::AccountErrors mLastError;

      String mTURNServer;
      String mTURNUsername;
      String mTURNPassword;
      String mSTUNServer;

      typedef std::map<PUID, IAccountDelegatePtr> DelegateMap;
      DelegateMap mDelegates;

      IConversationThreadDelegatePtr mConversationThreadDelegate;
      ICallDelegatePtr mCallDelegate;

      IPeerFilesPtr mPeerFiles;
      ContactPtr mContactSelf;
      bool mGeneratedPeerFile;

      String mNetworkURI;
      String mPassword;

      ElementPtr mGeneratedPublicProfileXML;
      ElementPtr mGeneratedPrivateProfileXML;

      IBootstrappedNetworkPtr mBootstrapper;
      IBootstrappedNetworkFetchSignedSaltQueryPtr mFetchSignedSalt;
      stack::IAccountPtr mStackAccount;

      typedef std::map<String, IContactForAccountPtr> ContactMap;
      ContactMap mContacts;

      IConnectionSubscriptionPtr mConnectionSubscription;

      IPublicationPtr mSubscribersPermissionDocument;

      typedef String ContactID;
      typedef std::map<ContactID, ContactSubscriptionPtr> ContactSubscriptionMap;
      ContactSubscriptionMap mContactSubscriptions;

      typedef String BaseThreadID;
      typedef std::map<BaseThreadID, IConversationThreadForAccountPtr> ConversationThreadMap;
      ConversationThreadMap mConversationThreads;

      ICallTransportForAccountPtr mCallTransport;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::internal::IAccountAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()

ZS_DECLARE_PROXY_BEGIN(hookflash::internal::Account::IContactSubscriptionAsyncDelegate)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
