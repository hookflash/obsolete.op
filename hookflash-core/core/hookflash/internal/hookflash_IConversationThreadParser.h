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
#include <hookflash/IConversationThread.h>
#include <hookflash/ICall.h>

#include <hookflash/stack/hookflashTypes.h>
#include <hookflash/stack/IHelper.h>
#include <hookflash/services/IICESocket.h>

#include <zsLib/Log.h>

#define HOOKFLASH_CONVESATION_THREAD_BASE_THREAD_INDEX (3)

namespace hookflash
{
  namespace internal
  {
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadParser
    #pragma mark

    interaction IConversationThreadParser
    {
      typedef stack::IPublicationPtr IPublicationPtr;

      //-----------------------------------------------------------------------
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

      class Contacts;
      typedef boost::shared_ptr<Contacts> ContactsPtr;
      typedef boost::weak_ptr<Contacts> ContactsWeakPtr;

      class Dialog;
      typedef boost::shared_ptr<Dialog> DialogPtr;
      typedef boost::weak_ptr<Dialog> DialogWeakPtr;

      class Details;
      typedef boost::shared_ptr<Details> DetailsPtr;
      typedef boost::weak_ptr<Details> DetailsWeakPtr;

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadParser::Message
      #pragma mark

      class Message
      {
      public:
        typedef zsLib::XML::ElementPtr ElementPtr;
        typedef zsLib::String String;
        typedef zsLib::Time Time;

        typedef stack::IPeerFilesPtr IPeerFilesPtr;

      public:
        static MessagePtr create(
                                 const char *messageID,
                                 const char *fromContactID,
                                 const char *mimeType,
                                 const char *body,
                                 Time sent,
                                 IPeerFilesPtr signer
                                 );

        static MessagePtr create(ElementPtr messageBundleEl);

        const ElementPtr &messageBundleElement() const {return mBundleEl;}

        const String &messageID() const         {return mMessageID;}
        const String &fromContactID() const     {return mFromContactID;}
        const String &mimeType() const          {return mMimeType;}
        const String &body() const              {return mBody;}
        const Time &sent() const                {return mSent;}

      protected:
        MessageWeakPtr mThisWeak;
        ElementPtr mBundleEl;

        String mMessageID;
        String mFromContactID;
        String mMimeType;
        String mBody;
        Time mSent;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadParser::MessageReceipts
      #pragma mark

      class MessageReceipts
      {
      public:
        typedef zsLib::UINT UINT;
        typedef zsLib::String String;
        typedef zsLib::Time Time;
        typedef zsLib::XML::ElementPtr ElementPtr;

        typedef String MessageID;
        typedef Time ReceiptTime;
        typedef std::map<MessageID, ReceiptTime> MessageReceiptMap;
        typedef std::list<MessageID> MessageIDList;

        static MessageReceiptsPtr create(UINT version);
        static MessageReceiptsPtr create(UINT version, const String &messageID);
        static MessageReceiptsPtr create(UINT version, const MessageIDList &messageIDs);
        static MessageReceiptsPtr create(UINT version, const MessageReceiptMap &messageReceipts);

        static MessageReceiptsPtr create(ElementPtr messageReceiptsEl);

        ElementPtr receiptsElement() const          {return mReceiptsEl;}

        UINT version() const                        {return mVersion;}
        const MessageReceiptMap &receipts() const  {return mReceipts;}

      protected:
        MessageReceiptsWeakPtr mThisWeak;
        ElementPtr mReceiptsEl;

        UINT mVersion;
        MessageReceiptMap mReceipts;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadParser::Contacts
      #pragma mark

      class ThreadContact
      {
      public:
        typedef zsLib::UINT UINT;
        typedef zsLib::String String;
        typedef zsLib::XML::ElementPtr ElementPtr;

        static ThreadContactPtr create(
                                       IContactForConversationThreadPtr contact,
                                       ElementPtr profileBundleEl
                                       );

        IContactForConversationThreadPtr contact() const  {return mContact;}
        ElementPtr profileBundleElement() const           {return mProfileBundleEl;}

      protected:
        IContactForConversationThreadPtr mContact;
        ElementPtr mProfileBundleEl;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadParser::Contacts
      #pragma mark

      class Contacts
      {
      public:
        typedef zsLib::UINT UINT;
        typedef zsLib::String String;
        typedef zsLib::XML::ElementPtr ElementPtr;

        typedef String ContactID;
        typedef std::list<ThreadContactPtr> ContactList;
        typedef std::list<ContactID> ContactIDList;
        typedef std::map<ContactID, ThreadContactPtr> ContactMap;

        static ContactsPtr create(
                                  UINT version,
                                  const ContactList &contacts,
                                  const ContactList &addContacts,
                                  const ContactIDList &removeContacts
                                  );

        static ContactsPtr create(
                                  IAccountForConversationThreadPtr account,
                                  ElementPtr contactsEl
                                  );

        ElementPtr contactsElement() const          {return mContactsEl;}

        UINT version() const                        {return mVersion;}
        const ContactMap &contacts() const          {return mContacts;}
        const ContactMap &addContacts() const       {return mAddContacts;}
        const ContactIDList &removeContacts() const {return mRemoveContacts;}

      protected:
        ContactsWeakPtr mThisWeak;
        ElementPtr mContactsEl;

        UINT mVersion;
        ContactMap mContacts;
        ContactMap mAddContacts;
        ContactIDList mRemoveContacts;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
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

        typedef zsLib::String String;
        typedef zsLib::ULONG ULONG;
        typedef zsLib::ULONG UINT;
        typedef zsLib::XML::ElementPtr ElementPtr;
        typedef stack::IPeerFilesPtr IPeerFilesPtr;

        struct Codec;

        struct Description;
        typedef boost::shared_ptr<Description> DescriptionPtr;
        typedef boost::weak_ptr<Description> DescriptionWeakPtr;

        struct Codec
        {
          typedef zsLib::String String;
          typedef zsLib::UINT UINT;

          UINT mCodecID;

          String mName;
          UINT mPTime;
          UINT mRate;
          UINT mChannels;

          Codec() : mCodecID(0), mPTime(0), mRate(0), mChannels(0) {}
        };
        typedef std::list<Codec> CodecList;

        struct Description
        {
          typedef zsLib::String String;
          typedef zsLib::ULONG ULONG;
          typedef zsLib::UINT UINT;

          typedef UINT FoundationIndex;
          typedef services::IICESocket::CandidateList CandidateList;
          typedef std::map<FoundationIndex, CandidateList> CandidateLists;

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
        };
        typedef std::list<DescriptionPtr> DescriptionList;

        static DialogPtr create(
                                UINT version,
                                const char *dialogID,
                                DialogStates state,
                                DialogClosedReasons closedReason,
                                const char *callerContactID,
                                const char *callerLocationID,
                                const char *calleeContactID,
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
        const String &callerContactID() const       {return mCallerContactID;}
        const String &callerLocationID() const      {return mCallerLocationID;}
        const String &calleeContactID() const       {return mCalleeContactID;}
        const String &calleeLocationID() const      {return mCalleeLocationID;}
        const String &replacesDialogID() const      {return mReplacesDialogID;}
        const DescriptionList &descriptions() const {return mDescriptions;}

        String getDebugValuesString() const;

      private:
        DialogWeakPtr mThisWeak;
        ElementPtr mDialogBundleEl;

        UINT mVersion;
        String mDialogID;
        DialogStates mState;
        DialogClosedReasons mClosedReason;
        String mClosedReasonMessage;
        String mCallerContactID;
        String mCallerLocationID;
        String mCalleeContactID;
        String mCalleeLocationID;
        String mReplacesDialogID;
        DescriptionList mDescriptions;
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
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

        typedef zsLib::String String;
        typedef zsLib::UINT UINT;
        typedef zsLib::Time Time;
        typedef zsLib::XML::ElementPtr ElementPtr;

        static const char *toString(ConversationThreadStates state);
        static ConversationThreadStates toConversationThreadState(const char *state);

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

      protected:
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

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IConversationThreadParser::Thread
      #pragma mark

      class Thread
      {
      public:
        typedef zsLib::Exceptions::InvalidArgument InvalidArgument;

        enum ThreadTypes
        {
          ThreadType_Host,
          ThreadType_Slave,
        };

        static const char *toString(ThreadTypes type);
        static ThreadTypes toThreadTypes(const char *type) throw (InvalidArgument);

        typedef zsLib::PUID PUID;
        typedef zsLib::UINT UINT;
        typedef zsLib::String String;
        typedef zsLib::Time Time;
        typedef zsLib::XML::DocumentPtr DocumentPtr;

        typedef String MessageID;
        typedef std::list<MessagePtr> MessageList;
        typedef std::map<MessageID, MessagePtr> MessageMap;

        typedef Contacts::ContactMap ContactMap;
        typedef Contacts::ContactList ContactList;
        typedef Contacts::ContactIDList ContactIDList;

        typedef MessageReceipts::MessageReceiptMap MessageReceiptMap;

        typedef String DialogID;
        typedef std::map<DialogID, DialogPtr> DialogMap;
        typedef std::list<DialogPtr> DialogList;
        typedef std::list<DialogID> DialogIDList;
        typedef Dialog::DialogStates DialogStates;

        typedef Details::ConversationThreadStates ConversationThreadStates;

        typedef Dialog::Description Description;
        typedef Dialog::DescriptionPtr DescriptionPtr;

        typedef String DescriptionID;
        typedef std::pair<DialogID, DescriptionPtr> ChangedDescription;
        typedef std::map<DescriptionID, ChangedDescription> ChangedDescriptionMap;
        typedef std::list<DescriptionPtr> DescriptionList;
        typedef std::list<DescriptionID> DescriptionIDList;

      protected:
        Thread();

      public:
        ~Thread();

        static ThreadPtr create(
                                IAccountForConversationThreadPtr account,
                                IPublicationPtr publication
                                );

        bool updateFrom(
                        IAccountForConversationThreadPtr account,
                        IPublicationPtr publication
                        );

        static ThreadPtr create(
                                ThreadTypes threadType,         // needed for document name
                                const char *creatorContactID,
                                const char *creatorLocationID,
                                const char *baseThreadID,
                                const char *hostThreadID,
                                const char *topic,
                                const char *replaces,
                                ConversationThreadStates state,
                                const char *peerHostContactID = NULL,
                                const char *peerHostLocationID = NULL
                                );

        void updateBegin();
        bool updateEnd();

        void setState(Details::ConversationThreadStates state);

        void setContacts(const ContactMap &contacts);
        void setContactsToAdd(const ContactMap &contactsToAdd);
        void setContactsToRemove(const ContactIDList &contactsToRemoved);

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
        ContactsPtr contacts() const                              {return mContacts;}
        const MessageList &messages() const                       {return mMessageList;}
        const MessageMap &messagesAsMap() const                   {return mMessageMap;}
        MessageReceiptsPtr messageReceipts() const                {return mMessageReceipts;}
        const DialogMap &dialogs() const                          {return mDialogs;}

        // obtain a list of changes since the last updateFrom was called
        bool detailsChanged() const                               {return mDetailsChanged;}
        const ContactMap &contactsChanged() const                 {return mContactsChanged;}
        const ContactIDList &contactsRemoved() const              {return mContactsRemoved;}
        const ContactMap &contactsToAddChanged() const            {return mContactsToAddChanged;}
        const ContactIDList &contactsToAddRemoved() const         {return mContactsToAddRemoved;}
        const ContactIDList &contactsToRemoveChanged() const      {return mContactsToRemoveChanged;}
        const ContactIDList &contactsToRemoveRemoved() const      {return mContactsToRemoveRemoved;}
        const MessageList &messagedChanged() const                {return mMessagesChanged;}
        Time messagedChangedTime() const                          {return mMessagesChangedTime;}
        const MessageReceiptMap &messageReceiptsChanged() const   {return mMessageReceiptsChanged;}
        const DialogMap &dialogsChanged() const                   {return mDialogsChanged;}
        const DialogIDList &dialogsRemoved() const                {return mDialogsRemoved;}
        const ChangedDescriptionMap &descriptionsChanged() const  {return mDescriptionsChanged;}
        const DescriptionIDList &descriptionsRemoved() const      {return mDescriptionsRemoved;}

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
        ContactsPtr mContacts;
        UINT mMessagesVersion;
        MessageList mMessageList;
        MessageMap mMessageMap;
        MessageReceiptsPtr mMessageReceipts;
        UINT mDialogsVersion;
        DialogMap mDialogs;

        DocumentPtr mChangesDoc;

        bool mDetailsChanged;
        ContactMap mContactsChanged;
        ContactIDList mContactsRemoved;
        ContactMap mContactsToAddChanged;
        ContactIDList mContactsToAddRemoved;
        ContactIDList mContactsToRemoveChanged;
        ContactIDList mContactsToRemoveRemoved;
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
