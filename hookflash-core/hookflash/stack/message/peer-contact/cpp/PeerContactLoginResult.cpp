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

#include <hookflash/stack/message/peer-contact/PeerContactLoginResult.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/IPeerFilePublic.h>

#include <zsLib/XML.h>
#include <zsLib/helpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_contact
      {
        //---------------------------------------------------------------------
        PeerContactLoginResultPtr PeerContactLoginResult::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<PeerContactLoginResult>(message);
        }

        //---------------------------------------------------------------------
        PeerContactLoginResult::PeerContactLoginResult() :
          mPeerFilesRegenerate(-1)
        {
        }

        //---------------------------------------------------------------------
        PeerContactLoginResultPtr PeerContactLoginResult::create(
                                                                 ElementPtr root,
                                                                 IMessageSourcePtr messageSource
                                                                 )
        {
          PeerContactLoginResultPtr ret(new PeerContactLoginResult);
          IMessageHelper::fill(*ret, root, messageSource);

          ret->mContactUserID = IMessageHelper::getElementTextAndDecode(root->findFirstChildElement("contactUserID"));
          ret->mContactAccessToken = IMessageHelper::getElementTextAndDecode(root->findFirstChildElement("contactAccessToken"));
          ret->mContactAccessSecret = IMessageHelper::getElementTextAndDecode(root->findFirstChildElement("contactAccessSecret"));
          ret->mContactAccessExpires = IMessageHelper::stringToTime(IMessageHelper::getElementTextAndDecode(root->findFirstChildElement("contactAccessExpires")));
          String regenerate = IMessageHelper::getElementTextAndDecode(root->findFirstChildElement("contactAccessExpires"));
          if (!regenerate.isEmpty()) {
            ret->mPeerFilesRegenerate = ("true" == regenerate ? 1 : 0);
          }

          return ret;
        }

        //---------------------------------------------------------------------
        bool PeerContactLoginResult::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_ContactUserID:         return (!mContactUserID.isEmpty());
            case AttributeType_ContactAccessToken:    return (!mContactAccessToken.isEmpty());
            case AttributeType_ContactAccessSecret:   return (!mContactAccessSecret.isEmpty());
            case AttributeType_ContactAccessExpires:  return (Time() != mContactAccessExpires);
            case AttributeType_PeerFilesRegenerate:   return (mPeerFilesRegenerate >= 0);
            default:                                  break;
          }
          return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
        }
      }
    }
  }
}
