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

#include <hookflash/provisioning/message/internal/provisioning_message_MultiPartyAPNSPushRequest.h>
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

      MultiPartyAPNSPushRequestPtr MultiPartyAPNSPushRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<MultiPartyAPNSPushRequest>(message);
      }

      MultiPartyAPNSPushRequest::MultiPartyAPNSPushRequest()
      {
      }

      bool MultiPartyAPNSPushRequest::hasAttribute(AttributeTypes type) const
      {
        switch ((MultiPartyAPNSPushRequest::AttributeTypes)type)
        {
          case AttributeType_UserID:                      return !mUserID.isEmpty();
          case AttributeType_AccessKey:                   return !mAccessKey.isEmpty();
          case AttributeType_AccessSecret:                return !mAccessSecret.isEmpty();

          case AttributeType_LocationID:                  return !mLocationID.isEmpty();

          case AttributeType_MessageType:                 return !mMessageType.isEmpty();
          case AttributeType_MessageData:                 return !mMessageData.isEmpty();

          case AttributeType_UserIDs:                     return (mUserIDs.size() > 0);
        }
        return false;
      }

      DocumentPtr MultiPartyAPNSPushRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::MultiPartyAPNSPushRequest::encode(*this);
      }

      MultiPartyAPNSPushRequestPtr MultiPartyAPNSPushRequest::create()
      {
        return internal::MultiPartyAPNSPushRequest::create();
      }

      namespace internal
      {
        MultiPartyAPNSPushRequestPtr MultiPartyAPNSPushRequest::create()
        {
          MultiPartyAPNSPushRequestPtr ret(new message::MultiPartyAPNSPushRequest);

          return ret;
        }

        DocumentPtr MultiPartyAPNSPushRequest::encode(message::MultiPartyAPNSPushRequest &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          if (msg.hasAttribute(message::MultiPartyAPNSPushRequest::AttributeType_UserID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("userID", msg.mUserID));
          }

          if (msg.hasAttribute(message::MultiPartyAPNSPushRequest::AttributeType_AccessKey)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accessKey", msg.mAccessKey));
          }

          if (msg.hasAttribute(message::MultiPartyAPNSPushRequest::AttributeType_AccessSecret)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accessSecret", msg.mAccessSecret));
          }

          if (msg.hasAttribute(message::MultiPartyAPNSPushRequest::AttributeType_LocationID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("locationID", msg.mLocationID));
          }

          if (msg.hasAttribute(message::MultiPartyAPNSPushRequest::AttributeType_MessageType)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("messageType", msg.mMessageType));
          }

          if (msg.hasAttribute(message::MultiPartyAPNSPushRequest::AttributeType_MessageData)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("messageData", msg.mMessageData));
          }

          if (msg.hasAttribute(message::MultiPartyAPNSPushRequest::AttributeType_UserIDs)) {
            String usersStr;

            typedef provisioning::IAccount::UserIDList UserIDList;
            for (UserIDList::iterator iter = msg.mUserIDs.begin(); iter != msg.mUserIDs.end(); ++iter)
            {
              const String &userID = (*iter);

              if (!userID.isEmpty()) {
                if (!usersStr.isEmpty()) {
                  usersStr += "," + userID;
                } else {
                  usersStr += userID;
                }
              }
            }

            if (!usersStr.isEmpty()) {
              root->adoptAsLastChild(IMessageHelper::createElementWithText("users", usersStr));
            }
          }

          return ret;
        }

      }
    }
  }
}
