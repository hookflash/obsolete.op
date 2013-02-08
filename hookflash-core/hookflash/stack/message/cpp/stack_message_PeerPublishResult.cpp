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

#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <hookflash/stack/message/internal/stack_message_PeerPublishRequest.h>
#include <hookflash/stack/message/internal/stack_message_PeerPublishResult.h>

#include <zsLib/Numeric.h>
#include <zsLib/zsHelpers.h>

namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef zsLib::XML::ElementPtr ElementPtr;
      typedef zsLib::XML::DocumentPtr DocumentPtr;

      PeerPublishResultPtr PeerPublishResult::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerPublishResult>(message);
      }

      PeerPublishResult::PeerPublishResult()
      {
      }

      PeerPublishResultPtr PeerPublishResult::create(PeerPublishRequestPtr request)
      {
        return internal::PeerPublishResult::create(request);
      }

      zsLib::XML::DocumentPtr PeerPublishResult::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerPublishResult::encode(*this);
      }

      bool PeerPublishResult::hasAttribute(PeerPublishResult::AttributeTypes type) const
      {
        switch (type)
        {
          case AttributeType_PublicationMetaData: return mPublicationMetaData;
        }
        return MessageResult::hasAttribute((MessageResult::AttributeTypes)type);
      }

      namespace internal
      {
        PeerPublishResultPtr PeerPublishResult::create(ElementPtr root)
        {
          PeerPublishResultPtr ret(new message::PeerPublishResult);

          ret->mID = IMessageHelper::getAttributeID(root);
          ret->mTime = IMessageHelper::getAttributeEpoch(root);

          stack::IPublicationPtr publication;
          MessageHelper::fillFrom(ret, root, publication, ret->mPublicationMetaData);

          return ret;
        }

        PeerPublishResultPtr PeerPublishResult::create(PeerPublishRequestPtr request)
        {
          PeerPublishResultPtr ret(new message::PeerPublishResult);

          ret->mID = request->mID;
          ret->mTime = zsLib::now();

          return ret;
        }

        DocumentPtr PeerPublishResult::encode(message:: PeerPublishResult &msg)
        {
          return MessageHelper::createDocument(msg, msg.mPublicationMetaData);
        }
      }
    }
  }
}
