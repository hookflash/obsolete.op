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

#include <hookflash/provisioning/message/internal/provisioning_message_LookupProfileRequest.h>
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

      LookupProfileRequestPtr LookupProfileRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<LookupProfileRequest>(message);
      }

      LookupProfileRequest::LookupProfileRequest()
      {
      }

      bool LookupProfileRequest::hasAttribute(AttributeTypes type) const
      {
        switch ((LookupProfileRequest::AttributeTypes)type)
        {
          case AttributeType_Identities:    return (mIdentities.size() > 0);
        }
        return false;
      }

      DocumentPtr LookupProfileRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::LookupProfileRequest::encode(*this);
      }

      LookupProfileRequestPtr LookupProfileRequest::create()
      {
        return internal::LookupProfileRequest::create();
      }

      namespace internal
      {
        LookupProfileRequestPtr LookupProfileRequest::create()
        {
          LookupProfileRequestPtr ret(new message::LookupProfileRequest);

          return ret;
        }

        DocumentPtr LookupProfileRequest::encode(message::LookupProfileRequest &msg)
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(msg);
          ElementPtr root = ret->getFirstChildElement();

          ElementPtr identitiesEl = IMessageHelper::createElement("identities");

          typedef message::LookupProfileRequest::IdentityIDList IdentityList;
          typedef provisioning::IAccount::IdentityID IdentityID;

          for (int loop = provisioning::IAccount::IdentityType_None; loop <= provisioning::IAccount::IdentityType_Last; ++loop)
          {
            String str;
            for (IdentityList::iterator iter = msg.mIdentities.begin(); iter != msg.mIdentities.end(); ++iter)
            {
              const IdentityID &identityID = (*iter);
              if (identityID.first == (provisioning::IAccount::IdentityTypes)loop) {
                if (!str.isEmpty()) {
                  str += "," + identityID.second;
                } else {
                  str += identityID.second;
                }
              }
            }

            if (!str.isEmpty()) {
              ElementPtr identityEl = IMessageHelper::createElement("identity");
              identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("type", provisioning::IAccount::toCodeString((provisioning::IAccount::IdentityTypes)loop)));
              identityEl->adoptAsLastChild(IMessageHelper::createElementWithText("uniqueIDs", str));

              identitiesEl->adoptAsLastChild(identityEl);
            }
          }

          if (identitiesEl->hasChildren()) {
            root->adoptAsLastChild(identitiesEl);
          }

          return ret;
        }

      }
    }
  }
}
