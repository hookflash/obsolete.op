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


#include <hookflash/internal/hookflash_ConversationThread.h>
#include <hookflash/internal/hookflash_ConversationThreadHost.h>
#include <hookflash/internal/hookflash_ConversationThreadSlave.h>
#include <hookflash/internal/hookflash_Account.h>
#include <hookflash/internal/hookflash_Contact.h>
#include <hookflash/internal/hookflash_Call.h>

#include <hookflash/stack/IPeerLocation.h>

#include <hookflash/services/IHelper.h>

#include <cryptopp/crc.h>

#include <zsLib/Log.h>
#include <zsLib/Stringize.h>
#include <zsLib/zsHelpers.h>

namespace hookflash { ZS_DECLARE_SUBSYSTEM(hookflash) }

using namespace zsLib::XML;
using zsLib::Stringize;

namespace hookflash
{
  namespace internal
  {
    typedef zsLib::BYTE BYTE;
    typedef zsLib::UINT UINT;
    typedef zsLib::DWORD DWORD;
    typedef stack::IPublicationRepositoryPtr IPublicationRepositoryPtr;
    typedef CryptoPP::CRC32 CRC32;

    //-------------------------------------------------------------------------
    static void convert(const IConversationThreadParser::Contacts::ContactMap &input, IConversationThread::ContactList &output)
    {
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
      typedef IConversationThreadParser::ThreadContactPtr ThreadContactPtr;

      for (ContactMap::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const ThreadContactPtr &contact = (*iter).second;
        output.push_back(contact->contact()->convertIContact());
      }
    }

    //-------------------------------------------------------------------------
    static void convert(const IConversationThreadParser::Contacts::ContactMap &input, IConversationThread::ContactInfoList &output)
    {
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
      typedef IConversationThreadParser::ThreadContactPtr ThreadContactPtr;
      typedef IConversationThread::ContactInfo ContactInfo;

      for (ContactMap::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const ThreadContactPtr &contact = (*iter).second;
        ContactInfo info;
        info.mContact = contact->contact()->convertIContact();
        info.mProfileBundleEl = contact->profileBundleElement();
        output.push_back(info);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //------------------------------- ------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadForAccount
    #pragma mark

    //-------------------------------------------------------------------------
    IConversationThreadForAccountPtr IConversationThreadForAccount::convert(IConversationThreadPtr thread)
    {
      return boost::dynamic_pointer_cast<ConversationThread>(thread);
    }

    //-------------------------------------------------------------------------
    IConversationThreadForAccountPtr IConversationThreadForAccount::create(
                                                                           IAccountForConversationThreadPtr account,
                                                                           IPeerLocationPtr peerLocation,
                                                                           IPublicationMetaDataPtr metaData,
                                                                           const SplitMap &split
                                                                           )
    {
      return ConversationThread::create(account, peerLocation, metaData, split);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadForCall
    #pragma mark

    //-------------------------------------------------------------------------
    IConversationThreadForCallPtr IConversationThreadForCall::convert(IConversationThreadPtr thread)
    {
      return boost::dynamic_pointer_cast<ConversationThread>(thread);
    }

    //-------------------------------------------------------------------------
    IConversationThreadForCallPtr IConversationThreadForCall::convert(IConversationThreadForHostOrSlavePtr thread)
    {
      return boost::dynamic_pointer_cast<ConversationThread>(thread);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThread
    #pragma mark

    //-------------------------------------------------------------------------
    const char *ConversationThread::toString(ConversationThreadStates state)
    {
      switch (state)
      {
        case ConversationThreadState_Pending:       return "Pending";
        case ConversationThreadState_Ready:         return "Ready";
        case ConversationThreadState_ShuttingDown:  return "Shutting down";
        case ConversationThreadState_Shutdown:      return "Shutdown";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    ConversationThread::ConversationThread(
                                           IAccountForConversationThreadPtr account,
                                           const char *threadID
                                           ) :
      MessageQueueAssociator(account->getAssociatedMessageQueue()),
      mID(zsLib::createPUID()),
      mAccount(account),
      mDelegate(account->getConversationThreadDelegate()),
      mThreadID(threadID ? zsLib::String(threadID) : services::IHelper::randomString(32)),
      mCurrentState(ConversationThreadState_Pending),
      mMustNotifyAboutNewThread(false),
      mHandleContactsChangedCRC(0)
    {
      ZS_LOG_BASIC(log("created"))
    }

    //-------------------------------------------------------------------------
    void ConversationThread::init()
    {
      ZS_LOG_DEBUG(log("initialized"))
      IConversationThreadAsyncProxy::create(mThisWeak.lock())->onStep();
    }

    //-------------------------------------------------------------------------
    ConversationThread::~ConversationThread()
    {
      ZS_LOG_BASIC(log("destroyed"))
      mThisWeak.reset();
      cancel();
    }

    //-------------------------------------------------------------------------
    ConversationThreadPtr ConversationThread::convert(IConversationThreadPtr thread)
    {
      return boost::dynamic_pointer_cast<ConversationThread>(thread);
    }

    //-------------------------------------------------------------------------
    ConversationThreadPtr ConversationThread::convert(IConversationThreadForCallPtr thread)
    {
      return boost::dynamic_pointer_cast<ConversationThread>(thread);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThread => IConversationThread
    #pragma mark

    //-------------------------------------------------------------------------
    ConversationThreadPtr ConversationThread::create(
                                                     hookflash::IAccountPtr account,
                                                     ElementPtr profileElementEl
                                                     )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!account)

      ConversationThreadPtr pThis(new ConversationThread(IAccountForConversationThread::convert(account), NULL));
      pThis->mThisWeak = pThis;
      pThis->init();

      ZS_LOG_DEBUG(pThis->log("created for API caller"))

      // add "ourself" to the contact list...
      ContactInfoList contacts;
      ContactInfo info;
      info.mContact = account->getSelfContact();
      info.mProfileBundleEl = profileElementEl;
      contacts.push_back(info);

      pThis->addContacts(contacts);

      pThis->mAccount.lock()->notifyConversationThreadCreated(pThis); // safe to use mLock() weak pointer because account object must still be valid
      pThis->handleContactsChanged();
      return pThis;
    }

    //-------------------------------------------------------------------------
    const zsLib::String &ConversationThread::getThreadID() const
    {
      AutoRecursiveLock lock(getLock());
      return mThreadID;
    }

    //-------------------------------------------------------------------------
    bool ConversationThread::amIHost() const
    {
      AutoRecursiveLock lock(getLock());
      if (!mLastOpenThread) return true;

      return mLastOpenThread->isHost();
    }

    //-------------------------------------------------------------------------
    void ConversationThread::getContacts(ContactList &contactList) const
    {
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;

      AutoRecursiveLock lock(getLock());

      if (!mLastOpenThread) {
        ZS_LOG_WARNING(Detail, log("no contacts have been added to this conversation thread"))
        return;
      }

      ContactMap contacts;
      mLastOpenThread->getContacts(contacts);

      internal::convert(contacts, contactList);
    }

    //-------------------------------------------------------------------------
    ElementPtr ConversationThread::getProfileBundle(IContactPtr contact) const
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!contact)

      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
      typedef IConversationThreadParser::ThreadContactPtr ThreadContactPtr;

      AutoRecursiveLock lock(getLock());
      
      if (!mLastOpenThread) {
        ZS_LOG_WARNING(Detail, log("cannot get profile bundle no contacts have been added to this conversation thread"))
        return ElementPtr();
      }
      
      ContactMap contacts;
      mLastOpenThread->getContacts(contacts);

      ContactMap::iterator found = contacts.find(contact->getContactID());
      if (found == contacts.end()) {
        ZS_LOG_WARNING(Detail, log("cannot get profile bundle as contact was not found") + ", contact ID=" + contact->getContactID())
        return ElementPtr();
      }
      const ThreadContactPtr &threadContact = (*found).second;
      return threadContact->profileBundleElement();
    }

    //-------------------------------------------------------------------------
    IConversationThread::ContactStates ConversationThread::getContactState(IContactPtr contact) const
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!contact)

      AutoRecursiveLock lock(getLock());

      if (!mLastOpenThread) {
        ZS_LOG_WARNING(Detail, log("no conversation thread was ever openned"))
        return IConversationThread::ContactState_NotApplicable;
      }

      return mLastOpenThread->getContactState(IContactForConversationThread::convert(contact));
    }

    //-------------------------------------------------------------------------
    void ConversationThread::addContacts(const ContactInfoList &inputContacts)
    {
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
      
      ContactInfoList contacts;
      for (ContactInfoList::const_iterator iter = inputContacts.begin(); iter != inputContacts.end(); ++iter)
      {
        const ContactInfo &sourceInfo = (*iter);
        
        ContactInfo destInfo;
        destInfo.mContact = sourceInfo.mContact;
        if (sourceInfo.mProfileBundleEl) {
          destInfo.mProfileBundleEl = sourceInfo.mProfileBundleEl->clone()->toElement();
        }
        contacts.push_back(destInfo);
      }

      if (contacts.size() < 1) {
        ZS_LOG_WARNING(Debug, log("called add contacts method but did not specify any contacts to add"))
        return;
      }

      AutoRecursiveLock lock(getLock());

      if (!mLastOpenThread) {
        mOpenThread = IConversationThreadHostForConversationThread::create(mThisWeak.lock());
        ZS_THROW_BAD_STATE_IF(!mOpenThread)
        mThreads[mOpenThread->getThreadID()] = mOpenThread;
        mLastOpenThread = mOpenThread;
      }

      if (mLastOpenThread->safeToChangeContacts()) {
        ZS_LOG_DEBUG(log("able to add contacts to the current thread"))
        mLastOpenThread->addContacts(contacts);
        
        // just in case...
        handleContactsChanged();
        handleLastOpenThreadChanged();
        return;
      }

      ContactInfoList newContacts = contacts;

      ContactMap oldContacts;
      mLastOpenThread->getContacts(oldContacts);

      internal::convert(oldContacts, newContacts);

      if (mLastOpenThread->isHost()) {
        mLastOpenThread->toHost()->close();
      }

      // not safe to add contacts, we need to create a newly open thread...
      mOpenThread = IConversationThreadHostForConversationThread::create(mThisWeak.lock());
      ZS_THROW_BAD_STATE_IF(!mOpenThread)
      mThreads[mOpenThread->getThreadID()] = mOpenThread;
      
      mLastOpenThread = mOpenThread;

      mOpenThread->addContacts(newContacts);

      handleContactsChanged();
      handleLastOpenThreadChanged();
    }

    //-------------------------------------------------------------------------
    void ConversationThread::removeContacts(const ContactList &contacts)
    {
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;

      if (contacts.size() < 1) return;

      AutoRecursiveLock lock(getLock());

      if (!mLastOpenThread) {
        ZS_LOG_DEBUG(log("no need to remove any contacts as there was no last open thread"))
        return;
      }

      if (mLastOpenThread->safeToChangeContacts()) {
        ZS_LOG_DEBUG(log("able to add contacts to the current thread"))
        mLastOpenThread->removeContacts(contacts);
        handleContactsChanged();
        return;
      }

      ContactMap oldContacts;
      mLastOpenThread->getContacts(oldContacts);

      for (ContactList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
      {
        const IContactPtr &contact = (*iter);
        ContactMap::iterator found = oldContacts.find(contact->getContactID());
        if (found != oldContacts.end()) {
          oldContacts.erase(found);
        }
      }

      if (mLastOpenThread->isHost()) {
        mLastOpenThread->toHost()->close();
      }

      // not safe to add contacts, we need to create a newly open thread...
      mOpenThread = IConversationThreadHostForConversationThread::create(mThisWeak.lock());
      ZS_THROW_BAD_STATE_IF(!mOpenThread)
      mThreads[mOpenThread->getThreadID()] = mOpenThread;
      mLastOpenThread = mOpenThread;

      // convert the old contacts into a new contact list
      ContactInfoList newContacts;
      internal::convert(oldContacts, newContacts);

      if (newContacts.size() > 0) {
        mOpenThread->addContacts(newContacts);
      }

      handleContactsChanged();
      handleLastOpenThreadChanged();

      //***********************************************************************
      //***********************************************************************
      //***********************************************************************
      //***********************************************************************
      // HERE - check if trying to remove "self"
    }

    //-------------------------------------------------------------------------
    void ConversationThread::sendMessage(
                                         const char *messageID,
                                         const char *messageType,
                                         const char *body
                                         )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!messageID)
      ZS_THROW_INVALID_ARGUMENT_IF('\0' == *messageID)
      ZS_THROW_INVALID_ARGUMENT_IF(!messageType)
      ZS_THROW_INVALID_ARGUMENT_IF('\0' == *messageType)
      ZS_THROW_INVALID_ARGUMENT_IF(!body)
      ZS_THROW_INVALID_ARGUMENT_IF('\0' == *body)

      typedef stack::IPeerFilesPtr IPeerFilesPtr;
      typedef IConversationThreadParser::Message Message;

      AutoRecursiveLock lock(getLock());

      IAccountForConversationThreadPtr account = mAccount.lock();
      if (!account) {
        ZS_LOG_WARNING(Detail, log("account is gone thus unable to send the message") + ", message ID=" + messageID)
        return;
      }

      IPeerFilesPtr peerFiles = account->getPeerFiles();
      if (!peerFiles) {
        ZS_LOG_WARNING(Detail, log("peer files are not generated thus unable to send the message") + ", message ID=" + messageID)
        return;
      }

      MessagePtr message = Message::create(messageID, account->getContactID(), messageType, body, zsLib::now(), peerFiles);
      if (!message) {
        ZS_LOG_ERROR(Detail, log("failed to create message object") + ", message ID=" + messageID)
        return;
      }

      mPendingDeliveryMessages.push_back(message);
      mMessageDeliveryStates[messageID] = IConversationThread::MessageDeliveryState_Discovering;

      step();
    }

    //-------------------------------------------------------------------------
    bool ConversationThread::getMessage(
                                        const char *messageID,
                                        IContactPtr &outFrom,
                                        String &outMessageType,
                                        String &outMessage,
                                        Time &outTime
                                        ) const
    {
      AutoRecursiveLock lock(getLock());
      ZS_THROW_INVALID_ARGUMENT_IF(!messageID)

      IAccountForConversationThreadPtr account = mAccount.lock();
      if (!account) {
        ZS_LOG_WARNING(Detail, log("account is gone thus unable to fetch message") + ", message ID=" + messageID)
        return false;
      }

      MessageReceivedMap::const_iterator found = mReceivedOrPushedMessages.find(messageID);
      if (found == mReceivedOrPushedMessages.end()) {
        ZS_LOG_WARNING(Detail, log("unable to locate any message with the message ID provided") + ", message ID=" + messageID)
        return false;
      }

      const MessagePtr &message = (*found).second;

      const String &contactID = message->fromContactID();
      IContactForConversationThreadPtr contact = account->findContact(contactID);
      if (!contact) {
        ZS_LOG_ERROR(Detail, log("unable to find the contact for the message") + ", message ID=" + messageID + ", contact ID=" + contactID)
        return false;
      }

      outFrom = contact->convertIContact();
      outMessageType = message->mimeType();
      outMessage = message->body();
      outTime = message->sent();

      ZS_LOG_DEBUG(log("obtained message information") + ", message ID=" + messageID + ", contact ID=" + contactID + (contact->isSelf() ? " (self)" : "") + ", type=" + outMessageType + ", message=" + outMessage + ", time=" + Stringize<Time>(outTime).string())
      return true;
    }

    //-------------------------------------------------------------------------
    bool ConversationThread::getMessageDeliveryState(
                                                     const char *messageID,
                                                     MessageDeliveryStates &outDeliveryState
                                                     ) const
    {
      AutoRecursiveLock lock(getLock());
      outDeliveryState = IConversationThread::MessageDeliveryState_Discovering;
      MessageDeliveryStatesMap::const_iterator found = mMessageDeliveryStates.find(messageID);
      if (found == mMessageDeliveryStates.end()) {
        ZS_LOG_WARNING(Detail, log("unable to find message delivery state for message ID"))
        return false;
      }
      outDeliveryState = (*found).second;
      return true;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThread => IConversationThreadForAccount
    #pragma mark

    //-------------------------------------------------------------------------
    ConversationThreadPtr ConversationThread::create(
                                                     IAccountForConversationThreadPtr account,
                                                     IPeerLocationPtr peerLocation,
                                                     IPublicationMetaDataPtr metaData,
                                                     const SplitMap &split
                                                     )
    {
      ConversationThreadPtr pThis(new ConversationThread(account, stack::IHelper::get(split, HOOKFLASH_CONVERSATION_THREAD_BASE_THREAD_ID_INDEX)));
      pThis->mThisWeak = pThis;
      pThis->mMustNotifyAboutNewThread = true;
      pThis->init();
      pThis->notifyPublicationUpdated(peerLocation, metaData, split);
      if (pThis->mThreads.size() < 1) {
        ZS_LOG_WARNING(Detail, pThis->log("publication did not result in a slave thread being created thus aborting"))
        return ConversationThreadPtr();
      }
      return pThis;
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyPublicationUpdated(
                                                      IPeerLocationPtr peerLocation,
                                                      IPublicationMetaDataPtr metaData,
                                                      const SplitMap &split
                                                      )
    {
      AutoRecursiveLock lock(getLock());

      String hostThreadID = stack::IHelper::get(split, HOOKFLASH_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX);
      ZS_THROW_INVALID_ARGUMENT_IF(hostThreadID.size() < 1)

      ThreadMap::iterator found = mThreads.find(hostThreadID);
      if (found == mThreads.end()) {
        // could not find the publication... must be a host document or something is wrong...
        String type = stack::IHelper::get(split, HOOKFLASH_CONVERSATION_THREAD_TYPE_INDEX);
        if (type != "host") {
          // whatever this is it cannot be understood...
          ZS_LOG_WARNING(Detail, log("expecting a host document type but received something else") + ", type=" + type + ", name=" + metaData->getName())
          return;
        }

        ZS_LOG_DEBUG(log("creating a new conversation thread slave for updated publication") + ", host thread ID=" + hostThreadID + ", name=" + metaData->getName())

        IConversationThreadSlaveForConversationThreadPtr slave = IConversationThreadSlaveForConversationThread::create(mThisWeak.lock(), peerLocation, metaData, split);
        if (!slave) {
          ZS_LOG_WARNING(Detail, log("slave was not created for host document") + ", name=" + metaData->getName())
          return;
        }

        mThreads[hostThreadID] = slave;
      } else {
        ZS_LOG_DEBUG(log("reusing existing conversation thread slave for updated publication") + ", host thread ID=" + hostThreadID + ", name=" + metaData->getName())
        IConversationThreadHostSlaveBasePtr &thread = (*found).second;
        thread->notifyPublicationUpdated(peerLocation, metaData, split);
      }
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyPublicationGone(
                                                   IPeerLocationPtr peerLocation,
                                                   IPublicationMetaDataPtr metaData,
                                                   const SplitMap &split
                                                   )
    {
      AutoRecursiveLock lock(getLock());
      String hostThreadID = stack::IHelper::get(split, HOOKFLASH_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX);
      ZS_THROW_INVALID_ARGUMENT_IF(hostThreadID.size() < 1)

      ThreadMap::iterator found = mThreads.find(hostThreadID);
      if (found == mThreads.end()) {
        ZS_LOG_WARNING(Detail, log("notification about a publication that is gone for a conversation that does not exist") + ", name=" + metaData->getName())
        return;
      }

      IConversationThreadHostSlaveBasePtr &thread = (*found).second;
      thread->notifyPublicationGone(peerLocation, metaData, split);
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyPeerDisconnected(IPeerLocationPtr peerLocation)
    {
      AutoRecursiveLock lock(getLock());

      ZS_LOG_DEBUG(log("peer disconnected notification received") + ", contact ID=" + peerLocation->getContactID() + ", location ID=" + peerLocation->getLocationID())

      for (ThreadMap::iterator iter = mThreads.begin(); iter != mThreads.end(); ++iter)
      {
        IConversationThreadHostSlaveBasePtr &thread = (*iter).second;
        thread->notifyPeerDisconnected(peerLocation);
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThread => IConversationThreadForHostOrSlave
    #pragma mark

    //-------------------------------------------------------------------------
    IAccountForConversationThreadPtr ConversationThread::getAccount() const
    {
      AutoRecursiveLock lock(getLock());
      return mAccount.lock();
    }

    //-------------------------------------------------------------------------
    IPublicationRepositoryPtr ConversationThread::getRepository() const
    {
      AutoRecursiveLock lock(getLock());
      IAccountForConversationThreadPtr account = mAccount.lock();
      if (!account) return IPublicationRepositoryPtr();
      return account->getRepository();
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyStateChanged(IConversationThreadHostSlaveBasePtr thread)
    {
      AutoRecursiveLock lock(getLock());
      step();
      handleContactsChanged();
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyContactState(
                                                IConversationThreadHostSlaveBasePtr thread,
                                                IContactForConversationThreadPtr contact,
                                                ContactStates state
                                                )
    {
      AutoRecursiveLock lock(getLock());

      if (mLastOpenThread != thread) {
        ZS_LOG_TRACE(log("will not notify about contact states if not the last opened thread") + ", from host/slave thead ID=" + thread->getThreadID() + ", contact ID=" + contact->getContactID() + ", reported state=" + IConversationThread::toString(state))
        return;
      }

      bool changed = false;
      ContactStates lastState = IConversationThread::ContactState_NotApplicable;

      ContactStateMap::iterator found = mLastReportedContactStates.find(contact->getContactID());
      if (found != mLastReportedContactStates.end()) {
        ContactStatePair &statePair = (*found).second;
        lastState = statePair.second;
        changed = (lastState != state);
      } else {
        changed = true;
      }

      if (!changed) return;

      ZS_LOG_DEBUG(log("contact state changed") + ", contact ID=" + contact->getContactID() + ", old state=" + IConversationThread::toString(lastState) + ", new state=" + IConversationThread::toString(state))
      
      if (!mDelegate) {
        ZS_LOG_WARNING(Detail, log("conversation thread delegate not found"))
        return;
      }

      // remember the last reported state so it isn't repeated
      mLastReportedContactStates[contact->getContactID()] = ContactStatePair(contact, state);

      try {
        mDelegate->onConversationThreadContactStateChanged(mThisWeak.lock(), contact->convertIContact(), state);
      } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
        ZS_LOG_WARNING(Detail, log("conversation thread delegate gone"))
      }
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyMessageReceived(MessagePtr message)
    {
      AutoRecursiveLock lock(getLock());

      if ((isShutdown()) ||
          (isShuttingDown())) {
        ZS_LOG_WARNING(Detail, log("message received after already shutdown"))
        return;
      }

      MessageReceivedMap::iterator found = mReceivedOrPushedMessages.find(message->messageID());
      if (found != mReceivedOrPushedMessages.end()) {
        ZS_LOG_DEBUG(log("message received already delivered to delegate (thus ignoring)") + ", message ID=" + message->messageID());
        return;
      }

      // remember that this message is received
      mReceivedOrPushedMessages[message->messageID()] = message;
      ZS_LOG_DEBUG(log("message received and is being delivered to delegate") + ", id=" + message->messageID() + ", type=" + message->mimeType() + ", body=" + message->body());

      try {
        mDelegate->onConversationThreadMessage(mThisWeak.lock(), message->messageID());
      } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
        ZS_LOG_WARNING(Detail, log("delegate is gone"))
      }
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyMessageDeliveryStateChanged(
                                                               const char *messageID,
                                                               IConversationThread::MessageDeliveryStates state
                                                               )
    {
      AutoRecursiveLock lock(getLock());

      if ((isShutdown()) ||
          (isShuttingDown())) {
        ZS_LOG_WARNING(Detail, log("message delivery state change received after already shutdown"))
        return;
      }

      bool stateChanged = false;

      MessageDeliveryStatesMap::iterator found = mMessageDeliveryStates.find(messageID);
      if (found != mMessageDeliveryStates.end()) {
        MessageDeliveryStates &deliveryState = (*found).second;
        if (state > deliveryState) {
          ZS_LOG_DEBUG(log("message delivery state has changed") + ", message ID=" + messageID + ", old delivery state=" + IConversationThread::toString(deliveryState) + ", new delivery state=" + IConversationThread::toString(state))
          // this state has a higher priority than the old state
          deliveryState = state;
          stateChanged = true;
        } else {
          ZS_LOG_DEBUG(log("message delivery state is being ignored since it has less significance") + ", message ID=" + messageID + ", old delivery state=" + IConversationThread::toString(deliveryState) + ", new delivery state=" + IConversationThread::toString(state))
        }
      } else {
        ZS_LOG_DEBUG(log("message delivery state has changed") + ", message ID=" + messageID + ", delivery state=" + IConversationThread::toString(state))
        mMessageDeliveryStates[messageID] = state;
        stateChanged = true;
      }

      if (stateChanged) {
        try {
          mDelegate->onConversationThreadMessageDeliveryStateChanged(mThisWeak.lock(), messageID, state);
        } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate is gone"))
        }
      }
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyMessagePush(
                                               MessagePtr message,
                                               IContactForConversationThreadPtr toContact
                                               )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!message)
      ZS_THROW_INVALID_ARGUMENT_IF(!toContact)

      AutoRecursiveLock lock(getLock());

      if ((isShutdown()) ||
          (isShuttingDown())) {
        ZS_LOG_WARNING(Detail, log("ignoring message push notification received while shutdown") + ", message ID=" + message->messageID())
        return;
      }

      MessageReceivedMap::iterator found = mReceivedOrPushedMessages.find(message->messageID());
      if (found == mReceivedOrPushedMessages.end()) {
        mReceivedOrPushedMessages[message->messageID()] = message;
      }

      try {
        ZS_LOG_DEBUG(log("requesting push notification for conversation thread message") + ", message ID=" + message->messageID() + ", to contact=" + toContact->getContactID() + (toContact->isSelf() ? " (self)" : ""))
        mDelegate->onConversationThreadPushMessage(mThisWeak.lock(), message->messageID(), toContact->convertIContact());
      } catch(IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
        ZS_LOG_WARNING(Detail, log("unable to push message as delegate was gone"))
      }
    }

    //-------------------------------------------------------------------------
    void ConversationThread::requestAddIncomingCallHandler(
                                                           const char *dialogID,
                                                           IConversationThreadHostSlaveBasePtr hostOrSlaveThread,
                                                           ICallForConversationThreadPtr newCall
                                                           )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!dialogID)
      ZS_THROW_INVALID_ARGUMENT_IF(!hostOrSlaveThread)
      ZS_THROW_INVALID_ARGUMENT_IF(!newCall)

      AutoRecursiveLock lock(getLock());
      CallHandlerMap::iterator found = mCallHandlers.find(dialogID);
      if (found != mCallHandlers.end()) {
        ZS_LOG_WARNING(Detail, log("already have a call handler for this call thus ignoring request to add one") + ", call ID=" + dialogID)
        return;
      }

      ZS_LOG_DEBUG(log("call handler added for incoming call") + ", call ID=" + dialogID)
      mCallHandlers[dialogID] = CallHandlerPair(hostOrSlaveThread, newCall);
    }

    //-------------------------------------------------------------------------
    void ConversationThread::requestRemoveIncomingCallHandler(const char *dialogID)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!dialogID)

      AutoRecursiveLock lock(getLock());
      CallHandlerMap::iterator found = mCallHandlers.find(dialogID);
      if (found == mCallHandlers.end()) {
        ZS_LOG_WARNING(Detail, log("unable to find incoming call handler to remove") + ", call ID=" + dialogID)
        return;
      }
      ZS_LOG_DEBUG(log("removing incoming call handler") + ", call ID=" + dialogID)
      mCallHandlers.erase(found);
      ZS_LOG_DEBUG(log("incoming call handler removed") + ", total handlers=" + Stringize<size_t>(mCallHandlers.size()).string())
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyPossibleCallReplyStateChange(const char *dialogID)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!dialogID)

      AutoRecursiveLock lock(getLock());
      CallHandlerMap::iterator found = mCallHandlers.find(dialogID);
      if (found == mCallHandlers.end()) {
        ZS_LOG_WARNING(Detail, log("unable to find call handler for call") + ", call ID=" + dialogID)
        return;
      }

      ZS_LOG_DEBUG(log("nudging the call to notify about a potential call reply state change") + ", call ID=" + dialogID)

      CallHandlerPair &handlerPair = (*found).second;
      ICallForConversationThreadPtr &call = handlerPair.second;

      call->notifyConversationThreadUpdated();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThread => IConversationThreadForHost
    #pragma mark

    //-------------------------------------------------------------------------
    bool ConversationThread::inConversation(IContactForConversationThreadPtr contact) const
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!contact)
      AutoRecursiveLock lock(getLock());
      if (!mLastOpenThread) {
        return false;
      }
      return mLastOpenThread->inConversation(contact);
    }

    //-------------------------------------------------------------------------
    void ConversationThread::removeContacts(const ContactIDList &contacts)
    {
      if (contacts.size() < 1) return;
      AutoRecursiveLock lock(getLock());

      IAccountForConversationThreadPtr account = mAccount.lock();
      if (!account) {
        ZS_LOG_WARNING(Detail, log("account is gone"))
        return;
      }

      ContactList contactList;
      for (ContactIDList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
      {
        const String &contactID = (*iter);
        IContactForConversationThreadPtr contact = account->findContact(contactID);
        if (!contact) {
          ZS_LOG_WARNING(Detail, log("could not find contact ID in contact list") + ", contact ID=" + contactID)
          continue;
        }

        ZS_LOG_DEBUG(log("need to remove contact") + ", contact ID=" + contactID)
        contactList.push_back(contact->convertIContact());
      }

      if (contactList.size() < 1) {
        ZS_LOG_DEBUG(log("no contacts found needing to be removed"))
        return;
      }

      removeContacts(contactList);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThread => IConversationThreadForSlave
    #pragma mark

    //-------------------------------------------------------------------------
    void ConversationThread::notifyAboutNewThreadNowIfNotNotified(IConversationThreadSlaveForConversationThreadPtr slave)
    {
      AutoRecursiveLock lock(getLock());
      if (!mMustNotifyAboutNewThread) {
        ZS_LOG_DEBUG(log("no need to notify about thread creation as it is already created"))
        return;
      }

      IAccountForConversationThreadPtr account = mAccount.lock();
      if (!account) {
        ZS_LOG_WARNING(Detail, log("unable to notify about new thread as account object is gone"))
        return;
      }

      ZS_LOG_DEBUG(log("notifying that new conversation thread is now created"))
      
      if (!mLastOpenThread) {
        ZS_LOG_DEBUG(log("slave has now become last open thread"))
        mLastOpenThread = slave;
      }

      account->notifyConversationThreadCreated(mThisWeak.lock());
      mMustNotifyAboutNewThread = false;

      handleLastOpenThreadChanged();
    }

    //-------------------------------------------------------------------------
    void ConversationThread::convertSlaveToClosedHost(
                                                      IConversationThreadSlaveForConversationThreadPtr slave,
                                                      ThreadPtr originalHost,
                                                      ThreadPtr originalSlave
                                                      )
    {
      typedef IConversationThreadParser::ContactsPtr ContactsPtr;
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
      typedef IConversationThreadParser::Thread::MessageMap MessageMap;

      AutoRecursiveLock lock(getLock());

      if ((isShuttingDown()) ||
          (isShutdown())) {
        ZS_LOG_WARNING(Detail, log("unable to convert slave to closed host as basee thread is shutting down/shutdown") + ", slave thread ID=" + slave->getThreadID())
        return;
      }

      ZS_LOG_DEBUG(log("converting slave to closed host") + ", slave thread ID=" + slave->getThreadID())

      if (mOpenThread == slave) {
        ZS_LOG_DETAIL(log("slave thread is no longer considered 'open'") + ", slave thread ID=" + slave->getThreadID())
        mOpenThread.reset();
      }

      ContactsPtr contacts = originalHost->contacts();

      const ContactMap &originalContacts = contacts->contacts();

      IConversationThreadHostForConversationThreadPtr newClosedHost = IConversationThreadHostForConversationThread::create(mThisWeak.lock(), IConversationThreadParser::Details::ConversationThreadState_Closed);
      ZS_THROW_BAD_STATE_IF(!newClosedHost)
      mThreads[newClosedHost->getThreadID()] = newClosedHost;

      ZS_LOG_DEBUG(log("new closed host created for slave") + ", slave thread ID=" + slave->getThreadID() + ", new closed thread ID=" + newClosedHost->getThreadID())

      ContactInfoList newContacts;
      internal::convert(originalContacts, newContacts);

      newClosedHost->addContacts(newContacts);

      // gather all the messages
      MessageList messages;
      MessageMap messagesAsMap;

      const MessageList &hostMessages = originalHost->messages();
      const MessageList &slaveMessages = originalSlave->messages();

      for (MessageList::const_iterator iter = hostMessages.begin(); iter != hostMessages.end(); ++iter)
      {
        const MessagePtr &message = (*iter);
        MessageMap::iterator found = messagesAsMap.find(message->messageID());
        if (found != messagesAsMap.end()) {
          ZS_LOG_TRACE(log("igoring host message as message as already added") + ", message ID=" + message->messageID())
          continue;
        }
        ZS_LOG_TRACE(log("add host message to new closed host thread") + ", message ID=" + message->messageID())
        messages.push_back(message);
        messagesAsMap[message->messageID()] = message;
      }

      // NOTE:
      // Order does matter for the messages... ensure the most likely
      // undelivered messages are added last to the conversation thread.
      for (MessageList::const_iterator iter = slaveMessages.begin(); iter != slaveMessages.end(); ++iter)
      {
        const MessagePtr &message = (*iter);
        MessageMap::iterator found = messagesAsMap.find(message->messageID());
        if (found != messagesAsMap.end()) {
          ZS_LOG_TRACE(log("igoring slave message as message as already added") + ", message ID=" + message->messageID())
          continue;
        }
        ZS_LOG_TRACE(log("add slave message to new closed host thread") + ", message ID=" + message->messageID())
        messages.push_back(message);
        messagesAsMap[message->messageID()] = message;
      }

      ZS_THROW_BAD_STATE_IF(messages.size() < 1)

      ZS_LOG_DEBUG(log("requesting to deliver messages to closed thread"))
      newClosedHost->sendMessages(messages);

      // force a step on the conversation thread to cleanup anything state wise...
      ZS_LOG_DEBUG(log("forcing step"))
      IConversationThreadAsyncProxy::create(mThisWeak.lock())->onStep();
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThread => IConversationThreadForCall
    #pragma mark

    //-------------------------------------------------------------------------
    bool ConversationThread::placeCall(ICallForConversationThreadPtr call)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!call)

      AutoRecursiveLock lock(getLock());

      if ((isShuttingDown()) ||
          (isShutdown())) {
        return false;
      }

      ZS_LOG_DEBUG(log("adding all to pending list") + ", call ID=" + call->getCallID())

      mPendingCalls[call->getCallID()] = call;

      ZS_LOG_DEBUG(log("forcing step"))
      IConversationThreadAsyncProxy::create(mThisWeak.lock())->onStep();
      return true;
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyCallStateChanged(ICallForConversationThreadPtr call)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!call)

      AutoRecursiveLock lock(getLock());

      CallHandlerMap::iterator found = mCallHandlers.find(call->getCallID());
      if (found == mCallHandlers.end()) {
        ZS_LOG_DEBUG(log("call is not known yet to the conversation thread") + ", call ID=" + call->getCallID())
        return;
      }
      IConversationThreadHostSlaveBasePtr &thread = (*found).second.first;
      thread->notifyCallStateChanged(call);
    }

    //-------------------------------------------------------------------------
    void ConversationThread::notifyCallCleanup(ICallForConversationThreadPtr call)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!call)

      AutoRecursiveLock lock(getLock());

      PendingCallMap::iterator foundPending = mPendingCalls.find(call->getCallID());
      if (foundPending != mPendingCalls.end()) {
        ZS_LOG_DEBUG(log("call found on pending list thus removing") + ", call ID=" + call->getCallID())
        mPendingCalls.erase(foundPending);
        ZS_LOG_DEBUG(log("call found on pending list removed") + ", total pending=" + Stringize<size_t>(mPendingCalls.size()).string())
      }

      CallHandlerMap::iterator found = mCallHandlers.find(call->getCallID());
      if (found == mCallHandlers.end()) {
        ZS_LOG_WARNING(Detail, log("did not find any call handlers for this call") + ", call ID=" + call->getCallID())
        return;
      }

      ZS_LOG_DEBUG(log("call found and is being removed") + ", call ID=" + call->getCallID())
      IConversationThreadHostSlaveBasePtr &thread = (*found).second.first;
      thread->notifyCallCleanup(call);
      mCallHandlers.erase(found);

      ZS_LOG_DEBUG(log("call handler removed") + ", total handlers=" + Stringize<size_t>(mCallHandlers.size()).string())
    }

    //-------------------------------------------------------------------------
    void ConversationThread::gatherDialogReplies(
                                                 const char *callID,
                                                 LocationDialogMap &outDialogs
                                                 ) const
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!callID)

      AutoRecursiveLock lock(getLock());

      CallHandlerMap::const_iterator found = mCallHandlers.find(callID);
      if (found == mCallHandlers.end()) {
        ZS_LOG_DEBUG(log("no replies found for this call") + ", call ID=" + callID)
        return;
      }

      const IConversationThreadHostSlaveBasePtr &thread = (*found).second.first;

      ZS_LOG_DEBUG(log("requesting replies from thread") + ", call ID=" + callID + ", from " + (thread->isHost() ? "host" : "slave") + " thread ID=" + thread->getThreadID())
      thread->gatherDialogReplies(callID, outDialogs);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark ConversationThread => (internal)
    #pragma mark

    //-------------------------------------------------------------------------
    zsLib::String ConversationThread::log(const char *message) const
    {
      return String("ConversationThread [") + Stringize<PUID>(mID).string() + "] " + message + ", thread ID=" + mThreadID;
    }

    //-------------------------------------------------------------------------
    zsLib::RecursiveLock &ConversationThread::getLock() const
    {
      IAccountForConversationThreadPtr account = mAccount.lock();
      if (!account) return mBogusLock;
      return account->getLock();
    }

    //-------------------------------------------------------------------------
    void ConversationThread::cancel()
    {
      ZS_LOG_DEBUG(log("cancel"))

      AutoRecursiveLock lock(getLock());

      if (isShutdown()) {
        ZS_LOG_DEBUG(log("cancel called but already shutdown"))
        return;
      }

      if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

      setState(ConversationThreadState_ShuttingDown);

      for (ThreadMap::iterator iter = mThreads.begin(); iter != mThreads.end(); ++iter)
      {
        IConversationThreadHostSlaveBasePtr &thread = (*iter).second;
        thread->shutdown();
      }

      if (mGracefulShutdownReference) {
        for (ThreadMap::iterator iter = mThreads.begin(); iter != mThreads.end(); ++iter)
        {
          IConversationThreadHostSlaveBasePtr &thread = (*iter).second;
          if (!thread->isShutdown()) {
            ZS_LOG_DEBUG(log("waiting for thread to shutdown") + ", thread ID" + (*iter).first)
            return;
          }
        }
      }

      setState(ConversationThreadState_Shutdown);

      mGracefulShutdownReference.reset();

      mDelegate.reset();

      mOpenThread.reset();
      mLastOpenThread.reset();

      mThreads.clear();

      mReceivedOrPushedMessages.clear();
      mMessageDeliveryStates.clear();
      mPendingDeliveryMessages.clear();
    }

    //-------------------------------------------------------------------------
    void ConversationThread::step()
    {
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;

      ZS_LOG_DEBUG(log("step"))

      AutoRecursiveLock lock(getLock());

      if ((isShuttingDown())
          || (isShutdown())) {
        cancel();
        return;
      }

      ZS_LOG_DEBUG(log("step continued"))

      setState(ConversationThreadState_Ready);

      // figure out how many threads are open and which was open last
      UINT totalOpen = 0;
      IConversationThreadHostSlaveBasePtr mostRecentOpen;
      Time mostRecentOpenTime;
      for (ThreadMap::iterator iter = mThreads.begin(); iter != mThreads.end(); ++iter)
      {
        IConversationThreadHostSlaveBasePtr &thread = (*iter).second;
        if (thread->isHostThreadOpen()) {
          ++totalOpen;
          if (1 == totalOpen) {
            mostRecentOpen = thread;
            mostRecentOpenTime = thread->getHostCreationTime();
          } else {
            Time created = thread->getHostCreationTime();
            if (created > mostRecentOpenTime) {
              ZS_LOG_DEBUG(log("thread found is the most recent (thus chosing)") + ", found " + (thread->isHost() ? "host" : "slave") + " thread ID=" + thread->getThreadID() + ", thread creation time=" + Stringize<Time>(created).string())
              mostRecentOpen = thread;
              mostRecentOpenTime = thread->getHostCreationTime();
            } else {
              ZS_LOG_DEBUG(log("thread found is older than the most recent") + ", found " + (thread->isHost() ? "host" : "slave") + " thread ID=" + thread->getThreadID() + ", thread creation time=" + Stringize<Time>(created).string())
            }
          }
        }
      }

      ZS_LOG_DEBUG(log("finished counting open threads") + ", total open=" + Stringize<UINT>(totalOpen).string())

      if (totalOpen > 1) {
        ZS_LOG_DEBUG(log("found more than one thread open (thus will close any hosts that are not the most recent)"))

        // can only have one thread open maxmimum, all other host threads must be closed...
        for (ThreadMap::iterator iter = mThreads.begin(); iter != mThreads.end(); ++iter)
        {
          IConversationThreadHostSlaveBasePtr &thread = (*iter).second;
          if (thread != mostRecentOpen) {
            if (thread->isHost()) {
              ZS_LOG_DEBUG(log("due to more than one thread being open host is being closed") + ", host thread ID=" + thread->getThreadID())
              // close the thread...
              thread->toHost()->close();
            }
          }
        }
      }
      
      if (mostRecentOpen) {
        // remember which thread is open now...
        ZS_LOG_DEBUG(log("determined which thread is the most recent open") + ", most recent open " + (mostRecentOpen->isHost() ? "host" : "slave") + " thread ID=" + mostRecentOpen->getThreadID())
        mOpenThread = mostRecentOpen;
        mLastOpenThread = mostRecentOpen;
      }

      if (0 == totalOpen) {
        ZS_LOG_DEBUG(log("no open conversation threads found"))
        mOpenThread.reset();
      }

      bool mustHaveOpenThread = false;

      if (mPendingDeliveryMessages.size() > 0) {
        ZS_LOG_DEBUG(log("messages are pending delivery") + ", total pending=" + Stringize<size_t>(mPendingDeliveryMessages.size()).string())
        mustHaveOpenThread = true;
      }

      if (mPendingCalls.size() > 0) {
        ZS_LOG_DEBUG(log("calls are pending being placed") + ", total pending=" + Stringize<size_t>(mPendingCalls.size()).string())
        mustHaveOpenThread = true;
      }

      ZS_LOG_DEBUG(log("finished checking if must have open thread") + ", must have=" + (mustHaveOpenThread ? "true" : "false"))
      if (mustHaveOpenThread) {
        if (!mOpenThread) {
          // create a host thread since there is no open thread...
          mOpenThread = IConversationThreadHostForConversationThread::create(mThisWeak.lock());
          ZS_THROW_BAD_STATE_IF(!mOpenThread)

          ZS_LOG_DEBUG(log("no thread found to be open thus creating a new host") + ", host thread ID=" + mOpenThread->getThreadID())
          mThreads[mOpenThread->getThreadID()] = mOpenThread;

          if (mLastOpenThread) {
            // make sure the contacts from the last open thread are carried over...
            ContactMap contacts;
            mLastOpenThread->getContacts(contacts);

            ZS_LOG_DEBUG(log("contacts from last open thread are being brought into new thread") + ", total contacts=" + Stringize<size_t>(contacts.size()).string())

            ContactInfoList addContacts;
            internal::convert(contacts, addContacts);

            mOpenThread->addContacts(addContacts);
          }

          mLastOpenThread = mOpenThread;
        }
      }

      if (mOpenThread) {
        ZS_LOG_DEBUG(log("thread has open thread") + ", open " + (mOpenThread->isHost() ? "host" : "slave") + " thread ID=" + mOpenThread->getThreadID())
        if (mPendingDeliveryMessages.size() > 0) {
          bool sent = mOpenThread->sendMessages(mPendingDeliveryMessages);
          if (sent) {
            ZS_LOG_DEBUG(log("messages were accepted by open thread"))
            mPendingDeliveryMessages.clear();
          }
        }

        if (mPendingCalls.size() > 0) {
          bool sent = mOpenThread->placeCalls(mPendingCalls);
          if (sent) {
            // remember that this thread is handling all these placed calls...
            for (PendingCallMap::iterator iter = mPendingCalls.begin(); iter != mPendingCalls.end(); ++iter) {
              const CallID &callID = (*iter).first;
              ICallForConversationThreadPtr &call = (*iter).second;
              ZS_LOG_DEBUG(log("call placed and now handled via open thread") + ", call ID=" + callID)
              mCallHandlers[callID] = CallHandlerPair(mOpenThread, call);

              // nudge the call to tell it state has changed...
              call->notifyConversationThreadUpdated();
            }

            ZS_LOG_DEBUG(log("pending calls are now removed"))
            mPendingCalls.clear();
          } else {
            ZS_LOG_WARNING(Detail, log("unable to place calls via open thread"))
          }
        }
      }
      
      handleLastOpenThreadChanged();

      ZS_LOG_DEBUG(log("step completed"))
    }

    //-------------------------------------------------------------------------
    void ConversationThread::setState(ConversationThreadStates state)
    {
      if (state == mCurrentState) return;

      ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

      mCurrentState = state;
    }

    //-------------------------------------------------------------------------
    void ConversationThread::handleLastOpenThreadChanged()
    {
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
      typedef IConversationThreadParser::ThreadContactPtr ThreadContractPtr;

      if (mHandleThreadChanged == mLastOpenThread) {
        ZS_LOG_DEBUG(log("last open thread did not change"))
        return;
      }

      ZS_LOG_DEBUG(log("last open thread changed"))
      mHandleThreadChanged = mLastOpenThread;
      
      if (!mLastOpenThread) {
        ZS_LOG_WARNING(Detail, log("last open thread has become NULL"))
        return;
      }

      if (!mDelegate) {
        ZS_LOG_WARNING(Detail, log("delegate is NULL"))
        return;
      }

      ContactMap contacts;
      mLastOpenThread->getContacts(contacts);

      for (ContactMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
        const ThreadContractPtr &threadContact = (*iter).second;
        IContactForConversationThreadPtr contact = threadContact->contact();
        ContactStates state = mLastOpenThread->getContactState(contact);

        bool changed = false;

        ContactStateMap::iterator found = mLastReportedContactStates.find(contact->getContactID());
        if (found != mLastReportedContactStates.end()) {
          ContactStatePair &statePair = (*found).second;
          if (statePair.second != state) {
            statePair.second = state;
            changed = true;
          }
        } else {
          mLastReportedContactStates[contact->getContactID()] = ContactStatePair(contact, state);
          changed = true;
        }

        try {
          ZS_LOG_DEBUG(log("notifying of contact state changed") + +", contact ID=" + contact->getContactID() + ", state=" + IConversationThread::toString(state))
          mDelegate->onConversationThreadContactStateChanged(mThisWeak.lock(), contact->convertIContact(), state);
        } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("conversation thread delegate gone"))
        }
      }

      handleContactsChanged();
    }

    //-------------------------------------------------------------------------
    void ConversationThread::handleContactsChanged()
    {
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
      typedef IConversationThreadParser::ThreadContactPtr ThreadContractPtr;

      if (!mLastOpenThread) {
        ZS_LOG_WARNING(Detail, log("last open thread has become NULL"))
        return;
      }
      
      if (!mDelegate) {
        ZS_LOG_WARNING(Detail, log("delegate is NULL"))
        return;
      }

      DWORD crcValue = 0;

      ContactMap contacts;
      mLastOpenThread->getContacts(contacts);

      CRC32 crc;
      for (ContactMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
        const String &contactID = (*iter).first;

        crc.Update((const BYTE *)(contactID.c_str()), contactID.length());
        crc.Update((const BYTE *)(":"), strlen(":"));
      }

      crc.Final((BYTE *)(&crcValue));
      
      if (mHandleContactsChangedCRC == crcValue) {
        ZS_LOG_DEBUG(log("contact change not detected, CRC value=") + Stringize<DWORD>(crcValue).string())
        return;
      }

      mHandleContactsChangedCRC = crcValue;
      ZS_LOG_DEBUG(log("contact change detected, CRC value=") + Stringize<DWORD>(crcValue).string())

      try {
        mDelegate->onConversationThreadContactsChanged(mThisWeak.lock());
      } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
        ZS_LOG_WARNING(Detail, log("conversation thread delegate gone"))
      }
    }
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  #pragma mark
  #pragma mark IConversationThread
  #pragma mark

  //-------------------------------------------------------------------------
  IConversationThreadPtr IConversationThread::create(
                                                     hookflash::IAccountPtr account,
                                                     ElementPtr profileBundleEl
                                                     )
  {
    return internal::ConversationThread::create(account, profileBundleEl);
  }

  //---------------------------------------------------------------------------
  const char *IConversationThread::toString(MessageDeliveryStates state)
  {
    switch (state) {
      case MessageDeliveryState_Discovering:      return "Discovering";
      case MessageDeliveryState_Delivered:        return "Delivered";
      case MessageDeliveryState_UserNotAvailable: return "User not available";
    }
    return "UNDEFINED";
  }

  //---------------------------------------------------------------------------
  const char *IConversationThread::toString(ContactStates state)
  {
    switch (state) {
      case ContactState_NotApplicable:  return "Not applicable";
      case ContactState_Finding:        return "Finding";
      case ContactState_Connected:      return "Connected";
      case ContactState_Disconnected:   return "Disconnected";
    }

    return "UNDEFINED";
  }
}
