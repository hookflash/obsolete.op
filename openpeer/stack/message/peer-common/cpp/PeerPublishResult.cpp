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

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/peer-common/PeerPublishRequest.h>
#include <hookflash/stack/message/peer-common/PeerPublishResult.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      namespace peer_common
      {
        //---------------------------------------------------------------------
        PeerPublishResultPtr PeerPublishResult::convert(MessagePtr message)
        {
          return boost::dynamic_pointer_cast<PeerPublishResult>(message);
        }

        //---------------------------------------------------------------------
        PeerPublishResult::PeerPublishResult()
        {
        }

        //---------------------------------------------------------------------
        PeerPublishResultPtr PeerPublishResult::create(PeerPublishRequestPtr request)
        {
          PeerPublishResultPtr ret(new PeerPublishResult);

          ret->mDomain = request->domain();
          ret->mID = request->mID;

          return ret;
        }

        //---------------------------------------------------------------------
        PeerPublishResultPtr PeerPublishResult::create(
                                                       ElementPtr root,
                                                       IMessageSourcePtr messageSource
                                                       )
        {
          PeerPublishResultPtr ret(new PeerPublishResult);
          IMessageHelper::fill(*ret, root, messageSource);

          stack::IPublicationPtr publication;
          internal::MessageHelper::fillFrom(messageSource, ret, root, publication, ret->mPublicationMetaData);

          return ret;
        }

        //---------------------------------------------------------------------
        DocumentPtr PeerPublishResult::encode()
        {
          return internal::MessageHelper::createDocument(*this, mPublicationMetaData);
        }

        //---------------------------------------------------------------------
        bool PeerPublishResult::hasAttribute(PeerPublishResult::AttributeTypes type) const
        {
          switch (type)
          {
            case AttributeType_PublicationMetaData: return mPublicationMetaData;
          }
          return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
        }

      }
    }
  }
}
