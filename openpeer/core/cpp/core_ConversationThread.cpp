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


#include <openpeer/core/internal/core_ConversationThread.h>
#include <openpeer/core/internal/core_ConversationThreadHost.h>
#include <openpeer/core/internal/core_ConversationThreadSlave.h>
#include <openpeer/core/internal/core_Account.h>
#include <openpeer/core/internal/core_Contact.h>
#include <openpeer/core/internal/core_Call.h>
#include <openpeer/core/internal/core_Helper.h>
#include <openpeer/core/internal/core_Stack.h>

#include <openpeer/stack/IHelper.h>

#include <cryptopp/crc.h>

#include <zsLib/Stringize.h>
#include <zsLib/XML.h>
#include <zsLib/helpers.h>


namespace openpeer { namespace core { ZS_DECLARE_SUBSYSTEM(openpeer_core) } }

namespace openpeer
{
  namespace core
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef IConversationThreadParser::ThreadContactMap ThreadContactMap;
      typedef IConversationThreadParser::ThreadContactPtr ThreadContactPtr;
      typedef IConversationThreadParser::ThreadContactsPtr ThreadContactsPtr;
      typedef IConversationThreadParser::Message Message;
      typedef IConversationThreadParser::MessageMap MessageMap;

      typedef CryptoPP::CRC32 CRC32;

      //-----------------------------------------------------------------------
      static void convert(const ThreadContactMap &input, ContactList &output)
      {
        for (ThreadContactMap::const_iterator iter = input.begin(); iter != input.end(); ++iter)
        {
          const ThreadContactPtr &contact = (*iter).second;
          output.push_back(contact->contact());
        }
      }

      //-----------------------------------------------------------------------
      static void convert(const ThreadContactMap &input, ContactProfileInfoList &output)
      {
        for (ThreadContactMap::const_iterator iter = input.begin(); iter != input.end(); ++iter)
        {
          const ThreadContactPtr &contact = (*iter).second;
          ContactProfileInfo info;
          info.mContact = contact->contact();
          info.mProfileBundleEl = contact->profileBundleElement();
          output.push_back(info);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadHostSlaveBase
      #pragma mark

      //-----------------------------------------------------------------------
      String IConversationThreadHostSlaveBase::toDebugValueString(IConversationThreadHostSlaveBasePtr hostOrSlave, bool includeComaPrefix)
      {
        if (!hostOrSlave) return includeComaPrefix ? String(", host or slave=(null") : String("host or slave=(null)");
        if (hostOrSlave->isHost()) return ConversationThreadHost::toDebugString(hostOrSlave->toHost(), includeComaPrefix);
        return ConversationThreadSlave::toDebugString(hostOrSlave->toSlave(), includeComaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //------------------------------- ---------------------------------------
      #pragma mark
      #pragma mark IConversationThreadForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadPtr IConversationThreadForAccount::create(
                                                                  AccountPtr account,
                                                                  ILocationPtr peerLocation,
                                                                  IPublicationMetaDataPtr metaData,
                                                                  const SplitMap &split
                                                                  )
      {
        return IConversationThreadFactory::singleton().createConversationThread(account, peerLocation, metaData, split);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadForCall
      #pragma mark

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
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

      //-----------------------------------------------------------------------
      ConversationThread::ConversationThread(
                                             IMessageQueuePtr queue,
                                             AccountPtr account,
                                             const char *threadID
                                             ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mAccount(account),
        mDelegate(account->forConversationThread().getConversationThreadDelegate()),
        mThreadID(threadID ? String(threadID) : stack::IHelper::randomString(32)),
        mCurrentState(ConversationThreadState_Pending),
        mMustNotifyAboutNewThread(false),
        mHandleContactsChangedCRC(0)
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void ConversationThread::init()
      {
        ZS_LOG_DEBUG(log("initialized"))
        IConversationThreadAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      ConversationThread::~ConversationThread()
      {
        if(isNoop()) return;
        
        ZS_LOG_BASIC(log("destroyed"))
        mThisWeak.reset();
        cancel();
      }

      //-----------------------------------------------------------------------
      ConversationThreadPtr ConversationThread::convert(IConversationThreadPtr thread)
      {
        return boost::dynamic_pointer_cast<ConversationThread>(thread);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      String ConversationThread::toDebugString(IConversationThreadPtr thread, bool includeCommaPrefix)
      {
        if (!thread) return includeCommaPrefix ? String(", conversation thread=(null)") : String("conversation thread=(null)");
        return ConversationThread::convert(thread)->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      ConversationThreadPtr ConversationThread::create(
                                                       IAccountPtr inAccount,
                                                       ElementPtr profileElementEl
                                                       )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)

        AccountPtr account = Account::convert(inAccount);

        ConversationThreadPtr pThis(new ConversationThread(IStackForInternal::queueCore(), Account::convert(account), NULL));
        pThis->mThisWeak = pThis;
        pThis->init();

        ZS_LOG_DEBUG(pThis->log("created for API caller"))

        // add "ourself" to the contact list...
        ContactProfileInfoList contacts;
        ContactProfileInfo info;
        info.mContact = account->forConversationThread().getSelfContact();
        info.mProfileBundleEl = profileElementEl;
        contacts.push_back(info);

        pThis->addContacts(contacts);

        account->forConversationThread().notifyConversationThreadCreated(pThis); // safe to use mLock() weak pointer because account object must still be valid
        pThis->handleContactsChanged();
        return pThis;
      }

      //-----------------------------------------------------------------------
      ConversationThreadListPtr ConversationThread::getConversationThreads(IAccountPtr inAccount)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)

        AccountPtr account = Account::convert(inAccount);

        ConversationThreadListPtr result(new ConversationThreadList);
        account->forConversationThread().getConversationThreads(*result);

        return result;
      }

      //-----------------------------------------------------------------------
      ConversationThreadPtr ConversationThread::getConversationThreadByID(
                                                                          IAccountPtr inAccount,
                                                                          const char *threadID
                                                                          )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inAccount)
        ZS_THROW_INVALID_ARGUMENT_IF(!threadID)

        AccountPtr account = Account::convert(inAccount);

        return account->forConversationThread().getConversationThreadByID(threadID);
      }

      //-----------------------------------------------------------------------
      String ConversationThread::getThreadID() const
      {
        AutoRecursiveLock lock(getLock());
        return mThreadID;
      }

      //-----------------------------------------------------------------------
      bool ConversationThread::amIHost() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mLastOpenThread) return true;

        return mLastOpenThread->isHost();
      }

      //-----------------------------------------------------------------------
      IAccountPtr ConversationThread::getAssociatedAccount() const
      {
        AutoRecursiveLock lock(getLock());
        return mAccount.lock();
      }

      //-----------------------------------------------------------------------
      ContactListPtr ConversationThread::getContacts() const
      {
        AutoRecursiveLock lock(getLock());

        ContactListPtr result(new ContactList);

        if (!mLastOpenThread) {
          ZS_LOG_WARNING(Detail, log("no contacts have been added to this conversation thread"))
          return result;
        }

        ThreadContactMap contacts;
        mLastOpenThread->getContacts(contacts);

        internal::convert(contacts, *result);

        return result;
      }

      //-----------------------------------------------------------------------
      ElementPtr ConversationThread::getProfileBundle(IContactPtr contact) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!contact)

        AutoRecursiveLock lock(getLock());

        if (!mLastOpenThread) {
          ZS_LOG_WARNING(Detail, log("cannot get profile bundle no contacts have been added to this conversation thread"))
          return ElementPtr();
        }

        ThreadContactMap contacts;
        mLastOpenThread->getContacts(contacts);

        ThreadContactMap::iterator found = contacts.find(contact->getPeerURI());
        if (found == contacts.end()) {
          ZS_LOG_WARNING(Detail, log("cannot get profile bundle as contact was not found") + IContact::toDebugString(contact))
          return ElementPtr();
        }
        const ThreadContactPtr &threadContact = (*found).second;
        return threadContact->profileBundleElement();
      }

      //-----------------------------------------------------------------------
      IConversationThread::ContactStates ConversationThread::getContactState(IContactPtr inContact) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!inContact)

        ContactPtr contact = Contact::convert(inContact);

        AutoRecursiveLock lock(getLock());

        if (!mLastOpenThread) {
          ZS_LOG_WARNING(Detail, log("no conversation thread was ever openned"))
          return IConversationThread::ContactState_NotApplicable;
        }

        return mLastOpenThread->getContactState(contact);
      }

      //-----------------------------------------------------------------------
      void ConversationThread::addContacts(const ContactProfileInfoList &inputContacts)
      {
        ContactProfileInfoList contacts;
        for (ContactProfileInfoList::const_iterator iter = inputContacts.begin(); iter != inputContacts.end(); ++iter)
        {
          const ContactProfileInfo &sourceInfo = (*iter);

          ContactProfileInfo destInfo;
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

        ContactProfileInfoList newContacts = contacts;

        ThreadContactMap oldContacts;
        mLastOpenThread->getContacts(oldContacts);

        internal::convert(oldContacts, newContacts);

        if (mLastOpenThread->isHost()) {
          mLastOpenThread->toHost()->forConversationThread().close();
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

      //-----------------------------------------------------------------------
      void ConversationThread::removeContacts(const ContactList &contacts)
      {
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

        ThreadContactMap oldContacts;
        mLastOpenThread->getContacts(oldContacts);

        for (ContactList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
        {
          const IContactPtr &contact = (*iter);
          ThreadContactMap::iterator found = oldContacts.find(contact->getPeerURI());
          if (found != oldContacts.end()) {
            oldContacts.erase(found);
          }
        }

        if (mLastOpenThread->isHost()) {
          mLastOpenThread->toHost()->forConversationThread().close();
        }

        // not safe to add contacts, we need to create a newly open thread...
        mOpenThread = IConversationThreadHostForConversationThread::create(mThisWeak.lock());
        ZS_THROW_BAD_STATE_IF(!mOpenThread)
        mThreads[mOpenThread->getThreadID()] = mOpenThread;
        mLastOpenThread = mOpenThread;

        // convert the old contacts into a new contact list
        ContactProfileInfoList newContacts;
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

      //-----------------------------------------------------------------------
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

        AutoRecursiveLock lock(getLock());

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("account is gone thus unable to send the message") + ", message ID=" + messageID)
          return;
        }

        IPeerFilesPtr peerFiles = account->forConversationThread().getPeerFiles();
        if (!peerFiles) {
          ZS_LOG_WARNING(Detail, log("peer files are not generated thus unable to send the message") + ", message ID=" + messageID)
          return;
        }

        MessagePtr message = Message::create(messageID, account->forConversationThread().getSelfContact()->forConversationThread().getPeerURI(), messageType, body, zsLib::now(), peerFiles);
        if (!message) {
          ZS_LOG_ERROR(Detail, log("failed to create message object") + ", message ID=" + messageID)
          return;
        }

        mPendingDeliveryMessages.push_back(message);
        mMessageDeliveryStates[messageID] = IConversationThread::MessageDeliveryState_Discovering;

        step();
      }


      //-----------------------------------------------------------------------
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

        AccountPtr account = mAccount.lock();
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

        const String &peerURI = message->fromPeerURI();
        ContactPtr contact = account->forConversationThread().findContact(peerURI);
        if (!contact) {
          ZS_LOG_ERROR(Detail, log("unable to find the contact for the message") + ", message ID=" + messageID + ", peer URI=" + peerURI)
          return false;
        }

        outFrom = contact;
        outMessageType = message->mimeType();
        outMessage = message->body();
        outTime = message->sent();

        ZS_LOG_DEBUG(log("obtained message information") + ", message ID=" + messageID + ", peer URI=" + peerURI + IContact::toDebugString(contact) + ", type=" + outMessageType + ", message=" + outMessage + ", time=" + Stringize<Time>(outTime).string())
        return true;
      }

      //-----------------------------------------------------------------------
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

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThreadForAccount
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadPtr ConversationThread::create(
                                                       AccountPtr account,
                                                       ILocationPtr peerLocation,
                                                       IPublicationMetaDataPtr metaData,
                                                       const SplitMap &split
                                                       )
      {
        ConversationThreadPtr pThis(new ConversationThread(IStackForInternal::queueCore(), account, stack::IHelper::get(split, OPENPEER_CONVERSATION_THREAD_BASE_THREAD_ID_INDEX)));
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

      //-----------------------------------------------------------------------
      void ConversationThread::notifyPublicationUpdated(
                                                        ILocationPtr peerLocation,
                                                        IPublicationMetaDataPtr metaData,
                                                        const SplitMap &split
                                                        )
      {
        AutoRecursiveLock lock(getLock());

        String hostThreadID = stack::IHelper::get(split, OPENPEER_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX);
        ZS_THROW_INVALID_ARGUMENT_IF(hostThreadID.size() < 1)

        ThreadMap::iterator found = mThreads.find(hostThreadID);
        if (found == mThreads.end()) {
          // could not find the publication... must be a host document or something is wrong...
          String type = stack::IHelper::get(split, OPENPEER_CONVERSATION_THREAD_TYPE_INDEX);
          if (type != "host") {
            // whatever this is it cannot be understood...
            ZS_LOG_WARNING(Detail, log("expecting a host document type but received something else") + ", type=" + type + IPublicationMetaData::toDebugString(metaData))
            return;
          }

          ZS_LOG_DEBUG(log("creating a new conversation thread slave for updated publication") + ", host thread ID=" + hostThreadID + IPublicationMetaData::toDebugString(metaData))

          ConversationThreadSlavePtr slave = IConversationThreadSlaveForConversationThread::create(mThisWeak.lock(), peerLocation, metaData, split);
          if (!slave) {
            ZS_LOG_WARNING(Detail, log("slave was not created for host document") + IPublicationMetaData::toDebugString(metaData))
            return;
          }

          mThreads[hostThreadID] = slave;
        } else {
          ZS_LOG_DEBUG(log("reusing existing conversation thread slave for updated publication") + ", host thread ID=" + hostThreadID + IPublicationMetaData::toDebugString(metaData))
          IConversationThreadHostSlaveBasePtr &thread = (*found).second;
          thread->notifyPublicationUpdated(peerLocation, metaData, split);
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThread::notifyPublicationGone(
                                                     ILocationPtr peerLocation,
                                                     IPublicationMetaDataPtr metaData,
                                                     const SplitMap &split
                                                     )
      {
        AutoRecursiveLock lock(getLock());
        String hostThreadID = stack::IHelper::get(split, OPENPEER_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX);
        ZS_THROW_INVALID_ARGUMENT_IF(hostThreadID.size() < 1)

        ThreadMap::iterator found = mThreads.find(hostThreadID);
        if (found == mThreads.end()) {
          ZS_LOG_WARNING(Detail, log("notification about a publication that is gone for a conversation that does not exist") + IPublicationMetaData::toDebugString(metaData))
          return;
        }

        IConversationThreadHostSlaveBasePtr &thread = (*found).second;
        thread->notifyPublicationGone(peerLocation, metaData, split);
      }

      //-----------------------------------------------------------------------
      void ConversationThread::notifyPeerDisconnected(ILocationPtr peerLocation)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("peer disconnected notification received") + ILocation::toDebugString(peerLocation))

        for (ThreadMap::iterator iter = mThreads.begin(); iter != mThreads.end(); ++iter)
        {
          IConversationThreadHostSlaveBasePtr &thread = (*iter).second;
          thread->notifyPeerDisconnected(peerLocation);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThreadForHostOrSlave
      #pragma mark

      //-----------------------------------------------------------------------
      AccountPtr ConversationThread::getAccount() const
      {
        AutoRecursiveLock lock(getLock());
        return mAccount.lock();
      }

      //-----------------------------------------------------------------------
      IPublicationRepositoryPtr ConversationThread::getRepository() const
      {
        AutoRecursiveLock lock(getLock());
        AccountPtr account = mAccount.lock();
        if (!account) return IPublicationRepositoryPtr();
        return account->forConversationThread().getRepository();
      }

      //-----------------------------------------------------------------------
      void ConversationThread::notifyStateChanged(IConversationThreadHostSlaveBasePtr thread)
      {
        AutoRecursiveLock lock(getLock());
        step();
        handleContactsChanged();
      }

      //-----------------------------------------------------------------------
      void ConversationThread::notifyContactState(
                                                  IConversationThreadHostSlaveBasePtr thread,
                                                  ContactPtr contact,
                                                  ContactStates state
                                                  )
      {
        AutoRecursiveLock lock(getLock());

        if (mLastOpenThread != thread) {
          ZS_LOG_TRACE(log("will not notify about contact states if not the last opened thread") + ", from host/slave thead ID=" + thread->getThreadID() + IContact::toDebugString(contact) + ", reported state=" + IConversationThread::toString(state))
          return;
        }

        bool changed = false;
        ContactStates lastState = IConversationThread::ContactState_NotApplicable;

        ContactStateMap::iterator found = mLastReportedContactStates.find(contact->forConversationThread().getPeerURI());
        if (found != mLastReportedContactStates.end()) {
          ContactStatePair &statePair = (*found).second;
          lastState = statePair.second;
          changed = (lastState != state);
        } else {
          changed = true;
        }

        if (!changed) return;

        ZS_LOG_DEBUG(log("contact state changed") + ", old state=" + IConversationThread::toString(lastState) + ", new state=" + IConversationThread::toString(state) + IContact::toDebugString(contact))

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("conversation thread delegate not found"))
          return;
        }

        // remember the last reported state so it isn't repeated
        mLastReportedContactStates[contact->forConversationThread().getPeerURI()] = ContactStatePair(contact, state);

        try {
          mDelegate->onConversationThreadContactStateChanged(mThisWeak.lock(), contact, state);
        } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("conversation thread delegate gone"))
        }
      }

      //-----------------------------------------------------------------------
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
          ZS_LOG_DEBUG(log("message received already delivered to delegate (thus ignoring)") + message->getDebugValueString())
          return;
        }

        // remember that this message is received
        mReceivedOrPushedMessages[message->messageID()] = message;
        ZS_LOG_DEBUG(log("message received and is being delivered to delegate") + message->getDebugValueString())

        try {
          mDelegate->onConversationThreadMessage(mThisWeak.lock(), message->messageID());
        } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("delegate is gone"))
        }
      }

      //-----------------------------------------------------------------------
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

      //-----------------------------------------------------------------------
      void ConversationThread::notifyMessagePush(
                                                 MessagePtr message,
                                                 ContactPtr toContact
                                                 )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!message)
        ZS_THROW_INVALID_ARGUMENT_IF(!toContact)

        AutoRecursiveLock lock(getLock());

        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_WARNING(Detail, log("ignoring message push notification received while shutdown") + message->getDebugValueString())
          return;
        }

        MessageReceivedMap::iterator found = mReceivedOrPushedMessages.find(message->messageID());
        if (found == mReceivedOrPushedMessages.end()) {
          mReceivedOrPushedMessages[message->messageID()] = message;
        }

        try {
          ZS_LOG_DEBUG(log("requesting push notification for conversation thread message") + message->getDebugValueString() + IContact::toDebugString(toContact))
          mDelegate->onConversationThreadPushMessage(mThisWeak.lock(), message->messageID(), toContact);
        } catch(IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
          ZS_LOG_WARNING(Detail, log("unable to push message as delegate was gone"))
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThread::requestAddIncomingCallHandler(
                                                             const char *dialogID,
                                                             IConversationThreadHostSlaveBasePtr hostOrSlaveThread,
                                                             CallPtr newCall
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

      //-----------------------------------------------------------------------
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
        ZS_LOG_DEBUG(log("incoming call handler removed") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
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
        CallPtr &call = handlerPair.second;

        call->forConversationThread().notifyConversationThreadUpdated();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThreadForHost
      #pragma mark

      //-----------------------------------------------------------------------
      bool ConversationThread::inConversation(ContactPtr contact) const
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!contact)
        AutoRecursiveLock lock(getLock());
        if (!mLastOpenThread) {
          return false;
        }
        return mLastOpenThread->inConversation(contact);
      }

      //-----------------------------------------------------------------------
      void ConversationThread::removeContacts(const ContactURIList &contacts)
      {
        if (contacts.size() < 1) return;
        AutoRecursiveLock lock(getLock());

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("account is gone"))
          return;
        }

        ContactList contactList;
        for (ContactURIList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
        {
          const String &peerURI = (*iter);
          ContactPtr contact = account->forConversationThread().findContact(peerURI);
          if (!contact) {
            ZS_LOG_WARNING(Detail, log("could not find peer URI in contact list") + ", peer URI=" + peerURI)
            continue;
          }

          ZS_LOG_DEBUG(log("need to remove contact") + IContact::toDebugString(contact))
          contactList.push_back(contact);
        }

        if (contactList.size() < 1) {
          ZS_LOG_DEBUG(log("no contacts found needing to be removed"))
          return;
        }

        removeContacts(contactList);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThreadForSlave
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThread::notifyAboutNewThreadNowIfNotNotified(ConversationThreadSlavePtr slave)
      {
        AutoRecursiveLock lock(getLock());
        if (!mMustNotifyAboutNewThread) {
          ZS_LOG_DEBUG(log("no need to notify about thread creation as it is already created"))
          return;
        }

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("unable to notify about new thread as account object is gone"))
          return;
        }

        ZS_LOG_DEBUG(log("notifying that new conversation thread is now created"))

        if (!mLastOpenThread) {
          ZS_LOG_DEBUG(log("slave has now become last open thread"))
          mLastOpenThread = slave;
        }

        account->forConversationThread().notifyConversationThreadCreated(mThisWeak.lock());
        mMustNotifyAboutNewThread = false;

        handleLastOpenThreadChanged();
      }

      //-----------------------------------------------------------------------
      void ConversationThread::convertSlaveToClosedHost(
                                                        ConversationThreadSlavePtr slave,
                                                        ThreadPtr originalHost,
                                                        ThreadPtr originalSlave
                                                        )
      {
        AutoRecursiveLock lock(getLock());

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("unable to convert slave to closed host as basee thread is shutting down/shutdown") + ConversationThreadSlave::toDebugString(slave))
          return;
        }

        ZS_LOG_DEBUG(log("converting slave to closed host") + ", slave thread ID=" + ConversationThreadSlave::toDebugString(slave))

        if (mOpenThread == slave) {
          ZS_LOG_DETAIL(log("slave thread is no longer considered 'open'") + ConversationThreadSlave::toDebugString(slave))
          mOpenThread.reset();
        }

        ThreadContactsPtr contacts = originalHost->contacts();

        const ThreadContactMap &originalContacts = contacts->contacts();

        ConversationThreadHostPtr newClosedHost = IConversationThreadHostForConversationThread::create(mThisWeak.lock(), IConversationThreadParser::Details::ConversationThreadState_Closed);
        ZS_THROW_BAD_STATE_IF(!newClosedHost)
        mThreads[newClosedHost->forConversationThread().getThreadID()] = newClosedHost;

        ZS_LOG_DEBUG(log("new closed host created for slave") + ", slave: " + ConversationThreadSlave::toDebugString(slave, false) + ", new closed thread ID: " + ConversationThreadHost::toDebugString(newClosedHost))

        ContactProfileInfoList newContacts;
        internal::convert(originalContacts, newContacts);

        newClosedHost->forConversationThread().addContacts(newContacts);

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
            ZS_LOG_TRACE(log("igoring host message as message as already added") + message->getDebugValueString())
            continue;
          }
          ZS_LOG_TRACE(log("add host message to new closed host thread") + message->getDebugValueString())
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
            ZS_LOG_TRACE(log("igoring slave message as message as already added") + message->getDebugValueString())
            continue;
          }
          ZS_LOG_TRACE(log("add slave message to new closed host thread") + message->getDebugValueString())
          messages.push_back(message);
          messagesAsMap[message->messageID()] = message;
        }

        ZS_THROW_BAD_STATE_IF(messages.size() < 1)

        ZS_LOG_DEBUG(log("requesting to deliver messages to closed thread"))
        newClosedHost->forConversationThread().sendMessages(messages);

        // force a step on the conversation thread to cleanup anything state wise...
        ZS_LOG_DEBUG(log("forcing step"))
        IConversationThreadAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThreadForCall
      #pragma mark

      //-----------------------------------------------------------------------
      bool ConversationThread::placeCall(CallPtr call)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!call)

        AutoRecursiveLock lock(getLock());

        if ((isShuttingDown()) ||
            (isShutdown())) {
          return false;
        }

        ZS_LOG_DEBUG(log("adding all to pending list") + Call::toDebugString(call))

        mPendingCalls[call->forConversationThread().getCallID()] = call;

        ZS_LOG_DEBUG(log("forcing step"))
        IConversationThreadAsyncProxy::create(mThisWeak.lock())->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      void ConversationThread::notifyCallStateChanged(CallPtr call)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!call)

        AutoRecursiveLock lock(getLock());

        CallHandlerMap::iterator found = mCallHandlers.find(call->forConversationThread().getCallID());
        if (found == mCallHandlers.end()) {
          ZS_LOG_DEBUG(log("call is not known yet to the conversation thread") + Call::toDebugString(call))
          return;
        }
        IConversationThreadHostSlaveBasePtr &thread = (*found).second.first;
        thread->notifyCallStateChanged(call);
      }

      //-----------------------------------------------------------------------
      void ConversationThread::notifyCallCleanup(CallPtr call)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!call)

        AutoRecursiveLock lock(getLock());

        PendingCallMap::iterator foundPending = mPendingCalls.find(call->forConversationThread().getCallID());
        if (foundPending != mPendingCalls.end()) {
          ZS_LOG_DEBUG(log("call found on pending list thus removing") + Call::toDebugString(call))
          mPendingCalls.erase(foundPending);
          ZS_LOG_DEBUG(log("call found on pending list removed") + getDebugValueString())
        }

        CallHandlerMap::iterator found = mCallHandlers.find(call->forConversationThread().getCallID());
        if (found == mCallHandlers.end()) {
          ZS_LOG_WARNING(Detail, log("did not find any call handlers for this call") + Call::toDebugString(call))
          return;
        }

        ZS_LOG_DEBUG(log("call found and is being removed") + Call::toDebugString(call))
        IConversationThreadHostSlaveBasePtr &thread = (*found).second.first;
        thread->notifyCallCleanup(call);
        mCallHandlers.erase(found);

        ZS_LOG_DEBUG(log("call handler removed") + getDebugValueString())
      }

      //-----------------------------------------------------------------------
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

        ZS_LOG_DEBUG(log("requesting replies from thread") + ", call ID=" + callID + IConversationThreadHostSlaveBase::toDebugValueString(thread))
        thread->gatherDialogReplies(callID, outDialogs);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String ConversationThread::log(const char *message) const
      {
        return String("ConversationThread [") + Stringize<typeof(mID)>(mID).string() + "] " + message + ", thread ID=" + mThreadID;
      }

      //-----------------------------------------------------------------------
      String ConversationThread::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("thread id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("thread id (s)", mThreadID, firstTime) +
               Helper::getDebugValue("current state", toString(mCurrentState), firstTime) +
               Helper::getDebugValue("must notify", mMustNotifyAboutNewThread ? String("true") : String(), firstTime) +
               IConversationThreadHostSlaveBase::toDebugValueString(mOpenThread) +
               IConversationThreadHostSlaveBase::toDebugValueString(mLastOpenThread) +
               IConversationThreadHostSlaveBase::toDebugValueString(mHandleThreadChanged) +
               Helper::getDebugValue("crc", 0 != mHandleContactsChangedCRC ? Stringize<typeof(mHandleContactsChangedCRC)>(mHandleContactsChangedCRC).string() : String(), firstTime) +
               Helper::getDebugValue("threads", mThreads.size() > 0 ? Stringize<size_t>(mThreads.size()).string() : String(), firstTime) +
               Helper::getDebugValue("received or pushed", mReceivedOrPushedMessages.size() > 0 ? Stringize<size_t>(mReceivedOrPushedMessages.size()).string() : String(), firstTime) +
               Helper::getDebugValue("delivery states", mMessageDeliveryStates.size() > 0 ? Stringize<size_t>(mMessageDeliveryStates.size()).string() : String(), firstTime) +
               Helper::getDebugValue("pending delivery", mPendingDeliveryMessages.size() > 0 ? Stringize<size_t>(mPendingDeliveryMessages.size()).string() : String(), firstTime) +
               Helper::getDebugValue("pending calls", mPendingCalls.size() > 0 ? Stringize<size_t>(mPendingCalls.size()).string() : String(), firstTime) +
               Helper::getDebugValue("call handlers", mCallHandlers.size() > 0 ? Stringize<size_t>(mCallHandlers.size()).string() : String(), firstTime) +
               Helper::getDebugValue("last reported", mLastReportedContactStates.size() > 0 ? Stringize<size_t>(mLastReportedContactStates.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &ConversationThread::getLock() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) return mBogusLock;
        return account->forConversationThread().getLock();
      }

      //-----------------------------------------------------------------------
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

      //-----------------------------------------------------------------------
      void ConversationThread::step()
      {
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
                ZS_LOG_DEBUG(log("thread found is the most recent (thus chosing)") + IConversationThreadHostSlaveBase::toDebugValueString(thread))
                mostRecentOpen = thread;
                mostRecentOpenTime = thread->getHostCreationTime();
              } else {
                ZS_LOG_DEBUG(log("thread found is older than the most recent") + IConversationThreadHostSlaveBase::toDebugValueString(thread))
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
                ZS_LOG_DEBUG(log("due to more than one thread being open host is being closed") + IConversationThreadHostSlaveBase::toDebugValueString(thread))
                // close the thread...
                thread->toHost()->forConversationThread().close();
              }
            }
          }
        }

        if (mostRecentOpen) {
          // remember which thread is open now...
          ZS_LOG_DEBUG(log("determined which thread is the most recent open") + IConversationThreadHostSlaveBase::toDebugValueString(mostRecentOpen))
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

            ZS_LOG_DEBUG(log("no thread found to be open thus creating a new host") + IConversationThreadHostSlaveBase::toDebugValueString(mOpenThread))
            mThreads[mOpenThread->getThreadID()] = mOpenThread;

            if (mLastOpenThread) {
              // make sure the contacts from the last open thread are carried over...
              ThreadContactMap contacts;
              mLastOpenThread->getContacts(contacts);

              ZS_LOG_DEBUG(log("contacts from last open thread are being brought into new thread") + ", total contacts=" + Stringize<size_t>(contacts.size()).string())

              ContactProfileInfoList addContacts;
              internal::convert(contacts, addContacts);

              mOpenThread->addContacts(addContacts);
            }

            mLastOpenThread = mOpenThread;
          }
        }

        if (mOpenThread) {
          ZS_LOG_DEBUG(log("thread has open thread") + IConversationThreadHostSlaveBase::toDebugValueString(mOpenThread))
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
                CallPtr &call = (*iter).second;
                ZS_LOG_DEBUG(log("call placed and now handled via open thread") + Call::toDebugString(call))
                mCallHandlers[callID] = CallHandlerPair(mOpenThread, call);

                // nudge the call to tell it state has changed...
                call->forConversationThread().notifyConversationThreadUpdated();
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

      //-----------------------------------------------------------------------
      void ConversationThread::setState(ConversationThreadStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;
      }

      //-----------------------------------------------------------------------
      void ConversationThread::handleLastOpenThreadChanged()
      {
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

        ThreadContactMap contacts;
        mLastOpenThread->getContacts(contacts);

        for (ThreadContactMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
          const ThreadContactPtr &threadContact = (*iter).second;
          ContactPtr contact = threadContact->contact();
          ContactStates state = mLastOpenThread->getContactState(contact);

          bool changed = false;

          ContactStateMap::iterator found = mLastReportedContactStates.find(contact->forConversationThread().getPeerURI());
          if (found != mLastReportedContactStates.end()) {
            ContactStatePair &statePair = (*found).second;
            if (statePair.second != state) {
              statePair.second = state;
              changed = true;
            }
          } else {
            mLastReportedContactStates[contact->forConversationThread().getPeerURI()] = ContactStatePair(contact, state);
            changed = true;
          }

          try {
            ZS_LOG_DEBUG(log("notifying of contact state changed") + ", state=" + IConversationThread::toString(state) + IContact::toDebugString(contact))
            mDelegate->onConversationThreadContactStateChanged(mThisWeak.lock(), contact, state);
          } catch (IConversationThreadDelegateProxy::Exceptions::DelegateGone &) {
            ZS_LOG_WARNING(Detail, log("conversation thread delegate gone"))
          }
        }

        handleContactsChanged();
      }

      //-----------------------------------------------------------------------
      void ConversationThread::handleContactsChanged()
      {
        if (!mLastOpenThread) {
          ZS_LOG_WARNING(Detail, log("last open thread has become NULL"))
          return;
        }

        if (!mDelegate) {
          ZS_LOG_WARNING(Detail, log("delegate is NULL"))
          return;
        }

        DWORD crcValue = 0;

        ThreadContactMap contacts;
        mLastOpenThread->getContacts(contacts);

        CRC32 crc;
        for (ThreadContactMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter) {
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

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThread
    #pragma mark

    //-------------------------------------------------------------------------
    const char *IConversationThread::toString(MessageDeliveryStates state)
    {
      switch (state) {
        case MessageDeliveryState_Discovering:      return "Discovering";
        case MessageDeliveryState_Delivered:        return "Delivered";
        case MessageDeliveryState_UserNotAvailable: return "User not available";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
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

    //-----------------------------------------------------------------------
    String IConversationThread::toDebugString(IConversationThreadPtr thread, bool includeCommaPrefix)
    {
      return internal::ConversationThread::toDebugString(thread, includeCommaPrefix);
    }

    //-----------------------------------------------------------------------
    IConversationThreadPtr IConversationThread::create(
                                                       IAccountPtr account,
                                                       ElementPtr profileBundleEl
                                                       )
    {
      return internal::IConversationThreadFactory::singleton().createConversationThread(account, profileBundleEl);
    }

    //-----------------------------------------------------------------------
    ConversationThreadListPtr IConversationThread::getConversationThreads(IAccountPtr account)
    {
      return internal::ConversationThread::getConversationThreads(account);
    }

    //-----------------------------------------------------------------------
    IConversationThreadPtr IConversationThread::getConversationThreadByID(
                                                                          IAccountPtr account,
                                                                          const char *threadID
                                                                          )
    {
      return internal::ConversationThread::getConversationThreadByID(account, threadID);
    }
  }
}
