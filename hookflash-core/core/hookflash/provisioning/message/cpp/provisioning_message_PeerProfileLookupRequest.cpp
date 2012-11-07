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

#include <hookflash/provisioning/message/internal/provisioning_message_PeerProfileLookupRequest.h>
#include <hookflash/stack/message/IMessageHelper.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      typedef zsLib::String String;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef stack::message::IMessageHelper IMessageHelper;

      PeerProfileLookupRequestPtr PeerProfileLookupRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerProfileLookupRequest>(message);
      }

      PeerProfileLookupRequest::PeerProfileLookupRequest()
      {
      }

      bool PeerProfileLookupRequest::hasAttribute(AttributeTypes type) const
      {
        switch ((PeerProfileLookupRequest::AttributeTypes)type)
        {
          case AttributeType_UserIDs:                     return mUserIDs.size() > 0;
          case AttributeType_ContactIDs:                  return mContactIDs.size() > 0;
        }
        return false;
      }

      DocumentPtr PeerProfileLookupRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerProfileLookupRequest::encode(*this);
      }

      PeerProfileLookupRequestPtr PeerProfileLookupRequest::create()
      {
        return internal::PeerProfileLookupRequest::create();
      }

      namespace internal
      {
        PeerProfileLookupRequestPtr PeerProfileLookupRequest::create()
        {
          PeerProfileLookupRequestPtr ret(new message::PeerProfileLookupRequest);

          return ret;
        }

        DocumentPtr PeerProfileLookupRequest::encode(message::PeerProfileLookupRequest &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          ElementPtr usersEl = IMessageHelper::createElement("users");

          typedef message::PeerProfileLookupRequest::UserIDList UserIDList;
          typedef message::PeerProfileLookupRequest::ContactIDList ContactIDList;

          UserIDList::iterator iter1 = msg.mUserIDs.begin();
          ContactIDList::iterator iter2 = msg.mContactIDs.begin();
          for (; (iter1 != msg.mUserIDs.end()) && (iter2 != msg.mContactIDs.end()); ++iter1, ++iter2)
          {
            const String &userID = (*iter1);
            const String &contactID = (*iter2);

            if ((!userID.isEmpty()) &&
                (!contactID.isEmpty())) {
              ElementPtr userEl = IMessageHelper::createElement("user");

              userEl->adoptAsLastChild(IMessageHelper::createElementWithText("userID", userID));
              userEl->adoptAsLastChild(IMessageHelper::createElementWithText("contactID", contactID));

              usersEl->adoptAsLastChild(userEl);
            }
          }

          if (usersEl->hasChildren()) {
            root->adoptAsLastChild(usersEl);
          }

          return ret;
        }

      }
    }
  }
}
