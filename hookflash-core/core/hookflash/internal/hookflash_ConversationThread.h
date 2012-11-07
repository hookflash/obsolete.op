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
#include <hookflash/internal/hookflash_IConversationThreadParser.h>
#include <hookflash/IConversationThread.h>

#include <hookflash/stack/hookflashTypes.h>
#include <hookflash/stack/IHelper.h>

#define HOOKFLASH_CONVERSATION_THREAD_TYPE_INDEX (2)
#define HOOKFLASH_CONVERSATION_THREAD_BASE_THREAD_ID_INDEX (3)
#define HOOKFLASH_CONVERSATION_THREAD_HOST_THREAD_ID_INDEX (4)

#define HOOKFLASH_CONVERSATION_THREAD_MAX_WAIT_DELIVERY_TIME_BEFORE_PUSH_IN_SECONDS (30)

namespace hookflash
{
  namespace internal
  {
    // host publishes these documents:
    // /threads/1.0/host/base-thread-id/host-thread-id/state       - current state of the thread (includes list of all participants)
    // /threads/1.0/host/base-thread-id/host-thread-id/permissions - all participant contact IDs that are part of the conversation thread

    // /threads/1.0/subscribers/permissions                        - contact ID of the slave added to this document

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
      typedef zsLib::String String;
      typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
      typedef stack::IHelper::SplitMap SplitMap;
      typedef stack::IPeerLocationPtr IPeerLocationPtr;

      static IConversationThreadForAccountPtr convert(IConversationThreadPtr thread);
      virtual IConversationThreadPtr convertIConversationThread() const = 0;

      static IConversationThreadForAccountPtr create(
                                                     IAccountForConversationThreadPtr account,
                                                     IPeerLocationPtr peerLocation,
                                                     IPublicationMetaDataPtr metaData,
                                                     const SplitMap &split
                                                     );

      virtual const String &getThreadID() const = 0;

      virtual void notifyPublicationUpdated(
                                            IPeerLocationPtr peerLocation,
                                            IPublicationMetaDataPtr metaData,
                                            const SplitMap &split
                                            ) = 0;

      virtual void notifyPublicationGone(
                                         IPeerLocationPtr peerLocation,
                                         IPublicationMetaDataPtr metaData,
                                         const SplitMap &split
                                         ) = 0;

      virtual void notifyPeerDisconnected(IPeerLocationPtr peerLocation) = 0;
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
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::String String;
      typedef IConversationThreadParser::DialogPtr DialogPtr;
      typedef String LocationID;
      typedef std::map<LocationID, DialogPtr> LocationDialogMap;

      static IConversationThreadForCallPtr convert(IConversationThreadPtr thread);
      static IConversationThreadForCallPtr convert(IConversationThreadForHostOrSlavePtr thread);

      virtual IConversationThreadPtr convertIConversationThread() const = 0;

      virtual RecursiveLock &getLock() const = 0;
      virtual IAccountForConversationThreadPtr getAccount() const = 0;

      virtual bool placeCall(ICallForConversationThreadPtr call) = 0;
      virtual void notifyCallStateChanged(ICallForConversationThreadPtr call) = 0;
      virtual void notifyCallCleanup(ICallForConversationThreadPtr call) = 0;

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
      typedef zsLib::DWORD DWORD;
      typedef zsLib::String String;
      typedef zsLib::Time Time;
      typedef stack::IHelper::SplitMap SplitMap;
      typedef stack::IPublicationMetaDataPtr IPublicationMetaDataPtr;
      typedef stack::IPeerLocationPtr IPeerLocationPtr;
      typedef IConversationThreadParser::MessagePtr MessagePtr;
      typedef std::list<MessagePtr> MessageList;
      typedef IConversationThread::ContactList ContactList;
      typedef IConversationThread::ContactInfoList ContactInfoList;
      typedef IConversationThreadParser::Contacts::ContactMap ContactMap;
      typedef String CallID;
      typedef std::map<CallID, ICallForConversationThreadPtr> PendingCallMap;
      typedef IConversationThreadForCall::LocationDialogMap LocationDialogMap;
      typedef IConversationThread::ContactStates ContactStates;

      virtual const String &getThreadID() const = 0;

      virtual bool isHost() const = 0;
      virtual bool isSlave() const = 0;

      virtual IConversationThreadHostForConversationThreadPtr toHost() const = 0;
      virtual IConversationThreadSlaveForConversationThreadPtr toSlave() const = 0;

      virtual void shutdown() = 0;
      virtual bool isShutdown() const = 0;

      virtual bool isHostThreadOpen() const = 0;

      virtual void notifyPublicationUpdated(
                                            IPeerLocationPtr peerLocation,
                                            IPublicationMetaDataPtr metaData,
                                            const SplitMap &split
                                            ) = 0;

      virtual void notifyPublicationGone(
                                         IPeerLocationPtr peerLocation,
                                         IPublicationMetaDataPtr metaData,
                                         const SplitMap &split
                                         ) = 0;

      virtual void notifyPeerDisconnected(IPeerLocationPtr peerLocation) = 0;

      virtual Time getHostCreationTime() const = 0;

      virtual bool safeToChangeContacts() const = 0;

      virtual void getContacts(ContactMap &outContacts) const = 0;
      virtual bool inConversation(IContactForConversationThreadPtr contact) const = 0;
      virtual void addContacts(const ContactInfoList &contacts) = 0;
      virtual void removeContacts(const ContactList &contacts) = 0;

      virtual ContactStates getContactState(IContactForConversationThreadPtr contact) const = 0;

      virtual bool sendMessages(const MessageList &messages) = 0;

      virtual bool placeCalls(const PendingCallMap &pendingCalls) = 0;
      virtual void notifyCallStateChanged(ICallForConversationThreadPtr call) = 0;
      virtual void notifyCallCleanup(ICallForConversationThreadPtr call) = 0;

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
      typedef zsLib::String String;
      typedef stack::IPublicationRepositoryPtr IPublicationRepositoryPtr;
      typedef IConversationThreadParser::MessagePtr MessagePtr;
      typedef IConversationThread::ContactStates ContactStates;

      virtual IConversationThreadPtr convertIConversationThread() const = 0;

      virtual IAccountForConversationThreadPtr getAccount() const = 0;
      virtual stack::IPublicationRepositoryPtr getRepository() const = 0;

      virtual const String &getThreadID() const = 0;

      virtual void notifyStateChanged(IConversationThreadHostSlaveBasePtr thread) = 0;

      virtual void notifyContactState(
                                      IConversationThreadHostSlaveBasePtr thread,
                                      IContactForConversationThreadPtr contact,
                                      ContactStates state
                                      ) = 0;

      virtual void notifyMessageReceived(MessagePtr message) = 0;
      virtual void notifyMessageDeliveryStateChanged(
                                                     const char *messageID,
                                                     IConversationThread::MessageDeliveryStates state
                                                     ) = 0;
      virtual void notifyMessagePush(
                                     MessagePtr message,
                                     IContactForConversationThreadPtr toContact
                                     ) = 0;

      virtual void requestAddIncomingCallHandler(
                                                 const char *dialogID,
                                                 IConversationThreadHostSlaveBasePtr hostOrSlaveThread,
                                                 ICallForConversationThreadPtr newCall
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
      typedef IConversationThread::ContactList ContactList;
      typedef IConversationThread::ContactInfoList ContactInfoList;
      typedef IConversationThreadParser::Contacts::ContactIDList ContactIDList;

      virtual bool inConversation(IContactForConversationThreadPtr contact) const = 0;

      virtual void addContacts(const ContactInfoList &contacts) = 0;
      virtual void removeContacts(const ContactIDList &contacts) = 0;
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

      virtual void notifyAboutNewThreadNowIfNotNotified(IConversationThreadSlaveForConversationThreadPtr slave) = 0;

      virtual void convertSlaveToClosedHost(
                                            IConversationThreadSlaveForConversationThreadPtr slave,
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

    class ConversationThread  : public zsLib::MessageQueueAssociator,
                                public IConversationThread,
                                public IConversationThreadForAccount,
                                public IConversationThreadForHost,
                                public IConversationThreadForSlave,
                                public IConversationThreadForCall,
                                public IConversationThreadAsync
    {
    public:
      enum ConversationThreadStates
      {
        ConversationThreadState_Pending,
        ConversationThreadState_Ready,
        ConversationThreadState_ShuttingDown,
        ConversationThreadState_Shutdown,
      };

      static const char *toString(ConversationThreadStates state);

      typedef zsLib::MessageQueueAssociator MessageQueueAssociator;
      typedef zsLib::RecursiveLock RecursiveLock;
      typedef zsLib::AutoRecursiveLock AutoRecursiveLock;
      typedef zsLib::PUID PUID;
      typedef zsLib::DWORD DWORD;
      typedef zsLib::Time Time;
      typedef zsLib::String String;
      typedef stack::IPublicationRepositoryPtr IPublicationRepositoryPtr;
      typedef IConversationThread::ContactList ContactList;
      typedef IConversationThread::ContactInfoList ContactInfoList;
      typedef IConversationThreadParser::Contacts::ContactIDList ContactIDList;
      typedef IConversationThread::MessageDeliveryStates MessageDeliveryStates;
      typedef IConversationThread::ContactStates ContactStates;

      friend interaction IConversationThreadForCall;

    protected:
      ConversationThread(
                         IAccountForConversationThreadPtr account,
                         const char *threadID
                         );

      void init();

    public:
      ~ConversationThread();

      static ConversationThreadPtr convert(IConversationThreadPtr thread);
      static ConversationThreadPtr convert(IConversationThreadForCallPtr thread);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThread
      #pragma mark

      static ConversationThreadPtr create(
                                          hookflash::IAccountPtr account,
                                          ElementPtr profileBundleEl
                                          );

      virtual const String &getThreadID() const;

      virtual bool amIHost() const;

      virtual void getContacts(ContactList &contactList) const;
      virtual ElementPtr getProfileBundle(IContactPtr contact) const;
      virtual ContactStates getContactState(IContactPtr contact) const;

      virtual void addContacts(const ContactInfoList &contacts);
      virtual void removeContacts(const ContactList &contacts);

      virtual void sendMessage(
                               const char *messageID,
                               const char *messageType,
                               const char *message
                               );

      virtual bool getMessage(
                              const char *messageID,
                              IContactPtr &outFrom,
                              String &outMessageType,
                              String &outMessage,
                              Time &outTime
                              ) const;

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
                                          IAccountForConversationThreadPtr account,
                                          IPeerLocationPtr peerLocation,
                                          IPublicationMetaDataPtr metaData,
                                          const SplitMap &split
                                          );

      // (duplicate) virtual const String &getThreadID() const;
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

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThreadForHostOrSlave
      #pragma mark

      virtual IConversationThreadPtr convertIConversationThread() const {return mThisWeak.lock();}

      virtual IAccountForConversationThreadPtr getAccount() const;
      virtual IPublicationRepositoryPtr getRepository() const;

      // (duplicate) virtual const String &getThreadID() const;

      virtual void notifyStateChanged(IConversationThreadHostSlaveBasePtr thread);

      virtual void notifyContactState(
                                      IConversationThreadHostSlaveBasePtr thread,
                                      IContactForConversationThreadPtr contact,
                                      ContactStates state
                                      );

      virtual void notifyMessageReceived(MessagePtr message);
      virtual void notifyMessageDeliveryStateChanged(
                                                     const char *messageID,
                                                     IConversationThread::MessageDeliveryStates state
                                                     );
      virtual void notifyMessagePush(
                                     MessagePtr message,
                                     IContactForConversationThreadPtr toContact
                                     );

      virtual void requestAddIncomingCallHandler(
                                                 const char *dialogID,
                                                 IConversationThreadHostSlaveBasePtr hostOrSlaveThread,
                                                 ICallForConversationThreadPtr newCall
                                                 );
      virtual void requestRemoveIncomingCallHandler(const char *dialogID);

      virtual void notifyPossibleCallReplyStateChange(const char *dialogID);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThreadForHost
      #pragma mark

      virtual bool inConversation(IContactForConversationThreadPtr contact) const;
      // (duplicate) virtual void addContacts(const ContactList &contacts);
      virtual void removeContacts(const ContactIDList &contacts);

      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark ConversationThread => IConversationThreadForSlave
      #pragma mark

      virtual void notifyAboutNewThreadNowIfNotNotified(IConversationThreadSlaveForConversationThreadPtr slave);

      virtual void convertSlaveToClosedHost(
                                            IConversationThreadSlaveForConversationThreadPtr slave,
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
      virtual bool placeCall(ICallForConversationThreadPtr call);
      virtual void notifyCallStateChanged(ICallForConversationThreadPtr call);
      virtual void notifyCallCleanup(ICallForConversationThreadPtr call);

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

      IAccountForConversationThreadWeakPtr mAccount;

      String mThreadID;

      ConversationThreadStates mCurrentState;
      bool mMustNotifyAboutNewThread;

      IConversationThreadHostSlaveBasePtr mOpenThread;      // if there is an open thread, this is valid
      IConversationThreadHostSlaveBasePtr mLastOpenThread;  // if there is was an open thread, this is valid

      IConversationThreadHostSlaveBasePtr mHandleThreadChanged;
      DWORD mHandleContactsChangedCRC;

      typedef String HostThreadID;
      typedef std::map<HostThreadID, IConversationThreadHostSlaveBasePtr> ThreadMap;

      ThreadMap mThreads;

      typedef String MessageID;
      typedef std::map<MessageID, MessageDeliveryStates> MessageDeliveryStatesMap;
      typedef std::map<MessageID, MessagePtr> MessageReceivedMap;
      typedef std::list<MessagePtr> MessageList;

      MessageReceivedMap mReceivedOrPushedMessages;   // remembered so the "get" of the message can be done later

      MessageDeliveryStatesMap mMessageDeliveryStates;
      MessageList mPendingDeliveryMessages;

      typedef String CallID;
      typedef std::map<CallID, ICallForConversationThreadPtr> PendingCallMap;
      PendingCallMap mPendingCalls;

      typedef std::pair<IConversationThreadHostSlaveBasePtr, ICallForConversationThreadPtr> CallHandlerPair;
      typedef std::map<CallID, CallHandlerPair> CallHandlerMap;
      CallHandlerMap mCallHandlers;

      // used to remember the last notified state for a contact
      typedef String ContactID;
      typedef std::pair<IContactForConversationThreadPtr, ContactStates> ContactStatePair;
      typedef std::map<ContactID, ContactStatePair> ContactStateMap;
      ContactStateMap mLastReportedContactStates;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(hookflash::internal::IConversationThreadAsync)
ZS_DECLARE_PROXY_METHOD_0(onStep)
ZS_DECLARE_PROXY_END()
