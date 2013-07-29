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


#include <openpeer/core/internal/core_ConversationThreadHost.h>
#include <openpeer/core/internal/core_ConversationThread.h>
#include <openpeer/core/internal/core_Account.h>
#include <openpeer/core/internal/core_Contact.h>
#include <openpeer/core/internal/core_Call.h>
#include <openpeer/core/internal/core_Helper.h>
#include <openpeer/core/internal/core_Stack.h>

#include <openpeer/stack/IPublication.h>
#include <openpeer/stack/IHelper.h>

#include <openpeer/services/IHelper.h>

#include <zsLib/Stringize.h>
#include <zsLib/helpers.h>

namespace openpeer { namespace core { ZS_DECLARE_SUBSYSTEM(openpeer_core) } }


namespace openpeer
{
  namespace core
  {
    namespace internal
    {
      using zsLib::Stringize;

      typedef IConversationThreadParser::Thread Thread;
      typedef IConversationThreadParser::ThreadPtr ThreadPtr;
      typedef IConversationThreadParser::ThreadContact ThreadContact;
      typedef IConversationThreadParser::ThreadContactPtr ThreadContactPtr;
      typedef IConversationThreadParser::DialogList DialogList;
      typedef IConversationThreadParser::DialogMap DialogMap;
      typedef IConversationThreadParser::DialogIDList DialogIDList;
      typedef IConversationThreadParser::MessageReceiptMap MessageReceiptMap;
      typedef IConversationThreadParser::MessageMap MessageMap;
      

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadHostForConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadHostPtr IConversationThreadHostForConversationThread::create(
                                                                                     ConversationThreadPtr baseThread,
                                                                                     IConversationThreadParser::Details::ConversationThreadStates state
                                                                                     )
      {
        return IConversationThreadHostFactory::singleton().createConversationThreadHost(baseThread, state);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost
      #pragma mark

      //-----------------------------------------------------------------------
      const char *ConversationThreadHost::toString(ConversationThreadHostStates state)
      {
        switch (state)
        {
          case ConversationThreadHostState_Pending:       return "Pending";
          case ConversationThreadHostState_Ready:         return "Ready";
          case ConversationThreadHostState_ShuttingDown:  return "Shutting down";
          case ConversationThreadHostState_Shutdown:      return "Shutdown";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      ConversationThreadHost::ConversationThreadHost(
                                                     IMessageQueuePtr queue,
                                                     AccountPtr account,
                                                     ConversationThreadPtr baseThread,
                                                     const char *threadID
                                                     ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mThreadID(threadID ? String(threadID) : services::IHelper::randomString(32)),
        mBaseThread(baseThread),
        mCurrentState(ConversationThreadHostState_Pending),
        mAccount(account),
        mSelfContact(account->forConversationThread().getSelfContact())
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::init(IConversationThreadParser::Details::ConversationThreadStates state)
      {
        AccountPtr account = mAccount.lock();
        ZS_THROW_INVALID_ASSUMPTION_IF(!account)

        AutoRecursiveLock lock(account->forConversationThread().getLock());

        ConversationThreadPtr baseThread = mBaseThread.lock();

        mHostThread = Thread::create(account, Thread::ThreadType_Host, account->forConversationThread().getSelfLocation(), baseThread->forHostOrSlave().getThreadID(), mThreadID, "", "", state);
        ZS_THROW_BAD_STATE_IF(!mHostThread)

        publish(true, true);

        step();
      }

      //-----------------------------------------------------------------------
      ConversationThreadHost::~ConversationThreadHost()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::toDebugString(ConversationThreadHostPtr host, bool includeCommaPrefix)
      {
        if (!host) return includeCommaPrefix ? String(", host thread=(null)") : String("host thread=(null)");
        return host->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => IConversationThreadHostSlaveBase
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadHostPtr ConversationThreadHost::create(
                                                               ConversationThreadPtr baseThread,
                                                               IConversationThreadParser::Details::ConversationThreadStates state
                                                               )
      {
        AccountPtr account = baseThread->forHostOrSlave().getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, "unable to create a new conversation thread object as account object is null")
          return ConversationThreadHostPtr();
        }

        ConversationThreadHostPtr pThis(new ConversationThreadHost(IStackForInternal::queueCore(), account, baseThread, NULL));
        pThis->mThisWeak = pThis;
        pThis->init(state);
        return pThis;
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::getThreadID() const
      {
        AutoRecursiveLock lock(getLock());
        return mThreadID;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::shutdown()
      {
        AutoRecursiveLock lock(getLock());
        cancel();
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadHost::isHostThreadOpen() const
      {
        typedef IConversationThreadParser::Details Details;

        if (!mHostThread) return false;
        if (!mHostThread->details()) return false;
        return Details::ConversationThreadState_Open == mHostThread->details()->state();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyPublicationUpdated(
                                                            ILocationPtr peerLocation,
                                                            IPublicationMetaDataPtr metaData,
                                                            const SplitMap &split
                                                            )
      {
        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_WARNING(Detail, log("notification of an updated document after shutdown") + IPublicationMetaData::toDebugString(metaData))
          return;
        }

        PeerContactPtr peerContact = findContact(peerLocation);
        if (!peerContact) {
          ZS_LOG_WARNING(Detail, log("failed to notify of an updated publication because unable to find peer contact") + IPublicationMetaData::toDebugString(metaData))
          return;
        }

        peerContact->notifyPublicationUpdated(peerLocation, metaData, split);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyPublicationGone(
                                                         ILocationPtr peerLocation,
                                                         IPublicationMetaDataPtr metaData,
                                                         const SplitMap &split
                                                         )
      {
        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("notification of a document gone after shutdown") + IPublicationMetaData::toDebugString(metaData))
          return;
        }

        PeerContactPtr peerContact = findContact(peerLocation);
        if (!peerContact) {
          ZS_LOG_WARNING(Detail, log("failed to notify of a publication being gone because unable to find peer contact") + IPublicationMetaData::toDebugString(metaData))
          return;
        }

        peerContact->notifyPublicationGone(peerLocation, metaData, split);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyPeerDisconnected(ILocationPtr peerLocation)
      {
        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("notification of a peer shutdown") + ILocation::toDebugString(peerLocation))
          return;
        }
        PeerContactPtr peerContact = findContact(peerLocation);

        if (!peerContact) {
          ZS_LOG_WARNING(Detail, log("failed to notify of a peer disconnection because unable to find peer contact"))
          return;
        }

        peerContact->notifyPeerDisconnected(peerLocation);
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadHost::sendMessages(const MessageList &messages)
      {
        AutoRecursiveLock lock(getLock());

        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("unable to send messages to a thread that is shutdown"))
          return false;
        }

        ZS_THROW_BAD_STATE_IF(!mHostThread)

        mHostThread->updateBegin();
        mHostThread->addMessages(messages);
        publish(mHostThread->updateEnd(), false);

        step();
        return true;
      }

      //-----------------------------------------------------------------------
      Time ConversationThreadHost::getHostCreationTime() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mHostThread) return Time();
        return mHostThread->details()->created();
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadHost::safeToChangeContacts() const
      {
        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("cannot change contacts during shutdown"))
          return false;
        }

        ZS_THROW_INVALID_ASSUMPTION_IF(!mHostThread)

        if (!isHostThreadOpen()) {
          if (mHostThread->contacts()->contacts().size() > 0) {
            ZS_LOG_DEBUG(log("not safe to add contacts since host thread is not open"))
            return false;
          }
        }

        if (mHostThread->messages().size() > 0) {
          ZS_LOG_DEBUG(log("not safe to add messages with outstanding messages"))
          return false;
        }

        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        // HERE - check dialog state?

        return true;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::getContacts(ThreadContactMap &outContacts) const
      {
        AutoRecursiveLock lock(getLock());
        if (!mHostThread) {
          ZS_LOG_DEBUG(log("cannot get contacts without a host thread"))
          return;
        }

        outContacts = mHostThread->contacts()->contacts();
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadHost::inConversation(ContactPtr contact) const
      {
        AutoRecursiveLock lock(getLock());
        if (!mHostThread) {
          ZS_LOG_DEBUG(log("cannot check if contact is in conversation without a host thread"))
          return false;
        }

        const ThreadContactMap &contacts = mHostThread->contacts()->contacts();
        ThreadContactMap::const_iterator found = contacts.find(contact->forConversationThread().getPeerURI());
        return found != contacts.end();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::addContacts(const ContactProfileInfoList &contacts)
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_ASSUMPTION_IF(!safeToChangeContacts())

        bool foundSelf = false;

        const ThreadContactMap &oldContacts = mHostThread->contacts()->contacts();

        ThreadContactMap contactMap = oldContacts;

        for (ContactProfileInfoList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
        {
          const ContactProfileInfo &info = (*iter);

          ContactPtr contact = Contact::convert(info.mContact);
          if (contact->forConversationThread().getPeerURI() == mSelfContact->forConversationThread().getPeerURI()) {
            ThreadContactPtr threadContact = ThreadContact::create(mSelfContact, info.mProfileBundleEl);

            // do not allow self to become a 'PeerContact'
            foundSelf = true;

            // make sure we are on the final list though...
            contactMap[mSelfContact->forConversationThread().getPeerURI()] = threadContact;
            continue;
          }

          ThreadContactPtr threadContact = ThreadContact::create(contact, info.mProfileBundleEl);

          // remember this contact as needing to be added to the final list...
          contactMap[contact->forConversationThread().getPeerURI()] = threadContact;

          PeerContactMap::iterator found = mPeerContacts.find(contact->forConversationThread().getPeerURI());
          if (found == mPeerContacts.end()) {
            PeerContactPtr peerContact = PeerContact::create(getAssociatedMessageQueue(), mThisWeak.lock(), contact, info.mProfileBundleEl);
            if (peerContact) {
              mPeerContacts[contact->forConversationThread().getPeerURI()] = peerContact;
            }
          }
        }

        // publish changes...
        mHostThread->updateBegin();
        mHostThread->setContacts(contactMap);
        publish(mHostThread->updateEnd(), true);

        IConversationThreadHostAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::removeContacts(const ContactList &contacts)
      {
        AutoRecursiveLock lock(getLock());

        ZS_THROW_INVALID_ASSUMPTION_IF(!safeToChangeContacts())

        const ThreadContactMap &oldContacts = mHostThread->contacts()->contacts();

        ThreadContactMap contactMap = oldContacts;

        // get ride of all those on the list that need to be removed...
        for (ContactList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
        {
          ContactPtr contact = Contact::convert(*iter);
          ThreadContactMap::iterator found = contactMap.find(contact->forConversationThread().getPeerURI());
          if (found == contactMap.end()) continue;

          contactMap.erase(found);
        }

        mHostThread->updateBegin();
        mHostThread->setContacts(contactMap);
        publish(mHostThread->updateEnd(), true);

        IConversationThreadHostAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      IConversationThread::ContactStates ConversationThreadHost::getContactState(ContactPtr contact) const
      {
        AutoRecursiveLock lock(getLock());

        PeerContactMap::const_iterator found = mPeerContacts.find(contact->forConversationThread().getPeerURI());
        if (found == mPeerContacts.end()) {
          ZS_LOG_WARNING(Detail, log("contact was not found as part of the conversation") + IContact::toDebugString(contact))
          return IConversationThread::ContactState_NotApplicable;
        }

        const PeerContactPtr &peerContact = (*found).second;
        return peerContact->getContactState();
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadHost::placeCalls(const PendingCallMap &pendingCalls)
      {
        if (pendingCalls.size() < 1) {
          ZS_LOG_WARNING(Detail, log("requiest to place calls but no calls to place"))
          return false;
        }

        AutoRecursiveLock lock(getLock());
        if (!mHostThread) {
          ZS_LOG_DEBUG(log("no host thread to clean call from..."))
          return false;
        }

        DialogList additions;

        const DialogMap &dialogs = mHostThread->dialogs();
        for (PendingCallMap::const_iterator iter = pendingCalls.begin(); iter != pendingCalls.end(); ++iter)
        {
          const CallPtr &call = (*iter).second;
          DialogMap::const_iterator found = dialogs.find(call->forConversationThread().getCallID());

          if (found == dialogs.end()) {
            ZS_LOG_DEBUG(log("adding call") +
                              ", call ID=" + call->forConversationThread().getCallID() +
                              ", caller peer URI=" + call->forConversationThread().getCaller()->forConversationThread().getPeerURI() + (call->forConversationThread().getCaller()->forConversationThread().isSelf() ? " (self)" : "") +
                              ", callee peer URI=" + call->forConversationThread().getCallee()->forConversationThread().getPeerURI() + (call->forConversationThread().getCallee()->forConversationThread().isSelf() ? " (self)" : ""))

            additions.push_back(call->forConversationThread().getDialog());
          }
        }

        // publish the changes now...
        mHostThread->updateBegin();
        mHostThread->updateDialogs(additions);
        publish(mHostThread->updateEnd(), true);

        IConversationThreadHostAsyncProxy::create(mThisWeak.lock())->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyCallStateChanged(CallPtr call)
      {
        AutoRecursiveLock lock(getLock());

        if (!mHostThread) {
          ZS_LOG_DEBUG(log("no host thread to clean call from..."))
          return;
        }

        DialogPtr dialog = call->forConversationThread().getDialog();
        if (!dialog) {
          ZS_LOG_DEBUG(log("call does not have a dialog yet and is not ready"))
          return;
        }

        DialogList updates;
        updates.push_back(dialog);

        // publish the changes now...
        mHostThread->updateBegin();
        mHostThread->updateDialogs(updates);
        publish(mHostThread->updateEnd(), true);

        IConversationThreadHostAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyCallCleanup(CallPtr call)
      {
        ZS_LOG_DEBUG(log("call cleanup called") + ", call ID=" + ICall::toDebugString(call))

        AutoRecursiveLock lock(getLock());

        if (!mHostThread) {
          ZS_LOG_DEBUG(log("no host thread to clean call from..."))
          return;
        }

        const DialogMap &dialogs = mHostThread->dialogs();
        DialogMap::const_iterator found = dialogs.find(call->forConversationThread().getCallID());

        if (found == dialogs.end()) {
          ZS_LOG_WARNING(Detail, log("this call is not present on the host conversation thread to remove"))
          return;
        }

        DialogIDList removals;
        removals.push_back(call->forConversationThread().getCallID());

        // publish the changes now...
        mHostThread->updateBegin();
        mHostThread->removeDialogs(removals);
        publish(mHostThread->updateEnd(), true);

        IConversationThreadHostAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::gatherDialogReplies(
                                                       const char *callID,
                                                       LocationDialogMap &outDialogs
                                                       ) const
      {
        AutoRecursiveLock lock(getLock());
        for (PeerContactMap::const_iterator iter = mPeerContacts.begin(); iter != mPeerContacts.end(); ++iter)
        {
          const PeerContactPtr &peerContact = (*iter).second;
          peerContact->gatherDialogReplies(callID, outDialogs);
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => IConversationThreadHostForConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThreadHost::close()
      {
        typedef IConversationThreadParser::Details Details;

        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("cannot close as already shutting down"))
          return;
        }

        ZS_THROW_BAD_STATE_IF(!mHostThread)

        mHostThread->updateBegin();
        mHostThread->setState(Details::ConversationThreadState_Closed);
        publish(mHostThread->updateEnd(), false);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => friend PeerContact
      #pragma mark

      //-----------------------------------------------------------------------
      ThreadPtr ConversationThreadHost::getHostThread() const
      {
        AutoRecursiveLock lock(getLock());
        return mHostThread;
      }

      //-----------------------------------------------------------------------
      AccountPtr ConversationThreadHost::getAccount() const
      {
        AutoRecursiveLock lock(getLock());
        return mAccount.lock();
      }

      //-----------------------------------------------------------------------
      IPublicationRepositoryPtr ConversationThreadHost::getRepository() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("account is gone thus cannot get stack repository"))
          return IPublicationRepositoryPtr();
        }

        return account->forConversationThread().getRepository();
      }

      //-----------------------------------------------------------------------
      ConversationThreadPtr ConversationThreadHost::getBaseThread() const
      {
        AutoRecursiveLock lock(getLock());
        return mBaseThread.lock();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyMessagesReceived(const MessageList &messages)
      {
        AutoRecursiveLock lock(getLock());

        ConversationThreadPtr baseThread = mBaseThread.lock();
        if (!baseThread) {
          ZS_LOG_DEBUG(log("unable to notify of messages received since conversation thread host object is gone"))
          return;
        }

        if (!mHostThread) {
          ZS_LOG_DEBUG(log("unable to notify of messages received since host thread object is NULL"))
          return;
        }

        // tell the base thread about the received messages (but only if we are in the conversation)...
        if (inConversation(mSelfContact)) {
          for (MessageList::const_iterator iter = messages.begin(); iter != messages.end(); ++iter)
          {
            const MessagePtr &message = (*iter);
            ZS_LOG_TRACE(log("notifying of message received") + message->getDebugValueString())
            baseThread->forHostOrSlave().notifyMessageReceived(message);
          }
        }

        // any received messages have to be republished to the host thread...
        mHostThread->updateBegin();
        mHostThread->addMessages(messages);
        publish(mHostThread->updateEnd(), false);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyMessageDeliveryStateChanged(
                                                                     const String &messageID,
                                                                     IConversationThread::MessageDeliveryStates state
                                                                     )
      {
        AutoRecursiveLock lock(getLock());

        MessageDeliveryStatesMap::iterator found = mMessageDeliveryStates.find(messageID);
        if (found != mMessageDeliveryStates.end()) {
          IConversationThread::MessageDeliveryStates &deliveryState = (*found).second;
          if (state <= deliveryState) {
            ZS_LOG_DEBUG(log("no need to change delievery state") + ", current state=" + IConversationThread::toString(state) + ", reported state=" + IConversationThread::toString(deliveryState))
            return;
          }
        }

        mMessageDeliveryStates[messageID] = state;

        ConversationThreadPtr baseThread = mBaseThread.lock();
        if (!baseThread) {
          ZS_LOG_WARNING(Detail, log("unable to notify of messages received since conversation thread host object is gone"))
          return;
        }
        return baseThread->forHostOrSlave().notifyMessageDeliveryStateChanged(messageID, state);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyMessagePush(
                                                     MessagePtr message,
                                                     ContactPtr toContact
                                                     )
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadPtr baseThread = mBaseThread.lock();
        if (!baseThread) {
          ZS_LOG_WARNING(Detail, log("unable to notify about pushed message as base is gone"))
          return;
        }
        return baseThread->forHostOrSlave().notifyMessagePush(message, toContact);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyStateChanged(PeerContactPtr peerContact)
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::notifyContactState(
                                                      ContactPtr contact,
                                                      ContactStates state
                                                      )
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadPtr baseThread = mBaseThread.lock();
        if (!baseThread) {
          ZS_LOG_WARNING(Detail, log("unable to notify about contact state as base is gone"))
          return;
        }
        return baseThread->forHostOrSlave().notifyContactState(mThisWeak.lock(), contact, state);
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadHost::hasCallPlacedTo(ContactPtr toContact)
      {
        ZS_LOG_TRACE(log("checking for calls placed to") + IContact::toDebugString(toContact))

        AutoRecursiveLock lock(getLock());

        ZS_THROW_INVALID_ASSUMPTION_IF(!mHostThread)

        const DialogMap &dialogs = mHostThread->dialogs();
        for (DialogMap::const_iterator iter = dialogs.begin(); iter != dialogs.end(); ++iter)
        {
          const DialogPtr &dialog = (*iter).second;
          ZS_LOG_TRACE(log("thread has call (thus comparing)") + dialog->getDebugValueString())

          if (dialog->calleePeerURI() == toContact->forConversationThread().getPeerURI()) {
            ZS_LOG_TRACE(log("found call placed to") + IContact::toDebugString(toContact))
            return true;
          }
        }

        ZS_LOG_TRACE(log("no calls found to this peer URI"))
        return false;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String ConversationThreadHost::log(const char *message) const
      {
        String baseThreadID;
        ConversationThreadPtr baseThread = mBaseThread.lock();

        if (baseThread) baseThreadID = baseThread->forHostOrSlave().getThreadID();

        return String("ConversationThreadHost [") + Stringize<PUID>(mID).string() + "] "  + message + ", base thread ID=" + baseThreadID + ", thread ID=" + mThreadID;
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadPtr base = mBaseThread.lock();
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("host thread id=", Stringize<PUID>(mID).string(), firstTime) +
               Helper::getDebugValue("host thread id (s)=", mThreadID, firstTime) +
               Helper::getDebugValue("base thread id (s)=", base ? base->forHostOrSlave().getThreadID() : String(), firstTime) +
               Helper::getDebugValue("state=", toString(mCurrentState), firstTime) +
               Helper::getDebugValue("delivery states=", mMessageDeliveryStates.size() > 0 ? Stringize<size_t>(mMessageDeliveryStates.size()).string() : String(), firstTime) +
               Helper::getDebugValue("peer contacts=", mPeerContacts.size() > 0 ? Stringize<size_t>(mPeerContacts.size()).string() : String(), firstTime) +
               Thread::toDebugString(mHostThread);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &ConversationThreadHost::getLock() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) return mBogusLock;
        return account->forConversationThread().getLock();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::cancel()
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) return;

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        setState(ConversationThreadHostState_ShuttingDown);
        setState(ConversationThreadHostState_Shutdown);

        mGracefulShutdownReference.reset();

        mPeerContacts.clear();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::step()
      {
        ZS_LOG_TRACE(log("step"))

        AutoRecursiveLock lock(getLock());
        if ((isShuttingDown()) || (isShutdown())) {cancel();}

        if (!mHostThread) {
          ZS_LOG_WARNING(Detail, log("host thread object is NULL thus shutting down"))
          cancel();
          return;
        }

        setState(ConversationThreadHostState_Ready);

        MessageReceiptMap receipts;
        ThreadContactMap contactsToAdd;
        ContactURIList contactsToRemove;

        for (PeerContactMap::iterator iter = mPeerContacts.begin(); iter != mPeerContacts.end(); ++iter)
        {
          PeerContactPtr &peerContact = (*iter).second;
          peerContact->gatherMessageReceipts(receipts);
          peerContact->gatherContactsToAdd(contactsToAdd);
          peerContact->gatherContactsToRemove(contactsToRemove);
          peerContact->notifyStep();
        }

        mHostThread->updateBegin();
        mHostThread->setReceived(receipts);
        publish(mHostThread->updateEnd(), false);

        if ((contactsToAdd.size() > 0) ||
            (contactsToRemove.size() > 0)) {
          ContactProfileInfoList contactsAsList;
          for (ThreadContactMap::iterator iter = contactsToAdd.begin(); iter != contactsToAdd.end(); ++iter)
          {
            ThreadContactPtr &threadContact = (*iter).second;
            ContactProfileInfo info;
            info.mContact = threadContact->contact();
            info.mProfileBundleEl = threadContact->profileBundleElement();
            contactsAsList.push_back(info);
          }

          if (!safeToChangeContacts()) {
            // not safe to add these contacts to the current object...
            ConversationThreadPtr baseThread = mBaseThread.lock();
            if (!baseThread) {
              ZS_LOG_WARNING(Detail, log("cannot add or remove contacts becaues base thread object is gone"))
              return;
            }
            baseThread->forHost().addContacts(contactsAsList);
            baseThread->forHost().removeContacts(contactsToRemove);
          } else {
            removeContacts(contactsToRemove);
            addContacts(contactsAsList);
          }
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::setState(ConversationThreadHostStates state)
      {
        AutoRecursiveLock lock(getLock());
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::publish(
                                            bool publishHostPublication,
                                            bool publishHostPermissionPublication
                                            ) const
      {
        AutoRecursiveLock lock(getLock());

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("account is gone thus cannot publish any publications"))
          return;
        }

        IPublicationRepositoryPtr repo = account->forConversationThread().getRepository();
        if (!repo) {
          ZS_LOG_WARNING(Detail, log("publication repository was NULL thus cannot publish any publications"))
          return;
        }

        if (!mHostThread) {
          ZS_LOG_WARNING(Detail, log("host thread is not available thus cannot publish any publications"))
          return;
        }

        if (publishHostPermissionPublication) {
          ZS_LOG_DEBUG(log("publishing host thread permission document"))
          repo->publish(IPublicationPublisherDelegateProxy::createNoop(getAssociatedMessageQueue()), mHostThread->permissionPublication());
        }

        if (publishHostPublication) {
          ZS_LOG_DEBUG(log("publishing host thread document"))
          repo->publish(IPublicationPublisherDelegateProxy::createNoop(getAssociatedMessageQueue()), mHostThread->publication());
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::removeContacts(const ContactURIList &contacts)
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_ASSUMPTION_IF(!safeToChangeContacts())

        ThreadContactMap contactMap;

        for (ContactURIList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
        {
          const String &contactID = (*iter);
          PeerContactMap::iterator found = mPeerContacts.find(contactID);
          if (found == mPeerContacts.end()) continue;

          // remove this peer contact...
          PeerContactPtr peerContact = (*found).second;
          mPeerContacts.erase(found);

          peerContact->cancel();
        }

        // calculate current contacts map...
        for (PeerContactMap::iterator iter = mPeerContacts.begin(); iter != mPeerContacts.end(); ++iter)
        {
          const String contactID = (*iter).first;
          PeerContactPtr &peerContact = (*iter).second;
          ThreadContactPtr threadContact(ThreadContact::create(peerContact->getContact(), peerContact->getProfileBundle()));
          contactMap[contactID] = threadContact;
        }

        // publish changes...
        mHostThread->updateBegin();
        mHostThread->setContacts(contactMap);
        publish(mHostThread->updateEnd(), true);
      }

      //-----------------------------------------------------------------------
      ConversationThreadHost::PeerContactPtr ConversationThreadHost::findContact(ILocationPtr peerLocation) const
      {
        PeerContactMap::const_iterator found = mPeerContacts.find(peerLocation->getPeerURI());
        if (found == mPeerContacts.end()) {
          ZS_LOG_WARNING(Detail, log("failed to find any peer contact for the location given") + ILocation::toDebugString(peerLocation))
          return PeerContactPtr();
        }
        const PeerContactPtr &peerContact = (*found).second;
        return peerContact;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerContact
      #pragma mark

      //-----------------------------------------------------------------------
      const char *ConversationThreadHost::PeerContact::toString(PeerContactStates state)
      {
        switch (state)
        {
          case PeerContactState_Pending:      return "Pending";
          case PeerContactState_Ready:        return "Ready";
          case PeerContactState_ShuttingDown: return "Shutting down";
          case PeerContactState_Shutdown:     return "Shutdown";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      ConversationThreadHost::PeerContact::PeerContact(
                                                       IMessageQueuePtr queue,
                                                       ConversationThreadHostPtr host,
                                                       ContactPtr contact,
                                                       ElementPtr profileBundleEl
                                                       ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mCurrentState(PeerContactState_Pending),
        mOuter(host),
        mContact(contact),
        mProfileBundleEl(profileBundleEl)
      {
        ZS_LOG_DEBUG(log("created") + IContact::toDebugString(contact))
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::init()
      {
      }

      //-----------------------------------------------------------------------
      ConversationThreadHost::PeerContact::~PeerContact()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::PeerContact::toDebugString(PeerContactPtr contact, bool includeCommaPrefix)
      {
        if (!contact) return includeCommaPrefix ? ", host peer contact=(null)" : "host peer contact=(null)";
        return contact->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerContact => friend ConversationThreadHost
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadHost::PeerContactPtr ConversationThreadHost::PeerContact::create(
                                                                                         IMessageQueuePtr queue,
                                                                                         ConversationThreadHostPtr host,
                                                                                         ContactPtr contact,
                                                                                         ElementPtr profileBundleEl
                                                                                         )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!host)
        ZS_THROW_INVALID_ARGUMENT_IF(!contact)
        PeerContactPtr pThis(new PeerContact(queue, host, contact, profileBundleEl));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::notifyPublicationUpdated(
                                                                         ILocationPtr peerLocation,
                                                                         IPublicationMetaDataPtr metaData,
                                                                         const SplitMap &split
                                                                         )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("received publication update notification after shutdown"))
          return;
        }

        PeerLocationPtr location = findPeerLocation(peerLocation);
        if (!location) {
          location = PeerLocation::create(getAssociatedMessageQueue(), mThisWeak.lock(), peerLocation);
          if (!location) {
            ZS_LOG_WARNING(Detail, log("failed to create peer location") + ILocation::toDebugString(peerLocation))
            return;
          }
          ZS_LOG_DEBUG(log("created new oeer location") + IContact::toDebugString(mContact))
          mPeerLocations[location->getLocationID()] = location;
        }

        location->notifyPublicationUpdated(peerLocation, metaData, split);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::notifyPublicationGone(
                                                                      ILocationPtr peerLocation,
                                                                      IPublicationMetaDataPtr metaData,
                                                                      const SplitMap &split
                                                                      )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("received publication gone notification after shutdown"))
          return;
        }
        PeerLocationPtr location = findPeerLocation(peerLocation);
        if (!location) {
          ZS_LOG_WARNING(Detail, log("location was not found to pass on publication gone notification") + ILocation::toDebugString(peerLocation))
          return;
        }
        location->notifyPublicationGone(peerLocation, metaData, split);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::notifyPeerDisconnected(ILocationPtr peerLocation)
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("received notification of peer disconnection after shutdown"))
          return;
        }

        PeerLocationPtr location = findPeerLocation(peerLocation);
        if (!location) {
          ZS_LOG_WARNING(Detail, log("unable to pass on peer disconnection notification as location is not available") + ILocation::toDebugString(peerLocation))
          return;
        }
        location->notifyPeerDisconnected(peerLocation);
      }

      //-----------------------------------------------------------------------
      ContactPtr ConversationThreadHost::PeerContact::getContact() const
      {
        AutoRecursiveLock lock(getLock());
        return mContact;
      }

      //-----------------------------------------------------------------------
      const ElementPtr &ConversationThreadHost::PeerContact::getProfileBundle() const
      {
        AutoRecursiveLock lock(getLock());
        return mProfileBundleEl;
      }

      //-----------------------------------------------------------------------
      IConversationThread::ContactStates ConversationThreadHost::PeerContact::getContactState() const
      {
        AutoRecursiveLock lock(getLock());

        // first check to see if any locations are connected...
        for (PeerLocationMap::const_iterator iter = mPeerLocations.begin(); iter != mPeerLocations.end(); ++iter)
        {
          const PeerLocationPtr &peerLocation = (*iter).second;
          if (peerLocation->isConnected()) {
            return IConversationThread::ContactState_Connected;
          }
        }

        if (mContact) {
          switch (mContact->forConversationThread().getPeer()->getFindState()) {
            case IPeer::PeerFindState_Idle:       return (mPeerLocations.size() > 0 ? IConversationThread::ContactState_Disconnected : IConversationThread::ContactState_NotApplicable);
            case IPeer::PeerFindState_Finding:    return IConversationThread::ContactState_Finding;
            case IPeer::PeerFindState_Completed:  return (mPeerLocations.size() > 0 ? IConversationThread::ContactState_Disconnected : IConversationThread::ContactState_NotApplicable);
          }
        }

        return IConversationThread::ContactState_NotApplicable;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::gatherMessageReceipts(MessageReceiptMap &receipts) const
      {
        AutoRecursiveLock lock(getLock());
        for (PeerLocationMap::const_iterator iter = mPeerLocations.begin(); iter != mPeerLocations.end(); ++iter)
        {
          const PeerLocationPtr &peerLocation = (*iter).second;
          peerLocation->gatherMessageReceipts(receipts);
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::gatherContactsToAdd(ThreadContactMap &contacts) const
      {
        AutoRecursiveLock lock(getLock());
        if (!isStillPartOfCurrentConversation(mContact)) {
          ZS_LOG_WARNING(Debug, log("ignoring request to add contacts as peer contact is not part of current conversation"))
          return;
        }

        for (PeerLocationMap::const_iterator iter = mPeerLocations.begin(); iter != mPeerLocations.end(); ++iter)
        {
          const PeerLocationPtr &peerLocation = (*iter).second;
          peerLocation->gatherContactsToAdd(contacts);
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::gatherContactsToRemove(ContactURIList &contacts) const
      {
        AutoRecursiveLock lock(getLock());
        if (!isStillPartOfCurrentConversation(mContact)) {
          ZS_LOG_WARNING(Debug, log("ignoring request to remove contacts as peer contact is not part of current conversation"))
          return;
        }

        for (PeerLocationMap::const_iterator iter = mPeerLocations.begin(); iter != mPeerLocations.end(); ++iter)
        {
          const PeerLocationPtr &peerLocation = (*iter).second;
          peerLocation->gatherContactsToRemove(contacts);
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::gatherDialogReplies(
                                                                    const char *callID,
                                                                    LocationDialogMap &outDialogs
                                                                    ) const
      {
        AutoRecursiveLock lock(getLock());
        for (PeerLocationMap::const_iterator iter = mPeerLocations.begin(); iter != mPeerLocations.end(); ++iter)
        {
          const PeerLocationPtr &peerLocation = (*iter).second;
          peerLocation->gatherDialogReplies(callID, outDialogs);
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::notifyStep(bool performStepAsync)
      {
        if (performStepAsync) {
          IPeerContactAsyncProxy::create(mThisWeak.lock())->onStep();
          return;
        }

        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerContact => IPeerSubscriptionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription)
      {
        AutoRecursiveLock lock(getLock());
        if (subscription != mSlaveSubscription) {
          ZS_LOG_WARNING(Detail, log("ignoring shutdown notification on obsolete slave peer subscription (probably okay)"))
          return;
        }

        mSlaveSubscription.reset();
        step();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::onPeerSubscriptionFindStateChanged(
                                                                                   IPeerSubscriptionPtr subscription,
                                                                                   IPeerPtr peer,
                                                                                   PeerFindStates state
                                                                                   )
      {
        AutoRecursiveLock lock(getLock());
        if (subscription != mSlaveSubscription) {
          ZS_LOG_WARNING(Detail, log("notified of subscription state from obsolete subscription (probably okay)"))
          return;
        }
        ZS_LOG_DEBUG(log("notified peer subscription state changed") + ", state=" + IPeer::toString(state))
        step();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::onPeerSubscriptionLocationConnectionStateChanged(
                                                                                                 IPeerSubscriptionPtr subscription,
                                                                                                 ILocationPtr location,
                                                                                                 LocationConnectionStates state
                                                                                                 )
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::onPeerSubscriptionMessageIncoming(
                                                                                  IPeerSubscriptionPtr subscription,
                                                                                  IMessageIncomingPtr message
                                                                                  )
      {
        // IGNORED
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerContact => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::onTimer(TimerPtr timer)
      {
        ZS_LOG_DEBUG(log("timer"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerContact => friend ConversationThreadHost::PeerLocation
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadHostPtr ConversationThreadHost::PeerContact::getOuter() const
      {
        AutoRecursiveLock lock(getLock());
        return mOuter.lock();
      }

      //-----------------------------------------------------------------------
      ConversationThreadPtr ConversationThreadHost::PeerContact::getBaseThread() const
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("failed to obtain conversation thread because conversation thread host object is gone"))
          return ConversationThreadPtr();
        }
        return outer->getBaseThread();
      }

      //-----------------------------------------------------------------------
      ThreadPtr ConversationThreadHost::PeerContact::getHostThread() const
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("failed to obtain repository because conversation thread host object is gone"))
          return ThreadPtr();
        }
        return outer->getHostThread();
      }

      //-----------------------------------------------------------------------
      AccountPtr ConversationThreadHost::PeerContact::getAccount() const
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) return AccountPtr();
        return outer->getAccount();
      }

      //-----------------------------------------------------------------------
      IPublicationRepositoryPtr ConversationThreadHost::PeerContact::getRepository() const
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("failed to obtain repository because conversation thread host object is gone"))
          return IPublicationRepositoryPtr();
        }
        return outer->getRepository();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::notifyMessagesReceived(const MessageList &messages)
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_DEBUG(log("unable to notify of messages received since conversation thread host object is gone"))
          return;
        }
        return outer->notifyMessagesReceived(messages);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::notifyMessageDeliveryStateChanged(
                                                                                  const String &messageID,
                                                                                  IConversationThread::MessageDeliveryStates state
                                                                                  )
      {
        AutoRecursiveLock lock(getLock());

        MessageDeliveryStatesMap::iterator found = mMessageDeliveryStates.find(messageID);
        if (found != mMessageDeliveryStates.end()) {
          DeliveryStatePair &deliveryStatePair = (*found).second;
          IConversationThread::MessageDeliveryStates &deliveryState = deliveryStatePair.second;
          if (state <= deliveryState) {
            ZS_LOG_DEBUG(log("no need to change delievery state") + ", current state=" + IConversationThread::toString(state) + ", reported state=" + IConversationThread::toString(deliveryState))
            return;
          }
        }

        mMessageDeliveryStates[messageID] = DeliveryStatePair(zsLib::now(), state);

        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_DEBUG(log("unable to notify of messages received since conversation thread host object is gone"))
          return;
        }
        return outer->notifyMessageDeliveryStateChanged(messageID, state);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::notifyStateChanged(PeerLocationPtr peerLocation)
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_DEBUG(log("unable to notify conversation thread host of state change"))
          return;
        }
        return outer->notifyStateChanged(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::notifyPeerLocationShutdown(PeerLocationPtr location)
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("notified peer location shutdown") + IContact::toDebugString(mContact) + PeerLocation::toDebugString(location))

        PeerLocationMap::iterator found = mPeerLocations.find(location->getLocationID());
        if (found != mPeerLocations.end()) {
          ZS_LOG_TRACE(log("peer location removed from map") + IContact::toDebugString(mContact) + PeerLocation::toDebugString(location))
          mPeerLocations.erase(found);
        }

        PeerContactPtr pThis = mThisWeak.lock();
        if (pThis) {
          IPeerContactAsyncProxy::create(mThisWeak.lock())->onStep();
        }
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerContact => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      RecursiveLock &ConversationThreadHost::PeerContact::getLock() const
      {
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::PeerContact::log(const char *message) const
      {
        String peerURI;
        if (mContact) {
          peerURI = mContact->forConversationThread().getPeerURI();
        }
        return String("ConversationThreadHost::PeerContact [") + Stringize<typeof(mID)>(mID).string() + "] " + message + ", peer URI=" + peerURI;
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::PeerContact::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("host peer contact id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               IContact::toDebugString(mContact) +
               Helper::getDebugValue("profile bundle", mProfileBundleEl ? String("true") : String(), firstTime) +
               IPeerSubscription::toDebugString(mSlaveSubscription) +
               Helper::getDebugValue("slave delivery timer", mSlaveMessageDeliveryTimer ? String("true") : String(), firstTime) +
               Helper::getDebugValue("locations", mPeerLocations.size() > 0 ? Stringize<size_t>(mPeerLocations.size()).string() : String(), firstTime) +
               Helper::getDebugValue("delivery states", mMessageDeliveryStates.size() > 0 ? Stringize<size_t>(mMessageDeliveryStates.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::cancel()
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) return;

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        setState(PeerContactState_ShuttingDown);

        if (mSlaveSubscription) {
          mSlaveSubscription->cancel();
          mSlaveSubscription.reset();
        }

        if (mSlaveMessageDeliveryTimer) {
          mSlaveMessageDeliveryTimer->cancel();
          mSlaveMessageDeliveryTimer.reset();
        }

        // cancel all the locations
        for (PeerLocationMap::iterator peerIter = mPeerLocations.begin(); peerIter != mPeerLocations.end(); )
        {
          PeerLocationMap::iterator current = peerIter;
          ++peerIter;

          PeerLocationPtr &location = (*current).second;
          location->cancel();
        }

        setState(PeerContactState_Shutdown);

        mGracefulShutdownReference.reset();

        // mContact.reset();  // DO NOT RESET -- LEAVE THIS TO THE DESTRUCTOR

        mPeerLocations.clear();
        mMessageDeliveryStates.clear();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::step()
      {
        ZS_LOG_TRACE(log("step"))

        AutoRecursiveLock lock(getLock());
        if ((isShuttingDown()) ||
            (isShutdown())) {cancel();}

        // check to see if there are any undelivered messages
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("shutting down peer contact since conversation thread host is shutdown"))
          cancel();
          return;
        }

        ThreadPtr hostThread = outer->getHostThread();
        if (!hostThread) {
          ZS_LOG_WARNING(Detail, log("host thread not available"))
          return;
        }

        AccountPtr account = getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("account is not available thus shutting down"))
          cancel();
          return;
        }

        stack::IAccountPtr stackAccount = account->forConversationThread().getStackAccount();
        if (!stackAccount) {
          ZS_LOG_WARNING(Detail, log("stack account is not available thus shutting down"))
          cancel();
          return;
        }

        setState(PeerContactState_Ready);

        for (PeerLocationMap::iterator peerIter = mPeerLocations.begin(); peerIter != mPeerLocations.end(); )
        {
          PeerLocationMap::iterator current = peerIter;
          ++peerIter;

          PeerLocationPtr &location = (*current).second;
          location->step();
        }

        // check to see if there are outstanding messages needing to be delivered to this contact
        bool requiresSubscription = false;
        bool requiresTimer = false;

        // check to see if there are undelivered messages, if so we will need a subscription...
        const MessageList &messages = hostThread->messages();
        MessageList::const_reverse_iterator last = messages.rbegin();
        if (last != messages.rend()) {
          const MessagePtr &message = (*last);

          // check to see if this message has been acknowledged before...
          MessageDeliveryStatesMap::iterator found = mMessageDeliveryStates.find(message->messageID());
          if (found != mMessageDeliveryStates.end()) {
            DeliveryStatePair &deliveryStatePair = (*found).second;
            IConversationThread::MessageDeliveryStates &deliveryState = deliveryStatePair.second;
            if (IConversationThread::MessageDeliveryState_Delivered != deliveryState) {
              ZS_LOG_DEBUG(log("still requires subscription because of undelivered message") + message->getDebugValueString() + ", was in delivery state=" + IConversationThread::toString(deliveryState))
              requiresTimer = (IConversationThread::MessageDeliveryState_UserNotAvailable != deliveryState);
              requiresSubscription = true;
            }
          } else {
            ZS_LOG_DEBUG(log("still requires subscription because of undelivered message") + message->getDebugValueString())
            requiresTimer = requiresSubscription = true;
          }
        }

        if (outer->hasCallPlacedTo(mContact)) {
          ZS_LOG_DEBUG(log("calls are being placed to this slave contact thus subscription is required"))
          // as there is a call being placed to this contact thus a subscription is required
          requiresSubscription = true;
        }

        if (requiresSubscription) {
          ZS_LOG_DEBUG(log("subscription to slave is required") + getDebugValueString())
          if (!mSlaveSubscription) {
            mSlaveSubscription = IPeerSubscription::subscribe(mContact->forConversationThread().getPeer(), mThisWeak.lock());
          }
        } else {
          ZS_LOG_DEBUG(log("subscription to slave is NOT required") + getDebugValueString())
          if (mSlaveSubscription) {
            mSlaveSubscription->cancel();
            mSlaveSubscription.reset();
          }
        }

        if (requiresTimer) {
          ZS_LOG_DEBUG(log("timer for peer slave is required") + getDebugValueString())
          if (!mSlaveMessageDeliveryTimer) {
            mSlaveMessageDeliveryTimer = Timer::create(mThisWeak.lock(), Seconds(1));
          }
        } else {
          ZS_LOG_DEBUG(log("timer for peer slave is NOT required") + getDebugValueString())
          if (mSlaveMessageDeliveryTimer) {
            mSlaveMessageDeliveryTimer->cancel();
            mSlaveMessageDeliveryTimer.reset();
          }
        }

        Time tick = zsLib::now();

        // scope: fix the state of pending messages...
        if (mSlaveSubscription) {
          IPeerPtr peer = mContact->forConversationThread().getPeer();
          IPeer::PeerFindStates state = peer->getFindState();
          ZS_LOG_DEBUG(log("slave subscription state") + ", state=" + IPeer::toString(state))

          LocationListPtr peerLocations = peer->getLocationsForPeer(false);

          const MessageList &messages = hostThread->messages();

          // Search from the back of the list to the front for messages that
          // aren't delivered as they need to be marked as undeliverable
          // since there are no peer locations available for this user...
          for (MessageList::const_reverse_iterator iter = messages.rbegin(); iter != messages.rend(); ++iter) {
            const MessagePtr &message = (*iter);
            MessageDeliveryStatesMap::iterator found = mMessageDeliveryStates.find(message->messageID());

            Time lastStateChangeTime = tick;

            if (found != mMessageDeliveryStates.end()) {
              bool stopProcessing = false;
              DeliveryStatePair &deliveryStatePair = (*found).second;
              IConversationThread::MessageDeliveryStates &deliveryState = deliveryStatePair.second;
              lastStateChangeTime = deliveryStatePair.first;

              switch (deliveryState) {
                case IConversationThread::MessageDeliveryState_Discovering:   {
                  break;
                }
                case IConversationThread::MessageDeliveryState_Delivered:
                case IConversationThread::MessageDeliveryState_UserNotAvailable: {
                  stopProcessing = true;
                  break;
                }
              }

              if (stopProcessing) {
                ZS_LOG_DEBUG(log("processing undeliverable messages stopped since message already has a delivery state") + message->getDebugValueString())
                break;
              }
            } else {
              mMessageDeliveryStates[message->messageID()] = DeliveryStatePair(tick, IConversationThread::MessageDeliveryState_Discovering);
            }

            if (((IPeer::PeerFindState_Finding != state) &&
                 (peerLocations->size() < 1)) ||
                (lastStateChangeTime + Seconds(OPENPEER_CONVERSATION_THREAD_MAX_WAIT_DELIVERY_TIME_BEFORE_PUSH_IN_SECONDS) < tick)) {
              ZS_LOG_DEBUG(log("state must now be set to undeliverable") + ", message ID=" + message->getDebugValueString() + ", peer find state=" + IPeer::toString(state) + ", last state changed time=" + Stringize<Time>(lastStateChangeTime).string() + ", current time=" + Stringize<Time>(tick).string())
              mMessageDeliveryStates[message->messageID()] = DeliveryStatePair(zsLib::now(), IConversationThread::MessageDeliveryState_UserNotAvailable);
              outer->notifyMessageDeliveryStateChanged(message->messageID(), IConversationThread::MessageDeliveryState_UserNotAvailable);

              // tell the application to push this message out as a push notification
              outer->notifyMessagePush(message, mContact);
            }
          }
        }

        outer->notifyContactState(mContact, getContactState());
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerContact::setState(PeerContactStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", previous state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;
      }

      //-----------------------------------------------------------------------
      ConversationThreadHost::PeerLocationPtr ConversationThreadHost::PeerContact::findPeerLocation(ILocationPtr peerLocation) const
      {
        AutoRecursiveLock lock(getLock());
        PeerLocationMap::const_iterator found = mPeerLocations.find(peerLocation->getLocationID());
        if (found == mPeerLocations.end()) return PeerLocationPtr();
        const PeerLocationPtr &location = (*found).second;
        return location;
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadHost::PeerContact::isStillPartOfCurrentConversation(ContactPtr contact) const
      {
        ConversationThreadHostPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("cannot tell if still part of current conversation because conversation thread host is gone"))
          return false;
        }

        // only respond to contacts to add if this contact is part of the current "live" thread...
        ConversationThreadPtr baseThread = outer->getBaseThread();
        if (!baseThread) {
          ZS_LOG_WARNING(Detail, log("cannot tell if still part of current conversation because conversation thread is gone"))
          return false;
        }

        // check the base thread to see if this contact is still part of the conversation
        return baseThread->forHost().inConversation(contact);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerLocation
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadHost::PeerLocation::PeerLocation(
                                                         IMessageQueuePtr queue,
                                                         PeerContactPtr peerContact,
                                                         ILocationPtr peerLocation
                                                         ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mOuter(peerContact),
        mShutdown(false),
        mPeerLocation(peerLocation)
      {
        ZS_LOG_DEBUG(log("created"))
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::init()
      {
        PeerContactPtr outer = mOuter.lock();
        ZS_THROW_INVALID_ASSUMPTION_IF(!outer)

        IPublicationRepositoryPtr repo = outer->getRepository();
        if (repo) {
          mFetcher = IConversationThreadDocumentFetcher::create(mThisWeak.lock(), repo);
        }
      }

      //-----------------------------------------------------------------------
      ConversationThreadHost::PeerLocation::~PeerLocation()
      {
        mThisWeak.reset();
        ZS_LOG_DEBUG(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::PeerLocation::toDebugString(PeerLocationPtr peerLocation, bool includeCommaPrefix)
      {
        if (!peerLocation) return includeCommaPrefix ? ", host peer location=(null)" : "host peer location=(null)";
        return peerLocation->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerLocation => friend ConversationThreadHost::PeerContact
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadHost::PeerLocationPtr ConversationThreadHost::PeerLocation::create(
                                                                                           IMessageQueuePtr queue,
                                                                                           PeerContactPtr peerContact,
                                                                                           ILocationPtr peerLocation
                                                                                           )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!peerContact)
        ZS_THROW_INVALID_ARGUMENT_IF(!peerLocation)

        PeerLocationPtr pThis(new PeerLocation(queue, peerContact, peerLocation));
        pThis->mThisWeak = pThis;
        pThis->init();
        if (!pThis->mFetcher) return PeerLocationPtr();
        return pThis;
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::PeerLocation::getLocationID() const
      {
        AutoRecursiveLock lock(getLock());
        return mPeerLocation->getLocationID();
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadHost::PeerLocation::isConnected() const
      {
        AutoRecursiveLock lock(getLock());
        if (!mPeerLocation) return false;

        return mPeerLocation->isConnected();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::notifyPublicationUpdated(
                                                                          ILocationPtr peerLocation,
                                                                          IPublicationMetaDataPtr metaData,
                                                                          const SplitMap &split
                                                                          )
      {
        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("notification of an updated document after shutdown") + IPublicationMetaData::toDebugString(metaData))
          return;
        }
        mFetcher->notifyPublicationUpdated(peerLocation, metaData);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::notifyPublicationGone(
                                                                       ILocationPtr peerLocation,
                                                                       IPublicationMetaDataPtr metaData,
                                                                       const SplitMap &split
                                                                       )
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("notification that publication is gone after shutdown") + IPublicationMetaData::toDebugString(metaData))
          return;
        }
        mFetcher->notifyPublicationGone(peerLocation, metaData);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::notifyPeerDisconnected(ILocationPtr peerLocation)
      {
        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {
          ZS_LOG_DEBUG(log("notification of a peer shutdown") + ILocation::toDebugString(peerLocation))
          return;
        }
        mFetcher->notifyPeerDisconnected(peerLocation);

        if (mPeerLocation->getLocationID() == peerLocation->getLocationID()) {
          ZS_LOG_DEBUG(log("this peer location is gone thus this location must shutdown"))
          cancel();
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::gatherMessageReceipts(MessageReceiptMap &receipts) const
      {
        typedef zsLib::Time Time;

        AutoRecursiveLock lock(getLock());
        if (!mSlaveThread) {
          ZS_LOG_WARNING(Detail, log("unable to gather message receipts as there is no slave thread object"))
          return;
        }

        const MessageList &messages = mSlaveThread->messages();

        if (messages.size() < 1) {
          ZS_LOG_DEBUG(log("no messages receipts to acknowledge from this slave"))
          return;
        }

        const MessagePtr &lastMessage = messages.back();
        Time when = mSlaveThread->messagedChangedTime();

        // remember out when this message was received
        receipts[lastMessage->messageID()] = when;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::gatherContactsToAdd(ThreadContactMap &contacts) const
      {
        typedef IConversationThreadParser::ThreadContactPtr ThreadContactPtr;

        AutoRecursiveLock lock(getLock());
        if (!mSlaveThread) {
          ZS_LOG_WARNING(Detail, log("unable to gather contacts to add as there is no slave thread object"))
          return;
        }

        const ThreadContactMap &contactsToAdd = mSlaveThread->contacts()->addContacts();
        for (ThreadContactMap::const_iterator iter = contactsToAdd.begin(); iter != contactsToAdd.end(); ++iter)
        {
          const String &contactID = (*iter).first;
          const ThreadContactPtr &contact = (*iter).second;
          contacts[contactID] = contact;
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::gatherContactsToRemove(ContactURIList &contacts) const
      {
        AutoRecursiveLock lock(getLock());
        if (!mSlaveThread) {
          ZS_LOG_WARNING(Detail, log("unable to gather contacts to remove as there is no slave thread object"))
          return;
        }

        const ContactURIList &contactsToRemove = mSlaveThread->contacts()->removeContacts();
        for (ContactURIList::const_iterator iter = contactsToRemove.begin(); iter != contactsToRemove.end(); ++iter)
        {
          const String &contactID = (*iter);
          contacts.push_back(contactID);
        }
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::gatherDialogReplies(
                                                                     const char *callID,
                                                                     LocationDialogMap &outDialogs
                                                                     ) const
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_ASSUMPTION_IF(!mPeerLocation)

        if (!mSlaveThread) {
          ZS_LOG_WARNING(Detail, log("unable to gather dialog replies as slave thread object is invalid"))
          return;
        }

        const DialogMap &dialogs = mSlaveThread->dialogs();
        DialogMap::const_iterator found = dialogs.find(callID);
        if (found == dialogs.end()) return;

        const DialogPtr dialog = (*found).second;
        outDialogs[mPeerLocation->getLocationID()] = dialog;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerLocation => IConversationThreadDocumentFetcherDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::onConversationThreadDocumentFetcherPublicationUpdated(
                                                                                                       IConversationThreadDocumentFetcherPtr fetcher,
                                                                                                       ILocationPtr peerLocation,
                                                                                                       IPublicationPtr publication
                                                                                                       )
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!publication)

        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("publication was updated notification received") + ", name=" + publication->getName())

        if (isShutdown()) {
          ZS_LOG_WARNING(Detail, log("received notification of updated slave document when shutdown") + IPublication::toDebugString(publication))
          return;
        }

        PeerContactPtr outer = mOuter.lock();
        if (!outer) {
          ZS_LOG_WARNING(Detail, log("unable to update publication as peer contact object is gone") + IPublication::toDebugString(publication))
          return;
        }

        ConversationThreadHostPtr conversationThreadHost = outer->getOuter();
        if (!conversationThreadHost) {
          ZS_LOG_WARNING(Detail, log("unable to update publication as conversation thread host object is gone"))
          return;
        }

        AccountPtr account = outer->getAccount();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("unable to update publication as account object is gone") + IPublication::toDebugString(publication))
          return;
        }

        ConversationThreadPtr baseThread = outer->getBaseThread();
        if (!baseThread) {
          ZS_LOG_WARNING(Detail, log("unable to update publication as base conversation thread is gone"))
          return;
        }

        if (!mSlaveThread) {
          mSlaveThread = Thread::create(account, publication);
          if (!mSlaveThread) {
            ZS_LOG_WARNING(Detail, log("unable to create slave thread for publication") + IPublication::toDebugString(publication))
            return;
          }
        } else {
          mSlaveThread->updateFrom(account, publication);
        }

        //.......................................................................
        // NOTE: We don't need to check contact changes because the outer will
        //       automatically gather up all the contacts to add/remove.

        //.......................................................................
        // check for incoming calls that are gone...

        const DialogIDList &removedDialogs = mSlaveThread->dialogsRemoved();
        for (DialogIDList::const_iterator iter = removedDialogs.begin(); iter != removedDialogs.end(); ++iter) {
          const String &dialogID = (*iter);
          CallHandlers::iterator found = mIncomingCallHandlers.find(dialogID);
          if (found == mIncomingCallHandlers.end()) {
            baseThread->forHostOrSlave().notifyPossibleCallReplyStateChange(dialogID);
            continue;
          }

          // this incoming call is now gone, clean it out...
          CallPtr &call = (*found).second;

          ZS_LOG_DEBUG(log("call object is now gone") + Call::toDebugString(call))

          call->forConversationThread().notifyConversationThreadUpdated();

          mIncomingCallHandlers.erase(found);
          ZS_LOG_DEBUG(log("removed incoming call handler") + getDebugValueString())
        }

        //.......................................................................
        // check for dialogs that are now incoming/updated

        String selfPeerURI = account->forConversationThread().getSelfContact()->forConversationThread().getPeerURI();

        const DialogMap &changedDialogs = mSlaveThread->dialogsChanged();
        for (DialogMap::const_iterator iter = changedDialogs.begin(); iter != changedDialogs.end(); ++iter)
        {
          const String &dialogID = (*iter).first;
          const DialogPtr &dialog = (*iter).second;

          ZS_LOG_TRACE(log("call detected") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)

          CallHandlers::iterator found = mIncomingCallHandlers.find(dialogID);
          if (found != mIncomingCallHandlers.end()) {
            CallPtr &call = (*found).second;
            ZS_LOG_DEBUG(log("call object is updated") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)
            call->forConversationThread().notifyConversationThreadUpdated();
            continue;
          }

          if (dialog->calleePeerURI() != selfPeerURI) {
            if (dialog->callerPeerURI() == selfPeerURI) {
              ZS_LOG_DEBUG(log("call detected this must be a reply to a call previously placed") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)
              baseThread->forHostOrSlave().notifyPossibleCallReplyStateChange(dialogID);
            } else {
              ZS_LOG_WARNING(Detail, log("call detected but the call is not going to this contact") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)
            }
            continue;
          }

          const String callerContactID = dialog->callerPeerURI();
          ContactPtr contact = account->forConversationThread().findContact(callerContactID);
          if (!contact) {
            ZS_LOG_WARNING(Detail, log("while an incoming call was found the contact is not known to the account thus cannot accept the call") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)
            continue;
          }

          CallPtr call = ICallForConversationThread::createForIncomingCall(baseThread, contact, dialog);

          if (!call) {
            ZS_LOG_WARNING(Detail, log("unable to create incoming call from contact") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)
            continue;
          }

          ZS_LOG_DEBUG(log("found new call") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)

          // new call handler found...
          mIncomingCallHandlers[dialogID] = call;
          baseThread->forHostOrSlave().requestAddIncomingCallHandler(dialogID, conversationThreadHost, call);
        }

        //.......................................................................
        // examine all the newly received messages

        ThreadPtr hostThread = outer->getHostThread();
        if (hostThread) {
          const MessageList &messagesChanged = mSlaveThread->messagedChanged();
          outer->notifyMessagesReceived(messagesChanged);

          // examine all the acknowledged messages
          const MessageList &messages = hostThread->messages();
          const MessageMap &messagesMap = hostThread->messagesAsMap();

          // can only examine message receipts that are part of the slave thread...
          const MessageReceiptMap &messageReceiptsChanged = mSlaveThread->messageReceiptsChanged();
          for (MessageReceiptMap::const_iterator iter = messageReceiptsChanged.begin(); iter != messageReceiptsChanged.end(); ++iter)
          {
            const MessageID &id = (*iter).first;

            ZS_LOG_TRACE(log("examining message receipt") + ", receipt ID=" + id)

            // check to see if this receipt has already been marked as delivered...
            MessageDeliveryStatesMap::iterator found = mMessageDeliveryStates.find(id);
            if (found != mMessageDeliveryStates.end()) {
              IConversationThread::MessageDeliveryStates &deliveryState = (*found).second;
              if (IConversationThread::MessageDeliveryState_Delivered == deliveryState) {
                ZS_LOG_DEBUG(log("message receipt was already notified as delivered thus no need to notify any further") + ", message ID=" + id)
                continue;
              }
            }

            MessageMap::const_iterator foundInMap = messagesMap.find(id);
            if (foundInMap == messagesMap.end()) {
              ZS_LOG_WARNING(Detail, log("host never send this message to the slave (what is slave acking?)") + ", receipt id=" + id)
              continue;
            }

            bool foundMessageID = false;

            // Find this receipt on the host's message list... (might not exist if
            // receipt is for a message from a different contact)
            MessageList::const_reverse_iterator messageIter = messages.rbegin();
            for (; messageIter != messages.rend(); ++messageIter)
            {
              const MessagePtr &message = (*messageIter);
              if (message->messageID() == id) {
                ZS_LOG_TRACE(log("found message matching receipt") + ", receipt id=" + id)
                foundMessageID = true;
              }

              ZS_LOG_TRACE(log("processing host message") + ", found=" + (foundMessageID ? "true" : "false") + message->getDebugValueString())

              if (foundMessageID) {
                // first check if this delivery was already sent...
                found = mMessageDeliveryStates.find(message->messageID());
                if (found != mMessageDeliveryStates.end()) {
                  // check to see if this message was already marked as delivered
                  IConversationThread::MessageDeliveryStates &deliveryState = (*found).second;
                  if (IConversationThread::MessageDeliveryState_Delivered == deliveryState) {
                    // stop notifying of delivered since it's alerady been marked as delivered
                    ZS_LOG_DEBUG(log("message was already notified as delivered thus no need to notify any further") + message->getDebugValueString())
                    break;
                  }

                  ZS_LOG_DEBUG(log("message is now notified as delivered") + message->getDebugValueString() + ", was in state" + IConversationThread::toString(deliveryState))

                  // change the state to delivered since it wasn't delivered
                  deliveryState = IConversationThread::MessageDeliveryState_Delivered;
                } else {
                  ZS_LOG_DEBUG(log("message is now delivered") + message->getDebugValueString())
                  mMessageDeliveryStates[message->messageID()] = IConversationThread::MessageDeliveryState_Delivered;
                }

                // this message is now considered acknowledged so tell the master thread of the new state...
                outer->notifyMessageDeliveryStateChanged(id, IConversationThread::MessageDeliveryState_Delivered);
              }
            }
          }
        }

        outer->notifyStateChanged(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::onConversationThreadDocumentFetcherPublicationGone(
                                                                                                    IConversationThreadDocumentFetcherPtr fetcher,
                                                                                                    ILocationPtr peerLocation,
                                                                                                    IPublicationMetaDataPtr metaData
                                                                                                    )
      {
        AutoRecursiveLock lock(getLock());
        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        // HERE - self destruct?
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadHost::PeerLocation => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String ConversationThreadHost::PeerLocation::log(const char *message) const
      {
        ZS_THROW_INVALID_ASSUMPTION_IF(!mPeerLocation)
        String peerURI = mPeerLocation->getPeerURI();
        String locationID = mPeerLocation->getLocationID();

        return String("ConversationThreadHost::PeerLocation [") + Stringize<PUID>(mID).string() + "] " + message + ", peer peer URI=" + peerURI + ", peer location ID=" + locationID;
      }

      //-----------------------------------------------------------------------
      String ConversationThreadHost::PeerLocation::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("host peer location id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               ILocation::toDebugString(mPeerLocation) +
               Thread::toDebugString(mSlaveThread) +
               IConversationThreadDocumentFetcher::toDebugString(mFetcher) +
               Helper::getDebugValue("message delivery states", mMessageDeliveryStates.size() > 0 ? Stringize<size_t>(mMessageDeliveryStates.size()).string() : String(), firstTime) +
               Helper::getDebugValue("incoming call handlers", mIncomingCallHandlers.size() > 0 ? Stringize<size_t>(mIncomingCallHandlers.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &ConversationThreadHost::PeerLocation::getLock() const
      {
        PeerContactPtr outer = mOuter.lock();
        if (!outer) return mBogusLock;
        return outer->getLock();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        AutoRecursiveLock lock(getLock());

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already shutdown"))
          return;
        }

        mShutdown = true;

        PeerContactPtr outer = mOuter.lock();
        if (outer) {
          ConversationThreadPtr baseThread = outer->getBaseThread();
          if (!baseThread) {
            for (CallHandlers::iterator iter = mIncomingCallHandlers.begin(); iter != mIncomingCallHandlers.end(); ++iter)
            {
              const CallID &callID = (*iter).first;
              baseThread->forHostOrSlave().requestRemoveIncomingCallHandler(callID);
            }
          }

          PeerLocationPtr pThis = mThisWeak.lock();
          if (pThis) {
            outer->notifyPeerLocationShutdown(pThis);
          }
        }

        if (mFetcher) {
          mFetcher->cancel();
          mFetcher.reset();
        }

        mSlaveThread.reset();

        mMessageDeliveryStates.clear();

        mIncomingCallHandlers.clear();

        // mPeerLocation.reset(); // DO NOT RESET THIS - LEAVE ALIVE UNTIL OBJECT IS DESTROYED!
      }

      //-----------------------------------------------------------------------
      void ConversationThreadHost::PeerLocation::step()
      {
        ZS_LOG_TRACE(log("step"))

        AutoRecursiveLock lock(getLock());
        if (isShutdown()) {cancel();}

      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
    }
  }
}
