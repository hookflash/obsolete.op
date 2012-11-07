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


#include <hookflash/internal/hookflash_IConversationThreadParser.h>
#include <hookflash/internal/hookflash_ConversationThread.h>
#include <hookflash/internal/hookflash_Contact.h>
#include <hookflash/internal/hookflash_Account.h>

#include <hookflash/stack/IPeerFiles.h>
#include <hookflash/stack/IPeerFilePrivate.h>
#include <hookflash/stack/IPublication.h>
#include <hookflash/stack/IXMLDiff.h>

#include <zsLib/Log.h>
#include <zsLib/Stringize.h>
#include <zsLib/zsHelpers.h>
#include <zsLib/XML.h>
#include <zsLib/Numeric.h>
#include <zsLib/IPAddress.h>


namespace hookflash { ZS_DECLARE_SUBSYSTEM(hookflash) }

using namespace zsLib::XML;
using zsLib::Stringize;
using zsLib::Numeric;
typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

namespace hookflash
{
  namespace internal
  {
    typedef zsLib::CSTR CSTR;
    typedef zsLib::String String;
    typedef zsLib::Time Time;
    typedef zsLib::BYTE BYTE;
    typedef zsLib::WORD WORD;
    typedef zsLib::DWORD DWORD;
    typedef zsLib::ULONG ULONG;
    typedef zsLib::UINT UINT;
    typedef zsLib::IPAddress IPAddress;

    typedef services::IICESocket::Candidate Candidate;
    typedef services::IICESocket::CandidateList CandidateList;
    typedef services::IICESocket IICESocket;

    typedef stack::IPeerFilePrivatePtr IPeerFilePrivatePtr;
    typedef stack::IPeerFilePublic IPeerFilePublic;
    typedef stack::IPeerFilePublicPtr IPeerFilePublicPtr;
    typedef stack::IHelper::SplitMap SplitMap;
    typedef stack::IXMLDiff IXMLDiff;
    typedef stack::IPublication IPublication;
    typedef stack::IPublication::AutoRecursiveLockPtr AutoRecursiveLockPtr;

    typedef IConversationThreadParser::MessagePtr MessagePtr;
    typedef IConversationThreadParser::MessageReceiptsPtr MessageReceiptsPtr;
    typedef IConversationThreadParser::ThreadContact ThreadContact;
    typedef IConversationThreadParser::ThreadContactPtr ThreadContactPtr;    
    typedef IConversationThreadParser::Contacts Contacts;
    typedef IConversationThreadParser::ContactsPtr ContactsPtr;
    typedef IConversationThreadParser::Dialog Dialog;
    typedef IConversationThreadParser::DialogPtr DialogPtr;
    typedef IConversationThreadParser::Dialog::Description::CandidateLists CandidateLists;
    typedef IConversationThreadParser::DetailsPtr DetailsPtr;

    typedef IConversationThreadParser::Dialog::DialogStates DialogStates;
    typedef IConversationThreadParser::Details::ConversationThreadStates ConversationThreadStates;

    typedef IConversationThreadParser::Thread Thread;
    typedef IConversationThreadParser::ThreadPtr ThreadPtr;

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    static ElementPtr createElement(const char *elementName)
    {
      ElementPtr el = Element::create(elementName);
      return el;
    }

    //-------------------------------------------------------------------------
    static ElementPtr createElement(const char *elementName, const char *id)
    {
      ElementPtr el = Element::create();
      el->setValue(elementName);

      if ((id) &&
          (0 != *id)) {
        el->setAttribute("id", id);
      }
      return el;
    }

    //-------------------------------------------------------------------------
    static ElementPtr createElementWithText(const char *elementName, const char *text)
    {
      ElementPtr el = Element::create();
      el->setValue(elementName);

      if ((text) &&
          (0 != *text)) {
        TextPtr textNode = Text::create();
        textNode->setValue(text);
        el->adoptAsLastChild(textNode);
      }
      return el;
    }

    //-------------------------------------------------------------------------
    static ElementPtr createElementWithText(const char *elementName, const char *id, const char *text)
    {
      ElementPtr el = Element::create();
      el->setValue(elementName);

      if ((id) &&
          (0 != *id)) {
        el->setAttribute("id", id);
      }

      if ((text) &&
          (0 != *text)) {
        TextPtr textNode = Text::create();
        textNode->setValue(text);
        el->adoptAsLastChild(textNode);
      }
      return el;
    }

    //-------------------------------------------------------------------------
    static ElementPtr createElementWithTextAndEntityEncode(const char *elementName, const char *text)
    {
      ElementPtr el = Element::create();
      el->setValue(elementName);

      if ((text) &&
          (0 != *text)) {
        TextPtr textNode = Text::create();
        textNode->setValueAndEntityEncode(text);
        el->adoptAsLastChild(textNode);
      }
      return el;
    }

    //-------------------------------------------------------------------------
    static UINT getVersion(ElementPtr el) throw (Numeric<UINT>::ValueOutOfRange)
    {
      if (!el) return 0;
      AttributePtr versionAt = el->findAttribute("version");
      if (!versionAt) return 0;
      return Numeric<UINT>(versionAt->getValue());
    }

    //-------------------------------------------------------------------------
    static void convert(const Contacts::ContactIDList &input, Contacts::ContactMap &output)
    {
      typedef Contacts::ContactID ContactID;
      typedef Contacts::ContactIDList ContactIDList;
      typedef Contacts::ContactMap ContactMap;

      for (ContactIDList::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const ContactID &id = (*iter);
        output[id] = ThreadContactPtr();
      }
    }

    //-------------------------------------------------------------------------
    static void convert(const Contacts::ContactMap &input, Contacts::ContactIDList &output)
    {
      typedef Contacts::ContactID ContactID;
      typedef Contacts::ContactIDList ContactIDList;
      typedef Contacts::ContactMap ContactMap;

      for (ContactMap::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const ContactID &id = (*iter).first;
        output.push_back(id);
      }
    }

    //-------------------------------------------------------------------------
    static void convert(const Contacts::ContactMap &input, Contacts::ContactList &output)
    {
      typedef Contacts::ContactID ContactID;
      typedef Contacts::ContactIDList ContactIDList;
      typedef Contacts::ContactMap ContactMap;

      for (ContactMap::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const ThreadContactPtr &contact = (*iter).second;
        output.push_back(contact);
      }
    }

    //-------------------------------------------------------------------------
    static void convert(const Thread::DialogMap &input, Thread::DialogList &output)
    {
      typedef Thread::DialogMap DialogMap;

      for (DialogMap::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const DialogPtr &dialog = (*iter).second;
        output.push_back(dialog);
      }
    }

    //-------------------------------------------------------------------------
    static void convert(const Thread::DialogMap &input, Thread::DialogIDList &output)
    {
      typedef Thread::DialogMap DialogMap;

      for (DialogMap::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const DialogPtr &dialog = (*iter).second;
        output.push_back(dialog->dialogID());
      }
    }

    //-------------------------------------------------------------------------
    static void convert(const Thread::DialogList &input, Thread::DialogMap &output)
    {
      typedef Thread::DialogList DialogList;

      for (DialogList::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const DialogPtr &dialog = (*iter);
        output[dialog->dialogID()] = dialog;
      }
    }

    //-------------------------------------------------------------------------
    static void convert(const Thread::DialogList &input, Thread::DialogIDList &output)
    {
      typedef Thread::DialogList DialogList;

      for (DialogList::const_iterator iter = input.begin(); iter != input.end(); ++iter)
      {
        const DialogPtr &dialog = (*iter);
        output.push_back(dialog->dialogID());
      }
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadParser::Message
    #pragma mark

    // <thread>
    //  ...
    //  <messages>
    //   ...
    //   <messageBundle xmlns="http://www.hookflash.com/openpeer/1.0/message">
    //    <message id=”e041038922edbc0638cebbded884896” />
    //     <from id=”920bd1d88e4cc3ba0f95e24ea9168e272ff03b3b” />
    //     <sent>2002-Jan-01 10:00:01.123456789</sent>
    //     <mimeType>text/plain</mimeType>
    //     <body>This is a test message.</body>
    //    </message>
    //    <Signature xmlns="http://www.w3.org/2000/09/xmldsig#">
    //     <SignedInfo>
    //      <SignatureMethod Algorithm="http://www.w3.org/2000/09/xmldsig#dsa-sha1" />
    //      <Reference URI="#e041038922edbc0638cebbded884896">
    //       <DigestMethod Algorithm="http://www.w3.org/2000/09/xmldsig#sha1"/>
    //       <DigestValue>YUZSaWJFcFhXVzEwUzJOR2JITmFSazVYVm0xU2VsZHJWVFZpUmxwMVVXeHdW</DigestValue>
    //      </Reference>
    //     </SignedInfo>
    //     <SignatureValue>Y0ZoWFZ6RXdVekpPUjJKSVRtRlNhelZZVm0weFUyVnNa</SignatureValue>
    //    </Signature>
    //   </messageBundle>
    //   ...
    //  </messages>
    //  ...
    // </thread>

    //-------------------------------------------------------------------------
    MessagePtr IConversationThreadParser::Message::create(
                                                          const char *messageID,
                                                          const char *fromContactID,
                                                          const char *mimeType,
                                                          const char *body,
                                                          Time sent,
                                                          IPeerFilesPtr signer
                                                          )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!signer)

      MessagePtr pThis = MessagePtr(new Message);
      pThis->mThisWeak = pThis;
      pThis->mMessageID = Stringize<CSTR>(messageID);
      pThis->mFromContactID = Stringize<CSTR>(fromContactID);
      pThis->mMimeType = Stringize<CSTR>(mimeType);
      pThis->mBody = Stringize<CSTR>(body);
      pThis->mSent = sent;

      // now its time to generate the XML
      ElementPtr messageBundleEl = createElement("messageBundle");
      ElementPtr messageEl = createElement("message", messageID);
      ElementPtr fromEl = createElement("from", fromContactID);
      ElementPtr sentEl = createElementWithText("sent", Stringize<Time>(sent).string());
      ElementPtr mimeTypeEl = createElementWithText("mimeType", mimeType);
      ElementPtr bodyEl = createElementWithTextAndEntityEncode("body", body);

      messageBundleEl->adoptAsLastChild(messageEl);
      messageEl->adoptAsLastChild(fromEl);
      messageEl->adoptAsLastChild(sentEl);
      messageEl->adoptAsLastChild(mimeTypeEl);
      messageEl->adoptAsLastChild(bodyEl);

      IPeerFilePrivatePtr privatePeer = signer->getPrivate();
      ZS_THROW_INVALID_ARGUMENT_IF(!privatePeer)

      privatePeer->signElement(messageEl);

      pThis->mBundleEl = messageBundleEl;
      return pThis;
    }

    //-------------------------------------------------------------------------
    MessagePtr IConversationThreadParser::Message::create(ElementPtr messageBundleEl)
    {
      if (!messageBundleEl) return MessagePtr();

      MessagePtr pThis = MessagePtr(new Message);
      pThis->mThisWeak = pThis;

      try {
        ElementPtr messageEl = messageBundleEl->findFirstChildElementChecked("message");
        ElementPtr fromEl = messageEl->findFirstChildElementChecked("from");
        ElementPtr sentEl = messageEl->findFirstChildElementChecked("sent");
        ElementPtr mimeTypeEl = messageEl->findFirstChildElementChecked("mimeType");
        ElementPtr bodyEl = messageEl->findFirstChildElementChecked("body");

        pThis->mMessageID = messageEl->getAttributeValue("id");
        pThis->mFromContactID = fromEl->getAttributeValue("id");
        pThis->mMimeType = mimeTypeEl->getText();
        pThis->mBody = bodyEl->getTextAndEntityDecode();
        pThis->mSent = Numeric<Time>(sentEl->getText());
        pThis->mBundleEl = messageBundleEl;
      } catch (CheckFailed &) {
        ZS_LOG_ERROR(Detail, "message bundle XML parse element check failure")
        return MessagePtr();
      } catch (Numeric<Time>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "message bundle value out of range parse error")
        return MessagePtr();
      }

      if (pThis->mMessageID.size() < 1) return MessagePtr();
      if (pThis->mFromContactID.size() < 1) return MessagePtr();

      return pThis;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadParser::MessageReceipts
    #pragma mark

    // <thread>
    //  ...
    //  <receipts version="1">
    //   <receipt id="e041038922edbc0638cebbded884896">2002-Jan-01 10:00:01.123456789</receipt>
    //   <receipt id="920bd1d88e4cc3ba0f95e24ea9168e2">2002-Jan-01 10:00:01.123456789</receipt>
    //  </receipts>
    //  ...
    // </thread>

    //-------------------------------------------------------------------------
    MessageReceiptsPtr IConversationThreadParser::MessageReceipts::create(UINT version)
    {
      MessageReceiptMap receipts;
      return create(version, receipts);
    }

    //-------------------------------------------------------------------------
    MessageReceiptsPtr IConversationThreadParser::MessageReceipts::create(UINT version, const String &messageID)
    {
      MessageReceiptMap receipts;
      receipts[messageID] = zsLib::now();
      return create(version, receipts);
    }

    //-------------------------------------------------------------------------
    MessageReceiptsPtr IConversationThreadParser::MessageReceipts::create(UINT version, const MessageIDList &messageIDs)
    {
      MessageReceiptMap receipts;
      for (MessageIDList::const_iterator iter = messageIDs.begin(); iter != messageIDs.end(); ++iter)
      {
        const String &messageID = (*iter);
        receipts[messageID] = zsLib::now();
      }
      return create(version, receipts);
    }

    //-------------------------------------------------------------------------
    MessageReceiptsPtr IConversationThreadParser::MessageReceipts::create(UINT version, const MessageReceiptMap &messageReceipts)
    {
      MessageReceiptsPtr pThis(new MessageReceipts);
      pThis->mThisWeak = pThis;
      pThis->mVersion = version;
      pThis->mReceipts = messageReceipts;

      ElementPtr receiptsEl = createElement("receipts");
      receiptsEl->setAttribute("version", Stringize<UINT>(version).string());

      for (MessageReceiptMap::const_iterator iter = messageReceipts.begin(); iter != messageReceipts.end(); ++iter)
      {
        const String &messageID = (*iter).first;
        const Time &time = (*iter).second;
        ElementPtr receiptEl = createElementWithText("receipt", messageID, Stringize<Time>(time).string());
        receiptsEl->adoptAsLastChild(receiptEl);
      }

      pThis->mReceiptsEl = receiptsEl;
      return pThis;
    }

    //-------------------------------------------------------------------------
    MessageReceiptsPtr IConversationThreadParser::MessageReceipts::create(ElementPtr messageReceiptsEl)
    {
      if (!messageReceiptsEl) return MessageReceiptsPtr();

      MessageReceiptsPtr pThis(new MessageReceipts);
      pThis->mThisWeak = pThis;

      try {
        pThis->mVersion = Numeric<UINT>(messageReceiptsEl->getAttributeValue("version"));
        ElementPtr receiptEl = messageReceiptsEl->findFirstChildElement("receipt");
        while (receiptEl)
        {
          String id = receiptEl->getAttributeValue("id");
          String timeStr = receiptEl->getText();
          ZS_LOG_TRACE(String("Parsing receipt") + ", receipt ID=" + id + ", acknowledged at=" + timeStr)
          Time time = Numeric<Time>(timeStr);

          pThis->mReceipts[id] = time;
          ZS_LOG_TRACE(String("Found receipt") + ", receipt ID=" + id + ", acknowledged at=" + Stringize<Time>(time).string())

          receiptEl = receiptEl->findNextSiblingElement("receipt");
        }
      } catch(CheckFailed &) {
        return MessageReceiptsPtr();
      } catch (Numeric<UINT>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "message receipt parse value out of range")
        return MessageReceiptsPtr();
      } catch (Numeric<Time>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "message receipt parse value out of range")
        return MessageReceiptsPtr();
      }
      return pThis;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadParser::ThreadContact
    #pragma mark

    //-------------------------------------------------------------------------
    ThreadContactPtr IConversationThreadParser::ThreadContact::create(
                                                                      IContactForConversationThreadPtr contact,
                                                                      ElementPtr profileBundleEl
                                                                      )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!contact)
      
      ThreadContactPtr pThis(new ThreadContact);
      pThis->mContact = contact;
      pThis->mProfileBundleEl = profileBundleEl;
      return pThis;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadParser::Contacts
    #pragma mark

    // <thread>
    //  ...
    //  <contacts version="1">
    //   <contact id="920bd1d88e4cc3ba0f95e24ea9168e272ff03b3b" disposition="add"><peer version=”1”><sectionBundle xmlns="http://www.hookflash.com/openpeer/1.0/message"><section id=”A”> ... contents ...</section> ... </sectionBundle></peer></contact>
    //   <contact id="f95e24ea9168e242ff03b3920bd1d88a4cc3ba04" disposition="add"><profileBundle><profile></profile></profileBundle><peer version=”1”><sectionBundle xmlns="http://www.hookflash.com/openpeer/1.0/message"><section id=”A”> ... contents ...</section> ... </sectionBundle></peer></contact>
    //   <contact id="...">...</contact>
    //   <contact id="..." disposition="remove"></contact>
    //  <contacts>
    //  ...
    // </thread>

    //-------------------------------------------------------------------------
    ContactsPtr IConversationThreadParser::Contacts::create(
                                                            UINT version,
                                                            const ContactList &contacts,
                                                            const ContactList &addContacts,
                                                            const ContactIDList &removeContacts
                                                            )
    {
      ContactsPtr pThis(new Contacts());
      pThis->mThisWeak = pThis;
      pThis->mVersion = version;

      for (ContactList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
      {
        const ThreadContactPtr &contact = (*iter);
        pThis->mContacts[contact->contact()->getContactID()] = contact;
      }

      for (ContactList::const_iterator iter = addContacts.begin(); iter != addContacts.end(); ++iter)
      {
        const ThreadContactPtr &contact = (*iter);
        pThis->mAddContacts[contact->contact()->getContactID()] = contact;
      }

      pThis->mRemoveContacts = removeContacts;

      ElementPtr contactsEl = createElement("contacts");
      if (0 != version) {
        contactsEl->setAttribute("version", Stringize<UINT>(version).string());
      }

      for (ContactMap::const_iterator iter = pThis->mContacts.begin(); iter != pThis->mContacts.end(); ++iter)
      {
        const String &contactID = (*iter).first;
        const ThreadContactPtr &contact = (*iter).second;
        IPeerFilePublicPtr peerPublic = contact->contact()->getPeerFilePublic();
        if (!peerPublic) {
          ZS_LOG_ERROR(Detail, String("IConversationThreadParser::Contacts contact does not have a public peer file") + ", contact ID=" + contactID)
          return ContactsPtr();
        }

        ElementPtr contactEl = createElement("contact", contactID);
        ElementPtr peerEl = peerPublic->saveToXML();
        ElementPtr profileBundleEl = contact->profileBundleElement();
        contactEl->adoptAsLastChild(peerEl);
        if (profileBundleEl) {
          contactEl->adoptAsLastChild(profileBundleEl->clone()->toElement());
        }
        contactsEl->adoptAsLastChild(contactEl);
      }

      for (ContactMap::const_iterator iter = pThis->mAddContacts.begin(); iter != pThis->mAddContacts.end(); ++iter)
      {
        const String &contactID = (*iter).first;
        const ThreadContactPtr &contact = (*iter).second;
        IPeerFilePublicPtr peerPublic = contact->contact()->getPeerFilePublic();
        if (!peerPublic) {
          ZS_LOG_ERROR(Detail, String("IConversationThreadParser::Contacts contact does not have a public peer file") + ", contact ID=" + contactID)
          return ContactsPtr();
        }

        ElementPtr contactEl = createElement("contact", contactID);
        contactEl->setAttribute("disposition", "add");
        ElementPtr peerEl = peerPublic->saveToXML();
        ElementPtr profileBundleEl = contact->profileBundleElement();
        contactEl->adoptAsLastChild(peerEl);
        if (profileBundleEl) {
          contactEl->adoptAsLastChild(profileBundleEl);
        }
        contactsEl->adoptAsLastChild(contactEl);
      }

      for (ContactIDList::const_iterator iter = pThis->mRemoveContacts.begin(); iter != pThis->mRemoveContacts.end(); ++iter)
      {
        const String &contactID = (*iter);

        ElementPtr contactEl = createElement("contact", contactID);
        contactEl->setAttribute("disposition", "remove");
        contactsEl->adoptAsLastChild(contactEl);
      }

      pThis->mContactsEl = contactsEl;
      return pThis;
    }

    //-------------------------------------------------------------------------
    ContactsPtr IConversationThreadParser::Contacts::create(
                                                            IAccountForConversationThreadPtr account,
                                                            ElementPtr contactsEl
                                                            )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!account)

      if (!contactsEl) return ContactsPtr();

      ContactsPtr pThis(new Contacts());
      pThis->mThisWeak = pThis;

      ContactMap previousRemovals;

      try {
        pThis->mVersion = getVersion(contactsEl);

        ElementPtr contactEl = contactsEl->findFirstChildElement("contact");
        while (contactEl)
        {
          // scope: parse the contact information
          {
            String id = contactEl->getAttributeValue("id");
            String disposition = contactEl->getAttributeValue("disposition");

            if (disposition == "remove") {
              if (previousRemovals.find(id) != previousRemovals.end()) goto next;

              previousRemovals[id] = ThreadContactPtr();
              pThis->mRemoveContacts.push_back(id);

              goto next;
            }

            IContactForConversationThreadPtr contact = account->findContact(id);
            if (!contact) {
              IPeerFilePublicPtr peerFilePublic = IPeerFilePublic::loadFromXML(contactEl->findFirstChildElementChecked("peer"));
              if (!peerFilePublic) return ContactsPtr();

              contact = IContactForConversationThread::createFromPeerFilePublic(account, peerFilePublic);
              if (!contact) return ContactsPtr();
            }
            
            ElementPtr profileBundleEl = contactEl->findFirstChildElement("profileBundle");
            if (!profileBundleEl) {
              profileBundleEl = contactEl->findFirstChildElement("profile");
            }
            if (profileBundleEl) {
              // make sure we have our own private copy of the data
              profileBundleEl = profileBundleEl->clone()->toElement();
            }

            ThreadContactPtr threadContact = ThreadContact::create(contact, profileBundleEl);
            if (disposition == "add") {
              pThis->mAddContacts[contact->getContactID()] = threadContact;
              goto next;
            }
            pThis->mContacts[contact->getContactID()] = threadContact;
          }

        next:
          contactEl = contactEl->findNextSiblingElement("contact");
        }
      } catch(CheckFailed &) {
        ZS_LOG_ERROR(Detail, "contact XML element check parser failure")
        return ContactsPtr();
      } catch (Numeric<UINT>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "contact parser value out of range")
        return ContactsPtr();
      }
      return pThis;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadParser::Dialog
    #pragma mark

    // <thread>
    //  ...
    //  <dialogs version="1">
    //   ...
    //   <dialogBundle>
    //    <dialog id=”5dc367c8392dfba2d08a27d8a2139ef7232b1df1” version="1">
    //     <state>open</state>
    //     <caller id=”920bd1d88e4cc3ba0f95e24ea9168e272ff03b3b” />
    //     <callerLocation id=”ed4c0d9a26d962ab179ee88f4c2e8695ccac3d7c” />
    //     <callee id=”83208ba54e6edb8cd2173b3d7fe83acca2ac5f3f” />
    //     <calleeLocation id=”225456088d6cc20f0797bbfcadfad84dba6fc292” />
    //     <replaces id=”d0790f20bf8de96f09e8d96674a7f30” />
    //     <descriptions>
    //      <description id=”3d6bef7dfb0b79072733fc03391e9” type=”audio” version="1">
    //       <ssrc id=”0eb60347” />
    //       <security>
    //        <secret>T0dVek5qTmtOREJrTW1KbU56bGpabVJqWXpGaE1USmhORGN3T0RJMFlqZ3laR0ps</secret>
    //        <algorithm>aes256</algorithm>
    //       </security>
    //       <codecs>
    //        <codec id=”0”>
    //         <name>ulaw</name>
    //         <ptime>20</ptime>
    //         <rate>8000</rate>
    //         <channels>1</channels>
    //        </codec>
    //        <codec id=”101”>
    //         <name>telephone-event</name>
    //         <ptime>20</ptime>
    //         <rate>8000</rate>
    //         <channels>1</channels>
    //        </codec>
    //       </codecs>
    //       <candidates>
    //        <candidate>
    //         <ip format=”ipv4”>100.200.10.20</ip>
    //         <ports>
    //          <port>9549</port>
    //          <port>49939</port>
    //         </ports>
    //         <username>7475bd88ec76c0f791fde51e56770f0d</username>
    //         <password>ZDMyNGU3MDcxZDNlMGZiMT..VjNjgwMTBlMjVh</password>
    //         <transport>udp</transport>
    //         <protocol>rtp/savp</protocol>
    //        </candidate>
    //        <candidate>
    //         <ip format=”ipv4”>192.168.1.2</ip>
    //         <ports>
    //          <port>15861</port>
    //          <port>43043</port>
    //         </ports>
    //         <username>118b63d5945c91919a4af5de24ebf9d4</username>
    //         <password>MTE4YjYzZDU5NDVjOTE5MTlhNGFmNWRlMjRlYmY5ZDQ=</password>
    //         <transport>udp</transport>
    //         <protocol>rtp/savp</protocol>
    //        </candidate>
    //       </candidates>
    //      </description>
    //      <description id=”938f6910ceb45c2269c6e7a710334bc” type=”video”>
    //       ...
    //      </description>
    //     </descriptions>
    //    </dialog>
    //    <Signature xmlns="http://www.w3.org/2000/09/xmldsig#">
    //     <SignedInfo>
    //      <SignatureMethod Algorithm="http://www.w3.org/2000/09/xmldsig#dsa-sha1" />
    //      <Reference URI="#5dc367c8392dfba2d08a27d8a2139ef7232b1df1">
    //       <DigestMethod Algorithm="http://www.w3.org/2000/09/xmldsig#sha1"/>
    //       <DigestValue>YUZSaWJFcFhXVzEwUzJOR2JITmFSazVYVm0xU2VsZHJWVFZpUmxwMVVXeHdW</DigestValue>
    //      </Reference>
    //     </SignedInfo>
    //     <SignatureValue>Y0ZoWFZ6RXdVekpPUjJKSVRtRlNhelZZVm0weFUyVnNa</SignatureValue>
    //    </Signature>
    //   </dialogBundle>
    //   ...
    //  <dialogs>
    //  ...
    // </thread>

    //-------------------------------------------------------------------------
    IConversationThreadParser::Dialog::Dialog() :
      mVersion(0),
      mClosedReason(DialogClosedReason_None)
    {
    }

    //-------------------------------------------------------------------------
    const char *IConversationThreadParser::Dialog::toString(DialogStates state)
    {
      switch (state)
      {
        case DialogState_None:      return "none";
        case DialogState_Preparing: return "preparing";
        case DialogState_Incoming:  return "incoming";
        case DialogState_Placed:    return "placed";
        case DialogState_Early:     return "early";
        case DialogState_Ringing:   return "ringing";
        case DialogState_Ringback:  return "ringback";
        case DialogState_Open:      return "open";
        case DialogState_Active:    return "active";
        case DialogState_Inactive:  return "inactive";
        case DialogState_Hold:      return "hold";
        case DialogState_Closing:   return "closing";
        case DialogState_Closed:    return "closed";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    DialogStates IConversationThreadParser::Dialog::toDialogStates(const char *state)
    {
      for (int loop = (int)DialogState_First; loop <= DialogState_Last; ++loop)
      {
        const char *name = toString((DialogStates)loop);
        if (Stringize<CSTR>(name).string() == state) return (DialogStates)loop;
      }
      return DialogState_None;
    }

    //-------------------------------------------------------------------------
    const char *IConversationThreadParser::Dialog::toString(DialogClosedReasons reason)
    {
      switch (reason)
      {
        case DialogClosedReason_None:                   return "";
        case DialogClosedReason_User:                   return "OK";
        case DialogClosedReason_RequestTimeout:         return "Request Timeout";
        case DialogClosedReason_TemporarilyUnavailable: return "Temporarily Unavailable";
        case DialogClosedReason_Busy:                   return "Busy Here";
        case DialogClosedReason_RequestTerminated:      return "Request Terminated";
        case DialogClosedReason_NotAcceptableHere:      return "Not Acceptable Here";
        case DialogClosedReason_ServerInternalError:    return "Server Internal Error";
        case DialogClosedReason_Decline:                return "Decline";
      }
      return "Unknown";
    }

    //-------------------------------------------------------------------------
    static void mergeInto(CandidateList &final, const CandidateList &source)
    {
      for (CandidateList::const_iterator sourceIter = source.begin(); sourceIter != source.end(); ++sourceIter)
      {
        const Candidate &sourceCandidate = (*sourceIter);

        bool found = false;

        for (CandidateList::iterator finalIter = final.begin(); finalIter != final.end(); ++finalIter)
        {
          Candidate &finalCandidate = (*finalIter);
          if (finalCandidate.mType != sourceCandidate.mType) continue;
          if (!finalCandidate.mIPAddress.isAddressEqual(sourceCandidate.mIPAddress)) continue;
          if (finalCandidate.mPriority != sourceCandidate.mPriority) continue;
          if (finalCandidate.mLocalPreference != sourceCandidate.mLocalPreference) continue;
          if (finalCandidate.mUsernameFrag != sourceCandidate.mUsernameFrag) continue;
          if (finalCandidate.mPassword != sourceCandidate.mPassword) continue;
          if (finalCandidate.mProtocol != sourceCandidate.mProtocol) continue;

          found = true;
          break;
        }

        if (found) continue;
        Candidate candidate;
        candidate.mType = sourceCandidate.mType;
        candidate.mIPAddress = sourceCandidate.mIPAddress;
        candidate.mIPAddress.setPort(0);
        candidate.mPriority = sourceCandidate.mPriority;
        candidate.mLocalPreference = sourceCandidate.mLocalPreference;
        candidate.mUsernameFrag = sourceCandidate.mUsernameFrag;
        candidate.mPassword = sourceCandidate.mPassword;
        candidate.mProtocol = sourceCandidate.mProtocol;

        final.push_back(candidate);
      }
    }
    //-------------------------------------------------------------------------
    static const Candidate &find(const Candidate &finalCandidate, const CandidateList &source)
    {
      ZS_LOG_TRACE(String("Find against candidate") + finalCandidate.toDebugString())
      for (CandidateList::const_iterator sourceIter = source.begin(); sourceIter != source.end(); ++sourceIter)
      {
        const Candidate &sourceCandidate = (*sourceIter);
        ZS_LOG_TRACE(String("Find comparing against source candidate") + sourceCandidate.toDebugString())

        if (finalCandidate.mType != sourceCandidate.mType) continue;
        if (!finalCandidate.mIPAddress.isAddressEqual(sourceCandidate.mIPAddress)) continue;
        if (finalCandidate.mPriority != sourceCandidate.mPriority) continue;
        if (finalCandidate.mLocalPreference != sourceCandidate.mLocalPreference) continue;
        if (finalCandidate.mUsernameFrag != sourceCandidate.mUsernameFrag) continue;
        if (finalCandidate.mPassword != sourceCandidate.mPassword) continue;
        if (finalCandidate.mProtocol != sourceCandidate.mProtocol) continue;
        ZS_LOG_TRACE(String("Find is using source candidate") + sourceCandidate.toDebugString())
        return sourceCandidate;
      }

      ZS_LOG_WARNING(Detail, String("Find is using FINAL candidate") + finalCandidate.toDebugString())
      return finalCandidate;
    }

    //-------------------------------------------------------------------------
    Dialog::DescriptionPtr IConversationThreadParser::Dialog::Description::create()
    {
      DescriptionPtr pThis(new Description);
      return pThis;
    }

    //-------------------------------------------------------------------------
    DialogPtr IConversationThreadParser::Dialog::create(
                                                        UINT version,
                                                        const char *inDialogID,
                                                        DialogStates state,
                                                        DialogClosedReasons closedReason,
                                                        const char *callerContactID,
                                                        const char *callerLocationID,
                                                        const char *calleeContactID,
                                                        const char *calleeLocationID,
                                                        const char *replacesDialogID,
                                                        const DescriptionList &descriptions,
                                                        IPeerFilesPtr signer
                                                        )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!signer)
      ZS_THROW_INVALID_ARGUMENT_IF(!inDialogID)

      String dialogID(inDialogID);
      ZS_THROW_INVALID_ARGUMENT_IF(dialogID.isEmpty())

      DialogPtr pThis(new Dialog());
      pThis->mThisWeak = pThis;

      pThis->mVersion = version;
      pThis->mDialogID = Stringize<CSTR>(dialogID);
      pThis->mState = state;
      pThis->mClosedReason = closedReason;
      pThis->mClosedReasonMessage = toString(closedReason);
      pThis->mCallerContactID = Stringize<CSTR>(callerContactID);
      pThis->mCallerLocationID = Stringize<CSTR>(callerLocationID);
      pThis->mCalleeContactID = Stringize<CSTR>(calleeContactID);
      pThis->mCalleeLocationID = Stringize<CSTR>(calleeLocationID);
      pThis->mReplacesDialogID = Stringize<CSTR>(replacesDialogID);
      pThis->mDescriptions = descriptions;

      ElementPtr dialogBundleEl = createElement("dialogBundle", ("bundle_" + dialogID).c_str());
      ElementPtr dialogEl = createElement("dialog", dialogID);
      if (0 != version) {
        dialogEl->setAttribute("version", Stringize<UINT>(version).string());
      }
      dialogBundleEl->adoptAsLastChild(dialogEl);

      ElementPtr stateEl = createElementWithText("state", toString(state));
      ElementPtr closedEl;
      if (DialogClosedReason_None != closedReason) {
        closedEl = createElementWithText("closed", Stringize<DialogClosedReasons>(closedReason).string(), pThis->mClosedReasonMessage);
      }
      ElementPtr fromContactIDEl = createElement("caller", callerContactID);
      ElementPtr fromLocationIDEl = createElement("callerLocation", callerLocationID);
      ElementPtr toContactIDEl = createElement("callee", calleeContactID);
      ElementPtr toLocationIDEl = createElement("calleeLocation", calleeLocationID);
      ElementPtr replacesEl;
      if (!(pThis->mReplacesDialogID.isEmpty())) {
        replacesEl = createElement("replaces", replacesDialogID);
      }

      ElementPtr descriptionsEl = createElement("descriptions");

      dialogEl->adoptAsLastChild(stateEl);
      if (closedEl) {
        dialogEl->adoptAsLastChild(closedEl);
      }
      dialogEl->adoptAsLastChild(fromContactIDEl);
      dialogEl->adoptAsLastChild(fromLocationIDEl);
      dialogEl->adoptAsLastChild(toContactIDEl);
      dialogEl->adoptAsLastChild(toLocationIDEl);
      if (replacesEl) {
        dialogEl->adoptAsLastChild(replacesEl);
      }

      dialogEl->adoptAsLastChild(descriptionsEl);

      for (DescriptionList::const_iterator descIter = descriptions.begin(); descIter != descriptions.end(); ++descIter)
      {
        const DescriptionPtr &description = (*descIter);

        ElementPtr descriptionEl = createElement("description", description->mDescriptionID);
        descriptionEl->setAttribute("type", description->mType);
        if (0 != description->mVersion) {
          descriptionEl->setAttribute("version", Stringize<UINT>(description->mVersion).string());
        }

        ElementPtr ssrcEl = createElementWithText("ssrc", Stringize<ULONG>(description->mSSRC).string());
        ElementPtr securityEl = createElement("security");
        securityEl->setAttribute("cipher", description->mSecurityCipher);
        ElementPtr secretEl = createElementWithText("secret", description->mSecuritySecret);
        ElementPtr saltEl = createElementWithText("salt", description->mSecuritySalt);

        ElementPtr codecsEl = createElement("codecs");

        for (CodecList::const_iterator codecIter = description->mCodecs.begin(); codecIter != description->mCodecs.end(); ++codecIter)
        {
          const Codec &codec = (*codecIter);

          ElementPtr codecEl = createElement("codec", Stringize<UINT>(codec.mCodecID).string());

          ElementPtr nameEl = createElementWithText("name", codec.mName);
          ElementPtr pTimeEl = createElementWithText("ptime", Stringize<UINT>(codec.mPTime).string());
          ElementPtr rateEl = createElementWithText("rate", Stringize<UINT>(codec.mRate).string());
          ElementPtr channelsEl = createElementWithText("channels", Stringize<UINT>(codec.mChannels).string());

          codecEl->adoptAsLastChild(nameEl);
          codecEl->adoptAsLastChild(pTimeEl);
          codecEl->adoptAsLastChild(rateEl);
          codecEl->adoptAsLastChild(channelsEl);

          codecsEl->adoptAsLastChild(codecEl);
        }

        CandidateList finalList;
        for (Description::CandidateLists::const_iterator listsIter = description->mCandidateLists.begin(); listsIter != description->mCandidateLists.end(); ++listsIter)
        {
          const CandidateList &sourceList = (*listsIter).second;
          mergeInto(finalList, sourceList);
        }

        ElementPtr candidatesEl = createElement("candidates");

        for (CandidateList::iterator finalIter = finalList.begin(); finalIter != finalList.end(); ++finalIter)
        {
          Candidate &finalCandidate = (*finalIter);

          ElementPtr candidateEl = createElement("candidate");

          ElementPtr ipEl = createElementWithText("ip", finalCandidate.mIPAddress.string(false));
          ipEl->setAttribute("format", finalCandidate.mIPAddress.isIPv4() ? "ipv4" : "ipv6");

          ElementPtr portsEl = createElement("ports");

          for (Description::CandidateLists::const_iterator listsIter = description->mCandidateLists.begin(); listsIter != description->mCandidateLists.end(); ++listsIter)
          {
            const CandidateList &sourceList = (*listsIter).second;
            const Candidate &found = find(finalCandidate, sourceList);

            ElementPtr portEl = createElementWithText("port", Stringize<WORD>(found.mIPAddress.getPort()).string());
            portsEl->adoptAsLastChild(portEl);
          }

          ElementPtr usernameEl = createElementWithText("username", finalCandidate.mUsernameFrag);
          ElementPtr passwordEl = createElementWithText("password", finalCandidate.mPassword);
          ElementPtr transportEl = createElementWithText("transport", "udp");
          ElementPtr protocolEl = createElementWithText("protocol", (finalCandidate.mProtocol.isEmpty() ? "rtp/avp" : ""));
          ElementPtr priorityEl = createElementWithText("priority", Stringize<DWORD>(finalCandidate.mPriority).string());

          candidateEl->adoptAsLastChild(ipEl);
          candidateEl->adoptAsLastChild(portsEl);
          candidateEl->adoptAsLastChild(usernameEl);
          candidateEl->adoptAsLastChild(passwordEl);
          candidateEl->adoptAsLastChild(transportEl);
          candidateEl->adoptAsLastChild(protocolEl);
          candidateEl->adoptAsLastChild(priorityEl);

          candidatesEl->adoptAsLastChild(candidateEl);
        }

        securityEl->adoptAsLastChild(saltEl);
        securityEl->adoptAsLastChild(secretEl);

        descriptionEl->adoptAsLastChild(ssrcEl);
        descriptionEl->adoptAsLastChild(securityEl);
        descriptionEl->adoptAsLastChild(codecsEl);
        descriptionEl->adoptAsLastChild(candidatesEl);

        descriptionsEl->adoptAsLastChild(descriptionEl);
      }

      IPeerFilePrivatePtr privatePeer = signer->getPrivate();
      ZS_THROW_INVALID_ARGUMENT_IF(!privatePeer)

      privatePeer->signElement(dialogEl);

      pThis->mDialogBundleEl = dialogBundleEl;

      return pThis;
    }

    //-------------------------------------------------------------------------
    DialogPtr IConversationThreadParser::Dialog::create(ElementPtr dialogBundleEl)
    {
      if (!dialogBundleEl) return DialogPtr();

      DialogPtr pThis(new Dialog());
      pThis->mThisWeak = pThis;
      pThis->mDialogBundleEl = dialogBundleEl;

      try {
        ElementPtr dialogEl = dialogBundleEl->findFirstChildElementChecked("dialog");
        pThis->mDialogID = dialogEl->getAttributeValue("id");
        pThis->mVersion = getVersion(dialogEl);

        ElementPtr stateEl = dialogEl->findFirstChildElementChecked("state");
        pThis->mState = toDialogStates(stateEl->getText());

        ElementPtr closedEl = dialogEl->findFirstChildElement("closed");
        if (closedEl) {
          try {
            WORD value = Numeric<WORD>(closedEl->getAttributeValue("id"));
            pThis->mClosedReason = (DialogClosedReasons)value;
          } catch(Numeric<WORD>::ValueOutOfRange &) {
            ZS_LOG_DEBUG("Illegal value for closed reason")
            return DialogPtr();
          }
          pThis->mClosedReasonMessage = closedEl->getTextAndEntityDecode();
        }

        ElementPtr callerContactIDEl = dialogEl->findFirstChildElementChecked("caller");
        pThis->mCallerContactID = callerContactIDEl->getAttributeValue("id");

        ElementPtr callerLocationIDEl = dialogEl->findFirstChildElementChecked("callerLocation");
        pThis->mCallerLocationID = callerLocationIDEl->getAttributeValue("id");

        ElementPtr calleeContactIDEl = dialogEl->findFirstChildElementChecked("callee");
        pThis->mCalleeContactID = calleeContactIDEl->getAttributeValue("id");

        ElementPtr calleeLocationIDEl = dialogEl->findFirstChildElementChecked("calleeLocation");
        pThis->mCalleeLocationID = calleeLocationIDEl->getAttributeValue("id");

        ElementPtr replacesEl = dialogEl->findFirstChildElement("replaces");
        if (replacesEl) {
          pThis->mReplacesDialogID = replacesEl->getAttributeValue("id");
        }

        ElementPtr descriptionsEl = dialogEl->findFirstChildElement("descriptions");
        ElementPtr descriptionEl = (descriptionsEl ? descriptionsEl->findFirstChildElement("description") : ElementPtr());

        while (descriptionEl)
        {
          DescriptionPtr description = Description::create();

          description->mDescriptionID = descriptionEl->getAttributeValue("id");
          description->mType = descriptionEl->getAttributeValue("type");
          description->mVersion = getVersion(descriptionEl);

          ElementPtr ssrcEl = descriptionEl->findFirstChildElementChecked("ssrc");
          description->mSSRC = Numeric<ULONG>(ssrcEl->getText());

          ElementPtr securityEl = descriptionEl->findFirstChildElementChecked("security");
          ElementPtr secretEl = securityEl->findFirstChildElementChecked("secret");
          ElementPtr saltEl = securityEl->findFirstChildElementChecked("salt");

          description->mSecurityCipher = securityEl->getAttributeValue("cipher");
          description->mSecuritySecret = secretEl->getText();
          description->mSecuritySalt = saltEl->getText();

          ElementPtr codecsEl = descriptionEl->findFirstChildElement("codecs");
          ElementPtr codecEl = (codecsEl ? codecsEl->findFirstChildElement("codec") : ElementPtr());

          while (codecEl)
          {
            Codec codec;
            codec.mCodecID = Numeric<UINT>(codecEl->getAttributeValue("id"));
            codec.mName = codecEl->findFirstChildElementChecked("name")->getText();
            codec.mPTime = Numeric<UINT>(codecEl->findFirstChildElementChecked("ptime")->getText());
            codec.mRate = Numeric<UINT>(codecEl->findFirstChildElementChecked("rate")->getText());
            codec.mChannels = Numeric<UINT>(codecEl->findFirstChildElementChecked("channels")->getText());
            description->mCodecs.push_back(codec);
          }

          ElementPtr candidatesEl = descriptionEl->findFirstChildElement("candidates");
          ElementPtr candidateEl = candidatesEl->findFirstChildElement("candidate");
          while (candidateEl)
          {
            Candidate candidate;
            candidate.mIPAddress = IPAddress(candidateEl->findFirstChildElementChecked("ip")->getText());
            candidate.mUsernameFrag = candidateEl->findFirstChildElementChecked("username")->getText();
            candidate.mPassword = candidateEl->findFirstChildElementChecked("password")->getText();
            candidate.mProtocol = candidateEl->findFirstChildElementChecked("protocol")->getText();
            candidate.mPriority = Numeric<DWORD>(candidateEl->findFirstChildElementChecked("priority")->getText());

            ElementPtr portsEl = candidateEl->findFirstChildElement("ports");
            ElementPtr portEl = (portsEl ? portsEl->findFirstChildElement("port") : candidateEl->findFirstChildElement("port"));
            for (UINT index = 0; portEl; ++index)
            {
              WORD port = Numeric<WORD>(portEl->getText());
              if (0 != port) {
                candidate.mIPAddress.setPort(port);
                CandidateLists::iterator found = description->mCandidateLists.find(index);
                if (found == description->mCandidateLists.end()) {
                  CandidateList candidates;
                  candidates.push_back(candidate);
                  description->mCandidateLists[index] = candidates;
                } else {
                  CandidateList &list = (*found).second;
                  list.push_back(candidate);
                }
              }
              portEl = portEl->findNextSiblingElement("port");
            }

            candidateEl = candidateEl->findNextSiblingElement("candidate");
          }

          pThis->mDescriptions.push_back(description);

          descriptionEl = descriptionEl->findNextSiblingElement("description");
        }
      } catch(CheckFailed &) {
        ZS_LOG_ERROR(Detail, "dialog XML element parse check failed")
        return DialogPtr();
      } catch (Numeric<WORD>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "dialog parse value out of range")
        return DialogPtr();
      } catch (Numeric<DWORD>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "dialog parse value out of range")
        return DialogPtr();
      } catch (Numeric<ULONG>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "dialog parse value out of range")
        return DialogPtr();
      } catch (Numeric<UINT>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "dialog parse value out of range")
        return DialogPtr();
      } catch (IPAddress::Exceptions::ParseError &) {
        ZS_LOG_ERROR(Detail, "dialog parse IP address parse error")
        return DialogPtr();
      }
      return pThis;
    }

    //-------------------------------------------------------------------------
    static String appendValue(const char *name, const String &value)
    {
      if (value.isEmpty()) return String();
      return ", " + String(name) + "=" + value;
    }

    //-------------------------------------------------------------------------
    String IConversationThreadParser::Dialog::getDebugValuesString() const
    {
      return appendValue("version", (0 != mVersion ? Stringize<UINT>(mVersion).string() : String())) +
             appendValue("dialog ID", mDialogID) +
             appendValue("state", toString(mState)) +
             appendValue("closed reason", toString(mClosedReason)) +
             appendValue("closed reason message", mClosedReasonMessage) +
             appendValue("caller contact ID", mCallerContactID) +
             appendValue("callee contact ID", mCalleeContactID) +
             appendValue("replaces", mReplacesDialogID);
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadParser::Details
    #pragma mark

    // <thread>
    //  ...
    //  <details version="1">
    //    <threadBase id="..." />
    //    <threadHost id="..." />
    //    <replaces id="..." />
    //    <state>open</state>
    //    <created></created>
    //  </details>
    //  ...
    // </thread>

    //-------------------------------------------------------------------------
    const char *IConversationThreadParser::Details::toString(ConversationThreadStates state)
    {
      switch (state) {
        case ConversationThreadState_None:    return "";
        case ConversationThreadState_Open:    return "open";
        case ConversationThreadState_Closed:  return "closed";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    ConversationThreadStates IConversationThreadParser::Details::toConversationThreadState(const char *state)
    {
      String compare(state ? state : "");
      for (int loop = (int)ConversationThreadState_First; loop <= ConversationThreadState_Last; ++loop)
      {
        const char *name = toString((ConversationThreadStates)loop);
        if (name == compare) return (ConversationThreadStates)loop;
      }
      return ConversationThreadState_Closed;
    }

    //-------------------------------------------------------------------------
    DetailsPtr IConversationThreadParser::Details::create(
                                                          UINT version,
                                                          const char *baseThreadID,
                                                          const char *hostThreadID,
                                                          const char *topic,
                                                          const char *replaces,
                                                          ConversationThreadStates state
                                                          )
    {
      DetailsPtr pThis(new Details());
      pThis->mThisWeak = pThis;
      pThis->mVersion = version;
      pThis->mBaseThreadID = Stringize<CSTR>(baseThreadID);
      pThis->mHostThreadID = Stringize<CSTR>(hostThreadID);
      pThis->mReplacesThreadID = Stringize<CSTR>(replaces);
      pThis->mState = state;
      pThis->mTopic = Stringize<CSTR>(topic);
      pThis->mCreated = zsLib::now();

      ElementPtr detailsEl = createElement("details");
      if (0 != version) {
        detailsEl->setAttribute("version", Stringize<UINT>(version).string());
      }

      ElementPtr threadBaseEl = createElement("threadBase", baseThreadID);
      ElementPtr threadHostEl = createElement("threadHost", hostThreadID);
      ElementPtr replacesEl = createElement("replaces", replaces);
      ElementPtr stateEl = createElementWithText("state", toString(state));
      ElementPtr topicEl = createElementWithTextAndEntityEncode("topic", pThis->mTopic);
      ElementPtr createdEl = createElementWithText("created", Stringize<Time>(pThis->mCreated).string());

      detailsEl->adoptAsLastChild(threadBaseEl);
      detailsEl->adoptAsLastChild(threadHostEl);
      detailsEl->adoptAsLastChild(replacesEl);
      detailsEl->adoptAsLastChild(stateEl);
      detailsEl->adoptAsLastChild(topicEl);
      detailsEl->adoptAsLastChild(createdEl);

      pThis->mDetailsEl = detailsEl;
      return pThis;
    }

    //-------------------------------------------------------------------------
    DetailsPtr IConversationThreadParser::Details::create(ElementPtr detailsEl)
    {
      if (!detailsEl) return DetailsPtr();

      DetailsPtr pThis(new Details());
      pThis->mThisWeak = pThis;
      pThis->mDetailsEl = detailsEl;

      try {
        pThis->mVersion = 0;
        AttributePtr versionAt = detailsEl->findAttribute("version");
        if (versionAt) {
          pThis->mVersion = Numeric<UINT>(detailsEl->getAttributeValue("version"));
        }
        pThis->mBaseThreadID = detailsEl->findFirstChildElementChecked("threadBase")->getAttributeValue("id");
        pThis->mHostThreadID = detailsEl->findFirstChildElementChecked("threadHost")->getAttributeValue("id");
        pThis->mReplacesThreadID = detailsEl->findFirstChildElementChecked("replaces")->getAttributeValue("id");
        String state = detailsEl->findFirstChildElementChecked("state")->getText();
        state.trim();
        pThis->mState = toConversationThreadState(state);
        pThis->mTopic = detailsEl->findFirstChildElementChecked("topic")->getTextAndEntityDecode();
        pThis->mCreated = Numeric<Time>(detailsEl->findFirstChildElementChecked("created")->getText());
      } catch(CheckFailed &) {
        ZS_LOG_ERROR(Detail, "details XML element parse check failed")
        return DetailsPtr();
      } catch (Numeric<UINT>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "details parse value out of range")
        return DetailsPtr();
      } catch (Numeric<Time>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, "details parse value out of range")
        return DetailsPtr();
      }
      return pThis;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IConversationThreadParser::Thread
    #pragma mark

    // <thread>
    //  <details version="1">
    //   ...
    //  </details>
    //  <contacts version="1">
    //   ...
    //  </contacts>
    //  <messages version="1">
    //   ...
    //  </messages>
    //  <receipts version="1">
    //   ...
    //  </receipts>
    //  <dialogs version="1">
    //   ...
    //  </dialogs>
    // </thread>

    //-------------------------------------------------------------------------
    const char *IConversationThreadParser::Thread::toString(ThreadTypes type)
    {
      switch (type) {
        case ThreadType_Host:   return "host";
        case ThreadType_Slave:  return "slave";
      }
      return "UNDEFINED";
    }

    //-------------------------------------------------------------------------
    IConversationThreadParser::Thread::ThreadTypes IConversationThreadParser::Thread::toThreadTypes(const char *inType) throw (InvalidArgument)
    {
      String type = Stringize<CSTR>(inType);
      if (type == "host") return ThreadType_Host;
      if (type == "slave") return ThreadType_Slave;
      return ThreadType_Host;
    }

    //-------------------------------------------------------------------------
    IConversationThreadParser::Thread::Thread() :
      mID(zsLib::createPUID()),
      mType(ThreadType_Host),
      mCanModify(false),
      mModifying(false),
      mMessagesVersion(0),
      mDialogsVersion(0),
      mDetailsChanged(false)
    {
      ZS_LOG_DEBUG(log("created"))
    }

    //-------------------------------------------------------------------------
    IConversationThreadParser::Thread::~Thread()
    {
      ZS_LOG_DEBUG(log("destroyed"))
    }

    //-------------------------------------------------------------------------
    ThreadPtr IConversationThreadParser::Thread::create(
                                                        IAccountForConversationThreadPtr account,
                                                        IPublicationPtr publication
                                                        )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!account)
      ZS_THROW_INVALID_ARGUMENT_IF(!publication)

      ThreadPtr pThis(new Thread);
      pThis->mThisWeak = pThis;
      pThis->mPublication = publication;

      SplitMap result;
      stack::IHelper::split(publication->getName(), result);
      String type = stack::IHelper::get(result, HOOKFLASH_CONVERSATION_THREAD_TYPE_INDEX);

      try {
        pThis->mType = toThreadTypes(type);
      } catch (InvalidArgument &) {
        ZS_LOG_ERROR(Detail, pThis->log("thread type is invalid") + ", type specified=" + type)
        return ThreadPtr();
      }

      AutoRecursiveLockPtr lock;
      DocumentPtr doc = publication->getXML(lock);
      if (!doc) {
        ZS_LOG_ERROR(Detail, pThis->log("thread was unable to get XML from publication"))
        return ThreadPtr();
      }

      try {
        ElementPtr threadEl = doc->findFirstChildElementChecked("thread");

        // parse the details...
        pThis->mDetails = Details::create(threadEl->findFirstChildElementChecked("details"));
        if (!pThis->mDetails) {
          ZS_LOG_ERROR(Detail, "Unable to load thread details from publication")
          return ThreadPtr();
        }
        pThis->mDetailsChanged = true;

        pThis->mContacts = Contacts::create(account, threadEl->findFirstChildElementChecked("contacts"));
        if (!pThis->mContacts) {
          ZS_LOG_ERROR(Detail, pThis->log("unable to load thread contacts from publication"))
          return ThreadPtr();
        }

        // every contact is to be treated as a change...
        pThis->mContactsChanged = pThis->mContacts->contacts();
        pThis->mContactsToAddChanged = pThis->mContacts->addContacts();
        pThis->mContactsToRemoveChanged = pThis->mContacts->removeContacts();

        ElementPtr messagesEl = threadEl->findFirstChildElementChecked("messages");
        pThis->mMessagesVersion = getVersion(messagesEl);

        ElementPtr messageBundleEl = messagesEl->findFirstChildElement("messageBundle");
        while (messageBundleEl) {
          MessagePtr message = Message::create(messageBundleEl);
          if (!message) {
            ZS_LOG_ERROR(Detail, pThis->log("failed to parse message from thread document"))
            return ThreadPtr();
          }
          pThis->mMessageMap[message->messageID()] = message;
          pThis->mMessageList.push_back(message);
          pThis->mMessagesChangedTime = zsLib::now();
          messageBundleEl = messageBundleEl->findNextSiblingElement("messageBundle");
        }

        // every message found is a changed message...
        pThis->mMessagesChanged = pThis->mMessageList;

        pThis->mMessageReceipts = MessageReceipts::create(threadEl->findFirstChildElementChecked("receipts"));
        if (!pThis->mMessageReceipts) {
          ZS_LOG_ERROR(Detail, pThis->log("unable to load receipts in this publication"))
          return ThreadPtr();
        }
        // every message receipt is a changed message receipt
        pThis->mMessageReceiptsChanged = pThis->mMessageReceipts->receipts();

        ElementPtr dialogsEl = threadEl->findFirstChildElementChecked("dialogs");
        pThis->mDialogsVersion = getVersion(dialogsEl);

        ElementPtr dialogBundleEl = dialogsEl->findFirstChildElement("dialogBundle");
        while (dialogBundleEl) {
          DialogPtr dialog = Dialog::create(dialogBundleEl);
          if (!dialog) {
            ZS_LOG_ERROR(Detail, pThis->log("failed to parse dialog from thread document"))
            return ThreadPtr();
          }

          // every dialog found is a dialog changed...
          pThis->mDialogsChanged[dialog->dialogID()] = dialog;
          pThis->mDialogs[dialog->dialogID()] = dialog;

          // every description found in a dialog is a description changed...
          for (DescriptionList::const_iterator iter = dialog->descriptions().begin(); iter != dialog->descriptions().end(); ++iter)
          {
            const DescriptionPtr &description = (*iter);
            pThis->mDescriptionsChanged[description->mDescriptionID] = ChangedDescription(dialog->dialogID(), description);
          }

          dialogBundleEl = dialogBundleEl->findNextSiblingElement("dialogBundle");
        }

      } catch (zsLib::XML::Exceptions::CheckFailed &) {
        ZS_LOG_ERROR(Detail, pThis->log("failed to parse document"))
        return ThreadPtr();
      } catch (Numeric<UINT>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, pThis->log("failed to parse document as value was out of range"))
        return ThreadPtr();
      }

      return pThis;
    }

    //-------------------------------------------------------------------------
    bool IConversationThreadParser::Thread::updateFrom(
                                                       IAccountForConversationThreadPtr account,
                                                       IPublicationPtr publication
                                                       )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!account)
      ZS_THROW_INVALID_ARGUMENT_IF(!publication)

      ZS_THROW_INVALID_ARGUMENT_IF(publication->getName() != mPublication->getName())
      ZS_THROW_INVALID_ARGUMENT_IF(publication->getSource() != mPublication->getSource())

      AutoRecursiveLockPtr lock;
      DocumentPtr doc = publication->getXML(lock);
      if (!doc) {
        ZS_LOG_ERROR(Detail, log("publication document was NULL"))
        return false;
      }

      // reset all "changed"
      resetChanged();

      DetailsPtr details;
      ContactsPtr contacts;
      UINT messagesVersion = 0;
      MessageList messages;
      MessageReceiptsPtr receipts;
      UINT dialogsVersion = 0;
      DialogMap dialogs;
      DialogMap dialogsChanged;

      try {
        ElementPtr threadEl = doc->findFirstChildElementChecked("thread");

        // parse the details...
        ElementPtr detailsEl = threadEl->findFirstChildElementChecked("details");
        UINT version = getVersion(detailsEl);

        if (version > mDetails->version()) {
          // parse the details
          details = Details::create(detailsEl);
          if (!details) return false;
        }

        ElementPtr contactsEl = threadEl->findFirstChildElementChecked("contacts");
        version = getVersion(contactsEl);
        if (version > mContacts->version()) {
          contacts = Contacts::create(account, contactsEl);
          if (!contacts) return false;
        }

        ElementPtr messagesEl = threadEl->findFirstChildElementChecked("messages");
        messagesVersion = getVersion(messagesEl);

        if (messagesVersion > mMessagesVersion) {
          ElementPtr messageBundleEl = messagesEl->findLastChildElement("messageBundle");
          ElementPtr firstValidBundleEl;
          while (messageBundleEl) {
            ElementPtr messageEl = messageBundleEl->findFirstChildElement("message");
            if (!messageEl) {
              ZS_LOG_ERROR(Detail, log("missing <message ...> element inside messageBundle (which is illegal)"))
              return false;
            }
            String id = messageEl->getAttributeValue("id");
            if (id.size() < 1) {
              ZS_LOG_ERROR(Detail, log("ID attribute is missing from <message> (which is illegal)"))
              return false;
            }

            MessageMap::iterator found = mMessageMap.find(id);
            if (found != mMessageMap.end()) break;

            firstValidBundleEl = messageBundleEl;

            messageBundleEl = messageBundleEl->findPreviousSiblingElement("messageBundle");
          }

          messageBundleEl = firstValidBundleEl;
          while (messageBundleEl) {
            MessagePtr message = Message::create(messageBundleEl);
            if (!message) {
              ZS_LOG_ERROR(Detail, log("unable to parse message bundle"))
              return ThreadPtr();
            }

            messages.push_back(message);

            messageBundleEl = messageBundleEl->findNextSiblingElement("messageBundle");
          }
        }

        ElementPtr receiptsEl = threadEl->findFirstChildElementChecked("receipts");
        version = getVersion(receiptsEl);
        if (version > mMessageReceipts->version()) {
          receipts = MessageReceipts::create(receiptsEl);
        }

        ElementPtr dialogsEl = threadEl->findFirstChildElementChecked("dialogs");
        dialogsVersion = getVersion(dialogsEl);

        if (dialogsVersion > mDialogsVersion) {
          ElementPtr dialogBundleEl = dialogsEl->findFirstChildElement("dialogBundle");
          while (dialogBundleEl) {
            ElementPtr dialogEl = dialogBundleEl->findFirstChildElement("dialog");
            String id = dialogEl->getAttributeValue("id");

            bool update = false;
            DialogMap::iterator found = mDialogs.find(id);
            if (found != mDialogs.end()) {
              version = getVersion(dialogEl);
              DialogPtr &dialog = (*found).second;
              if (version > dialog->version()) {
                ZS_LOG_TRACE(log("dialog change detected") + ", dialog ID=" + dialog->getDebugValuesString())
                update = true;
              } else {
                dialogs[id] = dialog; // using existing dialog
                ZS_LOG_TRACE(log("using existing dialog") + ", dialog ID=" + dialog->getDebugValuesString())
              }
            } else {
              update = true;
              ZS_LOG_TRACE(log("new dialog detected") + ", dialog ID=" + id)
            }

            if (update) {
              DialogPtr dialog = Dialog::create(dialogBundleEl);
              if (!dialog) return false;
              dialogs[id] = dialog;
              dialogsChanged[id] = dialog;
            }

            dialogBundleEl = dialogBundleEl->findNextSiblingElement("dialogBundle");
          }
        } else {
          ZS_LOG_TRACE(log("dialogs did not change"))
          dialogs = mDialogs;
        }
      } catch (zsLib::XML::Exceptions::CheckFailed &) {
        ZS_LOG_ERROR(Detail, log("failed to update document as parsing failed"))
        return ThreadPtr();
      } catch (Numeric<UINT>::ValueOutOfRange &) {
        ZS_LOG_ERROR(Detail, log("failed to update document as value out of range"))
        return ThreadPtr();
      }

      if (details) {
        mDetails = details;
        mDetailsChanged = true;
      }

      if (contacts) {
        typedef Contacts::ContactID ContactID;

        // see which contacts have changed...
        const ContactMap &newContacts = contacts->contacts();
        const ContactMap &newAddContacts = contacts->addContacts();
        const ContactIDList &newRemoveContacts = contacts->removeContacts();

        ContactMap newRemoveContactsMap;
        convert(newRemoveContacts, newRemoveContactsMap);

        const ContactMap &oldContacts = mContacts->contacts();
        const ContactMap &oldtAddContacts = mContacts->addContacts();
        const ContactIDList &oldRemoveContacts = mContacts->removeContacts();

        ContactMap oldRemoveContactsMap;
        convert(oldRemoveContacts, oldRemoveContactsMap);

        // figure out which contacts are new...
        for (ContactMap::const_iterator iter = newContacts.begin(); iter != newContacts.end(); ++iter)
        {
          const ContactID &id = (*iter).first;
          const ThreadContactPtr &contact = (*iter).second;
          ContactMap::const_iterator found = oldContacts.find(id);
          if (found == oldContacts.end()) {
            // didn't find in the old list so it must be 'new'
            mContactsChanged[id] = contact;
          }
        }

        // figure out which "add" contacts are new...
        for (ContactMap::const_iterator iter = newAddContacts.begin(); iter != newAddContacts.end(); ++iter)
        {
          const ContactID &id = (*iter).first;
          const ThreadContactPtr &contact = (*iter).second;
          ContactMap::const_iterator found = oldtAddContacts.find(id);
          if (found == oldtAddContacts.end()) {
            // didn't find in the old list so it must be 'new'
            mContactsToAddChanged[id] = contact;
          }
        }

        // figure out which "remove" contacts are new...
        for (ContactMap::const_iterator iter = newRemoveContactsMap.begin(); iter != newRemoveContactsMap.end(); ++iter)
        {
          const ContactID &id = (*iter).first;
          ContactMap::const_iterator found = oldRemoveContactsMap.find(id);
          if (found == oldRemoveContactsMap.end()) {
            // didn't find in the old list so it must be 'new' removal
            mContactsToRemoveChanged.push_back(id);
          }
        }

        // figure out which contacts have been removed...
        for (ContactMap::const_iterator iter = oldContacts.begin(); iter != oldContacts.end(); ++iter)
        {
          const ContactID &id = (*iter).first;
          ContactMap::const_iterator found = newContacts.find(id);
          if (found == newContacts.end()) {
            // didn't find in the old list so it must be 'new'
            mContactsRemoved.push_back(id);
          }
        }

        // figure out which "add" contacts have been removed...
        for (ContactMap::const_iterator iter = oldtAddContacts.begin(); iter != oldtAddContacts.end(); ++iter)
        {
          const ContactID &id = (*iter).first;
          ContactMap::const_iterator found = newAddContacts.find(id);
          if (found == newAddContacts.end()) {
            // didn't find in the old list so it must be 'new'
            mContactsToAddRemoved.push_back(id);
          }
        }

        // figure out which "remove" contacts have been removed...
        for (ContactMap::const_iterator iter = oldRemoveContactsMap.begin(); iter != oldRemoveContactsMap.end(); ++iter)
        {
          const ContactID &id = (*iter).first;
          ContactMap::const_iterator found = newRemoveContactsMap.find(id);
          if (found == newRemoveContactsMap.end()) {
            // didn't find in the old list so it must be 'new'
            mContactsToAddRemoved.push_back(id);
          }
        }

        mContacts = contacts;
      }

      if (messagesVersion > mMessagesVersion) {
        for (MessageList::iterator iter = messages.begin(); iter != messages.end(); ++iter) {
          const MessagePtr &message = (*iter);
          mMessageList.push_back(message);
          mMessageMap[message->messageID()] = message;
          mMessagesChanged.push_back(message);
        }

        if (messages.size() > 0) {
          mMessagesChangedTime = zsLib::now();
        }
      }

      if (receipts) {
        // figure out which message receipts have changed
        const MessageReceiptMap &oldReceipts = mMessageReceipts->receipts();
        const MessageReceiptMap &newReceipts = receipts->receipts();

        for (MessageReceiptMap::const_iterator iter = newReceipts.begin(); iter != newReceipts.end(); ++iter)
        {
          const MessageID &id = (*iter).first;
          const Time &time = (*iter).second;
          MessageReceiptMap::const_iterator found = oldReceipts.find(id);
          if (found == oldReceipts.end()) {
            // did not have this receipt last time so it's new...
            mMessageReceiptsChanged[id] = time;
          }
        }
      }

      if (dialogsVersion > mDialogsVersion) {
        ChangedDescriptionMap oldDescriptions;
        ChangedDescriptionMap newDescriptions;

        // build list of old descriptions...
        for (DialogMap::iterator iter = mDialogs.begin(); iter != mDialogs.end(); ++iter)
        {
          DialogPtr &dialog = (*iter).second;
          for (DescriptionList::const_iterator descIter = dialog->descriptions().begin(); descIter != dialog->descriptions().end(); ++descIter)
          {
            const DescriptionPtr &description = (*descIter);

            ZS_LOG_TRACE(log("found old description") + ", dialog ID=" + dialog->dialogID() + ", description ID=" + description->mDescriptionID)
            oldDescriptions[description->mDescriptionID] = ChangedDescription(dialog->dialogID(), description);
          }
        }

        // build a list of new descriptions...
        for (DialogMap::iterator iter = dialogs.begin(); iter != dialogs.end(); ++iter)
        {
          DialogPtr &dialog = (*iter).second;
          for (DescriptionList::const_iterator descIter = dialog->descriptions().begin(); descIter != dialog->descriptions().end(); ++descIter)
          {
            const DescriptionPtr &description = (*descIter);

            ZS_LOG_TRACE(log("found new description") + ", dialog ID=" + dialog->dialogID() + ", description ID=" + description->mDescriptionID)
            newDescriptions[description->mDescriptionID] = ChangedDescription(dialog->dialogID(), description);
          }
        }

        // figure out which descriptions are new...
        for (ChangedDescriptionMap::iterator iter = newDescriptions.begin(); iter != newDescriptions.end(); ++iter)
        {
          const DescriptionID &descriptionID = (*iter).first;
          DescriptionPtr &description = (*iter).second.second;
          DialogID &dialogID = (*iter).second.first;
          ChangedDescription &changed = (*iter).second;

          ChangedDescriptionMap::iterator found = oldDescriptions.find(descriptionID);
          if (found != oldDescriptions.end()) {
            DescriptionPtr &oldDescription = (*found).second.second;
            if (description->mVersion > oldDescription->mVersion) {
              // this description has changed
              mDescriptionsChanged[descriptionID] = changed;
              ZS_LOG_TRACE(log("description change detected") + ", dialog ID=" + dialogID + ", description ID=" + description->mDescriptionID)
            }
          } else {
            // this is a new description entirely
            ZS_LOG_TRACE(log("new description detected") + ", dialog ID=" + dialogID + ", description ID=" + description->mDescriptionID)
            mDescriptionsChanged[descriptionID] = changed;
          }
        }

        // figure out which descriptions are removed...
        for (ChangedDescriptionMap::iterator iter = oldDescriptions.begin(); iter != oldDescriptions.end(); ++iter)
        {
          const DescriptionID &descriptionID = (*iter).first;
          DialogID &dialogID = (*iter).second.first;

          ChangedDescriptionMap::iterator found = newDescriptions.find(descriptionID);
          if (found == newDescriptions.end()) {
            // this description has now been removed entirely...
            mDescriptionsRemoved.push_back(descriptionID);
            ZS_LOG_TRACE(log("description removal detected") + ", dialog ID=" + dialogID + ", description ID=" + descriptionID)
          }
        }

        // figure out which dialogs are now gone
        for (DialogMap::iterator dialogIter = mDialogs.begin(); dialogIter != mDialogs.end(); )
        {
          DialogMap::iterator current = dialogIter;
          ++dialogIter;

          const DialogID &id = (*current).first;
          DialogMap::iterator found = dialogs.find(id);
          if (found == dialogs.end()) {
            // this is dialog is completely gone...
            ZS_LOG_TRACE(log("dialog detected removed") + ", dialog ID=" + id)
            mDialogsRemoved.push_back(id);
          }
        }

        mDialogs = dialogs;
        mDialogsChanged = dialogsChanged;

        // mDialogs have now been updated with all the changed dialogs and the
        // removed dialogs are all remembered and then erased from mDialogs...

        // remember the new dialog version...
        mDialogsVersion = dialogsVersion;
      }

      return true;
    }

    //-------------------------------------------------------------------------
    ThreadPtr IConversationThreadParser::Thread::create(
                                                        ThreadTypes threadType,         // needed for document name
                                                        const char *creatorContactID,
                                                        const char *creatorLocationID,
                                                        const char *baseThreadID,
                                                        const char *hostThreadID,
                                                        const char *topic,
                                                        const char *replaces,
                                                        ConversationThreadStates state,
                                                        const char *peerHostContactID,
                                                        const char *peerHostLocationID
                                                        )
    {
      ZS_THROW_INVALID_ARGUMENT_IF(!baseThreadID)
      ZS_THROW_INVALID_ARGUMENT_IF(!hostThreadID)

      ThreadPtr pThis = ThreadPtr(new Thread);
      pThis->mThisWeak = pThis;
      pThis->mType = threadType;
      pThis->mCanModify = true;

      pThis->mDetails = Details::create(1, baseThreadID, hostThreadID, topic, replaces, state);
      if (!pThis->mDetails) return ThreadPtr();

      ContactList empty;
      ContactIDList emptyIDs;
      pThis->mContacts = Contacts::create(1, empty, empty, emptyIDs);

      pThis->mMessagesVersion = 1;
      pThis->mMessageReceipts = MessageReceipts::create(1);
      pThis->mDialogsVersion = 1;

      DocumentPtr doc = Document::create();

      ElementPtr threadEl = Element::create("thread");
      ElementPtr messagesEl = Element::create("messages");
      if (0 != pThis->mMessagesVersion) {
        messagesEl->setAttribute("version", Stringize<UINT>(pThis->mMessagesVersion));
      }
      ElementPtr dialogsEl = Element::create("dialogs");
      if (0 != pThis->mDialogsVersion) {
        dialogsEl->setAttribute("version", Stringize<UINT>(pThis->mDialogsVersion));
      }

      doc->adoptAsLastChild(threadEl);
      threadEl->adoptAsLastChild(pThis->mDetails->detailsElement()->clone());
      threadEl->adoptAsLastChild(pThis->mContacts->contactsElement()->clone());
      threadEl->adoptAsLastChild(messagesEl);
      threadEl->adoptAsLastChild(pThis->mMessageReceipts->receiptsElement()->clone());
      threadEl->adoptAsLastChild(dialogsEl);

      String baseName = String("/threads/1.0/") + toString(threadType) + "/" + baseThreadID + "/" + hostThreadID + "/";
      String name = baseName + "state";
      String permissionName = baseName + "permissions";

      typedef IPublication::PublishToRelationshipsMap PublishToRelationshipsMap;
      typedef IPublication::RelationshipList RelationshipList;

      PublishToRelationshipsMap publishRelationships;
      PublishToRelationshipsMap publishEmptyRelationships;
      RelationshipList relationships;

      if (ThreadType_Slave == threadType) {
        // when acting as a slave the only contact that has permission to read the slave document is the host peer
        ZS_THROW_INVALID_ARGUMENT_IF(!peerHostContactID)

        String hostContactID = Stringize<CSTR>(peerHostContactID).string();
        ZS_LOG_TRACE(pThis->log("slave thread") + ", using host ID=" + hostContactID + ", thread ID=" + baseThreadID + ", host thread ID=" + hostThreadID)

        relationships.push_back(hostContactID);
      }

      // scope: add "all" permissions
      {
        typedef IPublication::ContactIDList ContactIDList;
        typedef IPublication::PermissionAndContactIDListPair PermissionAndContactIDListPair;

        ContactIDList empty;
        publishRelationships[permissionName] = PermissionAndContactIDListPair(IPublication::Permission_All, empty);
      }

      pThis->mPublication = IPublication::createForLocal(creatorContactID, creatorLocationID, name, "text/x-xml-openpeer", doc, publishRelationships);
      pThis->mPermissionPublication = IPublication::createForLocal(creatorContactID, creatorLocationID, permissionName, "text/x-xml-openpeer-permissions", relationships, publishEmptyRelationships);

      return pThis;
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::updateBegin()
    {
      ZS_THROW_INVALID_USAGE_IF(!mCanModify)
      ZS_THROW_INVALID_USAGE_IF(mModifying)
      ZS_THROW_BAD_STATE_IF(mChangesDoc)

      mModifying = true;

      resetChanged();
      mChangesDoc.reset();
    }

    //-------------------------------------------------------------------------
    bool IConversationThreadParser::Thread::updateEnd()
    {
      ZS_THROW_INVALID_USAGE_IF(!mCanModify)
      ZS_THROW_INVALID_USAGE_IF(!mModifying)

      mModifying = false;

      // scope: figure out XML difference document (needs to be scoped because
      //        of publication lock returned from getXML(...) could cause
      //        unintended deadlock
      {
        AutoRecursiveLockPtr lock;
        DocumentPtr doc = mPublication->getXML(lock);
        ElementPtr threadEl = doc->findFirstChildElementChecked("thread");

        // have the details changed since last time?
        if (mDetailsChanged) {
          ElementPtr detailsEl = threadEl->findFirstChildElementChecked("details");
          IXMLDiff::createDiffs(IXMLDiff::DiffAction_Replace, mChangesDoc, detailsEl, false, mDetails->detailsElement()->clone());
        }

        // have any contacts changed...
        if ((mContactsChanged.size() > 0) ||
            (mContactsRemoved.size() > 0) ||
            (mContactsToAddChanged.size() > 0) ||
            (mContactsToAddRemoved.size() > 0) ||
            (mContactsToRemoveChanged.size() > 0) ||
            (mContactsToRemoveRemoved.size() > 0))
        {
          typedef Contacts::ContactID ContactID;

          // something has changed with the contacts, calculate all the differences...
          ContactMap contacts;
          ContactMap addContacts;
          ContactIDList removeContacts;
          ContactMap removeContactsMap;

          // start with the contacts we already have...
          contacts = mContacts->contacts();
          addContacts = mContacts->addContacts();
          const ContactIDList &oldRemoveContacts = mContacts->removeContacts();
          convert(oldRemoveContacts, removeContactsMap);

          // go through and apply the updates...
          for (ContactMap::iterator iter = mContactsChanged.begin(); iter != mContactsChanged.end(); ++iter)
          {
            const ContactID &id = (*iter).first;
            ThreadContactPtr &contact = (*iter).second;

            // apply the change now...
            contacts[id] = contact;
          }

          // remove all contacts that do not belong anymore...
          for (ContactIDList::iterator iter = mContactsRemoved.begin(); iter != mContactsRemoved.end(); ++iter)
          {
            const ContactID &id = (*iter);
            ContactMap::iterator found = contacts.find(id);
            if (found == contacts.end()) continue;
            contacts.erase(found);
          }

          // update the "add" contacts next
          for (ContactMap::iterator iter = mContactsToAddChanged.begin(); iter != mContactsToAddChanged.end(); ++iter)
          {
            const ContactID &id = (*iter).first;
            ThreadContactPtr &contact = (*iter).second;

            // apply the change now...
            addContacts[id] = contact;
          }

          // remove all contacts that do not belong anymore...
          for (ContactIDList::iterator iter = mContactsToAddRemoved.begin(); iter != mContactsToAddRemoved.end(); ++iter)
          {
            const ContactID &id = (*iter);
            ContactMap::iterator found = addContacts.find(id);
            if (found == addContacts.end()) continue;
            addContacts.erase(found);
          }

          // update the "remove" contacts next
          convert(mContactsToRemoveChanged, removeContactsMap);

          // remove all contacts that do not belong anymore...
          for (ContactIDList::iterator iter = mContactsToRemoveRemoved.begin(); iter != mContactsToRemoveRemoved.end(); ++iter)
          {
            const ContactID &id = (*iter);
            ContactMap::iterator found = removeContactsMap.find(id);
            if (found == removeContactsMap.end()) continue;
            removeContactsMap.erase(found);
          }

          // move back into the actual "remove" contact list
          convert(removeContactsMap, removeContacts);

          // create a replacement contacts object
          ContactList contactsAsList;
          ContactList addContactsAsList;
          convert(contacts, contactsAsList);
          convert(addContacts, addContactsAsList);

          // this is the replacement "contacts" object
          mContacts = Contacts::create(mContacts->version() + 1, contactsAsList, addContactsAsList, removeContacts);
          ZS_THROW_BAD_STATE_IF(!mContacts)

          ElementPtr contactsEl = threadEl->findFirstChildElementChecked("contacts");

          // this is the replacement XML for the contacts...
          IXMLDiff::createDiffs(IXMLDiff::DiffAction_Replace, mChangesDoc, contactsEl, false, mContacts->contactsElement()->clone());
        }

        if (mMessagesChanged.size() > 0) {
          ElementPtr messagesEl = threadEl->findFirstChildElementChecked("messages");

          ++mMessagesVersion;

          ElementPtr setEl = Element::create();
          setEl->setAttribute("version", Stringize<UINT>(mMessagesVersion).string());

          // put the corrected version on the messages element...
          IXMLDiff::createDiffsForAttributes(mChangesDoc, messagesEl, false, setEl);

          // add the messages to the messages element
          for (MessageList::iterator iter = mMessagesChanged.begin(); iter != mMessagesChanged.end(); ++iter)
          {
            MessagePtr &message = (*iter);
            IXMLDiff::createDiffs(IXMLDiff::DiffAction_AdoptAsLastChild, mChangesDoc, messagesEl, false, message->messageBundleElement()->clone());

            // remember these messages in the thread document...
            mMessageList.push_back(message);
            mMessageMap[message->messageID()] = message;
          }
        }

        if (mMessageReceiptsChanged.size() > 0) {
          ElementPtr receiptsEl = threadEl->findFirstChildElementChecked("receipts");

          mMessageReceipts = MessageReceipts::create(mMessageReceipts->version()+1, mMessageReceiptsChanged);

          IXMLDiff::createDiffs(IXMLDiff::DiffAction_Replace, mChangesDoc, receiptsEl, false, mMessageReceipts->receiptsElement()->clone());
        }

        if ((mDialogsChanged.size() > 0) ||
            (mDialogsRemoved.size() > 0)) {

          // some dialogs have changed or been removed
          ElementPtr dialogsEl = threadEl->findFirstChildElementChecked("dialogs");
          ++mDialogsVersion;

          ElementPtr setEl = Element::create();
          setEl->setAttribute("version", Stringize<UINT>(mDialogsVersion).string());

          IXMLDiff::createDiffsForAttributes(mChangesDoc, dialogsEl, false, setEl);

          for (DialogMap::iterator iter = mDialogsChanged.begin(); iter != mDialogsChanged.end(); ++iter)
          {
            const DialogID &id = (*iter).first;
            DialogPtr &dialog = (*iter).second;

            // remember this dialog in the thread
            mDialogs[id] = dialog;

            bool found = false;

            // dialog is now changing...
            ElementPtr dialogBundleEl = dialogsEl->findFirstChildElement("dialogBundle");
            while (dialogBundleEl) {
              ElementPtr dialogEl = dialogBundleEl->findFirstChildElement("dialog");
              ElementPtr signatureEl = dialogBundleEl->findFirstChildElement("Signature");
              if (dialogEl->getAttributeValue("id") == id) {
                // found the element to "replace"... so create a diff...
                IXMLDiff::createDiffs(IXMLDiff::DiffAction_Replace, mChangesDoc, dialogBundleEl, false, dialog->dialogBundleElement()->clone());
                found = true;
                break;
              }
              dialogBundleEl = dialogBundleEl->findNextSiblingElement("dialogBundle");
            }

            if (!found) {
              // this dialog needs to be added isntead
              IXMLDiff::createDiffs(IXMLDiff::DiffAction_AdoptAsLastChild, mChangesDoc, dialogsEl, false, dialog->dialogBundleElement()->clone());
            }
          }

          for (DialogIDList::iterator iter = mDialogsRemoved.begin(); iter != mDialogsRemoved.end(); ++iter)
          {
            const DialogID &id = (*iter);

            DialogMap::iterator found = mDialogs.find(id);
            if (found != mDialogs.end()) {
              ZS_LOG_DEBUG(log("removing dialog from dialog map") + ", dialog ID=" + id)
              mDialogs.erase(found);
            }

            // dialog is now changing...
            ElementPtr dialogBundleEl = dialogsEl->findFirstChildElement("dialogBundle");
            while (dialogBundleEl) {
              ElementPtr dialogEl = dialogBundleEl->findFirstChildElement("dialog");
              if (dialogEl->getAttributeValue("id") == id) {
                // found the element to "replace"... so create a diff...
                IXMLDiff::createDiffs(IXMLDiff::DiffAction_Remove, mChangesDoc, dialogBundleEl, false);
                break;
              }
              dialogBundleEl = dialogBundleEl->findNextSiblingElement("dialogBundle");
            }
          }
        }

        if (!mChangesDoc) return false;  // nothing to do
      }

      // the changes need to be adopted/processed by the document
      mPublication->update(mChangesDoc);
      mChangesDoc.reset();

      // check if the permissions document needs updating too...
      if (ThreadType_Host == mType) {
        if ((mContactsChanged.size() > 0) ||
            (mContactsRemoved.size() > 0)) {

          // Only need to update this document when acting as a host and
          // the contacts that are allowed to read this document are all the
          // slave contacts.

          typedef IPublication::PublishToRelationshipsMap PublishToRelationshipsMap;
          typedef IPublication::RelationshipList RelationshipList;
          typedef Contacts::ContactID ContactID;

          RelationshipList relationships;
          PublishToRelationshipsMap publishEmptyRelationships;

          // add every contact that is part of this thread to the permission document list
          const ContactMap &contacts = mContacts->contacts();
          for (ContactMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
          {
            const ContactID &id = (*iter).first;
            relationships.push_back(id);
          }

          if (mPermissionPublication) {
            mPermissionPublication->update(relationships);
          } else {
            mPermissionPublication = IPublication::createForLocal(mPermissionPublication->getCreatorContactID(), mPermissionPublication->getCreatorLocationID(), mPermissionPublication->getName(), "text/x-xml-openpeer-permissions", relationships, publishEmptyRelationships);
          }
        }
      }

      resetChanged();
      return true;
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::setState(Details::ConversationThreadStates state)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      if (state == mDetails->state()) return; // nothing changed

      mDetailsChanged = true;
      mDetails = Details::create(mDetails->version()+1, mDetails->baseThreadID(), mDetails->hostThreadID(), mDetails->topic(), mDetails->replacesThreadID(), state);
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::setContacts(const ContactMap &contacts)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      typedef Contacts::ContactID ContactID;

      const ContactMap &oldContacts = mContacts->contacts();

      // clear current list of changes since a reset it happening on the changes
      mContactsChanged.clear();
      mContactsRemoved.clear();

      // figure out which contacts are new...
      for (ContactMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
      {
        const ContactID &id = (*iter).first;
        const ThreadContactPtr &contact = (*iter).second;

        // first check if it's already on the list...
        ContactMap::const_iterator found = oldContacts.find(id);
        if (found != oldContacts.end()) continue;       // already have this contact on the old list

        mContactsChanged[id] = contact;
      }

      // figure out which contacts need to be removed...
      for (ContactMap::const_iterator iter = oldContacts.begin(); iter != oldContacts.end(); ++iter)
      {
        const ContactID &id = (*iter).first;
        ContactMap::const_iterator found = contacts.find(id);
        if (found == contacts.end()) {
          // this contact is now gone and it must be removed...
          mContactsRemoved.push_back(id);
        }
      }
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::setContactsToAdd(const ContactMap &contacts)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      typedef Contacts::ContactID ContactID;

      const ContactMap &oldContactsToAdd = mContacts->addContacts();

      // clear current list of changes since a reset it happening on the changes
      mContactsToAddChanged.clear();
      mContactsToAddRemoved.clear();

      // figure out which contacts are new...
      for (ContactMap::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
      {
        const ContactID &id = (*iter).first;
        const ThreadContactPtr &contact = (*iter).second;

        // first check if it's already on the list...
        ContactMap::const_iterator found = oldContactsToAdd.find(id);
        if (found != oldContactsToAdd.end()) continue;       // already have this contact on the old list

        mContactsToAddChanged[id] = contact;
      }

      // figure out which contacts need to be removed...
      for (ContactMap::const_iterator iter = oldContactsToAdd.begin(); iter != oldContactsToAdd.end(); ++iter)
      {
        const ContactID &id = (*iter).first;
        ContactMap::const_iterator found = contacts.find(id);
        if (found == contacts.end()) {
          // this contact is now gone and it must be removed...
          mContactsToAddRemoved.push_back(id);
        }
      }
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::setContactsToRemove(const ContactIDList &contacts)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      typedef Contacts::ContactID ContactID;

      const ContactIDList &oldContactsToRemoveList = mContacts->removeContacts();
      ContactMap oldContactsToRemove;

      // create old contacts into a map instead
      for (ContactIDList::const_iterator iter = oldContactsToRemoveList.begin(); iter != oldContactsToRemoveList.end(); ++iter)
      {
        const ContactID &id = (*iter);
        oldContactsToRemove[id] = ThreadContactPtr();
      }

      // clear current list of changes since a reset it happening on the changes
      mContactsToRemoveChanged.clear();
      mContactsToRemoveRemoved.clear();

      // figure out which contacts are new...
      for (ContactIDList::const_iterator iter = contacts.begin(); iter != contacts.end(); ++iter)
      {
        const ContactID &id = (*iter);

        // first check if it's already on the list...
        ContactMap::const_iterator found = oldContactsToRemove.find(id);
        if (found != oldContactsToRemove.end()) continue;       // already have this contact on the old list

        mContactsToRemoveChanged.push_back(id);
      }

      ContactMap tempContacts;
      convert(contacts, tempContacts);

      // figure out which contacts need to be removed...
      for (ContactMap::const_iterator iter = oldContactsToRemove.begin(); iter != oldContactsToRemove.end(); ++iter)
      {
        const ContactID &id = (*iter).first;
        ContactMap::const_iterator found = tempContacts.find(id);
        if (found == tempContacts.end()) {
          // this contact is now gone and it must be removed...
          mContactsToRemoveRemoved.push_back(id);
        }
      }
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::addMessage(MessagePtr message)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      MessageList list;
      list.push_back(message);
      addMessages(list);
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::addMessages(const MessageList &messages)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      for (MessageList::const_iterator iter = messages.begin(); iter != messages.end(); ++iter)
      {
        const MessagePtr &message = (*iter);
        MessageMap::iterator found = mMessageMap.find(message->messageID());
        if (found != mMessageMap.end()) continue;   // this message is already part of the message list

        // this is a new message to be added to the list...
        mMessagesChanged.push_back(message);
        mMessagesChangedTime = zsLib::now();
      }
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::setReceived(MessagePtr message)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      mMessageReceiptsChanged.clear();

      mMessageReceiptsChanged[message->messageID()] = zsLib::now();
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::setReceived(const MessageReceiptMap &messages)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      if (mMessageReceiptsChanged.size() > 0) {
        // since there is already changes acknowledged just update the changes now...
        mMessageReceiptsChanged = messages;
        return;
      }

      bool changed = false;

      // check if anything is added...
      for (MessageReceiptMap::const_iterator iter = messages.begin(); iter != messages.end(); ++iter)
      {
        const MessageID &id = (*iter).first;
        const Time &time = (*iter).second;
        MessageReceiptMap::const_iterator found = mMessageReceipts->receipts().find(id);
        if (found == mMessageReceipts->receipts().end()) {
          changed = true;
          break;
        }
        const Time &oldTime = (*found).second;
        if (time != oldTime) {
          changed = true;
          break;
        }
      }

      if (!changed) {
        // check if anything is removed...
        for (MessageReceiptMap::const_iterator iter = mMessageReceipts->receipts().begin(); iter != mMessageReceipts->receipts().end(); ++iter)
        {
          const MessageID &id = (*iter).first;
          const Time &time = (*iter).second;
          MessageReceiptMap::const_iterator found = messages.find(id);
          if (found == messages.end()) {
            changed = true;
            break;
          }
          const Time &oldTime = (*found).second;
          if (time != oldTime) {
            changed = true;
            break;
          }
        }
      }

      if (!changed) {
        ZS_LOG_TRACE(log("no message receipts changed detected from previous received map (thus ignoring request to set received)"))
        // nothing changed thus do not cause an update
        return;
      }

      mMessageReceiptsChanged = messages;
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::addDialogs(const DialogList &dialogs)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      for (DialogList::const_iterator iter = dialogs.begin(); iter != dialogs.end(); ++iter)
      {
        const DialogPtr &dialog = (*iter);
        mDialogsChanged[dialog->dialogID()] = dialog;
      }
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::updateDialogs(const DialogList &dialogs)
    {
      // same logic...
      addDialogs(dialogs);
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::removeDialogs(const DialogIDList &dialogs)
    {
      ZS_THROW_INVALID_USAGE_IF((!mCanModify) || (!mModifying))

      for (DialogIDList::const_iterator iter = dialogs.begin(); iter != dialogs.end(); ++iter)
      {
        const String &dialogID = (*iter);
        mDialogsRemoved.push_back(dialogID);
      }
    }

    //-------------------------------------------------------------------------
    void IConversationThreadParser::Thread::resetChanged()
    {
      mDetailsChanged = false;
      mContactsChanged.clear();
      mContactsRemoved.clear();
      mContactsToAddChanged.clear();
      mContactsToAddRemoved.clear();
      mContactsToRemoveChanged.clear();
      mContactsToRemoveRemoved.clear();
      mMessagesChanged.clear();
      mMessageReceiptsChanged.clear();
      mDialogsChanged.clear();
      mDialogsRemoved.clear();
      mDescriptionsChanged.clear();
      mDescriptionsRemoved.clear();
    }

    //-------------------------------------------------------------------------
    String IConversationThreadParser::Thread::log(const char *message) const
    {
      return String("IConversationThreadParser::Thread [") + Stringize<PUID>(mID).string() + "] " + message;
    }

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
}
