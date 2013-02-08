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

#include <hookflash/provisioning/message/internal/provisioning_message_PeerProfileLookupResult.h>
#include <hookflash/stack/message/IMessageHelper.h>

#include <hookflash/stack/IHelper.h>

namespace hookflash
{
  namespace provisioning
  {
    namespace message
    {
      typedef zsLib::String String;
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef stack::message::IMessageHelper IMessageHelper;

      PeerProfileLookupResultPtr PeerProfileLookupResult::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerProfileLookupResult>(message);
      }

      PeerProfileLookupResult::PeerProfileLookupResult()
      {
      }

      bool PeerProfileLookupResult::hasAttribute(PeerProfileLookupResult::AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_UserIDs:           return (mUserIDs.size() > 0);
          case AttributeType_PublicPeerFiles:   return (mPublicPeerFiles.size() > 0);
          default:                              break;
        }
        return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
      }

      namespace internal
      {
        PeerProfileLookupResultPtr PeerProfileLookupResult::create(ElementPtr root)
        {
          PeerProfileLookupResultPtr ret(new message::PeerProfileLookupResult);

          ret->mID = IMessageHelper::getAttributeID(root);
          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          ElementPtr usersEl = root->findFirstChildElement("users");

          if (usersEl) {
            ElementPtr userEl = usersEl->findFirstChildElement("user");
            while (userEl)
            {
              String userID;
              String publicPeerFile;

              userID = IMessageHelper::getChildElementText(userEl, "userID");
              publicPeerFile = hookflash::stack::IHelper::convertFromBase64(IMessageHelper::getChildElementText(userEl, "publicPeerFile"));

              if (!userID.isEmpty()) {
                ret->mUserIDs.push_back(userID);
                ret->mPublicPeerFiles.push_back(publicPeerFile);
              }

              userEl = userEl->findNextSiblingElement("user");
            }
          }

          return ret;
        }
      }
    }
  }
}
