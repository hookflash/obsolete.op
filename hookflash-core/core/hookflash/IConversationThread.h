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

#include <hookflash/hookflashTypes.h>
#include <zsLib/Proxy.h>
#include <zsLib/XML.h>

#include <list>

namespace hookflash
{
  interaction IConversationThread
  {
    typedef zsLib::String String;
    typedef zsLib::Time Time;
    typedef zsLib::XML::ElementPtr ElementPtr;

    enum MessageDeliveryStates
    {
      MessageDeliveryState_Discovering      = 0,
      MessageDeliveryState_UserNotAvailable = 1,
      MessageDeliveryState_Delivered        = 2,
    };

    enum ContactStates
    {
      ContactState_NotApplicable,
      ContactState_Finding,
      ContactState_Connected,
      ContactState_Disconnected
    };

    struct ContactInfo
    {
      IContactPtr mContact;
      ElementPtr mProfileBundleEl;
    };

    typedef std::list<IContactPtr> ContactList;
    typedef std::list<ContactInfo> ContactInfoList;

    static const char *toString(MessageDeliveryStates state);
    static const char *toString(ContactStates state);

    static IConversationThreadPtr create(
                                         hookflash::IAccountPtr account,
                                         ElementPtr profileBundleEl
                                         );

    virtual const String &getThreadID() const = 0;

    virtual bool amIHost() const = 0;

    virtual void getContacts(ContactList &contactList) const = 0;
    virtual ElementPtr getProfileBundle(IContactPtr contact) const = 0;
    virtual ContactStates getContactState(IContactPtr contact) const = 0;

    virtual void addContacts(const ContactInfoList &contacts) = 0;
    virtual void removeContacts(const ContactList &contacts) = 0;

    // sending a message will cause the message to be delivered to all the contacts currently in the conversation
    virtual void sendMessage(
                             const char *messageID,
                             const char *messageType,
                             const char *message
                             ) = 0;

    // returns false if the message ID is not known
    virtual bool getMessage(
                            const char *messageID,
                            IContactPtr &outFrom,
                            String &outMessageType,
                            String &outMessage,
                            Time &outTime
                            ) const = 0;

    // returns false if the message ID is not known
    virtual bool getMessageDeliveryState(
                                         const char *messageID,
                                         MessageDeliveryStates &outDeliveryState
                                         ) const = 0;
  };

  interaction IConversationThreadDelegate
  {
    typedef IConversationThread::MessageDeliveryStates MessageDeliveryStates;
    typedef IConversationThread::ContactStates ContactStates;

    virtual void onConversationThreadNew(IConversationThreadPtr conversationThread) = 0;

    virtual void onConversationThreadContactsChanged(IConversationThreadPtr conversationThread) = 0;
    virtual void onConversationThreadContactStateChanged(
                                                         IConversationThreadPtr conversationThread,
                                                         IContactPtr contact,
                                                         ContactStates state
                                                         ) = 0;

    virtual void onConversationThreadMessage(
                                             IConversationThreadPtr conversationThread,
                                             const char *messageID
                                             ) = 0;

    virtual void onConversationThreadMessageDeliveryStateChanged(
                                                                 IConversationThreadPtr conversationThread,
                                                                 const char *messageID,
                                                                 MessageDeliveryStates state
                                                                 ) = 0;

    virtual void onConversationThreadPushMessage(
                                                 IConversationThreadPtr conversationThread,
                                                 const char *messageID,
                                                 IContactPtr contact
                                                 ) = 0;
  };
}

ZS_DECLARE_PROXY_BEGIN(hookflash::IConversationThreadDelegate)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::IConversationThread::MessageDeliveryStates, MessageDeliveryStates)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::IConversationThreadPtr, IConversationThreadPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::IContactPtr, IContactPtr)
ZS_DECLARE_PROXY_TYPEDEF(hookflash::IConversationThread::ContactStates, ContactStates)
ZS_DECLARE_PROXY_METHOD_1(onConversationThreadNew, IConversationThreadPtr)
ZS_DECLARE_PROXY_METHOD_1(onConversationThreadContactsChanged, IConversationThreadPtr)
ZS_DECLARE_PROXY_METHOD_3(onConversationThreadContactStateChanged, IConversationThreadPtr, IContactPtr, ContactStates)
ZS_DECLARE_PROXY_METHOD_2(onConversationThreadMessage, IConversationThreadPtr, const char *)
ZS_DECLARE_PROXY_METHOD_3(onConversationThreadMessageDeliveryStateChanged, IConversationThreadPtr, const char *, MessageDeliveryStates)
ZS_DECLARE_PROXY_METHOD_3(onConversationThreadPushMessage, IConversationThreadPtr, const char *, IContactPtr)
ZS_DECLARE_PROXY_END()
