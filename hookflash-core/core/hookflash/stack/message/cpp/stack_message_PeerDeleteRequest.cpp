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

#include <hookflash/stack/message/internal/stack_message_PeerDeleteRequest.h>
#include <hookflash/stack/message/internal/stack_message_MessageHelper.h>
#include <zsLib/Stringize.h>


namespace hookflash
{
  namespace stack
  {
    namespace message
    {
      typedef zsLib::XML::DocumentPtr DocumentPtr;

      PeerDeleteRequestPtr PeerDeleteRequest::convert(MessagePtr message)
      {
        return boost::dynamic_pointer_cast<PeerDeleteRequest>(message);
      }

      PeerDeleteRequest::PeerDeleteRequest()
      {
      }

      PeerDeleteRequestPtr PeerDeleteRequest::create()
      {
        return internal::PeerDeleteRequest::create();
      }

      DocumentPtr PeerDeleteRequest::encode(IPeerFilesPtr peerFile)
      {
        return internal::PeerDeleteRequest::encode(*this);
      }

      bool PeerDeleteRequest::hasAttribute(AttributeTypes type) const
      {
        switch (type) {
          case AttributeType_PublicationMetaData: return mPublicationMetaData;
        }
        return false;
      }

      namespace internal
      {
        PeerDeleteRequestPtr PeerDeleteRequest::create(ElementPtr root)
        {
          PeerDeleteRequestPtr ret(new message::PeerDeleteRequest);

          if (root)
          {
            ret->mID = IMessageHelper::getAttributeID(root);

            IPublicationPtr publication;
            MessageHelper::fillFrom(ret, root, publication, ret->mPublicationMetaData);
          }

          return ret;
        }

        DocumentPtr PeerDeleteRequest::encode(message::PeerDeleteRequest &msg)
        {
          return MessageHelper::createDocument(msg, msg.mPublicationMetaData);
        }
      }
    }
  }
}
