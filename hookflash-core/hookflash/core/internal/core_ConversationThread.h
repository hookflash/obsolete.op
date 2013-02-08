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
#include <hookflash/core/IConversationThread.h>
#include <hookflash/core/internal/core_IConversationThreadParser.h>

//#include <hookflash/stack/types.h>
#include <hookflash/stack/IHelper.h>

#define HOOKFLASH_CONVERSATION_THREAD_TYPE_INDEX (2)
#define HOOKFLASH_CONVERSATION_THREAD_BASE_THREAD_ID_INDEX (3)
#define HOOKFLASH_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX (4)

#define HOOKFLASH_CONVERSATION_THREAD_MAX_WAIT_DELIVERY_TIME_BEFORE_PUSH_IN_SECONDS (30)

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      typedef stack::IHelper::SplitMap SplitMap;
      typedef IConversationThreadParser::DialogPtr DialogPtr;
      typedef IConversationThreadParser::ThreadContactMap ThreadContactMap;
      typedef IConversationThreadParser::MessageList MessageList;
      typedef IConversationThreadParser::MessagePtr MessagePtr;
      typedef IConversationThreadParser::ContactURIList ContactURIList;

      // host publishes these documents:
      // /threads/1.0/host/base-thread-id/host-thread-id/state       - current state of the thread (includes list of all participants)
      // /threads/1.0/host/base-thread-id/host-thread-id/permissions - all participant peer URIs that are part of the conversation thread

      // /threads/1.0/subscribers/permissions                        - peer URI of the slave added to this document

      // slaves publishes these documents to their own machine:
      // /threads/1.0/slave/base-thread-id/host-thread-id/state
      // /threads/1.0/slave/base-thread-id/host-thread-id/permissions     - all who can receive this document (i.e. at minimal the current host)

      // /threads/1.0/subscribers/permissions

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadForAccount
      #pragma mark

      interaction IConversationThreadForAccount
      {
        IConversationThreadForAccount &forAccount() {return *this;}
        const IConversationThreadForAccount &forAccount() const {return *this;}

        static ConversationThreadPtr create(
                                            AccountPtr account,
                                            ILocationPtr peerLocation,
                                            IPublicationMetaDataPtr metaData,
                                            const SplitMap &split
                                            );

        virtual String getThreadID() const = 0;

        virtual void notifyPublicationUpdated(
                                              ILocationPtr peerLocation,
                                              IPublicationMetaDataPtr metaData,
                                              const SplitMap &split
                                              ) = 0;

        virtual void notifyPublicationGone(
                                           ILocationPtr peerLocation,
                                           IPublicationMetaDataPtr metaData,
                                           const SplitMap &split
                                           ) = 0;

        virtual void notifyPeerDisconnected(ILocationPtr peerLocation) = 0;
      };

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadForCall
      #pragma mark

      interaction IConversationThreadForCall
      {
        IConversationThreadForCall &forCall() {return *this;}
        const IConversationThreadForCall &forCall() const {return *this;}

        typedef String LocationID;
        typedef std::map<LocationID, DialogPtr> LocationDialogMap;

        virtual RecursiveLock &getLock() const = 0;
        virtual AccountPtr getAccount() const = 0;

        virtual bool placeCall(CallPtr call) = 0;
        virtual void notifyCallStateChanged(CallPtr call) = 0;
        virtual void notifyCallCleanup(CallPtr call) = 0;

        virtual void gatherDialogReplies(
                                         const char *callID,
                                         LocationDialogMap &outDialogs
                                         ) const = 0;
      };

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadHostSlaveBase
      #pragma mark

      interaction IConversationThreadHostSlaveBase
      {
        typedef String CallID;
        typedef std::map<CallID, CallPtr> PendingCallMap;

        typedef IConversationThreadForCall::LocationDialogMap LocationDialogMap;
        typedef IConversationThread::ContactStates ContactStates;

        static String toDebugValueString(IConversationThreadHostSlaveBasePtr hostOrSlave, bool includeComaPrefix = true);

        virtual String getThreadID() const = 0;

        virtual bool isHost() const = 0;
        virtual bool isSlave() const = 0;

        virtual ConversationThreadHostPtr toHost() const = 0;
        virtual ConversationThreadSlavePtr toSlave() const = 0;

        virtual void shutdown() = 0;
        virtual bool isShutdown() const = 0;

        virtual bool isHostThreadOpen() const = 0;

        virtual void notifyPublicationUpdated(
                                              ILocationPtr peerLocation,
                                              IPublicationMetaDataPtr metaData,
                                              const SplitMap &split
                                              ) = 0;

        virtual void notifyPublicationGone(
                                           ILocationPtr peerLocation,
                                           IPublicationMetaDataPtr metaData,
                                           const SplitMap &split
                                           ) = 0;

        virtual void notifyPeerDisconnected(ILocationPtr peerLocation) = 0;

        virtual Time getHostCreationTime() const = 0;

        virtual bool safeToChangeContacts() const = 0;

        virtual void getContacts(ThreadContactMap &outContacts) const = 0;
        virtual bool inConversation(ContactPtr contact) const = 0;
        virtual void addContacts(const ContactProfileInfoList &contacts) = 0;
        virtual void removeContacts(const ContactList &contacts) = 0;

        virtual ContactStates getContactState(ContactPtr contact) const = 0;

        virtual bool sendMessages(const MessageList &messages) = 0;

        virtual bool placeCalls(const PendingCallMap &pendingCalls) = 0;
        virtual void notifyCallStateChanged(CallPtr call) = 0;
        virtual void notifyCallCleanup(CallPtr call) = 0;

        virtual void gatherDialogReplies(
                                         const char *callID,
                                         LocationDialogMap &outDialogs
                                         ) const = 0;
      };

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadForHostOrSlave
      #pragma mark

      interaction IConversationThreadForHostOrSlave
      {
        IConversationThreadForHostOrSlave &forHostOrSlave() {return *this;}
        const IConversationThreadForHostOrSlave &forHostOrSlave() const {return *this;}

        typedef IConversationThread::ContactStates ContactStates;

        virtual AccountPtr getAccount() const = 0;
        virtual stack::IPublicationRepositoryPtr getRepository() const = 0;

        virtual String getThreadID() const = 0;

        virtual void notifyStateChanged(IConversationThreadHostSlaveBasePtr thread) = 0;

        virtual void notifyContactState(
                                        IConversationThreadHostSlaveBasePtr thread,
                                        ContactPtr contact,
                                        ContactStates state
                                        ) = 0;

        virtual void notifyMessageReceived(MessagePtr message) = 0;
        virtual void notifyMessageDeliveryStateChanged(
                                                       const char *messageID,
                                                       IConversationThread::MessageDeliveryStates state
                                                       ) = 0;
        virtual void notifyMessagePush(
                                       MessagePtr message,
                                       ContactPtr toContact
                                       ) = 0;

        virtual void requestAddIncomingCallHandler(
                                                   const char *dialogID,
                                                   IConversationThreadHostSlaveBasePtr hostOrSlaveThread,
                                                   CallPtr newCall
                                                   ) = 0;
        virtual void requestRemoveIncomingCallHandler(const char *dialogID) = 0;

        virtual void notifyPossibleCallReplyStateChange(const char *dialogID) = 0;
      };

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadForHost
      #pragma mark

      interaction IConversationThreadForHost : public IConversationThreadForHostOrSlave
      {
        IConversationThreadForHost &forHost() {return *this;}
        const IConversationThreadForHost &forHost() const {return *this;}

        virtual bool inConversation(ContactPtr contact) const = 0;

        virtual void addContacts(const ContactProfileInfoList &contacts) = 0;
        virtual void removeContacts(const ContactURIList &contacts) = 0;
      };

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadForSlave
      #pragma mark

      interaction IConversationThreadForSlave : public IConversationThreadForHostOrSlave
      {
        typedef IConversationThreadParser::ThreadPtr ThreadPtr;

        IConversationThreadForSlave &forSlave() {return *this;}
        const IConversationThreadForSlave &forSlave() const {return *this;}

        virtual void notifyAboutNewThreadNowIfNotNotified(ConversationThreadSlavePtr slave) = 0;

        virtual void convertSlaveToClosedHost(
                                              ConversationThreadSlavePtr slave,
                                              ThreadPtr originalHost,
                                              ThreadPtr originalSlave
                                              ) = 0;
      };

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadAsync
      #pragma mark

      interaction IConversationThreadAsync
      {
        virtual void onStep() = 0;
      };

      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      //-------------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread
      #pragma mark

      class ConversationThread  : public MessageQueueAssociator,
                                  public IConversationThread,
                                  public IConversationThreadForAccount,
                                  public IConversationThreadForCall,
                                  public IConversationThreadForHost,
                                  public IConversationThreadForSlave,
                                  public IConversationThreadAsync
      {
      public:
        friend interaction IConversationThread;
        friend interaction IConversationThreadForAccount;
        friend interaction IConversationThreadForCall;
        friend interaction IConversationThreadForHost;
        friend interaction IConversationThreadForSlave;

        enum ConversationThreadStates
        {
          ConversationThreadState_Pending,
          ConversationThreadState_Ready,
          ConversationThreadState_ShuttingDown,
          ConversationThreadState_Shutdown,
        };

        static const char *toString(ConversationThreadStates state);

        typedef IConversationThread::ContactStates ContactStates;

        typedef String MessageID;
        typedef std::map<MessageID, MessageDeliveryStates> MessageDeliveryStatesMap;
        typedef std::map<MessageID, MessagePtr> MessageReceivedMap;
        typedef std::list<MessagePtr> MessageList;

        typedef String HostThreadID;
        typedef std::map<HostThreadID, IConversationThreadHostSlaveBasePtr> ThreadMap;

        typedef String CallID;
        typedef std::map<CallID, CallPtr> PendingCallMap;

        typedef std::pair<IConversationThreadHostSlaveBasePtr, CallPtr> CallHandlerPair;
        typedef std::map<CallID, CallHandlerPair> CallHandlerMap;

        typedef String ContactID;
        typedef std::pair<ContactPtr, ContactStates> ContactStatePair;
        typedef std::map<ContactID, ContactStatePair> ContactStateMap;

      protected:
        ConversationThread(
                           IMessageQueuePtr queue,
                           AccountPtr account,
                           const char *threadID
                           );

        void init();

      public:
        ~ConversationThread();

        static ConversationThreadPtr convert(IConversationThreadPtr thread);

      protected:
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => IConversationThread
        #pragma mark

        static String toDebugString(IConversationThreadPtr thread, bool includeCommaPrefix = true);

        static ConversationThreadPtr create(
                                            IAccountPtr account,
                                            ElementPtr profileBundleEl
                                            );

        static ConversationThreadListPtr getConversationThreads(IAccountPtr account);
        static ConversationThreadPtr getConversationThreadByID(
                                                               IAccountPtr account,
                                                               const char *threadID
                                                               );

        virtual PUID getID() const {return mID;}

        virtual String getThreadID() const;

        virtual bool amIHost() const;
        virtual IAccountPtr getAssociatedAccount() const;

        virtual ContactListPtr getContacts() const;

        virtual ElementPtr getProfileBundle(IContactPtr contact) const;
        virtual ContactStates getContactState(IContactPtr contact) const;

        virtual void addContacts(const ContactProfileInfoList &contactProfileInfos);
        virtual void removeContacts(const ContactList &contacts);

        // sending a message will cause the message to be delivered to all the contacts currently in the conversation
        virtual void sendMessage(
                                 const char *messageID,
                                 const char *messageType,
                                 const char *message
                                 );

        // returns false if the message ID is not known
        virtual bool getMessage(
                                const char *messageID,
                                IContactPtr &outFrom,
                                String &outMessageType,
                                String &outMessage,
                                Time &outTime
                                ) const;

        // returns false if the message ID is not known
        virtual bool getMessageDeliveryState(
                                             const char *messageID,
                                             MessageDeliveryStates &outDeliveryState
                                             ) const;

        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => IConversationThreadForAccount
        #pragma mark

        // (duplicate) virtual IConversationThreadPtr convertIConversationThread() const;
        static ConversationThreadPtr create(
                                            AccountPtr account,
                                            ILocationPtr peerLocation,
                                            IPublicationMetaDataPtr metaData,
                                            const SplitMap &split
                                            );

        // (duplicate) virtual String getThreadID() const;
        virtual void notifyPublicationUpdated(
                                              ILocationPtr peerLocation,
                                              IPublicationMetaDataPtr metaData,
                                              const SplitMap &split
                                              );

        virtual void notifyPublicationGone(
                                           ILocationPtr peerLocation,
                                           IPublicationMetaDataPtr metaData,
                                           const SplitMap &split
                                           );

        virtual void notifyPeerDisconnected(ILocationPtr peerLocation);

        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => IConversationThreadForHostOrSlave
        #pragma mark

      public:
        IConversationThreadForHostOrSlave &forHostOrSlave() {return forHost().forHostOrSlave();}
        const IConversationThreadForHostOrSlave &forHostOrSlave() const {return forHost().forHostOrSlave();}

      protected:
        virtual AccountPtr getAccount() const;
        virtual IPublicationRepositoryPtr getRepository() const;

        // (duplicate) virtual String getThreadID() const;

        virtual void notifyStateChanged(IConversationThreadHostSlaveBasePtr thread);

        virtual void notifyContactState(
                                        IConversationThreadHostSlaveBasePtr thread,
                                        ContactPtr contact,
                                        ContactStates state
                                        );

        virtual void notifyMessageReceived(MessagePtr message);
        virtual void notifyMessageDeliveryStateChanged(
                                                       const char *messageID,
                                                       IConversationThread::MessageDeliveryStates state
                                                       );
        virtual void notifyMessagePush(
                                       MessagePtr message,
                                       ContactPtr toContact
                                       );

        virtual void requestAddIncomingCallHandler(
                                                   const char *dialogID,
                                                   IConversationThreadHostSlaveBasePtr hostOrSlaveThread,
                                                   CallPtr newCall
                                                   );
        virtual void requestRemoveIncomingCallHandler(const char *dialogID);

        virtual void notifyPossibleCallReplyStateChange(const char *dialogID);

        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => IConversationThreadForHost
        #pragma mark

        virtual bool inConversation(ContactPtr contact) const;
        // (duplicate) virtual void addContacts(const ContactList &contacts);
        virtual void removeContacts(const ContactURIList &contacts);

        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => IConversationThreadForSlave
        #pragma mark

        virtual void notifyAboutNewThreadNowIfNotNotified(ConversationThreadSlavePtr slave);

        virtual void convertSlaveToClosedHost(
                                              ConversationThreadSlavePtr slave,
                                              ThreadPtr originalHost,
                                              ThreadPtr originalSlave
                                              );

        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => IConversationThreadForCall
        #pragma mark

        // (duplicate) virtual IConversationThreadPtr convertIConversationThread() const;
        // (duplicate) virtual RecursiveLock &getLock() const;
        // (duplicate) virtual IAccountForConversationThreadPtr getAccount() const;
        virtual bool placeCall(CallPtr call);
        virtual void notifyCallStateChanged(CallPtr call);
        virtual void notifyCallCleanup(CallPtr call);

        virtual void gatherDialogReplies(
                                         const char *callID,
                                         LocationDialogMap &outDialogs
                                         ) const;

        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => IConversationThreadAsync
        #pragma mark

        virtual void onStep() {step();}

      protected:
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => (internal)
        #pragma mark

        bool isPending() const {return ConversationThreadState_Pending == mCurrentState;}
        bool isReady() const {return ConversationThreadState_Ready == mCurrentState;}
        bool isShuttingDown() const {return ConversationThreadState_ShuttingDown == mCurrentState;}
        bool isShutdown() const {return ConversationThreadState_Shutdown == mCurrentState;}

        String log(const char *message) const;
        virtual String getDebugValueString(bool includeCommaPrefix = true) const;

        RecursiveLock &getLock() const;

        void cancel();
        void step();

        void setState(ConversationThreadStates state);

        void handleLastOpenThreadChanged();
        void handleContactsChanged();

      protected:
        //-----------------------------------------------------------------------
        #pragma mark
        #pragma mark ConversationThread => (data)
        #pragma mark

        mutable RecursiveLock mBogusLock;
        PUID mID;
        ConversationThreadWeakPtr mThisWeak;
        ConversationThreadPtr mGracefulShutdownReference;

        IConversationThreadDelegatePtr mDelegate;

        AccountWeakPtr mAccount;

        String mThreadID;

        ConversationThreadStates mCurrentState;
        bool mMustNotifyAboutNewThread;

        IConversationThreadHostSlaveBasePtr mOpenThread;      // if there is an open thread, this is valid
        IConversationThreadHostSlaveBasePtr mLastOpenThread;  // if there is was an open thread, this is valid

        IConversationThreadHostSlaveBasePtr mHandleThreadChanged;
        DWORD mHandleContactsChangedCRC;

        ThreadMap mThreads;

        MessageReceivedMap mReceivedOrPushedMessages;   // remembered so the "get" of the message can be done later

        MessageDeliveryStatesMap mMessageDeliveryStates;
        MessageList mPendingDeliveryMessages;

        PendingCallMap mPendingCalls;

        CallHandlerMap mCallHandlers;

        // used to remember the last notified state for a contact
        ContactStateMap mLastReportedContactStates;
      };
    }
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::core::internal::IConversationThreadAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
