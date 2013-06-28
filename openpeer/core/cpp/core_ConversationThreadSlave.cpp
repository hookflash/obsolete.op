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


#include <openpeer/core/internal/core_ConversationThreadSlave.h>
#include <openpeer/core/internal/core_Account.h>
#include <openpeer/core/internal/core_Contact.h>
#include <openpeer/core/internal/core_Call.h>
#include <openpeer/core/internal/core_Helper.h>
#include <openpeer/core/internal/core_Stack.h>

#include <openpeer/stack/IPublication.h>
#include <openpeer/stack/IHelper.h>

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

      typedef IConversationThreadParser::ThreadContact ThreadContact;
      typedef IConversationThreadParser::ThreadContactPtr ThreadContactPtr;
      typedef IConversationThreadParser::DialogList DialogList;
      typedef IConversationThreadParser::DialogMap DialogMap;
      typedef IConversationThreadParser::DialogIDList DialogIDList;
      typedef IConversationThreadParser::Thread Thread;
      typedef IConversationThreadParser::MessageMap MessageMap;
      typedef IConversationThreadParser::MessageReceiptMap MessageReceiptMap;
      typedef IConversationThreadParser::Details Details;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadSlaveForConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadSlavePtr IConversationThreadSlaveForConversationThread::create(
                                                                                       ConversationThreadPtr baseThread,
                                                                                       ILocationPtr peerLocation,
                                                                                       IPublicationMetaDataPtr metaData,
                                                                                       const SplitMap &split
                                                                                       )
      {
        return IConversationThreadSlaveFactory::singleton().createConversationThreadSlave(baseThread, peerLocation, metaData, split);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave
      #pragma mark

      //-----------------------------------------------------------------------
      const char *ConversationThreadSlave::toString(ConversationThreadSlaveStates state)
      {
        switch (state)
        {
          case ConversationThreadSlaveState_Pending:       return "Pending";
          case ConversationThreadSlaveState_Ready:         return "Ready";
          case ConversationThreadSlaveState_ShuttingDown:  return "Shutting down";
          case ConversationThreadSlaveState_Shutdown:      return "Shutdown";
        }
        return "UNDEFINED";
      }

      //-----------------------------------------------------------------------
      ConversationThreadSlave::ConversationThreadSlave(
                                                       IMessageQueuePtr queue,
                                                       AccountPtr account,
                                                       ILocationPtr peerLocation,
                                                       ConversationThreadPtr baseThread,
                                                       const char *threadID
                                                       ) :
        MessageQueueAssociator(queue),
        mID(zsLib::createPUID()),
        mBaseThread(baseThread),
        mAccount(account),
        mThreadID(threadID ? String(threadID) : stack::IHelper::randomString(32)),
        mPeerLocation(peerLocation),
        mCurrentState(ConversationThreadSlaveState_Pending),
        mConvertedToHostBecauseOriginalHostLikelyGoneForever(false)
      {
        ZS_LOG_BASIC(log("created"))
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::init()
      {
        mFetcher = IConversationThreadDocumentFetcher::create(mThisWeak.lock(), mAccount.lock()->forConversationThread().getRepository());
      }

      //-----------------------------------------------------------------------
      ConversationThreadSlave::~ConversationThreadSlave()
      {
        if(isNoop()) return;
        
        mThisWeak.reset();
        ZS_LOG_BASIC(log("destroyed"))
        cancel();
      }

      //-----------------------------------------------------------------------
      String ConversationThreadSlave::toDebugString(ConversationThreadSlavePtr thread, bool includeCommaPrefix)
      {
        if (!thread) return includeCommaPrefix ? String(", slave thread=(null)") : String("slave thread=(null)");
        return thread->getDebugValueString(includeCommaPrefix);
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IConversationThreadHostSlaveBase
      #pragma mark

      //-----------------------------------------------------------------------
      String ConversationThreadSlave::getThreadID() const
      {
        AutoRecursiveLock lock(getLock());
        return mThreadID;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::shutdown()
      {
        AutoRecursiveLock lock(getLock());
        cancel();
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadSlave::isHostThreadOpen() const
      {
        typedef IConversationThreadParser::Details Details;

        if (mConvertedToHostBecauseOriginalHostLikelyGoneForever) return false;
        if (!mHostThread) return false;
        if (!mHostThread->details()) return false;
        return Details::ConversationThreadState_Open == mHostThread->details()->state();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::notifyPublicationUpdated(
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

        if (!mHostThread) {
          ZS_LOG_TRACE(log("holding an extra reference to ourselves until fetcher completes it's job"))
          mSelfHoldingStartupReferenceUntilPublicationFetchCompletes = mThisWeak.lock();
        }
        mFetcher->notifyPublicationUpdated(peerLocation, metaData);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::notifyPublicationGone(
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
        mFetcher->notifyPublicationGone(peerLocation, metaData);
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::notifyPeerDisconnected(ILocationPtr peerLocation)
      {
        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("notification of a peer shutdown") + ILocation::toDebugString(peerLocation))
          return;
        }
        mFetcher->notifyPeerDisconnected(peerLocation);
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadSlave::sendMessages(const MessageList &messages)
      {
        if (messages.size() < 1) return false;

        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("cannot send messages during shutdown"))
          return false;
        }

        if (!mSlaveThread) {
          ZS_LOG_DEBUG(log("cannot send messages without a slave thread object"))
          return false;
        }

        mSlaveThread->updateBegin();
        mSlaveThread->addMessages(messages);
        publish(mSlaveThread->updateEnd(), false);

        // kick the conversation thread step routine asynchronously to ensure
        // the thread has a subscription state to its peer
        IConversationThreadSlaveAsyncProxy::create(mThisWeak.lock())->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      Time ConversationThreadSlave::getHostCreationTime() const
      {
        AutoRecursiveLock lock(getLock());
        if (mHostThread) return Time();
        return mHostThread->details()->created();
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadSlave::safeToChangeContacts() const
      {
        AutoRecursiveLock lock(getLock());
        if ((isShutdown()) ||
            (isShuttingDown())) {
          ZS_LOG_DEBUG(log("cannot change contacts is shutting down"))
          return false;
        }
        if (!isHostThreadOpen()) {
          ZS_LOG_DEBUG(log("cannot change contacts since host thread is not open"))
          return false;
        }
        if (!mSlaveThread) {
          ZS_LOG_DEBUG(log("cannot change contacts since slave thread is not ready"))
          return false;
        }

        // we can ask the host to change contacts on our behalf...
        return true;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::getContacts(ThreadContactMap &outContacts) const
      {
        if (!mHostThread) {
          ZS_LOG_DEBUG(log("cannot get contacts without a host thread"))
          return;
        }

        outContacts = mHostThread->contacts()->contacts();
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadSlave::inConversation(ContactPtr contact) const
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
      void ConversationThreadSlave::addContacts(const ContactProfileInfoList &contacts)
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_ASSUMPTION_IF(!safeToChangeContacts())

        ThreadContactMap contactMap;
        for (ContactProfileInfoList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
        {
          const ContactProfileInfo &info = (*iter);
          ContactPtr contact = Contact::convert(info.mContact);
          ThreadContactPtr threadContact = ThreadContact::create(contact, info.mProfileBundleEl);
          contactMap[contact->forConversationThread().getPeerURI()] = threadContact;
        }

        mSlaveThread->updateBegin();
        mSlaveThread->setContactsToAdd(contactMap);
        publish(mSlaveThread->updateEnd(), false);

        IConversationThreadSlaveAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::removeContacts(const ContactList &contacts)
      {
        AutoRecursiveLock lock(getLock());
        ZS_THROW_INVALID_ASSUMPTION_IF(!safeToChangeContacts())

        ContactURIList contactIDList;
        for (ContactList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
        {
          ContactPtr contact = Contact::convert(*iter);
          contactIDList.push_back(contact->forConversationThread().getPeerURI());
        }

        mSlaveThread->updateBegin();
        mSlaveThread->setContactsToRemove(contactIDList);
        publish(mSlaveThread->updateEnd(), false);

        IConversationThreadSlaveAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      IConversationThread::ContactStates ConversationThreadSlave::getContactState(ContactPtr contact) const
      {
        AutoRecursiveLock lock(getLock());

        ContactPtr hostContact = getHostContact();
        if (!hostContact) {
          ZS_LOG_ERROR(Basic, log("host contact not found on slave thread"))
          return IConversationThread::ContactState_NotApplicable;
        }
        if (hostContact->forConversationThread().getPeerURI() != contact->forConversationThread().getPeerURI()) {
          ZS_LOG_DEBUG(log("contact state requested is no the host contact thus do not know state of peer connnection"))
          return IConversationThread::ContactState_NotApplicable;
        }

        if (mPeerLocation) {
          if (mPeerLocation->isConnected()) {
            return IConversationThread::ContactState_Connected;
          }
        }

        switch (hostContact->forConversationThread().getPeer()->getFindState()) {
          case IPeer::PeerFindState_Idle:       return (mPeerLocation ? IConversationThread::ContactState_Disconnected : IConversationThread::ContactState_NotApplicable);
          case IPeer::PeerFindState_Finding:    return IConversationThread::ContactState_Finding;
          case IPeer::PeerFindState_Completed:  return (mPeerLocation ? IConversationThread::ContactState_Disconnected : IConversationThread::ContactState_NotApplicable);
        }

        return IConversationThread::ContactState_NotApplicable;
      }

      //-----------------------------------------------------------------------
      bool ConversationThreadSlave::placeCalls(const PendingCallMap &pendingCalls)
      {
        AutoRecursiveLock lock(getLock());
        if (!mSlaveThread) {
          ZS_LOG_DEBUG(log("no host thread to clean call from..."))
          return false;
        }

        DialogList additions;

        const DialogMap &dialogs = mSlaveThread->dialogs();
        for (PendingCallMap::const_iterator iter = pendingCalls.begin(); iter != pendingCalls.end(); ++iter)
        {
          const CallPtr &call = (*iter).second;
          DialogMap::const_iterator found = dialogs.find(call->forConversationThread().getCallID());

          if (found == dialogs.end()) {
            ZS_LOG_DEBUG(log("added call") + call->forConversationThread().getDialog()->getDebugValueString())

            additions.push_back(call->forConversationThread().getDialog());
          }
        }

        // publish the changes now...
        mSlaveThread->updateBegin();
        mSlaveThread->updateDialogs(additions);
        publish(mSlaveThread->updateEnd(), false);

        IConversationThreadSlaveAsyncProxy::create(mThisWeak.lock())->onStep();
        return true;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::notifyCallStateChanged(CallPtr call)
      {
        AutoRecursiveLock lock(getLock());

        if (!mSlaveThread) {
          ZS_LOG_WARNING(Detail, log("no slave thread to change the call state call from..."))
          return;
        }
        if (!mHostThread) {
          ZS_LOG_WARNING(Detail, log("no host thread to check if call exists from..."))
        }

        DialogPtr dialog = call->forConversationThread().getDialog();
        if (!dialog) {
          ZS_LOG_DEBUG(log("call does not have a dialog yet and is not ready"))
          return;
        }

        DialogList updates;
        updates.push_back(dialog);

        // publish the changes now...
        mSlaveThread->updateBegin();
        mSlaveThread->updateDialogs(updates);
        publish(mSlaveThread->updateEnd(), false);

        IConversationThreadSlaveAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::notifyCallCleanup(CallPtr call)
      {
        ZS_THROW_INVALID_ARGUMENT_IF(!call)

        ZS_LOG_DEBUG(log("call cleanup called") + Call::toDebugString(call))

        AutoRecursiveLock lock(getLock());

        if (!mSlaveThread) {
          ZS_LOG_DEBUG(log("no slave thread to clean call from..."))
          return;
        }

        const DialogMap &dialogs = mSlaveThread->dialogs();
        DialogMap::const_iterator found = dialogs.find(call->forConversationThread().getCallID());

        if (found == dialogs.end()) {
          ZS_LOG_WARNING(Detail, log("this call is not present on the host conversation thread thus cannot be cleaned"))
          return;
        }

        DialogIDList removeCallIDs;
        removeCallIDs.push_back(call->forConversationThread().getCallID());

        // publish the changes now...
        mSlaveThread->updateBegin();
        mSlaveThread->removeDialogs(removeCallIDs);
        publish(mSlaveThread->updateEnd(), false);

        IConversationThreadSlaveAsyncProxy::create(mThisWeak.lock())->onStep();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::gatherDialogReplies(
                                                        const char *callID,
                                                        LocationDialogMap &outDialogs
                                                        ) const
      {
        AutoRecursiveLock lock(getLock());
        if (!mHostThread) {
          ZS_LOG_WARNING(Detail, log("unable to gather dialogs from slave's host as host thread object is not valid"))
          return;
        }
        if (!mPeerLocation) {
          ZS_LOG_WARNING(Detail, log("unable to gather dialogs from slave's host as peer location is not valid"))
          return;
        }

        const DialogMap &dialogs = mHostThread->dialogs();
        DialogMap::const_iterator found = dialogs.find(callID);
        if (found == dialogs.end()) {
          ZS_LOG_TRACE(log("did not find any dialog replies") + ", call ID=" + callID)
          return;
        }

        const DialogPtr &dialog = (*found).second;
        outDialogs[mPeerLocation->getLocationID()] = dialog;

        ZS_LOG_TRACE(log("found dialog reply") + ", call ID=" + callID)
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IConversationThreadSlaveForConversationThread
      #pragma mark

      //-----------------------------------------------------------------------
      ConversationThreadSlavePtr ConversationThreadSlave::create(
                                                                 ConversationThreadPtr baseThread,
                                                                 ILocationPtr peerLocation,
                                                                 IPublicationMetaDataPtr metaData,
                                                                 const SplitMap &split
                                                                 )
      {
        AccountPtr account = baseThread->forHostOrSlave().getAccount();
        if (!account) return ConversationThreadSlavePtr();

        String hostThreadID = stack::IHelper::get(split, OPENPEER_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX);
        ZS_THROW_INVALID_ARGUMENT_IF(hostThreadID.size() < 1)

        ConversationThreadSlavePtr pThis(new ConversationThreadSlave(IStackForInternal::queueCore(), account, peerLocation, baseThread, hostThreadID));
        pThis->mThisWeak = pThis;
        pThis->init();
        pThis->notifyPublicationUpdated(peerLocation, metaData, split);
        return pThis;
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IConversationThreadDocumentFetcherDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::onConversationThreadDocumentFetcherPublicationUpdated(
                                                                                          IConversationThreadDocumentFetcherPtr fetcher,
                                                                                          ILocationPtr peerLocation,
                                                                                          IPublicationPtr publication
                                                                                          )
      {
        ZS_LOG_DEBUG(log("publication was updated notification received") + IPublication::toDebugString(publication))

        AutoRecursiveLock lock(getLock());

        if (mSelfHoldingStartupReferenceUntilPublicationFetchCompletes) {
          ZS_LOG_DEBUG(log("extra reference to ourselves is removed as publication fetcher is complete"))
          mSelfHoldingStartupReferenceUntilPublicationFetchCompletes.reset();
        }

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_WARNING(Detail, log("notified about updated publication after shutdown") + IPublication::toDebugString(publication))
          return;
        }

        ZS_THROW_BAD_STATE_IF(fetcher != mFetcher)
        ZS_THROW_INVALID_ARGUMENT_IF(!publication)

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("account is gone thus thread must shutdown - happened when receiving updated document") + IPublication::toDebugString(publication))
          cancel();
          return;
        }

        ConversationThreadPtr baseThread = mBaseThread.lock();
        if (!baseThread) {
          ZS_LOG_WARNING(Detail, log("base thread is gone thus thread must shutdown - happened when receiving updated document") + IPublication::toDebugString(publication))
          cancel();
          return;
        }

        if (!mHostThread) {
          mHostThread = Thread::create(account, publication);
          if (!mHostThread) {
            ZS_LOG_WARNING(Detail, log("failed to parse conversation thread from host") + IPublication::toDebugString(publication))
            cancel();
            return;
          }
          baseThread->forSlave().notifyAboutNewThreadNowIfNotNotified(mThisWeak.lock());
        } else {
          mHostThread->updateFrom(account, publication);
        }


        //.......................................................................
        // examine all the newly received messages...

        const MessageList &messagesChanged = mHostThread->messagedChanged();
        for (MessageList::const_iterator iter = messagesChanged.begin(); iter != messagesChanged.end(); ++iter)
        {
          const MessagePtr &message = (*iter);

          if (mSlaveThread) {
            const MessageMap &sentMessages = mSlaveThread->messagesAsMap();
            MessageMap::const_iterator found = sentMessages.find(message->messageID());
            if (found != sentMessages.end()) {
              ZS_LOG_TRACE(log("no need to notify about messages sent by ourself") + message->getDebugValueString())
              continue;
            }
          }

          ZS_LOG_TRACE(log("notifying of message received") + message->getDebugValueString())
          baseThread->forHostOrSlave().notifyMessageReceived(message);
        }

        //.......................................................................
        // examine all the acknowledged messages

        if (mSlaveThread) {
          const MessageList &messages = mSlaveThread->messages();
          const MessageMap &messagesMap = mSlaveThread->messagesAsMap();

          // can only examine message receipts that are part of the slave thread...
          const MessageReceiptMap &messageReceiptsChanged = mHostThread->messageReceiptsChanged();
          for (MessageReceiptMap::const_iterator iter = messageReceiptsChanged.begin(); iter != messageReceiptsChanged.end(); ++iter)
          {
            const MessageID &id = (*iter).first;

            ZS_LOG_TRACE(log("examining message receipt") + ", receipt ID=" + id)

            // check to see if this receipt has already been marked as delivered...
            MessageDeliveryStatesMap::iterator found = mMessageDeliveryStates.find(id);
            if (found != mMessageDeliveryStates.end()) {
              DeliveryStatePair &deliveryStatePair = (*found).second;
              IConversationThread::MessageDeliveryStates &deliveryState = deliveryStatePair.second;
              if (IConversationThread::MessageDeliveryState_Delivered == deliveryState) {
                ZS_LOG_DEBUG(log("message receipt was already notified as delivered thus no need to notify any further") + ", message ID=" + id)
                continue;
              }
            }

            MessageMap::const_iterator foundInMap = messagesMap.find(id);
            if (foundInMap == messagesMap.end()) {
              ZS_LOG_WARNING(Detail, log("slave never send this message to the host (message receipt acking a different slave?)") + ", receipt id=" + id)
              continue;
            }

            bool foundMessageID = false;

            // find this receipt on the message list... (might not exist if
            // receipt is for a message from a different contact)
            MessageList::const_reverse_iterator messageIter = messages.rbegin();
            for (; messageIter != messages.rend(); ++messageIter)
            {
              const MessagePtr &message = (*messageIter);
              if (message->messageID() == id) {
                ZS_LOG_TRACE(log("found message matching receipt") + ", receipt id=" + id)
                foundMessageID = true;
              }

              ZS_LOG_TRACE(log("processing slave message") + ", found=" + (foundMessageID ? "true" : "false") + message->getDebugValueString())

              if (foundMessageID) {
                // first check if this delivery was already sent...
                found = mMessageDeliveryStates.find(message->messageID());
                if (found != mMessageDeliveryStates.end()) {
                  // check to see if this message was already marked as delivered
                  DeliveryStatePair &deliveryStatePair = (*found).second;
                  IConversationThread::MessageDeliveryStates &deliveryState = deliveryStatePair.second;
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
                  mMessageDeliveryStates[message->messageID()] = DeliveryStatePair(zsLib::now(), IConversationThread::MessageDeliveryState_Delivered);
                }

                // this message is now considered acknowledged so tell the master thread of the new state...
                baseThread->forHostOrSlave().notifyMessageDeliveryStateChanged(id, IConversationThread::MessageDeliveryState_Delivered);
              }
            }
          }
        }


        //.......................................................................
        // figure out what dialogs were incoming but need to be removed...

        const DialogIDList &removedDialogs = mHostThread->dialogsRemoved();
        for (DialogIDList::const_iterator iter = removedDialogs.begin(); iter != removedDialogs.end(); ++iter) {
          const String &dialogID = (*iter);
          CallHandlers::iterator found = mIncomingCallHandlers.find(dialogID);
          if (found == mIncomingCallHandlers.end()) {
            ZS_LOG_DEBUG(log("call object is not present (ignored)") + ", call ID=" + dialogID)
            continue;
          }

          // this incoming call is now gone, clean it out...
          CallPtr &call = (*found).second;

          ZS_LOG_DEBUG(log("call object is now gone") + Call::toDebugString(call))

          call->forConversationThread().notifyConversationThreadUpdated();

          mIncomingCallHandlers.erase(found);

          ZS_LOG_DEBUG(log("removed incoming calll handler") + ", total handlers=" + Stringize<size_t>(mIncomingCallHandlers.size()).string())
        }

        //.......................................................................
        // check for dialogs that are now incoming/updated

        const DialogMap &changedDialogs = mHostThread->dialogsChanged();
        for (DialogMap::const_iterator iter = changedDialogs.begin(); iter != changedDialogs.end(); ++iter)
        {
          const String &dialogID = (*iter).first;
          const DialogPtr &dialog = (*iter).second;

          String selfPeerURI = account->forConversationThread().getSelfContact()->forConversationThread().getPeerURI();

          ZS_LOG_TRACE("call detected" + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI + ILocation::toDebugString(mPeerLocation))

          CallHandlers::iterator found = mIncomingCallHandlers.find(dialogID);
          if (found != mIncomingCallHandlers.end()) {
            CallPtr &call = (*found).second;

            ZS_LOG_DEBUG(log("call object is updated") + dialog->getDebugValueString() + Call::toDebugString(call))
            call->forConversationThread().notifyConversationThreadUpdated();
            continue;
          }

          if (dialog->calleePeerURI() != selfPeerURI) {
            if (dialog->callerPeerURI() == selfPeerURI) {
              ZS_LOG_DEBUG(log("call detected this must be a reply to a call previously placed") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)
              baseThread->forHostOrSlave().notifyPossibleCallReplyStateChange(dialogID);
            } else {
              ZS_LOG_WARNING(Detail, log("incoming call detected but the call is not going to this contact") + dialog->getDebugValueString() + ", our peer URI=" + selfPeerURI)
            }
            continue;
          }

          String callerPeerURI = dialog->callerPeerURI();
          ContactPtr contact = account->forConversationThread().findContact(callerPeerURI);
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
          baseThread->forHostOrSlave().requestAddIncomingCallHandler(dialogID, mThisWeak.lock(), call);
        }


        //.......................................................................
        // create the slave thread if it's not created

        bool mustPublish = false;
        bool mustPublishPermission = false;

        if (!mSlaveThread) {
          mSlaveThread = Thread::create(
                                        account,
                                        IConversationThreadParser::Thread::ThreadType_Slave,
                                        account->forConversationThread().getSelfLocation(),
                                        baseThread->forHostOrSlave().getThreadID(),
                                        mThreadID,
                                        NULL,
                                        NULL,
                                        Details::ConversationThreadState_None,
                                        publication->getPublishedLocation()
                                        );

          if (!mSlaveThread) {
            ZS_LOG_WARNING(Detail, log("failed to create slave thread object - happened when receiving updated document") + IPublication::toDebugString(publication))
            cancel();
            return;
          }

          mustPublishPermission = mustPublish = true;
        }


        //.......................................................................
        // start updating the slave thread information...

        mSlaveThread->updateBegin();

        if (messagesChanged.size() > 0) {
          const MessagePtr &lastMessage = messagesChanged.back();
          mSlaveThread->setReceived(lastMessage);
        }


        //.......................................................................
        // figure out all the contact changes

        ThreadContactMap hostContacts = mHostThread->contacts()->contacts();

        ThreadContactMap replacementAddContacts;

        // check to see which contacts do not need to be added anymore and remove them...
        const ThreadContactMap &addContacts = mSlaveThread->contacts()->addContacts();
        for (ThreadContactMap::const_iterator iter = addContacts.begin(); iter != addContacts.end(); ++iter)
        {
          const String &contactID = (*iter).first;
          const ThreadContactPtr &threadContact = (*iter).second;

          ThreadContactMap::const_iterator found = hostContacts.find(contactID);
          if (found == hostContacts.end()) {
            // this contact still needs to be added...
            replacementAddContacts[contactID] = threadContact;
          }
        }


        //.......................................................................
        // these contacts still need to be added (if any)...

        mSlaveThread->setContactsToAdd(replacementAddContacts);


        //.......................................................................
        // figure out which contacts no longer need removal

        ContactURIList replacementRemoveContacts;
        const ContactURIList &removeContacts = mSlaveThread->contacts()->removeContacts();
        for (ContactURIList::const_iterator iter = removeContacts.begin(); iter != removeContacts.end(); ++iter)
        {
          const String &contactID = (*iter);

          ThreadContactMap::const_iterator found = hostContacts.find(contactID);
          if (found != hostContacts.end()) {
            // this contact still needs removing...
            replacementRemoveContacts.push_back(contactID);
          }
        }

        //.......................................................................
        // these contacts still need to be removed (if any)....

        mSlaveThread->setContactsToRemove(replacementRemoveContacts);


        //.......................................................................
        // publish the changes

        bool changesMade = mSlaveThread->updateEnd();
        mustPublish = changesMade || mustPublish;

        publish(mustPublish, mustPublishPermission);

        step();

        // notify the outer thread that this thread's state might have changed...
        baseThread->forHostOrSlave().notifyStateChanged(mThisWeak.lock());
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::onConversationThreadDocumentFetcherPublicationGone(
                                                                                       IConversationThreadDocumentFetcherPtr fetcher,
                                                                                       ILocationPtr peerLocation,
                                                                                       IPublicationMetaDataPtr metaData
                                                                                       )
      {
        AutoRecursiveLock lock(getLock());

        if (mSelfHoldingStartupReferenceUntilPublicationFetchCompletes) {
          ZS_LOG_WARNING(Detail, log("extra reference to ourselves is removed as publication fetcher is complete (albeit a failure case)"))
          mSelfHoldingStartupReferenceUntilPublicationFetchCompletes.reset();
        }

        if ((isShuttingDown()) ||
            (isShutdown())) {
          ZS_LOG_DEBUG(log("notified publication gone after shutdown") + IPublicationMetaData::toDebugString(metaData))
          return;
        }
        ZS_THROW_BAD_STATE_IF(fetcher != mFetcher)

        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        //*********************************************************************
        // HERE!!!! - self descruct?
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => ITimerDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::onTimer(TimerPtr timer)
      {
        ZS_LOG_DEBUG(log("timer"))
        step();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThreadSlave => IPeerSubscriptionDelegate
      #pragma mark

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::onPeerSubscriptionShutdown(IPeerSubscriptionPtr subscription)
      {
        AutoRecursiveLock lock(getLock());
        if (subscription != mHostSubscription) {
          ZS_LOG_DEBUG(log("received peer subscription shutdown on an obsolete subscription"))
          return;
        }

        // be sure it's truly gone...
        mHostSubscription->cancel();
        mHostSubscription.reset();

        step();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::onPeerSubscriptionFindStateChanged(
                                                                       IPeerSubscriptionPtr subscription,
                                                                       IPeerPtr peer,
                                                                       PeerFindStates state
                                                                       )
      {
        AutoRecursiveLock lock(getLock());
        step();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::onPeerSubscriptionLocationConnectionStateChanged(
                                                                                     IPeerSubscriptionPtr subscription,
                                                                                     ILocationPtr location,
                                                                                     LocationConnectionStates state
                                                                                     )
      {
        AutoRecursiveLock lock(getLock());

        ZS_LOG_DEBUG(log("peer subscriptions location changed called"))
        if (subscription != mHostSubscription) {
          ZS_LOG_WARNING(Detail, log("peer subscription notification came from obsolete subscription") + IPeerSubscription::toDebugString(subscription))
          return;
        }

        ContactPtr hostContact = getHostContact();
        if (!hostContact) {
          cancel();
          return;
        }

        IPeerPtr peer = hostContact->forConversationThread().getPeer();

        LocationListPtr peerLocations = peer->getLocationsForPeer(true);

        bool mustConvertToHost = false;

        if ((peerLocations->size() > 0) &&
            (mSlaveThread) &&
            (!mConvertedToHostBecauseOriginalHostLikelyGoneForever)) {
          ZS_LOG_DEBUG(log("peer locations are detected - checking if original host location is still active") + ILocation::toDebugString(mPeerLocation))

          bool foundHostLocation = false;
          for (LocationList::iterator iter = peerLocations->begin(); iter != peerLocations->end(); ++iter)
          {
            ILocationPtr &peerLocation = (*iter);
            ZS_LOG_TRACE(log("detected peer location") + ", peer peer URI=" + peerLocation->getPeerURI() + ILocation::toDebugString(peerLocation))
            if (peerLocation->getLocationID() == mPeerLocation->getLocationID()) {
              ZS_LOG_DEBUG(log("found host's peer location expected") + ILocation::toDebugString(peerLocation))
              foundHostLocation = true;
              break;
            }
          }

          if (!foundHostLocation) {
            ZS_LOG_WARNING(Detail, log("found the host contact's peer location but it's not the location expected") + ", expecting: " + ILocation::toDebugString(mPeerLocation, false))
            // found the host but it's not the host we expected... It's likely
            // the peer restarted their application thus we need to make sure
            // any undelivered messages get pushed into a new conversation thread

            const MessageList &messages = mSlaveThread->messages();
            if (messages.size() > 0) {
              const MessagePtr &message = messages.back();
              ZS_LOG_TRACE(log("examining message delivery state for message") + message->getDebugValueString())

              MessageDeliveryStatesMap::iterator found = mMessageDeliveryStates.find(message->messageID());
              if (found == mMessageDeliveryStates.end()) {
                ZS_LOG_DEBUG(log("found message that has not delivered yet") + message->getDebugValueString())
                mustConvertToHost = true;
                goto done_checking_for_undelivered_messages;
              }

              DeliveryStatePair &deliveryPair = (*found).second;
              MessageDeliveryStates &state = deliveryPair.second;
              if (IConversationThread::MessageDeliveryState_Delivered != state) {
                ZS_LOG_DEBUG(log("found message that has not delivered yet") + message->getDebugValueString() + ", state=" + IConversationThread::toString(state))
                mustConvertToHost = true;
                goto done_checking_for_undelivered_messages;
              }
            }
          }
        }

      done_checking_for_undelivered_messages:

        if (!mHostThread) {
          ZS_LOG_WARNING(Detail, log("no host thread associated with this slave"))
          mustConvertToHost = false;
        }

        if (mustConvertToHost) {
          ConversationThreadPtr baseThread = mBaseThread.lock();
          if (!baseThread) {
            ZS_LOG_WARNING(Detail, log("base conversation thread is gone (thus slave must self destruct)"))
            cancel();
            return;
          }

          ZS_LOG_DETAIL(log("converting slave to host as likely original host is gone forever and messages need delivering") + ILocation::toDebugString(mPeerLocation))
          mConvertedToHostBecauseOriginalHostLikelyGoneForever = true;
          baseThread->forSlave().convertSlaveToClosedHost(mThisWeak.lock(), mHostThread, mSlaveThread);
        }
        
        step();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::onPeerSubscriptionMessageIncoming(
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
      #pragma mark ConversationThreadSlave => (internal)
      #pragma mark

      //-----------------------------------------------------------------------
      String ConversationThreadSlave::log(const char *message) const
      {
        String baseThreadID;
        ConversationThreadPtr baseThread = mBaseThread.lock();

        if (baseThread) baseThreadID = baseThread->forHostOrSlave().getThreadID();

        return String("ConversationThreadSlave [") + Stringize<PUID>(mID).string() + "] " + message + ", base thread ID=" + baseThreadID + ", thread ID=" + mThreadID;
      }

      //-----------------------------------------------------------------------
      String ConversationThreadSlave::getDebugValueString(bool includeCommaPrefix) const
      {
        AutoRecursiveLock lock(getLock());
        ConversationThreadPtr base = mBaseThread.lock();
        bool firstTime = !includeCommaPrefix;
        return Helper::getDebugValue("slave thread id", Stringize<typeof(mID)>(mID).string(), firstTime) +
               Helper::getDebugValue("slave thread id (s)", mThreadID, firstTime) +
               Helper::getDebugValue("base thread id (s)", base ? base->forHostOrSlave().getThreadID() : String(), firstTime) +
               Helper::getDebugValue("state", toString(mCurrentState), firstTime) +
               ILocation::toDebugString(mPeerLocation) +
               IConversationThreadDocumentFetcher::toDebugString(mFetcher) +
               Thread::toDebugString(mHostThread) +
               Thread::toDebugString(mSlaveThread) +
               IPeerSubscription::toDebugString(mHostSubscription) +
               Helper::getDebugValue("convert to host", mConvertedToHostBecauseOriginalHostLikelyGoneForever ? String("true") : String(), firstTime) +
               Helper::getDebugValue("delivery states", mMessageDeliveryStates.size() > 0 ? Stringize<size_t>(mMessageDeliveryStates.size()).string() : String(), firstTime) +
               Helper::getDebugValue("incoming call handlers", mIncomingCallHandlers.size() > 0 ? Stringize<size_t>(mIncomingCallHandlers.size()).string() : String(), firstTime);
      }

      //-----------------------------------------------------------------------
      RecursiveLock &ConversationThreadSlave::getLock() const
      {
        AccountPtr account = mAccount.lock();
        if (!account) return mBogusLock;
        return account->forConversationThread().getLock();
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::cancel()
      {
        ZS_LOG_DEBUG(log("cancel called"))

        if (isShutdown()) {
          ZS_LOG_DEBUG(log("already shutdown"))
          return;
        }

        mSelfHoldingStartupReferenceUntilPublicationFetchCompletes.reset();

        if (!mGracefulShutdownReference) mGracefulShutdownReference = mThisWeak.lock();

        setState(ConversationThreadSlaveState_ShuttingDown);

        ConversationThreadPtr baseThread = mBaseThread.lock();

        if (baseThread) {
          for (CallHandlers::iterator iter = mIncomingCallHandlers.begin(); iter != mIncomingCallHandlers.end(); ++iter)
          {
            const CallID &callID = (*iter).first;
            baseThread->forHostOrSlave().requestRemoveIncomingCallHandler(callID);
          }
        }

        if (mFetcher) {
          mFetcher->cancel();
          mFetcher.reset();
        }

        if (mHostSubscription) {
          mHostSubscription->cancel();
          mHostSubscription.reset();
        }

        setState(ConversationThreadSlaveState_Shutdown);

        mGracefulShutdownReference.reset();

        mMessageDeliveryStates.clear();

        mHostThread.reset();
        mSlaveThread.reset();

        mPeerLocation.reset();

        ZS_LOG_DEBUG(log("cancel completed"))
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::step()
      {
        ZS_LOG_DEBUG(log("step"))

        AutoRecursiveLock lock(getLock());

        if ((isShuttingDown()) ||
            (isShutdown())) {
          cancel();
          return;
        }

        AccountPtr account = mAccount.lock();
        stack::IAccountPtr stackAccount = account->forConversationThread().getStackAccount();
        ConversationThreadPtr baseThread = mBaseThread.lock();
        if ((!account) ||
            (!stackAccount) ||
            (!baseThread)) {
          ZS_LOG_WARNING(Detail, log("account, stack account or base thread is gone thus thread must shutdown"))
          cancel();
          return;
        }

        if (!mHostThread) {
          ZS_LOG_DEBUG(log("waiting for host thread to be ready..."))
          return;
        }

        if (!mSlaveThread) {
          ZS_LOG_DEBUG(log("waiting for slave thread to be ready..."))
        }

        setState(ConversationThreadSlaveState_Ready);

        bool requiresSubscription = false;
        bool requiresTimer = false;

        // check to see if there are undelivered messages, if so we will need a subscription...
        const MessageList &messages = mSlaveThread->messages();
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

        if (mSlaveThread->dialogs().size() > 0) {
          ZS_LOG_DEBUG(log("slave thread has dialogs (i.e. calls) so a subscription is required"))
          requiresSubscription = true;
        }

        ContactPtr hostContact = getHostContact();
        if (!hostContact) {
          cancel();
          return;
        }

        if (requiresSubscription) {
          ZS_LOG_DEBUG(log("subscription to host is required") + getDebugValueString())
          if (!mHostSubscription) {
            mHostSubscription = IPeerSubscription::subscribe(hostContact->forConversationThread().getPeer(), mThisWeak.lock());
          }
        } else {
          ZS_LOG_DEBUG(log("subscription to host is NOT required") + getDebugValueString())
          if (mHostSubscription) {
            mHostSubscription->cancel();
            mHostSubscription.reset();
          }
        }

        if (requiresTimer) {
          ZS_LOG_DEBUG(log("timer for peer slave is required") + getDebugValueString())
          if (!mHostMessageDeliveryTimer) {
            mHostMessageDeliveryTimer = Timer::create(mThisWeak.lock(), Seconds(1));
          }
        } else {
          ZS_LOG_DEBUG(log("timer for peer slave is NOT required") + getDebugValueString())
          if (mHostMessageDeliveryTimer) {
            mHostMessageDeliveryTimer->cancel();
            mHostMessageDeliveryTimer.reset();
          }
        }

        Time tick = zsLib::now();

        // scope: fix the state of pending messages...
        if (mHostSubscription) {
          IPeerPtr peer = hostContact->forConversationThread().getPeer();
          IPeer::PeerFindStates state = peer->getFindState();
          ZS_LOG_DEBUG(log("host subscription state") + ", state=" + IPeer::toString(state))

          LocationListPtr peerLocations = peer->getLocationsForPeer(false);

          const MessageList &messages = mSlaveThread->messages();

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
              ZS_LOG_DEBUG(log("state must now be set to undeliverable") + message->getDebugValueString() + ", peer find state=" + IPeer::toString(state) + ", last state changed time=" + Stringize<Time>(lastStateChangeTime).string() + ", current time=" + Stringize<Time>(tick).string())
              mMessageDeliveryStates[message->messageID()] = DeliveryStatePair(zsLib::now(), IConversationThread::MessageDeliveryState_UserNotAvailable);
              baseThread->forHostOrSlave().notifyMessageDeliveryStateChanged(message->messageID(), IConversationThread::MessageDeliveryState_UserNotAvailable);

              // tell the application to push this message out as a push notification
              baseThread->forHostOrSlave().notifyMessagePush(message, hostContact);
            }
          }
        }

        baseThread->forHostOrSlave().notifyContactState(mThisWeak.lock(), hostContact, getContactState(hostContact));
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::setState(ConversationThreadSlaveStates state)
      {
        if (state == mCurrentState) return;

        ZS_LOG_BASIC(log("state changed") + ", old state=" + toString(mCurrentState) + ", new state=" + toString(state))

        mCurrentState = state;
      }

      //-----------------------------------------------------------------------
      void ConversationThreadSlave::publish(
                                            bool publishSlavePublication,
                                            bool publishSlavePermissionPublication
                                            ) const
      {
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

        if (!mSlaveThread) {
          ZS_LOG_WARNING(Detail, log("slave thread is not available thus cannot publish any publications"))
          return;
        }

        if (publishSlavePermissionPublication) {
          ZS_LOG_DEBUG(log("publishing slave thread permission document"))
          repo->publish(IPublicationPublisherDelegateProxy::createNoop(getAssociatedMessageQueue()), mSlaveThread->permissionPublication());
        }

        if (publishSlavePublication) {
          ZS_LOG_DEBUG(log("publishing slave thread document"))
          repo->publish(IPublicationPublisherDelegateProxy::createNoop(getAssociatedMessageQueue()), mSlaveThread->publication());
        }
      }

      //-----------------------------------------------------------------------
      ContactPtr ConversationThreadSlave::getHostContact() const
      {
        if (!mHostThread) {
          ZS_LOG_WARNING(Detail, log("cannot obtain host contact because host thread is NULL"))
          return ContactPtr();
        }
        IPublicationPtr publication = mHostThread->publication();
        if (!publication) {
          ZS_LOG_ERROR(Detail, log("cannot obtain host contact because host thread publication is NULL"))
          return ContactPtr();
        }

        AccountPtr account = mAccount.lock();
        if (!account) {
          ZS_LOG_WARNING(Detail, log("cannot obtain host contact because account is gone"))
          return ContactPtr();
        }

        ContactPtr contact = account->forConversationThread().findContact(publication->getCreatorLocation()->getPeer()->getPeerURI());
        if (!contact) {
          ZS_LOG_WARNING(Detail, log("cannot obtain host contact because contact was not found") + IPublication::toDebugString(publication))
        }
        return contact;
      }

    }
  }
}
