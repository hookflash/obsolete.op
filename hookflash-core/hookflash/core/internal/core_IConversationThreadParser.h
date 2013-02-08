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
//#include <hookflash/core/IConversationThread.h>
#include <hookflash/core/ICall.h>
//
//#include <hookflash/stack/types.h>
//#include <hookflash/stack/IHelper.h>
#include <hookflash/services/IICESocket.h>

#include <zsLib/Exception.h>

#define HOOKFLASH_CONVESATION_THREAD_BASE_THREAD_INDEX (3)

namespace hookflash
{
  namespace core
  {
    namespace internal
    {
      typedef zsLib::Exceptions::InvalidArgument InvalidArgument;

      typedef services::IICESocket::CandidateList CandidateList;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadParser
      #pragma mark

      interaction IConversationThreadParser
      {
        //---------------------------------------------------------------------
        class Thread;
        typedef boost::shared_ptr<Thread> ThreadPtr;
        typedef boost::weak_ptr<Thread> ThreadWeakPtr;

        class Message;
        typedef boost::shared_ptr<Message> MessagePtr;
        typedef boost::weak_ptr<Message> MessageWeakPtr;

        class MessageReceipts;
        typedef boost::shared_ptr<MessageReceipts> MessageReceiptsPtr;
        typedef boost::weak_ptr<MessageReceipts> MessageReceiptsWeakPtr;

        class ThreadContact;
        typedef boost::shared_ptr<ThreadContact> ThreadContactPtr;
        typedef boost::weak_ptr<ThreadContact> ThreadContactWeakPtr;

        class ThreadContacts;
        typedef boost::shared_ptr<ThreadContacts> ThreadContactsPtr;
        typedef boost::weak_ptr<ThreadContacts> ThreadContactsWeakPtr;

        class Dialog;
        typedef boost::shared_ptr<Dialog> DialogPtr;
        typedef boost::weak_ptr<Dialog> DialogWeakPtr;

        class Details;
        typedef boost::shared_ptr<Details> DetailsPtr;
        typedef boost::weak_ptr<Details> DetailsWeakPtr;

        //---------------------------------------------------------------------
        typedef String MessageID;
        typedef Time ReceiptTime;
        typedef std::map<MessageID, ReceiptTime> MessageReceiptMap;
        typedef std::list<MessageID> MessageIDList;

        typedef String PeerURI;
        typedef PeerURI ContactURI;

        typedef std::list<ThreadContactPtr> ThreadContactList;
        typedef std::list<ContactURI> ContactURIList;
        typedef std::map<ContactURI, ThreadContactPtr> ThreadContactMap;

        typedef UINT FoundationIndex;
        typedef services::IICESocket::CandidateList CandidateList;
        typedef std::map<FoundationIndex, CandidateList> CandidateLists;

        typedef String DialogID;
        typedef std::map<DialogID, DialogPtr> DialogMap;
        typedef std::list<DialogPtr> DialogList;
        typedef std::list<DialogID> DialogIDList;

        typedef std::list<MessagePtr> MessageList;
        typedef std::map<MessageID, MessagePtr> MessageMap;

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IConversationThreadParser::Message
        #pragma mark

        class Message
        {
        public:
          static String toDebugString(MessagePtr message, bool includeCommaPrefix = true);

          static MessagePtr create(
                                   const char *messageID,
                                   const char *fromPeerURI,
                                   const char *mimeType,
                                   const char *body,
                                   Time sent,
                                   IPeerFilesPtr signer
                                   );

          static MessagePtr create(ElementPtr messageBundleEl);

          const ElementPtr &messageBundleElement() const {return mBundleEl;}

          const String &messageID() const         {return mMessageID;}
          const String &fromPeerURI() const       {return mFromPeerURI;}
          const String &mimeType() const          {return mMimeType;}
          const String &body() const              {return mBody;}
          const Time &sent() const                {return mSent;}

          String getDebugValueString(bool includeCommaPrefix = true) const;

        protected:
          MessageWeakPtr mThisWeak;
          ElementPtr mBundleEl;

          PUID mID;
          String mMessageID;
          String mFromPeerURI;
          String mMimeType;
          String mBody;
          Time mSent;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IConversationThreadParser::MessageReceipts
        #pragma mark

        class MessageReceipts
        {
        public:
          static String toDebugString(MessageReceiptsPtr receipts, bool includeCommaPrefix = true);

          static MessageReceiptsPtr create(UINT version);
          static MessageReceiptsPtr create(UINT version, const String &messageID);
          static MessageReceiptsPtr create(UINT version, const MessageIDList &messageIDs);
          static MessageReceiptsPtr create(UINT version, const MessageReceiptMap &messageReceipts);

          static MessageReceiptsPtr create(ElementPtr messageReceiptsEl);

          ElementPtr receiptsElement() const          {return mReceiptsEl;}

          UINT version() const                        {return mVersion;}
          const MessageReceiptMap &receipts() const  {return mReceipts;}

          String getDebugValueString(bool includeCommaPrefix = true) const;

        protected:
          MessageReceiptsWeakPtr mThisWeak;

          PUID mID;
          ElementPtr mReceiptsEl;

          UINT mVersion;
          MessageReceiptMap mReceipts;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IConversationThreadParser::ThreadContacts
        #pragma mark

        class ThreadContact
        {
        public:
          static String toDebugString(ThreadContactPtr contact, bool includeCommaPrefix = true);

          static ThreadContactPtr create(
                                         ContactPtr contact,
                                         ElementPtr profileBundleEl
                                         );

          ContactPtr contact() const              {return mContact;}
          ElementPtr profileBundleElement() const {return mProfileBundleEl;}

          String getDebugValueString(bool includeCommaPrefix = true) const;

        protected:
          PUID mID;
          ContactPtr mContact;
          ElementPtr mProfileBundleEl;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IConversationThreadParser::ThreadContacts
        #pragma mark

        class ThreadContacts
        {
        public:
          static String toDebugString(ThreadContactsPtr threadContacts, bool includeCommaPrefix = true);

          static ThreadContactsPtr create(
                                    UINT version,
                                    const ThreadContactList &contacts,
                                    const ThreadContactList &addContacts,
                                    const ContactURIList &removeContacts
                                    );

          static ThreadContactsPtr create(
                                    AccountPtr account,
                                    ElementPtr contactsEl
                                    );

          ElementPtr contactsElement() const            {return mContactsEl;}

          UINT version() const                          {return mVersion;}
          const ThreadContactMap &contacts() const      {return mContacts;}
          const ThreadContactMap &addContacts() const   {return mAddContacts;}
          const ContactURIList &removeContacts() const  {return mRemoveContacts;}

          String getDebugValueString(bool includeCommaPrefix = true) const;

        protected:
          ThreadContactsWeakPtr mThisWeak;
          ElementPtr mContactsEl;

          PUID mID;
          UINT mVersion;
          ThreadContactMap mContacts;
          ThreadContactMap mAddContacts;
          ContactURIList mRemoveContacts;
        };

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IConversationThreadParser::Dialog
        #pragma mark

        class Dialog
        {
        protected:
          Dialog();

        public:
          enum DialogStates
          {
            DialogState_None            = ICall::CallState_None,
            DialogState_Preparing       = ICall::CallState_Preparing,
            DialogState_Placed          = ICall::CallState_Placed,
            DialogState_Incoming        = ICall::CallState_Incoming,
            DialogState_Early           = ICall::CallState_Early,
            DialogState_Ringing         = ICall::CallState_Ringing,
            DialogState_Ringback        = ICall::CallState_Ringback,
            DialogState_Open            = ICall::CallState_Open,
            DialogState_Active          = ICall::CallState_Active,
            DialogState_Inactive        = ICall::CallState_Inactive,
            DialogState_Hold            = ICall::CallState_Hold,
            DialogState_Closing         = ICall::CallState_Closing,
            DialogState_Closed          = ICall::CallState_Closed,

            DialogState_First = DialogState_None,
            DialogState_Last = DialogState_Closed,
          };

          enum DialogClosedReasons
          {
            DialogClosedReason_None                     = ICall::CallClosedReason_None,

            DialogClosedReason_User                     = ICall::CallClosedReason_User,
            DialogClosedReason_RequestTimeout           = ICall::CallClosedReason_RequestTimeout,
            DialogClosedReason_TemporarilyUnavailable   = ICall::CallClosedReason_TemporarilyUnavailable,
            DialogClosedReason_Busy                     = ICall::CallClosedReason_Busy,
            DialogClosedReason_RequestTerminated        = ICall::CallClosedReason_RequestTerminated,
            DialogClosedReason_NotAcceptableHere        = ICall::CallClosedReason_NotAcceptableHere,
            DialogClosedReason_ServerInternalError      = ICall::CallClosedReason_ServerInternalError,
            DialogClosedReason_Decline                  = ICall::CallClosedReason_Decline,
          };

          static const char *toString(DialogStates state);
          static DialogStates toDialogStates(const char *state);

          static const char *toString(DialogClosedReasons closedReason);

          struct Codec;

          struct Description;
          typedef boost::shared_ptr<Description> DescriptionPtr;
          typedef boost::weak_ptr<Description> DescriptionWeakPtr;

          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark IConversationThreadParser::Dialog::Codec
          #pragma mark

          struct Codec
          {
            UINT mCodecID;

            String mName;
            UINT mPTime;
            UINT mRate;
            UINT mChannels;

            Codec() : mCodecID(0), mPTime(0), mRate(0), mChannels(0) {}
            String getDebugValueString(bool includeCommaPrefix = true) const;
          };
          typedef std::list<Codec> CodecList;

          //---------------------------------------------------------------------
          #pragma mark
          #pragma mark IConversationThreadParser::Dialog::Description
          #pragma mark

          struct Description
          {
            static DescriptionPtr create();

            UINT mVersion;
            String mDescriptionID;
            String mType;
            ULONG mSSRC;

            String mSecurityCipher;
            String mSecuritySecret;
            String mSecuritySalt;

            CodecList mCodecs;
            CandidateLists mCandidateLists;

            Description() : mVersion(0), mSSRC(0) {}
            String getDebugValueString(bool includeCommaPrefix = true) const;
          };
          typedef std::list<DescriptionPtr> DescriptionList;

          static String toDebugString(DialogPtr dialog, bool includeCommaPrefix = true);

          static DialogPtr create(
                                  UINT version,
                                  const char *dialogID,
                                  DialogStates state,
                                  DialogClosedReasons closedReason,
                                  const char *callerContactURI,
                                  const char *callerLocationID,
                                  const char *calleeContactURI,
                                  const char *calleeLocationID,
                                  const char *replacesDialogID,
                                  const DescriptionList &descriptions,
                                  IPeerFilesPtr signer
                                  );

          static DialogPtr create(ElementPtr dialogBundleEl);

          ElementPtr dialogBundleElement() const      {return mDialogBundleEl;}
          UINT version() const                        {return mVersion;}
          const String &dialogID() const              {return mDialogID;}
          DialogStates dialogState() const            {return mState;}
          DialogClosedReasons closedReason() const    {return mClosedReason;}
          const String &closedReasonMessage() const   {return mClosedReasonMessage;}
          const String &callerPeerURI() const         {return mCallerContactURI;}
          const String &callerLocationID() const      {return mCallerLocationID;}
          const String &calleePeerURI() const         {return mCalleeContactURI;}
          const String &calleeLocationID() const      {return mCalleeLocationID;}
          const String &replacesDialogID() const      {return mReplacesDialogID;}
          const DescriptionList &descriptions() const {return mDescriptions;}

          String getDebugValueString(bool includeCommaPrefix = true) const;

        private:
          DialogWeakPtr mThisWeak;
          ElementPtr mDialogBundleEl;

          PUID mID;
          UINT mVersion;
          String mDialogID;
          DialogStates mState;
          DialogClosedReasons mClosedReason;
          String mClosedReasonMessage;
          String mCallerContactURI;
          String mCallerLocationID;
          String mCalleeContactURI;
          String mCalleeLocationID;
          String mReplacesDialogID;
          DescriptionList mDescriptions;
        };

        typedef Dialog::DialogStates DialogStates;

        typedef Dialog::Description Description;
        typedef Dialog::DescriptionPtr DescriptionPtr;

        typedef String DescriptionID;
        typedef std::pair<DialogID, DescriptionPtr> ChangedDescription;
        typedef std::map<DescriptionID, ChangedDescription> ChangedDescriptionMap;
        typedef std::list<DescriptionPtr> DescriptionList;
        typedef std::list<DescriptionID> DescriptionIDList;

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IConversationThreadParser::Details
        #pragma mark

        class Details
        {
        public:
          enum ConversationThreadStates
          {
            ConversationThreadState_None,     // slaves do not have state
            ConversationThreadState_Closed,
            ConversationThreadState_Open,

            ConversationThreadState_First = ConversationThreadState_None,
            ConversationThreadState_Last = ConversationThreadState_Open,
          };

          static const char *toString(ConversationThreadStates state);
          static ConversationThreadStates toConversationThreadState(const char *state);

          static String toDebugString(DetailsPtr details, bool includeCommaPrefix = true);

          static DetailsPtr create(
                                   UINT version,
                                   const char *baseThreadID,
                                   const char *hostThreadID,
                                   const char *topic,
                                   const char *replaces,
                                   ConversationThreadStates state
                                   );

          static DetailsPtr create(ElementPtr detailsEl);

          ElementPtr detailsElement() const       {return mDetailsEl;}
          UINT version() const                    {return mVersion;}
          const String &baseThreadID() const      {return mBaseThreadID;}
          const String &hostThreadID() const      {return mHostThreadID;}
          const String &replacesThreadID() const  {return mReplacesThreadID;}
          ConversationThreadStates state() const  {return mState;}
          const String &topic() const             {return mTopic;}
          Time created() const                    {return mCreated;}

          String getDebugValueString(bool includeCommaPrefix = true) const;

        protected:
          PUID mID;
          DetailsWeakPtr mThisWeak;
          ElementPtr mDetailsEl;

          UINT mVersion;
          String mBaseThreadID;
          String mHostThreadID;
          String mReplacesThreadID;
          ConversationThreadStates mState;
          String mTopic;
          Time mCreated;
        };

        typedef Details::ConversationThreadStates ConversationThreadStates;

        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        //---------------------------------------------------------------------
        #pragma mark
        #pragma mark IConversationThreadParser::Thread
        #pragma mark

        class Thread
        {
        public:
          enum ThreadTypes
          {
            ThreadType_Host,
            ThreadType_Slave,
          };

          static const char *toString(ThreadTypes type);
          static ThreadTypes toThreadTypes(const char *type) throw (InvalidArgument);

        protected:
          Thread();

        public:
          ~Thread();

          static String toDebugString(ThreadPtr thread, bool includeCommaPrefix = true);

          static ThreadPtr create(
                                  AccountPtr account,
                                  IPublicationPtr publication
                                  );

          bool updateFrom(
                          AccountPtr account,
                          IPublicationPtr publication
                          );

          static ThreadPtr create(
                                  AccountPtr account,
                                  ThreadTypes threadType,         // needed for document name
                                  ILocationPtr creatorLocation,
                                  const char *baseThreadID,
                                  const char *hostThreadID,
                                  const char *topic,
                                  const char *replaces,
                                  ConversationThreadStates state,
                                  ILocationPtr peerHostLocation = ILocationPtr()
                                  );

          void updateBegin();
          bool updateEnd();

          void setState(Details::ConversationThreadStates state);

          void setContacts(const ThreadContactMap &contacts);
          void setContactsToAdd(const ThreadContactMap &contactsToAdd);
          void setContactsToRemove(const ContactURIList &contactsToRemoved);

          void addMessage(MessagePtr message);
          void addMessages(const MessageList &messages);

          void setReceived(MessagePtr message);
          void setReceived(const MessageReceiptMap &messages);

          void addDialogs(const DialogList &dialogs);
          void updateDialogs(const DialogList &dialogs);
          void removeDialogs(const DialogIDList &dialogs);

          IPublicationPtr publication() const                       {return mPublication;}
          IPublicationPtr permissionPublication() const             {return mPermissionPublication;}
          DetailsPtr details() const                                {return mDetails;}
          ThreadContactsPtr contacts() const                              {return mContacts;}
          const MessageList &messages() const                       {return mMessageList;}
          const MessageMap &messagesAsMap() const                   {return mMessageMap;}
          MessageReceiptsPtr messageReceipts() const                {return mMessageReceipts;}
          const DialogMap &dialogs() const                          {return mDialogs;}

          // obtain a list of changes since the last updateFrom was called
          bool detailsChanged() const                               {return mDetailsChanged;}
          const ThreadContactMap &contactsChanged() const                 {return mContactsChanged;}
          const ContactURIList &contactsRemoved() const              {return mContactsRemoved;}
          const ThreadContactMap &contactsToAddChanged() const            {return mContactsToAddChanged;}
          const ContactURIList &contactsToAddRemoved() const         {return mContactsToAddRemoved;}
          const ContactURIList &contactsToRemoveChanged() const      {return mContactsToRemoveChanged;}
          const ContactURIList &contactsToRemoveRemoved() const      {return mContactsToRemoveRemoved;}
          const MessageList &messagedChanged() const                {return mMessagesChanged;}
          Time messagedChangedTime() const                          {return mMessagesChangedTime;}
          const MessageReceiptMap &messageReceiptsChanged() const   {return mMessageReceiptsChanged;}
          const DialogMap &dialogsChanged() const                   {return mDialogsChanged;}
          const DialogIDList &dialogsRemoved() const                {return mDialogsRemoved;}
          const ChangedDescriptionMap &descriptionsChanged() const  {return mDescriptionsChanged;}
          const DescriptionIDList &descriptionsRemoved() const      {return mDescriptionsRemoved;}

          String getDebugValueString(bool includeCommaPrefix = true) const;

        protected:
          void resetChanged();

          String log(const char *message) const;

        protected:
          PUID mID;
          ThreadWeakPtr mThisWeak;
          ThreadTypes mType;
          bool mCanModify;
          bool mModifying;

          IPublicationPtr mPublication;
          IPublicationPtr mPermissionPublication;

          DetailsPtr mDetails;
          ThreadContactsPtr mContacts;
          UINT mMessagesVersion;
          MessageList mMessageList;
          MessageMap mMessageMap;
          MessageReceiptsPtr mMessageReceipts;
          UINT mDialogsVersion;
          DialogMap mDialogs;

          DocumentPtr mChangesDoc;

          bool mDetailsChanged;
          ThreadContactMap mContactsChanged;
          ContactURIList mContactsRemoved;
          ThreadContactMap mContactsToAddChanged;
          ContactURIList mContactsToAddRemoved;
          ContactURIList mContactsToRemoveChanged;
          ContactURIList mContactsToRemoveRemoved;
          MessageList mMessagesChanged;
          Time mMessagesChangedTime;
          MessageReceiptMap mMessageReceiptsChanged;
          DialogMap mDialogsChanged;
          DialogIDList mDialogsRemoved;
          ChangedDescriptionMap mDescriptionsChanged;
          DescriptionIDList mDescriptionsRemoved;
        };
      };
    }
  }
}
