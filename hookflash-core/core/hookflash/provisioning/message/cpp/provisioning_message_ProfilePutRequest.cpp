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

#include <hookflash/provisioning/message/internal/provisioning_message_ProfilePutRequest.h>
#include <hookflash/stack/message/IMessageHelper.h>

#include <hookflash/stack/IHelper.h>

#include <zsLib/Stringize.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      using zsLib::Stringize;

      typedef zsLib::WORD WORD;
      typedef zsLib::XML::DocumentPtr DocumentPtr;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef stack::message::IMessageHelper IMessageHelper;
      typedef provisioning::IAccount::IdentityInfo IdentityInfo;
      typedef provisioning::IAccount::IdentityInfoList IdentityInfoList;

      ProfilePutRequestPtr ProfilePutRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<ProfilePutRequest>(message);
      }

      ProfilePutRequest::ProfilePutRequest()
      {
      }

      bool ProfilePutRequest::hasAttribute(AttributeTypes type) const
      {
        switch ((ProfilePutRequest::AttributeTypes)type)
        {
          case AttributeType_UserID:                      return !mUserID.isEmpty();
          case AttributeType_AccessKey:                   return !mAccessKey.isEmpty();
          case AttributeType_AccessSecret:                return !mAccessSecret.isEmpty();

          case AttributeType_LastProfileUpdateTimestamp:  return Time() != mLastProfileUpdateTimestamp;

          case AttributeType_PasswordNonce:               return !mPasswordNonce.isEmpty();
          case AttributeType_PasswordProofHash:           return !mPasswordProofHash.isEmpty();
          case AttributeType_ContactID:                   return !mContactID.isEmpty();
          case AttributeType_PrivatePeerFile:             return !mPrivatePeerFile.isEmpty();
          case AttributeType_PublicPeerFile:              return !mPublicPeerFile.isEmpty();

          case AttributeType_Name:                        return !mName.isEmpty();

          case AttributeType_Profiles:                    return (mProfiles.size() > 0);
        }
        return false;
      }

      DocumentPtr ProfilePutRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::ProfilePutRequest::encode(*this);
      }

      ProfilePutRequestPtr ProfilePutRequest::create()
      {
        return internal::ProfilePutRequest::create();
      }

      namespace internal
      {
        ProfilePutRequestPtr ProfilePutRequest::create()
        {
          ProfilePutRequestPtr ret(new message::ProfilePutRequest);

          return ret;
        }

        DocumentPtr ProfilePutRequest::encode(message::ProfilePutRequest &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_UserID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("userID", msg.mUserID));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_AccessKey)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accessKey", msg.mAccessKey));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_AccessSecret)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("accessSecret", msg.mAccessSecret));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_LastProfileUpdateTimestamp)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("lastProfileUpdateTimestamp", IMessageHelper::timeToString(msg.mLastProfileUpdateTimestamp)));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_PasswordNonce)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("passwordNonce", msg.mPasswordNonce));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_PasswordProofHash)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("passwordProofHash", msg.mPasswordProofHash));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_ContactID)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("contactID", msg.mContactID));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_PrivatePeerFile)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("privatePeerFile", hookflash::stack::IHelper::convertToBase64(msg.mPrivatePeerFile)));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_PublicPeerFile)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("publicPeerFile", hookflash::stack::IHelper::convertToBase64(msg.mPublicPeerFile)));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_Name)) {
            root->adoptAsLastChild(IMessageHelper::createElementWithText("name", msg.mName));
          }

          if (msg.hasAttribute(message::ProfilePutRequest::AttributeType_Profiles)) {
            ElementPtr profilesEl = IMessageHelper::createElement("profiles");

            for (IdentityInfoList::const_iterator iter = msg.mProfiles.begin(); iter != msg.mProfiles.end(); ++iter)
            {
              const IdentityInfo &info = (*iter);

              if (info.hasData()) {
                ElementPtr profileEl = IMessageHelper::createElement("profile");

                if (provisioning::IAccount::IdentityType_None != info.mType) {
                  profileEl->adoptAsLastChild(IMessageHelper::createElementWithText("identityType", provisioning::IAccount::toCodeString(info.mType)));
                }
                if (!info.mUniqueID.isEmpty()) {
                  profileEl->adoptAsLastChild(IMessageHelper::createElementWithText("identityUniqueID", info.mUniqueID));
                }
                if (!info.mUniqueIDProof.isEmpty()) {
                  profileEl->adoptAsLastChild(IMessageHelper::createElementWithText("identityUniqueIDProof", info.mUniqueIDProof));
                }
                if (provisioning::IAccount::IdentityValidationState_None != info.mValidationState) {
                  profileEl->adoptAsLastChild(IMessageHelper::createElementWithText("validationState", provisioning::IAccount::toString(info.mValidationState)));
                }
                if (!info.mValidationID.isEmpty()) {
                  profileEl->adoptAsLastChild(IMessageHelper::createElementWithText("validationID", info.mValidationID));
                }

                if (profileEl->hasChildren()) {
                  profileEl->adoptAsLastChild(IMessageHelper::createElementWithText("priority", Stringize<WORD>(info.mPriority)));
                  profileEl->adoptAsLastChild(IMessageHelper::createElementWithText("weight", Stringize<WORD>(info.mWeight)));
                }

                if (profileEl->hasChildren()) {
                  profilesEl->adoptAsLastChild(profileEl);
                }
              }
            }

            if (profilesEl->hasChildren()) {
              root->adoptAsLastChild(profilesEl);
            }
          }

          return ret;
        }

      }
    }
  }
}
