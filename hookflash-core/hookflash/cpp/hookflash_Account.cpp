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

#include <hookflash/internal/hookflash_Account.h>
#include <hookflash/internal/hookflash_Contact.h>
#include <hookflash/internal/hookflash_Client.h>
#include <hookflash/internal/hookflash_Stack.h>
#include <hookflash/internal/hookflash_ConversationThread.h>
#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPeerContactProfile.h>
#include <hookflash/stack/IPeerFilePublic.h>
#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/stack/IPeerSubscription.h>
#include <hookflash/stack/IPeerLocation.h>
#include <hookflash/stack/IPublication.h>
#include <hookflash/stack/IPublicationRepository.h>
#include <hookflash/IContact.h>
#include <hookflash/IConversationThread.h>
#include <hookflash/ICall.h>

#include <zsLib/Log.h>
#include <zsLib/XML.h>
#include <zsLib/Stringize.h>
#include <zsLib/zsHelpers.h>

#define HOOKFLASH_PEER_SUBSCRIPTION_AUTO_CLOSE_TIMEOUT_IN_SECONDS (60*3)

namespace hookflash { ZS_DECLARE_SUBSYSTEM(hookflash) }


namespace hookflash
{
  namespace internal
  {
    using zsLib::Stringize;

    typedef zsLib::CSTR CSTR;
    typedef zsLib::String String;
    typedef zsLib::Seconds Seconds;
    typedef zsLib::Timer Timer;
    typedef zsLib::RecursiveLock RecursiveLock;
    typedef zsLib::XML::Document Document;
    typedef zsLib::XML::DocumentPtr DocumentPtr;
    typedef zsLib::XML::ElementPtr ElementPtr;
    typedef stack::IPublicationRepositoryPtr IPublicationRepositoryPtr;
    typedef stack::IPeerFilesPtr IPeerFilesPtr;
    typedef stack::IConnectionSubscription IConnectionSubscription;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountForConversationThread
    #pragma mark

    IAccountForConversationThreadPtr IAccountForConversationThread::convert(IAccountPtr account)
    {
      return boost::dynamic_pointer_cast<Account>(account);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountForCall
    #pragma mark

    IAccountForCallPtr IAccountForCall::convert(IAccountForConversationThreadPtr account)
    {
      return boost::dynamic_pointer_cast<Account>(account);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountForContact
    #pragma mark

    IAccountForContactPtr IAccountForContact::convert(IAccountForConversationThreadPtr account)
    {
      return boost::dynamic_pointer_cast<Account>(account);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IAccountForProvisioningAccount
    #pragma mark

    IAccountForProvisioningAccountPtr IAccountForProvisioningAccount::convert(IAccountPtr account)
    {
      return boost::dynamic_pointer_cast<Account>(account);
    }

    //-------------------------------------------------------------------------
    IAccountForProvisioningAccountPtr IAccountForProvisioningAccount::loadExisting(
                                                                                   IStackPtr stack,
                                                                                   IAccountDelegatePtr openpeerDelegate,
                                                                                   ElementPtr privatePeerFile,
                                                                                   const char *password,
                                                                                   const char *networkURI,
                                                                                   const char *turnServer,
                                                                                   const char *turnServerUsername,
                                                                                   const char *turnServerPassword,
                                                                                   const char *stunServer
                                                                                   )
    {
      return Account::loadExisting(stack, openpeerDelegate, privatePeerFile, password, networkURI, turnServer, turnServerUsername, turnServerPassword, stunServer);
    }

    //-------------------------------------------------------------------------
    IAccountForProvisioningAccountPtr IAccountForProvisioningAccount::generate(
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
                                                                               )
    {
      return Account::generate(stack, openpeerDelegate, publicProfileXML, privateProfileXML, password, networkURI, turnServer, turnServerUsername, turnServerPassword, stunServer);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account
    #pragma mark

    //-------------------------------------------------------------------------
    Account::Account(
                     IMessageQueuePtr queue,
                     StackPtr stack
                     ) :
      MessageQueueAssociator(queue),
      mID(zsLib::createPUID()),
      mOuter(stack),
      mCurrentState(AccountState_Pending),
      mLastError(AccountError_None),
      mGeneratedPeerFile(false)
    {
      ZS_LOG_BASIC(log("created"))
    }

    //-------------------------------------------------------------------------
    Account::~Account()
    {
      mThisWeak.reset();
      ZS_LOG_BASIC(log("destroyed"))
      cancel();
    }

    //-------------------------------------------------------------------------
    void Account::init(
                       const char *networkURI,
                       const char *turnServer,
                       const char *turnServerUsername,
                       const char *turnServerPassword,
                       const char *stunServer
                       )
    {
      mTURNServer = turnServer;
      mTURNUsername = turnServerUsername;
      mTURNPassword = turnServerPassword;
      mSTUNServer = stunServer;

      AutoRecursiveLock lock(mLock);

      StackPtr stack = mOuter.lock();
      const String &userAgent = stack->getUserAgent();
      mBootstrapper = IBootstrappedNetwork::create(getAssociatedMessageQueue(), mThisWeak.lock(), userAgent, networkURI, turnServer, turnServerUsername, turnServerPassword, stunServer);

      if (mPeerFiles) {
        ZS_LOG_DEBUG(log("has been provided existing peer files") + ", peer files password=" + mPassword)
        mContactSelf = Contact::createSelfContact(mThisWeak.lock(), mPeerFiles);
      }

      ZS_LOG_DETAIL(log("discovery starting") + ", network URI=" + String(networkURI) + ", turn=" + String(turnServer) + ", user=" + String(turnServerUsername) + ", pass=" + String(turnServerPassword) + ", stun=" + String(stunServer))

      step();
    }

    //-------------------------------------------------------------------------
    AccountPtr Account::convert(IAccountPtr account)
    {
      return boost::dynamic_pointer_cast<Account>(account);
    }

    //-------------------------------------------------------------------------
    Account::AccountPtr Account::loadExisting(
                                              IStackPtr istack,
                                              IAccountDelegatePtr openpeerDelegate,
                                              ElementPtr privatePeerFile,
                                              const char *password,
                                              const char *networkURI,
                                              const char *turnServer,
                                              const char *turnServerUsername,
                                              const char *turnServerPassword,
                                              const char *stunServer
                                              )
    {
      typedef hookflash::internal::AccountPtr AccountPtr;

      ZS_THROW_INVALID_USAGE_IF(!privatePeerFile)
      ZS_THROW_INVALID_USAGE_IF(!password)
      ZS_THROW_INVALID_USAGE_IF(!networkURI)
      ZS_THROW_INVALID_USAGE_IF(!turnServer)
      ZS_THROW_INVALID_USAGE_IF(!turnServerUsername)
      ZS_THROW_INVALID_USAGE_IF(!turnServerPassword)
      ZS_THROW_INVALID_USAGE_IF(!stunServer)

      StackPtr stack = Stack::convert(istack);

      if (!privatePeerFile) return AccountPtr();

      IPeerFilesPtr peerFiles = IPeerFiles::loadPeerFilesFromXML(password, privatePeerFile);
      if (!peerFiles) return AccountPtr();

      AccountPtr pThis(new Account(Client::hookflashQueue(), stack));
      pThis->mThisWeak = pThis;
      pThis->mDelegates[0] = IAccountDelegateProxy::createWeak(Client::guiQueue(), openpeerDelegate);
      pThis->mConversationThreadDelegate = IConversationThreadDelegateProxy::createWeak(Client::guiQueue(), stack->mConversationThreadDelegate);
      pThis->mCallDelegate = ICallDelegateProxy::createWeak(Client::guiQueue(), stack->mCallDelegate);
      pThis->mPeerFiles = peerFiles;
      pThis->mNetworkURI = networkURI;
      pThis->mPassword = password;
      pThis->init(networkURI, turnServer, turnServerUsername, turnServerPassword, stunServer);
      return pThis;
    }

    //---------------------------------------------------------------------
    AccountPtr Account::generate(
                                 IStackPtr istack,
                                 hookflash::IAccountDelegatePtr openpeerDelegate,
                                 ElementPtr publicProfileXML,
                                 ElementPtr privateProfileXML,
                                 const char *password,
                                 const char *networkURI,
                                 const char *turnServer,
                                 const char *turnServerUsername,
                                 const char *turnServerPassword,
                                 const char *stunServer
                                 )
    {
      ZS_THROW_INVALID_USAGE_IF(!password)
      ZS_THROW_INVALID_USAGE_IF(!networkURI)
      ZS_THROW_INVALID_USAGE_IF(!turnServer)
      ZS_THROW_INVALID_USAGE_IF(!turnServerUsername)
      ZS_THROW_INVALID_USAGE_IF(!turnServerPassword)
      ZS_THROW_INVALID_USAGE_IF(!stunServer)

      StackPtr stack = Stack::convert(istack);

      AccountPtr pThis(new Account(Client::hookflashQueue(), stack));
      pThis->mThisWeak = pThis;
      pThis->mDelegates[0] = IAccountDelegateProxy::createWeak(Client::guiQueue(), openpeerDelegate);
      pThis->mConversationThreadDelegate = IConversationThreadDelegateProxy::createWeak(Client::guiQueue(), stack->mConversationThreadDelegate);
      pThis->mCallDelegate = ICallDelegateProxy::createWeak(Client::guiQueue(), stack->mCallDelegate);
      pThis->mGeneratedPublicProfileXML = publicProfileXML;
      pThis->mGeneratedPrivateProfileXML = privateProfileXML;
      pThis->mNetworkURI = networkURI;
      pThis->mPassword = password;
      pThis->init(networkURI, turnServer, turnServerUsername, turnServerPassword, stunServer);
      return pThis;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => IAccount
    #pragma mark

    void Account::shutdown()
    {
      AutoRecursiveLock lock(mLock);
      cancel();
    }

    //-------------------------------------------------------------------------
    hookflash::IAccount::AccountStates Account::getState() const
    {
      ZS_LOG_DEBUG(log("get account state called") + ", state=" + toString(mCurrentState))
      AutoRecursiveLock lock(mLock);
      return mCurrentState;
    }

    //-------------------------------------------------------------------------
    hookflash::IAccount::AccountErrors Account::getLastError() const
    {
      ZS_LOG_DEBUG(log("get last error called") + ", error=" + toString(mLastError))
      AutoRecursiveLock lock(mLock);
      return mLastError;
    }

    //-------------------------------------------------------------------------
    IAccountSubscriptionPtr Account::subscribe(IAccountDelegatePtr inDelegate)
    {
      AutoRecursiveLock lock(mLock);

      IAccountDelegatePtr delegate = IAccountDelegateProxy::createWeak(Client::guiQueue(), inDelegate);

      AccountSubscriptionPtr subscription = AccountSubscription::create(mThisWeak.lock());

      mDelegates[subscription->getID()] = delegate;

      return subscription;
    }

    //-------------------------------------------------------------------------
    ElementPtr Account::savePublicPeer()
    {
      if (!mPeerFiles) return ElementPtr();
      return mPeerFiles->getPublic()->saveToXML();
    }

    //-------------------------------------------------------------------------
    ElementPtr Account::savePrivatePeer()
    {
      if (!mPeerFiles) return ElementPtr();
      return mPeerFiles->savePrivateAndPublicToXML();
    }

    //-------------------------------------------------------------------------
    IContactPtr Account::getSelfContact() const
    {
      AutoRecursiveLock lock(mLock);
      return mContactSelf;
    }

    //-------------------------------------------------------------------------
    String Account::getLocationID() const
    {
      AutoRecursiveLock lock(mLock);
      if (!mStackAccount) return String();

      return mStackAccount->getLocationID();
    }

    //-------------------------------------------------------------------------
    void Account::notifyAboutContact(IContactPtr inContact)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!inContact)

      ContactPtr contact = Contact::convert(inContact);

      String contactID = contact->getContactID();
      ContactMap::iterator found = mContacts.find(contactID);
      if (found != mContacts.end()) return;

      mContacts[contactID] = IContactForAccount::convert(contact);
    }

    //-------------------------------------------------------------------------
    void Account::hintAboutContactLocation(
                                           IContactPtr contact,
                                           const char *locationID
                                           )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!contact)

      AutoRecursiveLock lock(mLock);

      ContactSubscriptionMap::iterator found = mContactSubscriptions.find(contact->getContactID());
      if (found != mContactSubscriptions.end()) {
        ContactSubscriptionPtr contactSubscription = (*found).second;

        if ((contactSubscription->isShuttingDown()) || (contactSubscription->isShutdown())) {
          // the contact subscription is dying, need to create a new one to replace the existing
          mContactSubscriptions.erase(found);
          found = mContactSubscriptions.end();
        }
      }

      if (found == mContactSubscriptions.end()) {
        // In this scenario we need to subscribe to this peer location since we
        // do not have a connection established to this peer as of yet.
        ContactSubscriptionPtr contactSubscription = ContactSubscription::create(mThisWeak.lock(), IContactForAccount::convert(contact));
        mContactSubscriptions[contact->getContactID()] = contactSubscription;
      }

      // We need to hint about the contact location to the stack just in case
      // the stack does not know about this location.
      if (mStackAccount) {
        mStackAccount->hintAboutNewPeerLocation(contact->getContactID(), locationID);
      }
    }

    //-------------------------------------------------------------------------
    IConversationThreadPtr Account::getConversationThreadByID(const char *threadID) const
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!threadID)
      AutoRecursiveLock lock(mLock);

      ConversationThreadMap::const_iterator found = mConversationThreads.find(threadID);
      if (found == mConversationThreads.end()) return IConversationThreadPtr();
      const IConversationThreadForAccountPtr &thread = (*found).second;
      return thread->convertIConversationThread();
    }

    //-------------------------------------------------------------------------
    void Account::getConversationThreads(ConversationThreadList &outConversationThreads) const
    {
      AutoRecursiveLock lock(mLock);

      for (ConversationThreadMap::const_iterator iter = mConversationThreads.begin(); iter != mConversationThreads.end(); ++iter)
      {
        const IConversationThreadForAccountPtr &thread = (*iter).second;
        outConversationThreads.push_back(thread->convertIConversationThread());
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => IAccountForConversationThread
    #pragma mark

    //-------------------------------------------------------------------------
    IConversationThreadDelegatePtr Account::getConversationThreadDelegate() const
    {
      AutoRecursiveLock lock(mLock);
      return mConversationThreadDelegate;
    }

    //-------------------------------------------------------------------------
    String Account::getContactID() const
    {
      if (!mContactSelf) {
        ZS_LOG_WARNING(Detail, log("failed to obtain contact ID of self as self contact is NULL"))
        return "";
      }

      return mContactSelf->getContactID();
    }

    //-------------------------------------------------------------------------
    IContactForConversationThreadPtr Account::findContact(const char *contactID) const
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!contactID)

      AutoRecursiveLock lock(mLock);
      ContactMap::const_iterator found = mContacts.find(contactID);
      if (found == mContacts.end()) {
        if (contactID == mContactSelf->getContactID()) {
          return mContactSelf;
        }
        return IContactForConversationThreadPtr();
      }
      const IContactForAccountPtr &contact = (*found).second;
      return IContactForConversationThread::convert(contact);
    }

    //-------------------------------------------------------------------------
    stack::IAccountPtr Account::getStackAccount() const
    {
      AutoRecursiveLock lock(mLock);
      return mStackAccount;
    }

    //-------------------------------------------------------------------------
    IPublicationRepositoryPtr Account::getRepository() const
    {
      AutoRecursiveLock lock(mLock);
      if (!mStackAccount) return IPublicationRepositoryPtr();
      return mStackAccount->getRepository();
    }

    //-------------------------------------------------------------------------
    IPeerFilesPtr Account::getPeerFiles() const
    {
      AutoRecursiveLock lock(mLock);
      return mPeerFiles;
    }

    //-------------------------------------------------------------------------
    void Account::notifyConversationThreadCreated(IConversationThreadForAccountPtr thread)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!thread)

      AutoRecursiveLock lock(mLock);

      if ((isShuttingDown()) ||
          (isShutdown())) {
        ZS_LOG_WARNING(Detail, log("cannot remember new thread or notify about it since account shutdown/shutting down"))
        return;
      }

      mConversationThreads[thread->getThreadID()] = thread;

      try {
        mConversationThreadDelegate->onConversationThreadNew(thread->convertIConversationThread());
      } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
        ZS_LOG_WARNING(Detail, log("could not notify of new conversation thread - conversation thread delegate is gone"))
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => IAccountForCall
    #pragma mark

    //-------------------------------------------------------------------------
    ICallTransportForAccountPtr Account::getCallTransport() const
    {
      AutoRecursiveLock lock(mLock);
      return mCallTransport;
    }

    //-------------------------------------------------------------------------
    ICallDelegatePtr Account::getCallDelegate() const
    {
      AutoRecursiveLock lock(mLock);
      return mCallDelegate;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => IAccountForContact
    #pragma mark

    //-------------------------------------------------------------------------
    String Account::getPassword() const
    {
      AutoRecursiveLock lock(mLock);
      return mPassword;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => ICallTransportDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::onCallTransportStateChanged(
                                              ICallTransportPtr inTransport,
                                              CallTransportStates state
                                              )
    {
      ZS_LOG_DEBUG(log("notified call transport state changed"))

      AutoRecursiveLock lock(mLock);
      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => IBootstrappedNetworkDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::onBootstrappedNetworkStateChanged(
                                                    stack::IBootstrappedNetworkPtr bootstrapper,
                                                    BootstrappedNetworkStates state
                                                    )
    {
      ZS_LOG_DEBUG(log("notified bootstrapper state changed"))
      AutoRecursiveLock lock(mLock);

      if (isShutdown()) {
        ZS_LOG_DEBUG(log("bootstapper state change ignored since account is already shutdown"))
        return;
      }

      if (bootstrapper != mBootstrapper) return;

      if (IBootstrappedNetwork::BootstrappedNetworkState_Shutdown == state) {
        ZS_LOG_DEBUG(log("bootstrapper is shutdown so account must shutdown"))
        setLastError(IAccount::AccountError_BootstrappedNetworkFailed);
        cancel();
        return;
      }
      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => IBootstrappedNetworkFetchSignedSaltQueryDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::onBootstrappedNetworkFetchSignedSaltComplete(
                                                               stack::IBootstrappedNetworkPtr bootstrapper,
                                                               IBootstrappedNetworkFetchSignedSaltQueryPtr query
                                                               )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!query)
      ZS_LOG_DEBUG(log("bootstrapper notified signed salt is ready"))

      AutoRecursiveLock lock(mLock);

      if (isShutdown()) {
        ZS_LOG_DEBUG(log("is already shutdown"))
        return;
      }

      if (bootstrapper != mBootstrapper) {
        ZS_LOG_DEBUG(log("bootstrapper notification comes from obsolete bootstrapper"))
        return;
      }

      if (query != mFetchSignedSalt) {
        ZS_LOG_DEBUG(log("bootstrapper notification comes from obsolete bootstrapper fetch signed salt query"))
        return;
      }

      ZS_LOG_DEBUG(log("bootstrapper fetching signed salt elements and generating peer files"))

      ElementPtr signedSalt = query->getNextSignedSalt();
      if (!signedSalt) {
        ZS_LOG_DEBUG(log("failed to create account due to inability to fetch signed salt"))
        setLastError(IAccount::AccountError_BootstrappedNetworkFailed);
        cancel();
        return;
      }

      mPeerFiles = IPeerFiles::generatePeerFile(mPassword, signedSalt);
      ZS_THROW_BAD_STATE_IF(!mPeerFiles)

      IPeerContactProfilePtr profile = mPeerFiles->getContactProfile();

      if (mGeneratedPublicProfileXML)
        profile->setPublicProfile(mGeneratedPublicProfileXML);

      if (mGeneratedPrivateProfileXML)
        profile->setPrivateProfile(mGeneratedPrivateProfileXML);

      if ((mGeneratedPublicProfileXML) ||
          (mGeneratedPrivateProfileXML))
        profile->setVersionNumber(1);

      // we don't need to remember this anymore...
      mGeneratedPublicProfileXML.reset();
      mGeneratedPrivateProfileXML.reset();

      mContactSelf = Contact::createSelfContact(mThisWeak.lock(), mPeerFiles);
      mGeneratedPeerFile = true;

      ZS_LOG_DEBUG(log("bootstrapper signed salt fetching should be completed"))

      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => stack::IAccountDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::onAccountStateChanged(
                                        stack::IAccountPtr account,
                                        stack::IAccount::AccountStates state
                                        )
    {
      ZS_LOG_DEBUG(log("notified stack account state changed"))

      AutoRecursiveLock lock(mLock);

      if (isShutdown()) return;

      if (account != mStackAccount) return;

      if (stack::IAccount::AccountState_Shutdown == account->getState()) {
        switch (account->getLastError()) {
          case hookflash::stack::IAccount::AccountError_None:                       setLastError(IAccount::AccountError_StackFailed); break;

          case hookflash::stack::IAccount::AccountError_InternalError:              setLastError(IAccount::AccountError_StackFailed); break;

          case hookflash::stack::IAccount::AccountError_BootstrappedNetworkFailed:  setLastError(IAccount::AccountError_BootstrappedNetworkFailed); break;
          case hookflash::stack::IAccount::AccountError_SocketUnexpectedlyClosed:   setLastError(IAccount::AccountError_StackFailed); break;
        }
        cancel();
        return;
      }

      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => IConnectionSubscriptionDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::onConnectionSubscriptionShutdown(IConnectionSubscriptionPtr subscription)
    {
      AutoRecursiveLock lock(mLock);
      if (subscription != mConnectionSubscription) return;

      mConnectionSubscription.reset();
      step();
    }

    //-------------------------------------------------------------------------
    void Account::onConnectionSubscriptionFinderConnectionStateChanged(
                                                                       IConnectionSubscriptionPtr subscription,
                                                                       ConnectionStates state
                                                                       )
    {
      //IGNORED
    }

    //-------------------------------------------------------------------------
    void Account::onConnectionSubscriptionPeerLocationConnectionStateChanged(
                                                                             IConnectionSubscriptionPtr subscription,
                                                                             IPeerLocationPtr location,
                                                                             ConnectionStates state
                                                                             )
    {
      ZS_LOG_TRACE(log("notified peer location state changed") + ", state=" + IConnectionSubscription::toString(state) + ", contact ID=" + location->getContactID() + ", location ID=" + location->getLocationID())

      AutoRecursiveLock lock(mLock);
      if (subscription != mConnectionSubscription) {
        ZS_LOG_WARNING(Detail, log("ignoring obsolete connection subscription"))
        return;
      }

      String contactID = location->getContactID();

      // see if there is a local contact with this contact ID
      ContactMap::iterator foundContact = mContacts.find(contactID);
      if (foundContact == mContacts.end()) {
        // did not find a contact with this contact ID - thus we need to create one
        IPeerFilePublicPtr peerFilePublic = location->getPeerFilePublic();
        if (!peerFilePublic) {
          ZS_LOG_ERROR(Detail, log("no public peer file for location provided") + ", contact ID=" + contactID + ", location ID=" + location->getLocationID())
          return;
        }
        IContactForAccount::createFromPeerFilePublic(mThisWeak.lock(), location->getPeerFilePublic());

        // attempt find once more as contact might now be registered
        foundContact = mContacts.find(contactID);
        if (foundContact == mContacts.end()) {
          ZS_LOG_WARNING(Detail, log("unable to find peer contact") + ", contact ID=" + contactID)
          return;
        }
      }

      IContactForAccountPtr contact = (*foundContact).second;

      ContactSubscriptionMap::iterator foundContactSubscription = mContactSubscriptions.find(contactID);
      ContactSubscriptionPtr contactSubscription;
      if (foundContactSubscription == mContactSubscriptions.end()) {
        switch (state) {
          case stack::IConnectionSubscription::ConnectionState_Pending:
          case stack::IConnectionSubscription::ConnectionState_Disconnecting:
          case stack::IConnectionSubscription::ConnectionState_Disconnected:   {
            ZS_LOG_DEBUG(log("no need to create contact subscription when the connection is not ready") + ", contact ID=" + contactID)
            return;
          }
          case stack::IConnectionSubscription::ConnectionState_Connected: break;
        }

        ZS_LOG_DEBUG(log("creating a new contact subscription") + ", contact ID=" + contactID + ", location ID=" + location->getLocationID())
        contactSubscription = ContactSubscription::create(mThisWeak.lock(), contact, location);
        mContactSubscriptions[contactID] = contactSubscription;
      } else {
        contactSubscription = (*foundContactSubscription).second;
      }

      ZS_LOG_DEBUG(log("notifying contact subscription about state") + ", contact ID=" + contactID + ", location ID=" + location->getLocationID() + ", state=" + stack::IConnectionSubscription::toString(state))
      contactSubscription->notifyAboutLocationState(location, state);
    }

    //-------------------------------------------------------------------------
    void Account::onConnectionSubscriptionIncomingMessage(
                                                          IConnectionSubscriptionPtr subscription,
                                                          IConnectionSubscriptionMessagePtr message
                                                          )
    {
      //IGNORED
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => IAccountAsyncDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::onStep()
    {
      AutoRecursiveLock lock(mLock);
      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => friend Account::AccountSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::notifySubscriptionCancelled(PUID id)
    {
      AutoRecursiveLock lock(mLock);
      DelegateMap::iterator found = mDelegates.find(id);
      if (found == mDelegates.end()) return;

      mDelegates.erase(found);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => friend Account::ContactSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::notifyContactSubscriptionShutdown(const String &contactID)
    {
      AutoRecursiveLock lock(mLock);
      ContactSubscriptionMap::iterator found = mContactSubscriptions.find(contactID);
      if (found == mContactSubscriptions.end()) return;

      mContactSubscriptions.erase(found);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => friend Account::LocationSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    IConversationThreadForAccountPtr Account::notifyPublicationUpdated(
                                                                       IPeerLocationPtr peerLocation,
                                                                       IPublicationMetaDataPtr metaData,
                                                                       const SplitMap &split
                                                                       )
    {
      if (isShutdown()) {
        ZS_LOG_WARNING(Debug, log("received updated publication document after account was shutdown thus ignoring"))
        return IConversationThreadForAccountPtr();
      }

      String baseThreadID = stack::IHelper::get(split, HOOKFLASH_CONVERSATION_THREAD_BASE_THREAD_ID_INDEX);
      String hostThreadID = stack::IHelper::get(split, HOOKFLASH_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX);
      if ((baseThreadID.size() < 1) ||
          (hostThreadID.size() < 1)) {
        ZS_LOG_WARNING(Debug, log("converation thread publication did not have a thread ID") + ", name=" + metaData->getName())
        return IConversationThreadForAccountPtr();
      }

      ConversationThreadMap::iterator found = mConversationThreads.find(baseThreadID);
      if (found != mConversationThreads.end()) {
        ZS_LOG_DEBUG(log("notify publication updated for existing thread") + ", thread ID=" + baseThreadID + ", name=" + metaData->getName())
        IConversationThreadForAccountPtr thread = (*found).second;
        thread->notifyPublicationUpdated(peerLocation, metaData, split);
        return thread;
      }

      ZS_LOG_DEBUG(log("notify publication for new thread") + ", thread ID=" + baseThreadID + ", name=" + metaData->getName())
      IConversationThreadForAccountPtr thread = IConversationThreadForAccount::create(mThisWeak.lock(), peerLocation, metaData, split);
      if (!thread) {
        ZS_LOG_WARNING(Debug, log("notify publication for new thread aborted"))
        return IConversationThreadForAccountPtr();
      }

      return thread;
    }

    //-------------------------------------------------------------------------
    void Account::notifyPublicationGone(
                                        IPeerLocationPtr peerLocation,
                                        IPublicationMetaDataPtr metaData,
                                        const SplitMap &split
                                        )
    {
      String baseThreadID = stack::IHelper::get(split, HOOKFLASH_CONVERSATION_THREAD_BASE_THREAD_ID_INDEX);
      String hostThreadID = stack::IHelper::get(split, HOOKFLASH_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX);
      if ((baseThreadID.size() < 1) ||
          (hostThreadID.size() < 1)) {
        ZS_LOG_WARNING(Debug, log("converation thread publication did not have a thread ID") + ", name=" + metaData->getName())
        return;
      }

      ConversationThreadMap::iterator found = mConversationThreads.find(baseThreadID);
      if (found == mConversationThreads.end()) {
        ZS_LOG_WARNING(Debug, log("notify publication gone for thread that did not exist") + ", thread ID=" + baseThreadID + ", name=" + metaData->getName())
        return;
      }

      ZS_LOG_DEBUG(log("notify publication gone for existing thread") + ", thread ID=" + baseThreadID + ", name=" + metaData->getName())
      IConversationThreadForAccountPtr thread = (*found).second;
      thread->notifyPublicationGone(peerLocation, metaData, split);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => internal
    #pragma mark

    //-------------------------------------------------------------------------
    String Account::log(const char *message) const
    {
      return String("hookflash::Account [") + Stringize<PUID>(mID).string() + "] " + message;
    }

    //-------------------------------------------------------------------------
    void Account::cancel()
    {
      AutoRecursiveLock lock(mLock);  // just in case

      ZS_LOG_DEBUG(log("cancel called"))

      if (isShutdown()) return;
      if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

      if (mFetchSignedSalt) {
        mFetchSignedSalt->cancel();
        mFetchSignedSalt.reset();
      }

      setState(AccountState_ShuttingDown);

      if (mCallTransport) {
        ZS_LOG_DEBUG(log("shutting down call transport"))
        mCallTransport->shutdown();
      }

      if (mStackAccount) {
        ZS_LOG_DEBUG(log("shutting down stack account"))
        mStackAccount->shutdown();
      }

      if (mBootstrapper) {
        ZS_LOG_DEBUG(log("shutting down bootstrapper"))
        mBootstrapper->close();
      }

      if (mConnectionSubscription) {
        mConnectionSubscription->cancel();
        mConnectionSubscription.reset();
      }

      if (mGracefulShutdownReference) {
        if (mStackAccount) {
          if (stack::IAccount::AccountState_Shutdown != mStackAccount->getState()) {
            ZS_LOG_DEBUG(log("waiting for stack account to shutdown"))
            return;
          }
        }

        if (mBootstrapper) {
          if (IBootstrappedNetwork::BootstrappedNetworkState_Shutdown != mBootstrapper->getState()) {
            ZS_LOG_DEBUG(log("waiting for bootstrapper to shutdown"))
            return;
          }
        }

        if (mCallTransport) {
          if (ICallTransport::CallTransportState_Shutdown != mCallTransport->getState()) {
            ZS_LOG_DEBUG(log("waiting for call transport to shutdown"))
            return;
          }
        }
      }

      setState(AccountState_Shutdown);

      mGracefulShutdownReference.reset();

      mDelegates.clear();

      mStackAccount.reset();
      mBootstrapper.reset();
      mCallTransport.reset();

      mConversationThreadDelegate.reset();
      mCallDelegate.reset();
      mPeerFiles.reset();

      ZS_LOG_DEBUG(log("shutdown complete"))
    }

    //-------------------------------------------------------------------------
    void Account::step()
    {
      if ((isShutdown()) ||
          (isShuttingDown())) {
        cancel();
        return;
      }

      if (IBootstrappedNetwork::BootstrappedNetworkState_Ready != mBootstrapper->getState()) {
        ZS_LOG_DEBUG(log("step boot strapper is not ready"))
        return;
      }

      if (!mPeerFiles) {
        if (!mFetchSignedSalt) {
          mFetchSignedSalt = mBootstrapper->fetchSignedSalt(mThisWeak.lock());
          if (!mFetchSignedSalt) {
            ZS_LOG_ERROR(Detail, log("failed to fetch signed salt"))
            setLastError(IAccount::AccountError_BootstrappedNetworkFailed);
            cancel();
            return;
          }
        }
        if (0 == mFetchSignedSalt->getTotalSignedSaltsAvailable()) {
          ZS_LOG_DEBUG(log("waiting for signed salt"))
          return;
        }

        ZS_LOG_DEBUG(log("waiting for peer file generation to complete"))
        return;
      }

      StackPtr stack = mOuter.lock();
      if (!stack) {
        ZS_LOG_ERROR(Detail, log("stack shutdown thus account must shutdown"))
        setLastError(IAccount::AccountError_InternalError);
        cancel();
        return;
      }

      if (mCallTransport) {
        ICallTransportForAccount::CallTransportStates state = mCallTransport->getState();
        if ((ICallTransport::CallTransportState_ShuttingDown == state) ||
            (ICallTransport::CallTransportState_Shutdown == state)){
          ZS_LOG_ERROR(Detail, log("premature shutdown of transport object (something is wrong)"))
          setLastError(IAccount::AccountError_CallTransportFailed);
          cancel();
          return;
        }
      }

      if (!mCallTransport) {
        mCallTransport = ICallTransportForAccount::create(mThisWeak.lock(), mTURNServer, mTURNUsername, mTURNPassword, mSTUNServer);

        if (!mCallTransport) {
          ZS_LOG_ERROR(Detail, log("failed to create call transport object thus shutting down"))
          setLastError(IAccount::AccountError_CallTransportFailed);
          cancel();
          return;
        }
      }

      if (!mStackAccount) {
        ZS_LOG_DEBUG(log("creating stack account"))
        mStackAccount = stack::IAccount::create(
                                                getAssociatedMessageQueue(),
                                                mBootstrapper,
                                                mThisWeak.lock(),
                                                mPeerFiles,
                                                mPassword,
                                                stack->getDeviceID(),
                                                stack->getUserAgent(),
                                                stack->getOS(),
                                                stack->getSystem()
                                                );

        if (!mStackAccount) {
          ZS_LOG_ERROR(Detail, log("failed to create stack account thus shutting down"))
          setLastError(IAccount::AccountError_StackFailed);
          cancel();
          return;
        }
      }

      if (stack::IAccount::AccountState_Ready != mStackAccount->getState()) {
        ZS_LOG_DEBUG(log("step stack account is pending"))
        return;
      }

      if (!mSubscribersPermissionDocument) {
        stack::IPublicationRepositoryPtr repository = mStackAccount->getRepository();
        if (!repository) {
          ZS_LOG_ERROR(Detail, log("repository on stack account is not valid thus account must shutdown"))
          setLastError(IAccount::AccountError_InternalError);
          cancel();
          return;
        }

        stack::IPublication::RelationshipList relationships;
        relationships.push_back(mContactSelf->getContactID());

        stack::IPublicationMetaData::PublishToRelationshipsMap empty;
        mSubscribersPermissionDocument = stack::IPublication::createForLocal(mContactSelf->getContactID(), mStackAccount->getLocationID(), "/threads/1.0/subscribers/permissions", "text/x-xml-openpeer-permissions", relationships, empty);
        if (!mSubscribersPermissionDocument) {
          ZS_LOG_ERROR(Detail, log("unable to create subscription permission document thus shutting down"))
          setLastError(IAccount::AccountError_InternalError);
          cancel();
          return;
        }

        IPublicationPublisherPtr publisher = repository->publish(IPublicationPublisherDelegateProxy::createNoop(getAssociatedMessageQueue()), mSubscribersPermissionDocument);
        if (!publisher->isComplete()) {
          ZS_LOG_ERROR(Detail, log("unable to publish local subscription permission document which should have happened instantly"))
          setLastError(IAccount::AccountError_InternalError);
          cancel();
          return;
        }
      }

      if (!mConnectionSubscription) {
        mConnectionSubscription = mStackAccount->subscribeToAllConnections(mThisWeak.lock());

        if (!mConnectionSubscription) {
          ZS_LOG_ERROR(Detail, log("unable to create a subscription to all connections"))
          setLastError(IAccount::AccountError_InternalError);
          cancel();
          return;
        }
      }

      if (ICallTransport::CallTransportState_Ready != mCallTransport->getState()) {
        ZS_LOG_DEBUG(log("waiting on call transport to be ready"))
        return;
      }

      setState(AccountState_Ready);
    }

    //-------------------------------------------------------------------------
    void Account::setState(hookflash::IAccount::AccountStates state)
    {
      if (mCurrentState == state) return;

      ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))
      mCurrentState = state;

      AccountPtr pThis = mThisWeak.lock();

      if (pThis) {
        for (DelegateMap::iterator delIter = mDelegates.begin(); delIter != mDelegates.end(); )
        {
          DelegateMap::iterator current = delIter;
          ++delIter;

          IAccountDelegatePtr &delegate = (*current).second;

          try {
            delegate->onAccountStateChanged(mThisWeak.lock(), state);
          } catch (IAccountDelegateProxy::Exceptions::DelegateGone &) {
            mDelegates.erase(current);
          }
        }
      }
    }

    //------------------------------------------------------------------------
    void Account::setLastError(hookflash::IAccount::AccountErrors error)
    {
      if (error == mLastError) return;

      if ((isShuttingDown()) ||
          (isShutdown())) {
        ZS_LOG_WARNING(Basic, log("account error ignored since already shutting down") + ", attempting to set=" + toString(error))
        return;
      }

      if (IAccount::AccountError_None != mLastError) {
        ZS_LOG_WARNING(Basic, log("account error ignored since last error was already set") + ", attempting to set=" + toString(error) + ", last error=" + toString(mLastError))
        return;
      }

      ZS_LOG_ERROR(Basic, log("account error") + ", error=" + toString(mLastError))
      mLastError = error;
    }

    //------------------------------------------------------------------------
    StackPtr Account::getOuter()
    {
      return mOuter.lock();
    }

    //-------------------------------------------------------------------------
    void Account::getServers(
                             String &outTURNServer,
                             String &outTURNUsername,
                             String &outTURNPassword,
                             String &outSTUNServer
                             )
    {
      AutoRecursiveLock lock(mLock);
      outTURNServer = mTURNServer;
      outTURNUsername = mTURNUsername;
      outTURNPassword = mTURNPassword;
      outSTUNServer = mSTUNServer;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => AccountSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    Account::AccountSubscription::AccountSubscription(AccountPtr outer) :
      mID(zsLib::createPUID()),
      mOuter(outer)
    {
    }

    //-------------------------------------------------------------------------
    void Account::AccountSubscription::init()
    {
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::AccountSubscription => friend Account
    #pragma mark

    //-------------------------------------------------------------------------
    Account::AccountSubscriptionPtr Account::AccountSubscription::create(AccountPtr outer)
    {
      AccountSubscriptionPtr pThis(new AccountSubscription(outer));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::AccountSubscription => IAccountSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    IAccountPtr Account::AccountSubscription::getAccount() const
    {
      return mOuter;
    }

    //-------------------------------------------------------------------------
    void Account::AccountSubscription::cancel()
    {
      mOuter->notifySubscriptionCancelled(mID);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::AccountSubscription => (internal)
    #pragma mark

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => ContactSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    const char *Account::ContactSubscription::toString(Account::ContactSubscription::ContactSubscriptionStates state)
    {
      switch (state) {
        case ContactSubscriptionState_Pending:      return "Pending";
        case ContactSubscriptionState_Ready:        return "Ready";
        case ContactSubscriptionState_ShuttingDown: return "Shutting down";
        case ContactSubscriptionState_Shutdown:     return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    Account::ContactSubscription::ContactSubscription(
                                                      AccountPtr outer,
                                                      IContactForAccountPtr contact
                                                      ) :
      MessageQueueAssociator(outer->getAssociatedMessageQueue()),
      mID(zsLib::createPUID()),
      mOuter(outer),
      mContact(contact),
      mCurrentState(ContactSubscriptionState_Pending)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!outer)
      ZS_THROW_INVALID_ARGUMENT_IF(!contact)
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::init(IPeerLocationPtr peerLocation)
    {
      if (!peerLocation) {
        ZS_LOG_DEBUG(log("creating a contact subscription to a hinted location") + mContact->getContactID())

        // If there isn't a peer location then this contact subscription came
        // into being because this contact was hinted that it wants to connect
        // with this user. As such we will need to open a peer subscription
        // to the contact to cause locations to be found/opened. If there
        // are active conversation threads then the conversation threads will
        // open their own peer subscriptions and thus this peer subscription
        // can be shutdown after a reasonable amount of time has passed to
        // try to connect to the peer.
        AccountPtr outer = mOuter.lock();
        ZS_THROW_BAD_STATE_IF(!outer)

        stack::IAccountPtr stackAccount = outer->getStackAccount();
        if (!stackAccount) {
          ZS_LOG_WARNING(Detail, log("stack account is not available thus unable to create contact subscription"))
          goto step;
        }

        stack::IPeerFilePublicPtr peerFilePublic = mContact->getPeerFilePublic();
        if (!peerFilePublic) {
          ZS_LOG_WARNING(Detail, log("public peer file for contact is not available"))
          goto step;
        }

        mPeerSubscription = stackAccount->subscribePeerLocations(peerFilePublic, mThisWeak.lock());
        ZS_THROW_BAD_STATE_IF(!mPeerSubscription)

        mPeerSubscriptionAutoCloseTimer = Timer::create(mThisWeak.lock(), Seconds(HOOKFLASH_PEER_SUBSCRIPTION_AUTO_CLOSE_TIMEOUT_IN_SECONDS), false);
      } else {
        mLocations[peerLocation->getLocationID()] = LocationSubscription::create(mThisWeak.lock(), peerLocation);
      }

    step:
      step();
    }

    //-------------------------------------------------------------------------
    Account::ContactSubscription::~ContactSubscription()
    {
      ZS_LOG_DEBUG(log("destructor called"))
      mThisWeak.reset();
      cancel();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::ContactSubscription => friend Account
    #pragma mark

    //-------------------------------------------------------------------------
    Account::ContactSubscriptionPtr Account::ContactSubscription::create(
                                                                         AccountPtr outer,
                                                                         IContactForAccountPtr contact,
                                                                         stack::IPeerLocationPtr peerLocation
                                                                         )
    {
      ContactSubscriptionPtr pThis(new ContactSubscription(outer, contact));
      pThis->mThisWeak = pThis;
      pThis->init(peerLocation);
      return pThis;
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::notifyAboutLocationState(
                                                                stack::IPeerLocationPtr location,
                                                                stack::IConnectionSubscription::ConnectionStates state
                                                                )
    {
      LocationSubscriptionMap::iterator found = mLocations.find(location->getLocationID());

      LocationSubscriptionPtr locationSubscription;
      if (found != mLocations.end()) {
        locationSubscription = (*found).second;
      }

      ZS_LOG_DEBUG(log("notifying about location state") + ", location ID=" + location->getLocationID() + ", state=" + stack::IConnectionSubscription::toString(state) + ", found=" + (found != mLocations.end() ? "true" : "false"))

      switch (state) {
        case stack::IConnectionSubscription::ConnectionState_Pending: {
          if (found == mLocations.end()) {
            ZS_LOG_DEBUG(log("pending state where location is not found thus do nothing"))
            return;  // only do something when its actually connected
          }

          ZS_LOG_DEBUG(log("pending state where location is found thus cancelling existing location"))

          // we must have had an old subscription laying around, kill it in favour of a new one that will come later...
          locationSubscription->cancel();
          break;
        }
        case stack::IConnectionSubscription::ConnectionState_Connected: {
          if (found != mLocations.end()) {
            // make sure the location that already exists isn't in the middle of a shutdown...
            if ((locationSubscription->isShuttingDown()) || (locationSubscription->isShutdown())) {
              ZS_LOG_WARNING(Debug, log("connected state where location subscription was shutting down thus forgetting location subscription early"))

              // forget about this location early since it must shutdown anyway...
              mLocations.erase(found);
              found = mLocations.end();
            }
          }

          if (found != mLocations.end()) {
            ZS_LOG_DEBUG(log("connected state where location subscription is pending or ready"))
            return;  // nothing to do since location already exists
          }

          ZS_LOG_DEBUG(log("creating location subscription for connected location"))

          // we have a new location, remember it...
          locationSubscription = LocationSubscription::create(mThisWeak.lock(), location);
          mLocations[location->getLocationID()] = locationSubscription;
          break;
        }
        case stack::IConnectionSubscription::ConnectionState_Disconnecting:
        case stack::IConnectionSubscription::ConnectionState_Disconnected:  {
          if (found == mLocations.end()) {
            ZS_LOG_DEBUG(log("ignoring disconnecting/disconnected state where there is no location subscription"))
            return;  // nothing to do as we don't have location anyway...
          }

          ZS_LOG_DEBUG(log("cancelling location subscription for disconnecting/disconnected location"))
          locationSubscription->cancel();
          break;
        }
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::ContactSubscription => IContactSubscriptionAsyncDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::onStep()
    {
      AutoRecursiveLock lock(getLock());
      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::ContactSubscription => IPeerSubscriptionDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription)
    {
      AutoRecursiveLock lock(getLock());
      if (mPeerSubscription != subscription) {
        ZS_LOG_DEBUG(log("ignoring peer subscription shutdown for obslete subscription"))
        return;
      }

      ZS_LOG_DEBUG(log("peer subscription shutdown"))

      mPeerSubscription.reset();
      step();
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::onPeerSubscriptionFindStateChanged(
                                                                          IPeerSubscriptionPtr subscription,
                                                                          PeerSubscriptionFindStates state
                                                                          )
    {
      AutoRecursiveLock lock(getLock());
      if (mPeerSubscription != subscription) {
        ZS_LOG_DEBUG(log("ignoring peer subscription shutdown for obslete subscription"))
        return;
      }

      ZS_LOG_DEBUG(log("peer subscription state changed") + ", state=" + IPeerSubscription::toString(state))
      step();
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::onPeerSubscriptionLocationsChanged(IPeerSubscriptionPtr subscription)
    {
      //IGNORED
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::onPeerSubscriptionMessage(
                                                                 IPeerSubscriptionPtr subscription,
                                                                 IPeerSubscriptionMessagePtr incomingMessage
                                                                 )
    {
      //IGNORED
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::ContactSubscription => ITimerDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::onTimer(TimerPtr timer)
    {
      AutoRecursiveLock lock(getLock());
      if (timer != mPeerSubscriptionAutoCloseTimer) return;

      ZS_LOG_DEBUG(log("timer fired") + ", reset peer subscription=" + (mPeerSubscription ? "true" : "false"))

      if (mPeerSubscription) {
        mPeerSubscription->cancel();
        mPeerSubscription.reset();
      }

      mPeerSubscriptionAutoCloseTimer->cancel();
      mPeerSubscriptionAutoCloseTimer.reset();

      step();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::ContactSubscription => friend LocationSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    AccountPtr Account::ContactSubscription::getOuter() const
    {
      AutoRecursiveLock lock(getLock());
      return mOuter.lock();
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::notifyLocationShutdown(const String &locationID)
    {
      AutoRecursiveLock lock(getLock());

      LocationSubscriptionMap::iterator found = mLocations.find(locationID);
      if (found == mLocations.end()) {
        ZS_LOG_DEBUG(log("location subscription not found in connection subscription list") + ", location ID=" + locationID)
        return;
      }

      ZS_LOG_DEBUG(log("erasing location subscription") + ", location ID=" + locationID)
      mLocations.erase(found);

      IContactSubscriptionAsyncDelegateProxy::create(mThisWeak.lock())->onStep();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::ContactSubscription => (internal)
    #pragma mark

    //-------------------------------------------------------------------------
    RecursiveLock &Account::ContactSubscription::getLock() const
    {
      AccountPtr outer = mOuter.lock();
      if (!outer) return mBogusLock;
      return outer->getLock();
    }

    //-------------------------------------------------------------------------
    String Account::ContactSubscription::log(const char *message) const
    {
      return String("hookflash::Account::ContactSubscription [") + Stringize<PUID>(mID).string() + "] " + message + ", contact ID=" + mContact->getContactID();
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::cancel()
    {
      if (isShutdown()) return;

      setState(ContactSubscriptionState_ShuttingDown);

      if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

      bool locationsShutdown = true;

      // clear out locations
      {
        for (LocationSubscriptionMap::iterator locIter = mLocations.begin(); locIter != mLocations.end(); )
        {
          LocationSubscriptionMap::iterator current = locIter;
          ++locIter;

          LocationSubscriptionPtr &location = (*current).second;

          location->cancel();
          if (!location->isShutdown()) locationsShutdown = false;
        }

        mLocations.clear();
      }

      if (mPeerSubscription) {
        mPeerSubscription->cancel();
      }

      if (mGracefulShutdownReference) {
        if (mPeerSubscription) {
          if (!mPeerSubscription->isShutdown()) {
            ZS_LOG_DEBUG(log("waiting for peer subscription to shutdown"))
            return;
          }
        }

        if (!locationsShutdown) {
          ZS_LOG_DEBUG(log("waiting for location to shutdown"))
          return;
        }
      }

      setState(ContactSubscriptionState_Shutdown);

      mGracefulShutdownReference.reset();

      if (mPeerSubscriptionAutoCloseTimer) {
        mPeerSubscriptionAutoCloseTimer->cancel();
        mPeerSubscriptionAutoCloseTimer.reset();
      }

      mLocations.clear();

      if (mPeerSubscription) {
        mPeerSubscription->cancel();
        mPeerSubscription.reset();
      }

      AccountPtr outer = mOuter.lock();
      if (outer) {
        outer->notifyContactSubscriptionShutdown(mContact->getContactID());
      }
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::step()
    {
      if ((isShuttingDown()) ||
          (isShutdown())) {
        cancel();
        return;
      }

      setState(ContactSubscriptionState_Ready);

      if (!mPeerSubscriptionAutoCloseTimer) {
        if (mLocations.size() == 0) {
          // there are no more locations... we should shut outselves down...
          cancel();
        }
      }
    }

    //-------------------------------------------------------------------------
    void Account::ContactSubscription::setState(ContactSubscriptionStates state)
    {
      if (state == mCurrentState) return;

      ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))
      mCurrentState = state;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account => LocationSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    const char *Account::LocationSubscription::toString(Account::LocationSubscription::LocationSubscriptionStates state)
    {
      switch (state) {
        case LocationSubscriptionState_Pending:      return "Pending";
        case LocationSubscriptionState_Ready:        return "Ready";
        case LocationSubscriptionState_ShuttingDown: return "Shutting down";
        case LocationSubscriptionState_Shutdown:     return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    Account::LocationSubscription::LocationSubscription(
                                                        ContactSubscriptionPtr outer,
                                                        IPeerLocationPtr peerLocation
                                                        ) :
      MessageQueueAssociator(outer->getAssociatedMessageQueue()),
      mID(zsLib::createPUID()),
      mOuter(outer),
      mPeerLocation(peerLocation),
      mCurrentState(LocationSubscriptionState_Pending)
    {
    }

    //-------------------------------------------------------------------------
    void Account::LocationSubscription::init()
    {
      step();
    }

    //-------------------------------------------------------------------------
    Account::LocationSubscription::~LocationSubscription()
    {
      ZS_LOG_DEBUG(log("destructor called"))
      mThisWeak.reset();
      cancel();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::LocationSubscription => friend ContactSubscription
    #pragma mark

    //-------------------------------------------------------------------------
    Account::LocationSubscriptionPtr Account::LocationSubscription::create(
                                                                           ContactSubscriptionPtr outer,
                                                                           IPeerLocationPtr peerLocation
                                                                           )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!peerLocation)
      LocationSubscriptionPtr pThis(new LocationSubscription(outer, peerLocation));
      pThis->mThisWeak = pThis;
      pThis->init();
      return pThis;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::LocationSubscription => IPublicationSubscriptionDelegate
    #pragma mark

    //-------------------------------------------------------------------------
    void Account::LocationSubscription::onPublicationSubscriptionStateChanged(
                                                                              IPublicationSubscriptionPtr subcription,
                                                                              PublicationSubscriptionStates state
                                                                              )
    {
      AutoRecursiveLock lock(getLock());
      if (subcription != mPublicationSubscription) {
        ZS_LOG_DEBUG(log("ignoring publication subscription state change for obsolete subscription"))
        return;
      }

      ZS_LOG_DEBUG(log("publication subscription state change") + ", state=" + stack::IPublicationSubscription::toString(state))

      if ((stack::IPublicationSubscription::PublicationSubscriptionState_ShuttingDown == mPublicationSubscription->getState()) ||
          (stack::IPublicationSubscription::PublicationSubscriptionState_ShuttingDown == mPublicationSubscription->getState())) {
        ZS_LOG_WARNING(Detail, log("failed to create a subscription to the peer"))
        mPublicationSubscription.reset();
        cancel();
        return;
      }

      step();
    }

    //-------------------------------------------------------------------------
    void Account::LocationSubscription::onPublicationSubscriptionPublicationUpdated(
                                                                                    IPublicationSubscriptionPtr subscription,
                                                                                    IPublicationMetaDataPtr metaData
                                                                                    )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!metaData)

      AutoRecursiveLock lock(getLock());
      if (subscription != mPublicationSubscription) {
        ZS_LOG_DEBUG(log("ignoring publication notification on obsolete publication subscription"))
        return;
      }

      String name = metaData->getName();

      SplitMap result;
      stack::IHelper::split(name, result);

      if (result.size() < 6) {
        ZS_LOG_WARNING(Debug, log("subscription path is too short") + ", path=" + name)
        return;
      }

      ContactSubscriptionPtr outer = mOuter.lock();
      if (!outer) {
        ZS_LOG_WARNING(Debug, log("unable to locate contact subscription"))
        return;
      }

      AccountPtr account = outer->getOuter();
      if (!account) {
        ZS_LOG_WARNING(Debug, log("unable to locate account"))
        return;
      }

      IConversationThreadForAccountPtr thread = account->notifyPublicationUpdated(mPeerLocation, metaData, result);
      if (!thread) {
        ZS_LOG_WARNING(Debug, log("publication did not result in a conversation thread"))
        return;
      }

      String threadID = thread->getThreadID();
      ConversationThreadMap::iterator found = mConversationThreads.find(threadID);
      if (found != mConversationThreads.end()) {
        ZS_LOG_DEBUG(log("already know about this conversation thread (thus nothing more to do)"))
        return;  // already know about this conversation thread
      }

      ZS_LOG_DEBUG(log("remembering converation thread for the future"))

      // remember this conversation thread is linked to this peer location
      mConversationThreads[threadID] = thread;
    }

    //-------------------------------------------------------------------------
    void Account::LocationSubscription::onPublicationSubscriptionPublicationGone(
                                                                                 IPublicationSubscriptionPtr subscription,
                                                                                 IPublicationMetaDataPtr metaData
                                                                                 )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!metaData)

      AutoRecursiveLock lock(getLock());
      if (subscription != mPublicationSubscription) {
        ZS_LOG_DEBUG(log("ignoring publication notification on obsolete publication subscription"))
        return;
      }

      String name = metaData->getName();

      SplitMap result;
      stack::IHelper::split(name, result);

      if (result.size() < 6) {
        ZS_LOG_WARNING(Debug, log("subscription path is too short") + ", path=" + name)
        return;
      }

      ContactSubscriptionPtr outer = mOuter.lock();
      if (!outer) {
        ZS_LOG_WARNING(Debug, log("unable to locate contact subscription"))
        return;
      }

      AccountPtr account = outer->getOuter();
      if (!account) {
        ZS_LOG_WARNING(Debug, log("unable to locate account"))
        return;
      }

      account->notifyPublicationGone(mPeerLocation, metaData, result);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark Account::LocationSubscription => (internal)
    #pragma mark

    //-------------------------------------------------------------------------
    RecursiveLock &Account::LocationSubscription::getLock() const
    {
      ContactSubscriptionPtr outer = mOuter.lock();
      if (!outer) return mBogusLock;
      return outer->getLock();
    }

    //-------------------------------------------------------------------------
    String Account::LocationSubscription::log(const char *message) const
    {
      return String("hookflash::Account::LocationSubscription [") + Stringize<PUID>(mID).string() + "] " + message + ", contact ID=" + getContactID() + ", location ID=" + getLocationID();
    }

    //-------------------------------------------------------------------------
    String Account::LocationSubscription::getContactID() const
    {
      static String empty;
      ContactSubscriptionPtr outer = mOuter.lock();
      if (outer) return outer->getContact()->getContactID();
      return empty;
    }

    //-------------------------------------------------------------------------
    String Account::LocationSubscription::getLocationID() const
    {
      if (!mPeerLocation) return String();
      return mPeerLocation->getLocationID();
    }

    //-------------------------------------------------------------------------
    void Account::LocationSubscription::cancel()
    {
      if (isShutdown()) return;

      setState(LocationSubscriptionState_ShuttingDown);

      if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

      // scope: notify all the conversation threads that the peer location is shutting down
      {
        for (ConversationThreadMap::iterator iter = mConversationThreads.begin(); iter != mConversationThreads.end(); ++iter)
        {
          IConversationThreadForAccountPtr &thread = (*iter).second;
          thread->notifyPeerDisconnected(mPeerLocation);
        }
        mConversationThreads.clear();
      }

      if (mPublicationSubscription) {
        mPublicationSubscription->cancel();
      }

      if (mGracefulShutdownReference) {
        if (mPublicationSubscription) {
          if (stack::IPublicationSubscription::PublicationSubscriptionState_Shutdown != mPublicationSubscription->getState()) {
            ZS_LOG_DEBUG(log("waiting for publication subscription to shutdown"))
            return;
          }
        }
      }

      setState(LocationSubscriptionState_Shutdown);

      ContactSubscriptionPtr outer = mOuter.lock();
      if ((outer) &&
          (mPeerLocation)) {
        outer->notifyLocationShutdown(getLocationID());
      }

      mPublicationSubscription.reset();
      mPeerLocation.reset();
    }

    //-------------------------------------------------------------------------
    void Account::LocationSubscription::step()
    {
      if ((isShuttingDown()) ||
          (isShutdown())) {
        cancel();
        return;
      }

      if (!mPublicationSubscription) {
        ContactSubscriptionPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("failed to obtain contact subscription"))
          return;
        }

        AccountPtr account = outer->getOuter();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("failed to obtain account"))
          return;
        }

        stack::IAccountPtr stackAccount = account->getStackAccount();
        if (!stackAccount) {
          ZS_LOG_WARNING(Detail, log("failed to obtain stack account"))
          return;
        }

        stack::IPublicationRepositoryPtr repository = stackAccount->getRepository();
        if (!repository) {
          ZS_LOG_WARNING(Detail, log("failed to obtain stack publication respository"))
          return;
        }

        stack::IPublicationMetaData::ContactIDList empty;
        stack::IPublicationRepository::SubscribeToRelationshipsMap relationships;
        relationships["/threads/1.0/subscribers/permissions"] = stack::IPublicationMetaData::PermissionAndContactIDListPair(stack::IPublicationMetaData::Permission_All, empty);

        ZS_LOG_DEBUG(log("subscribing to peer thread publications"))
        mPublicationSubscription = repository->subscribePeer(mThisWeak.lock(), "/threads/1.0/", relationships, outer->getContact()->getContactID(), getLocationID());
      }

      if (!mPublicationSubscription) {
        ZS_LOG_WARNING(Detail, log("failed to create publication subscription"))
        cancel();
        return;
      }

      if (stack::IPublicationSubscription::PublicationSubscriptionState_Established != mPublicationSubscription->getState()) {
        ZS_LOG_DEBUG(log("waiting for publication subscription to establish"))
        return;
      }

      setState(LocationSubscriptionState_Ready);
    }

    //-------------------------------------------------------------------------
    void Account::LocationSubscription::setState(LocationSubscriptionStates state)
    {
      if (state == mCurrentState) return;

      ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

      mCurrentState = state;
    }
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IAccount
  #pragma mark

  const char *IAccount::toString(AccountStates state)
  {
    switch (state) {
      case AccountState_Pending:      return "Pending";
      case AccountState_Ready:        return "Ready";
      case AccountState_ShuttingDown: return "Shutting down";
      case AccountState_Shutdown:     return "Shutdown";
    }
    return "UNDEFINED";
  }

  const char *IAccount::toString(AccountErrors errorCode)
  {
    switch (errorCode) {
      case AccountError_None:                       return "None";

      case AccountError_InternalError:              return "Internal error";
      case AccountError_BootstrappedNetworkFailed:  return "Bootstrapped network failed";
      case AccountError_StackFailed:                return "Stack failed";
      case AccountError_CallTransportFailed:        return "Call transport failed";
    }
    return "UNDEFINED";
  }
}
