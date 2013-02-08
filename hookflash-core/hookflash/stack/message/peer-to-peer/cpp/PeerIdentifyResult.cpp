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

#include <hookflash/stack/message/peer-to-peer/PeerIdentifyResult.h>
#include <hookflash/stack/message/peer-to-peer/PeerIdentifyRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>

#include <zsLib/XML.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_to_peer
      {
        typedef zsLib::XML::Exceptions::CheckFailed CheckFailed;

        using message::internal::MessageHelper;

        //---------------------------------------------------------------------
        PeerIdentifyResultPtr PeerIdentifyResult::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<PeerIdentifyResult>(message);
        }

        //---------------------------------------------------------------------
        PeerIdentifyResult::PeerIdentifyResult()
        {
        }

        //---------------------------------------------------------------------
        PeerIdentifyResultPtr PeerIdentifyResult::create(PeerIdentifyRequestPtr request)
        {
          PeerIdentifyResultPtr ret(new PeerIdentifyResult);

          ret->mDomain = request->domain();
          ret->mID = request->messageID();

          return ret;
        }

        //---------------------------------------------------------------------
        PeerIdentifyResultPtr PeerIdentifyResult::create(
                                                         ElementPtr root,
                                                         IMessageSourcePtr messageSource
                                                         )
        {
          PeerIdentifyResultPtr ret(new PeerIdentifyResult);

          ret->mID = IMessageHelper::getAttributeID(root);
          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          ret->mLocationInfo = MessageHelper::createLocation(root->findFirstChildElement("location"), messageSource);

          return ret;
        }

        //---------------------------------------------------------------------
        bool PeerIdentifyResult::hasAttribute(AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_LocationInfo:    return mLocationInfo.hasData();
            default:                            break;
          }
          return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
        }

        //---------------------------------------------------------------------
        DocumentPtr PeerIdentifyResult::encode()
        {
          DocumentPtr ret = IMessageHelper::createDocumentWithRoot(*this);
          ElementPtr root = ret->getFirstChildElement();

          if (hasAttribute(AttributeType_LocationInfo)) {
            root->adoptAsLastChild(MessageHelper::createElement(mLocationInfo));
          }

          return ret;
        }

      }
    }
  }
}
